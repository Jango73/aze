
// std
#include "stdlib.h"

// Qt
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QRegularExpression>

// AzeLib
#include "CRepository.h"
#include "CRemoteRepository.h"
#include "CError.h"
#include "CFileHashCache.h"
#include "CUtils.h"
#include "CStrings.h"
#include <CXMLNode.h>

// Application
#include "../client/CAzeClient.h"
#include "../common/CConstants.h"
#include "CTestAze.h"

// For debug info on tests, swap next two lines
#define SILENT_OR_DEBUG CConstants::s_sSwitchSilent
//#define SILENT_OR_DEBUG CConstants::s_sSwitchDebug

//-------------------------------------------------------------------------------------------------
// Some macros to hide boiler plate code

#define INIT() \
    clearRepository(); \
    QDir rootDir(m_sRootPath); \
    QVERIFY(rootDir.mkdir(m_sFilesFolderName)); \
    QDir filesDir(m_sFilesFolderPath); \
    QVERIFY(filesDir.mkdir(m_sFilesSubFolderName)); \
    exec({ CConstants::s_sSwitchInitRepository })

#define COMMIT(a) \
    exec({ \
    CConstants::s_sSwitchCommit, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    QString("--%1").arg(CConstants::s_sSwitchMessage), \
    a, \
    QString("--%1").arg(CConstants::s_sSwitchAuthor), \
    m_sAuthor \
    })

#define AMEND(a, b) \
    exec({ \
    CConstants::s_sSwitchAmend, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    QString("--%1").arg(CConstants::s_sSwitchMessage), \
    a, \
    QString("--%1").arg(CConstants::s_sSwitchAuthor), \
    b \
    })

#define AMEND_MESSAGE_ONLY(a) \
    exec({ \
    CConstants::s_sSwitchAmend, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    QString("--%1").arg(CConstants::s_sSwitchMessage), \
    a \
    })

#define STAGE(...) \
    exec({ \
    CConstants::s_sSwitchStage, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    __VA_ARGS__ \
    })

#define UNSTAGE(...) \
    exec({ \
    CConstants::s_sSwitchUnstage, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    __VA_ARGS__ \
    })

#define REMOVE(...) \
    exec({ \
    CConstants::s_sSwitchRemove, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    __VA_ARGS__ \
    })

#define LOG(a) \
    a = exec({ \
    CConstants::s_sSwitchLog, \
    })

#define LOG_GRAPH(a) \
    a = exec({ \
    CConstants::s_sSwitchLog, \
    QString("--%1").arg(CConstants::s_sSwitchGraph), \
    })

#define DELETE_BRANCH(a, b) \
    a = exec({ \
    CConstants::s_sSwitchDeleteBranch, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    b \
    })

#define DIFF_LAST(a) \
    a = exec({ \
    CConstants::s_sSwitchDiff, "tip~1 tip > " \
    })

#define DIFF_BEFORE_LAST(a) \
    a = exec({ \
    CConstants::s_sSwitchDiff, "tip~2 tip~1 > " \
    })

#define CREATE_BRANCH(a) \
    exec({ \
    CConstants::s_sSwitchCreateBranch, \
    QString("--%1").arg(SILENT_OR_DEBUG), a \
    })

#define SWITCH_TO_BRANCH(a) \
    exec({ \
    CConstants::s_sSwitchSwitchToBranch, \
    QString("--%1").arg(CConstants::s_sSwitchAllowFileDelete), \
    QString("--%1").arg(SILENT_OR_DEBUG), a \
    })

#define MERGE_BRANCH(a, b) \
    a = exec({ \
    CConstants::s_sSwitchMerge, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    b \
    })

#define SAVE_STASH(...) \
    exec({ \
    CConstants::s_sSwitchSaveStash, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    __VA_ARGS__ \
    })

#define POP_STASH(...) \
    exec({ \
    CConstants::s_sSwitchPopStash, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    __VA_ARGS__ \
    })

#define PATCH(a, b) \
    a = exec({ \
    CConstants::s_sSwitchPatch, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    b \
    })

#define STATUS(a) \
    a = exec({ \
    CConstants::s_sSwitchShowStatus, \
    QString("--%1").arg(SILENT_OR_DEBUG) \
    })

//-------------------------------------------------------------------------------------------------

static QString expectedCode(Aze::CError::EErrorCode eCode)
{
    return QString::number(Aze::CError::toExitCode(eCode));
}

//-------------------------------------------------------------------------------------------------

CTestAze::CTestAze(const QString& sArgument0)
    : m_sArgument0(sArgument0)
    , m_iCommitIndex(0)
{
    // Tests manipulate and delete files: run them in an isolated folder
    // under <repository>/tmp, never in the current working folder
    m_sRootPath = testDirectoryPath();
    QDir::setCurrent(m_sRootPath);

    m_sDataPath = "./.aze";
    m_sAuthor = "CTestAze";
    m_sBranch1 = "branch1";
    m_sBranch2 = "branch2";

    m_sInfoPath = QString("%1/%2")
            .arg(m_sDataPath)
            .arg(Aze::CStrings::s_sGeneralInformationFileName);

    m_sTrunkPath = QString("%1/%2/%3.%4")
            .arg(m_sDataPath)
            .arg(Aze::CStrings::s_sPathAzeBranchPath)
            .arg(Aze::CStrings::s_sDefaultBranchName)
            .arg(Aze::CStrings::s_sCompressedXMLExtension);

    m_sBranch1Path = QString("%1/%2/%3.%4")
            .arg(m_sDataPath)
            .arg(Aze::CStrings::s_sPathAzeBranchPath)
            .arg(m_sBranch1)
            .arg(Aze::CStrings::s_sCompressedXMLExtension);

    m_sFilesFolderName = "Files";
    m_sFilesFolderPath = QString("%1/%2").arg(m_sRootPath).arg(m_sFilesFolderName);

    m_sFilesSubFolderName = "OtherFiles";
    m_sFilesSubFolderPath = QString("%1/%2/%3").arg(m_sRootPath).arg(m_sFilesFolderName).arg(m_sFilesSubFolderName);
}

//-------------------------------------------------------------------------------------------------

void CTestAze::clearRepository()
{
    // Remove repo
    QDir repo(m_sDataPath);
    if (repo.exists())
        repo.removeRecursively();

    // Remove files
    QDir files(m_sFilesFolderName);
    if (files.exists())
        files.removeRecursively();
}

//-------------------------------------------------------------------------------------------------

QString CTestAze::testDirectoryPath()
{
    // Locate the repository root from the binary folder, then use tmp/
    QDir tFolder(QCoreApplication::applicationDirPath());
    while (not tFolder.exists("Aze.pro") and tFolder.cdUp())
    {
    }

    if (not tFolder.exists("Aze.pro"))
        qFatal("CTestAze: repository root (Aze.pro) not found, tests refuse to run outside a repository checkout");

    QString sTestPath = QString("%1/tmp/aze-tests").arg(tFolder.absolutePath());
    QDir().mkpath(sTestPath);

    return sTestPath;
}

//-------------------------------------------------------------------------------------------------

QMap<QString, QString> CTestAze::generateFiles(int iCount)
{
    QMap<QString, QString> mFiles;

    for (int i = 0; i < iCount; i++)
    {
        mFiles[QString("%1.txt").arg(m_tTextGenerator.word())] = m_tTextGenerator.text();
    }

    return mFiles;
}

//-------------------------------------------------------------------------------------------------

bool CTestAze::createFile(const QString& sName, const QString& sContent)
{
    QFile file(sName);

    if (file.open(QIODevice::WriteOnly))
    {
        file.write(sContent.toUtf8().constData());
        file.close();

        return true;
    }

    return false;
}

//-------------------------------------------------------------------------------------------------

bool CTestAze::deleteFile(const QString& sName)
{
    QFile file(sName);
    return file.remove();
}

//-------------------------------------------------------------------------------------------------

bool CTestAze::readFile(const QString& sName, QString& sContent)
{
    QFile file(sName);

    if (file.open(QIODevice::ReadOnly))
    {
        sContent = QString(file.readAll());
        file.close();

        return true;
    }

    return false;
}

//-------------------------------------------------------------------------------------------------

void CTestAze::createManyFiles()
{
    QDir filesDir(m_sFilesFolderPath);

    for (int iIndex = 0; iIndex < 5; iIndex++)
    {
        // Create some directory
        QString sNewFolder = m_tTextGenerator.word();
        QString sNewFolderPath = QString("%1/%2").arg(m_sFilesFolderName).arg(sNewFolder);

        QDir newFolder(sNewFolderPath);
        if (not newFolder.exists())
            QVERIFY(filesDir.mkdir(sNewFolder));

        // Create some files
        lStage.clear();
        QMap<QString, QString> mNewFiles = generateFiles(20 + rand() % 30);

        for (QString sNewFile : mNewFiles.keys())
        {
            lStage << QString("%1/%2")
                      .arg(sNewFolderPath)
                      .arg(sNewFile);

            QVERIFY(createFile(lStage.last(), mNewFiles[sNewFile]));
        }

        lStage.prepend(CConstants::s_sSwitchStage);

        exec(lStage);
        exec({
                 CConstants::s_sSwitchCommit,
                 QString("--%1").arg(CConstants::s_sSwitchMessage),
                 QString("Commit%1\nMultiline commit").arg(m_iCommitIndex++),
                 QString("--%1").arg(CConstants::s_sSwitchAuthor),
                 m_sAuthor
             });
    }
}

//-------------------------------------------------------------------------------------------------

QString CTestAze::exec(const QStringList& lInputArguments)
{
    QStringList lFinalArguments(lInputArguments);
    QString baOutput;
    QTextStream sOutput(&baOutput);

    lFinalArguments.prepend(m_sArgument0);
    int argc = lFinalArguments.count();
    char** argv = new char*[std::size_t(argc)];
    int index;

    // Add arguments
    for (index = 0; index < argc; index++)
    {
        argv[index] = new char[std::size_t(lFinalArguments[index].count() + 2)];
        strcpy(argv[index], lFinalArguments[index].toUtf8().constData());
    }

    // Instantiate and run aze
    Aze::CError::EErrorCode eCode = CAzeClient(argc, argv, &sOutput).run();

    for (index = 0; index < argc; index++)
        delete [] argv[index];

    delete [] argv;

    // If output is empty, return the error code
    if (baOutput.count() == 0)
    {
        return QString::number(Aze::CError::toExitCode(eCode));
    }

    return baOutput;
}

//-------------------------------------------------------------------------------------------------

void CTestAze::testCommit()
{
    QString sFile1Path = "Files/CommitTest1.txt";
    QString sFile2Path = "Files/OtherFiles/CommitTest2.txt";
    QString sDiffContent;
    QString sResult;

    QString sC1("C1\n--");
    QString sC2("C2\n--");
    QString sD12 = Aze::CUtils::fileDiffHeader(sFile1Path, sFile1Path) + "@@ -1,2 +1,2 @@\n-C1\n+C2\n --\n";
    QString sD23 = Aze::CUtils::fileDiffHeader(sFile2Path, sFile2Path) + "@@ -1,1 +1,2 @@\n-\n+C1\n+--\n";

    INIT();

    // C1
    QVERIFY(createFile(sFile1Path, sC1));
    STAGE(sFile1Path);
    COMMIT("C1");

    // C2
    QVERIFY(createFile(sFile1Path, sC2));
    STAGE(sFile1Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("C2");

    DIFF_LAST(sDiffContent);
    QVERIFY(sDiffContent == sD12);

    // C3
    QVERIFY(createFile(sFile2Path, sC1));
    STAGE(sFile2Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("C3");

    DIFF_LAST(sDiffContent);
    QVERIFY(sDiffContent == sD23);
}

//-------------------------------------------------------------------------------------------------

void CTestAze::testUnstage()
{
    QString sFile1Path = "Files/UnstageTest1.txt";
    QString sFile2Path = "Files/UnstageTest2.txt";
    QString sResult, sFileContent;
    QString sC1("U1\n--");
    QVERIFY(Aze::CError::eCouldNotUnstageFiles != Aze::CError::eCouldNotAddFiles);
    INIT();
    QVERIFY(createFile(sFile1Path, sC1));
    QVERIFY(createFile(sFile2Path, sC1));
    QVERIFY(STAGE(sFile1Path, sFile2Path) == expectedCode(Aze::CError::eNone));
    QVERIFY(UNSTAGE(sFile2Path) == expectedCode(Aze::CError::eNone));
    STATUS(sResult);
    QVERIFY(sResult.contains(sFile1Path));
    QVERIFY(not sResult.contains(sFile2Path));
    COMMIT("U1");
    Aze::CRepository oRepository(m_sRootPath);
    QVERIFY(oRepository.ok());
    QVERIFY(oRepository.tipCommit()->files().contains(sFile1Path));
    QVERIFY(not oRepository.tipCommit()->files().contains(sFile2Path));
    QVERIFY(readFile(sFile2Path, sFileContent));
    QVERIFY(sFileContent == sC1);
}

//-------------------------------------------------------------------------------------------------

void CTestAze::testAmend()
{
    QString sFilePath = "Files/AmendTest.txt";
    QString sResult, sFileContent;
    QString sC1("A1\n--");
    QString sC2("A2\n--");
    QString sC2Amended("A2-amended\n--");
    QString sAmendedMessage("A2-amended");
    QString sFinalMessage("A2-final");
    QString sAmendAuthor("AmendAuthor");

    INIT();

    QVERIFY(createFile(sFilePath, sC1));
    QVERIFY(STAGE(sFilePath) == expectedCode(Aze::CError::eNone));
    COMMIT("A1");

    QVERIFY(createFile(sFilePath, sC2));
    QVERIFY(STAGE(sFilePath) == expectedCode(Aze::CError::eNone));
    COMMIT("A2");

    Aze::CRepository oBeforeRepository(m_sRootPath);
    QVERIFY(oBeforeRepository.ok());
    QString sTipBefore = oBeforeRepository.tipCommit()->id();
    QStringList lParentsBefore = oBeforeRepository.tipCommit()->parents();
    QVERIFY(not sTipBefore.isEmpty());
    QVERIFY(lParentsBefore.count() == 1);

    QVERIFY(createFile(sFilePath, sC2Amended));
    QVERIFY(STAGE(sFilePath) == expectedCode(Aze::CError::eNone));
    QVERIFY(AMEND(sAmendedMessage, sAmendAuthor) == expectedCode(Aze::CError::eNone));

    Aze::CRepository oAmendedRepository(m_sRootPath);
    QVERIFY(oAmendedRepository.ok());
    QString sTipAfter = oAmendedRepository.tipCommit()->id();
    QVERIFY(sTipAfter != sTipBefore);
    QVERIFY(oAmendedRepository.tipCommit()->parents() == lParentsBefore);
    QCOMPARE(oAmendedRepository.tipCommit()->message(), sAmendedMessage);
    QCOMPARE(oAmendedRepository.tipCommit()->author(), sAmendAuthor);
    QVERIFY(readFile(sFilePath, sFileContent));
    QVERIFY(sFileContent == sC2Amended);

    LOG(sResult);
    QVERIFY(sResult.contains(sAmendedMessage));

    QVERIFY(AMEND_MESSAGE_ONLY(sFinalMessage) == expectedCode(Aze::CError::eNone));

    Aze::CRepository oFinalRepository(m_sRootPath);
    QVERIFY(oFinalRepository.ok());
    QVERIFY(oFinalRepository.tipCommit()->id() != sTipAfter);
    QVERIFY(oFinalRepository.tipCommit()->parents() == lParentsBefore);
    QCOMPARE(oFinalRepository.tipCommit()->message(), sFinalMessage);
    QCOMPARE(oFinalRepository.tipCommit()->author(), sAmendAuthor);
}

//-------------------------------------------------------------------------------------------------

void CTestAze::testDelete()
{
    QString sFile1Path = "Files/CommitTest1.txt";
    QString sFile2Path = "Files/CommitTest2.txt";
    QString sDiffContent;
    QString sResult;

    QString sF1("F1\n--");
    QString sF2("F2\n--");
    QString sD12 = Aze::CUtils::fileDiffHeader(sFile2Path, sFile2Path) + "@@ -1,2 +1,1 @@\n-F2\n---\n+\n";
    QString sD23 = Aze::CUtils::fileDiffHeader(sFile1Path, sFile1Path) + "@@ -1,2 +1,1 @@\n-F1\n---\n+\n";

    INIT();

    // C1
    QVERIFY(createFile(sFile1Path, sF1));
    QVERIFY(createFile(sFile2Path, sF2));
    STAGE(sFile1Path);
    STAGE(sFile2Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("C1");

    // C2
    QVERIFY(deleteFile(sFile2Path));
    STAGE(sFile2Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("C2");

    DIFF_LAST(sDiffContent);
    QVERIFY(sDiffContent == sD12);

    // C3
    REMOVE(sFile1Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("C3");

    DIFF_LAST(sDiffContent);
    QVERIFY(sDiffContent == sD23);
}

//-------------------------------------------------------------------------------------------------

void CTestAze::testPatch()
{
    QString sFile1Path = "Files/PatchTest1.txt";
    QString sFile2Path = "Files/PatchTest2.txt";
    QString sPatchPath = "Files/Patch.txt";
    QString sFileContent;
    QString sDiffPath = "Files/Diff.txt";
    QString sDiffContent;
    QString sResult;

    QString sC1("L1\nL2\nL3\nL4\n--");
    QString sC2("L2\nL3\nL4\nL5\n--");
    QString sC3("L6\nL7\nL8\nL9\nL3\nL4\n--");

    INIT();

    // C1
    QVERIFY(createFile(sFile1Path, sC1));
    QVERIFY(createFile(sFile2Path, sC1));
    STAGE(sFile1Path);
    STAGE(sFile2Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("C1");

    // C2
    QVERIFY(createFile(sFile1Path, sC2));
    QVERIFY(createFile(sFile2Path, sC2));
    STAGE(sFile1Path);
    STAGE(sFile2Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("C2");

    DIFF_LAST(sDiffContent);
    QVERIFY(createFile(sPatchPath, sDiffContent));

    QVERIFY(createFile(sFile1Path, sC1));
    QVERIFY(createFile(sFile2Path, sC1));
    PATCH(sResult, sPatchPath);
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));

    QVERIFY(readFile(sFile1Path, sFileContent));
    QVERIFY(sFileContent == sC2);
    QVERIFY(readFile(sFile2Path, sFileContent));
    QVERIFY(sFileContent == sC2);

    QVERIFY(createFile(sFile1Path, sC3));
    PATCH(sResult, sPatchPath);
    QVERIFY(sResult != expectedCode(Aze::CError::eNone));
}

//-------------------------------------------------------------------------------------------------

void CTestAze::testMerge()
{
    QString sFile1Path = "Files/MergeTest.txt";
    QString sFile1Content;
    QString sDiffPath = "Files/Diff.txt";
    QString sDiffContent;
    QString sResult;

    QString sCA1("CA1\n--");
    QString sCA2("CA1\nCA2\n--");
    QString sCA3("CA1\nCA2\nCA3\n--");

    QString sCA1Stash("CA1\nzz\n--");

    QString sCB1("CA1\nCA2\n--\nCB1\n--");
    QString sCB2("CA1\nCA2\n--\nCB1\nCB2\n--");
    QString sCB3("CA1\nCA2\n--\nCB1\nCB2\nCB3\n--");
    QString sCB4("CA1\nCA2\n--\nCB1\nCB2\nCB3\nCB4\n--");
    QString sCB5("CA1\nCA2\n--\nCB1\nCB2\nCB3\nCB4\nCB5\n--");
    QString sCB6("CA1\nCA2\n--\nCB1\nCB2\nCB3\nCB4\nCB5\nCB6\n--");
    QString sCB8("CA1\nCA2\nCA3\n--\nCX1\nCX2\nCB3\nCB4\nCB5\nCB6\n--\nCY1\nCY2\nCC3\nCC4");

    QString sCC1("CA1\nCA2\n--\nCB1\nCB2\nCB3\n--\nCC1");
    QString sCC2("CA1\nCA2\n--\nCB1\nCB2\nCB3\n--\nCC1\nCC2");
    QString sCC3("CA1\nCA2\n--\nCB1\nCB2\nCB3\n--\nCC1\nCC2\nCC3");
    QString sCC5("CA1\nCA2\nCA3\n--\nCB1\nCB2\nCB3\nCB4\nCB5\n--\nCC1\nCC2\nCC3\nCC4");
    QString sCC6("CA1\nCA2\nCA3\n--\nCZ1\nCZ2\nCB3\nCB4\nCB5\n--\nCZ1\nCZ2\nCC3\nCC4");

    QString sMA4("CA1\nCA2\nCA3\n--\nCB1\nCB2\nCB3\n--");
    QString sMA5("CA1\nCA2\nCA3\n--\nCB1\nCB2\nCB3\n--\nCC1\nCC2");
    QString sMA6("CA1\nCA2\nCA3\n--\nCB1\nCB2\nCB3\nCB4\nCB5\n--\nCC1\nCC2");
    QString sMA7("CA1\nCA2\nCA3\n--\nCB1\nCB2\nCB3\nCB4\nCB5\nCB6\n--\nCC1\nCC2");
    QString sMA8("CA1\nCA2\nCA3\n--\nCB1\nCB2\nCB3\nCB4\nCB5\nCB6\n--\nCC1\nCC2\nCC3\nCC4");

    QString sMB7("CA1\nCA2\nCA3\n--\nCB1\nCB2\nCB3\nCB4\nCB5\nCB6\n--\nCC1\nCC2\nCC3\nCC4");
    QString sMB9("CA1\nCA2\nCA3\n--\n<<<<<<< [0-9a-f]+ CC6\nCZ1\nCZ2\n|||||||\nCX1\nCX2\n>>>>>>> [0-9a-f]+ CB8\nCB3\nCB4\nCB5\nCB6\n--\n<<<<<<< [0-9a-f]+ CC6\nCZ1\nCZ2\n|||||||\nCY1\nCY2\n>>>>>>> [0-9a-f]+ CB8\nCC3\nCC4");

    QString sMC4("CA1\nCA2\nCA3\n--\nCB1\nCB2\nCB3\nCB4\nCB5\n--\nCC1\nCC2\nCC3");

    QString sDA2 = Aze::CUtils::fileDiffHeader(sFile1Path, sFile1Path) + "@@ -1,2 +1,3 @@\n CA1\n+CA2\n --\n";

    INIT();

    // Branch A
    CREATE_BRANCH("A");
    SWITCH_TO_BRANCH("A");

    // CA1
    QVERIFY(createFile(sFile1Path, sCA1));
    STAGE(sFile1Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("CA1");

    // Stash
    QVERIFY(createFile(sFile1Path, sCA1Stash));

    SAVE_STASH();
    QVERIFY(readFile(sFile1Path, sFile1Content));
    QVERIFY(sFile1Content == sCA1);

    POP_STASH();
    QVERIFY(readFile(sFile1Path, sFile1Content));
    QVERIFY(sFile1Content == sCA1Stash);

    QVERIFY(createFile(sFile1Path, sCA1));

    // CA2
    QVERIFY(createFile(sFile1Path, sCA2));
    STAGE(sFile1Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("CA2");

    DIFF_LAST(sDiffContent);
    QVERIFY(sDiffContent == sDA2);

    // Create branch B
    CREATE_BRANCH("B");

    // CA3
    QVERIFY(createFile(sFile1Path, sCA3));
    STAGE(sFile1Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("CA3");

    // Switch to branch B
    SWITCH_TO_BRANCH("B");

    // CB1
    QVERIFY(createFile(sFile1Path, sCB1));
    STAGE(sFile1Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("CB1");

    // CB2
    QVERIFY(createFile(sFile1Path, sCB2));
    STAGE(sFile1Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("CB2");

    // CB3
    QVERIFY(createFile(sFile1Path, sCB3));
    STAGE(sFile1Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("CB3");

    // Create branch C
    CREATE_BRANCH("C");

    // Switch to branch C
    SWITCH_TO_BRANCH("C");

    // CC1
    QVERIFY(createFile(sFile1Path, sCC1));
    STAGE(sFile1Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("CC1");

    // CC2
    QVERIFY(createFile(sFile1Path, sCC2));
    STAGE(sFile1Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("CC2");

    // Switch to branch A
    SWITCH_TO_BRANCH("A");

    // Merge B on A
    MERGE_BRANCH(sResult, "B");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));
    COMMIT("MA4");

    // Check merged file contents
    QVERIFY(readFile(sFile1Path, sFile1Content));
    QVERIFY(sFile1Content == sMA4);

    // Merge C on A
    MERGE_BRANCH(sResult, "C");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));
    COMMIT("MA5");

    // Check merged file contents
    QVERIFY(readFile(sFile1Path, sFile1Content));
    QVERIFY(sFile1Content == sMA5);

    // Switch to branch B
    SWITCH_TO_BRANCH("B");

    // CB4
    QVERIFY(createFile(sFile1Path, sCB4));
    STAGE(sFile1Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("CB4");

    // CB5
    QVERIFY(createFile(sFile1Path, sCB5));
    STAGE(sFile1Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("CB5");

    // Switch to branch C
    SWITCH_TO_BRANCH("C");

    // CC3
    QVERIFY(createFile(sFile1Path, sCC3));
    STAGE(sFile1Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("CC3");

    // Switch to branch A
    SWITCH_TO_BRANCH("A");

    // Merge B on A
    MERGE_BRANCH(sResult, "B");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));
    COMMIT("MA6");

    // Check merged file contents
    QVERIFY(readFile(sFile1Path, sFile1Content));
    QVERIFY(sFile1Content == sMA6);

    // Switch to branch C
    SWITCH_TO_BRANCH("C");

    // Merge A on C
    MERGE_BRANCH(sResult, "A");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));
    COMMIT("MC4");

    // Check merged file contents
    QVERIFY(readFile(sFile1Path, sFile1Content));
    QVERIFY(sFile1Content == sMC4);

    // CC5
    QVERIFY(createFile(sFile1Path, sCC5));
    STAGE(sFile1Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("CC5");

    // Switch to branch B
    SWITCH_TO_BRANCH("B");

    // CB6
    QVERIFY(createFile(sFile1Path, sCB6));
    STAGE(sFile1Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("CB6");

    // Switch to branch A
    SWITCH_TO_BRANCH("A");

    // Merge B on A
    MERGE_BRANCH(sResult, "B");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));
    COMMIT("MA7");

    // Check merged file contents
    QVERIFY(readFile(sFile1Path, sFile1Content));
    QVERIFY(sFile1Content == sMA7);

    // Merge C on A
    MERGE_BRANCH(sResult, "C");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));
    COMMIT("MA8");

    // Check merged file contents
    QVERIFY(readFile(sFile1Path, sFile1Content));
    QVERIFY(sFile1Content == sMA8);

    // Switch to branch B
    SWITCH_TO_BRANCH("B");

    // Merge A on B
    MERGE_BRANCH(sResult, "A");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));
    COMMIT("MB7");

    // Check merged file contents
    QVERIFY(readFile(sFile1Path, sFile1Content));
    QVERIFY(sFile1Content == sMB7);

    // CB8
    QVERIFY(createFile(sFile1Path, sCB8));
    STAGE(sFile1Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("CB8");

    // Switch to branch C
    SWITCH_TO_BRANCH("C");

    // CC6
    QVERIFY(createFile(sFile1Path, sCC6));
    STAGE(sFile1Path);
    STATUS(sResult); OUT_INFO(sResult);
    COMMIT("CC6");

    // Switch to branch B
    SWITCH_TO_BRANCH("B");

    // Merge C on B : conflict
    MERGE_BRANCH(sResult, "C");
    QVERIFY(sResult == expectedCode(Aze::CError::eMergeHasConflicts));

    // Check merged file contents
    QVERIFY(readFile(sFile1Path, sFile1Content));
    QVERIFY(QRegularExpression(sMB9).match(sFile1Content).hasMatch());

    // Log
    LOG(sResult);
    OUT_INFO(sResult);
}

//-------------------------------------------------------------------------------------------------

void CTestAze::testMergeErrorsAndResolution()
{
    QString sFile1Path = "Files/MergeExt1.txt";
    QString sFile2Path = "Files/MergeExt2.txt";
    QString sFileContent;
    QString sResult;
    QString sTrunkTipId;
    QString sSideTipId;

    QString sBase1("base1\n");
    QString sBase2("base2\n");
    QString sTrunk1("base1\ntrunk1\n");
    QString sSide1("base2\nside1\n");
    QString sTrunk2("base2\nside1\ntrunk2\n");
    QString sSide2("base2\nside1\nside2\n");
    QString sResolved("base2\nside1\nresolved\n");

    INIT();

    // C1
    QVERIFY(createFile(sFile1Path, sBase1));
    QVERIFY(createFile(sFile2Path, sBase2));
    STAGE(sFile1Path);
    STAGE(sFile2Path);
    COMMIT("C1");

    // Merge branch with self is refused with a message
    MERGE_BRANCH(sResult, "trunk");
    QVERIFY(sResult == Aze::CError::message(Aze::CError::eCannotMergeSelf) + "\n");

    // Merge unknown branch is refused with a distinct code
    MERGE_BRANCH(sResult, "no-such-branch");
    QVERIFY(sResult == expectedCode(Aze::CError::eUnknownBranch));
    QVERIFY(Aze::CError::eUnknownBranch != Aze::CError::eCouldNotMerge);

    // Merge with a non empty stage is refused without touching merge parents
    QVERIFY(createFile(sFile1Path, sTrunk1));
    STAGE(sFile1Path);
    MERGE_BRANCH(sResult, "no-such-branch");
    QVERIFY(sResult == expectedCode(Aze::CError::eStageNotEmpty));
    QVERIFY(Aze::CError::eStageNotEmpty != Aze::CError::eCouldNotMerge);
    UNSTAGE(sFile1Path);
    QVERIFY(createFile(sFile1Path, sBase1));

    // S1 : side branch changes second file only
    CREATE_BRANCH("side");
    SWITCH_TO_BRANCH("side");
    QVERIFY(createFile(sFile2Path, sSide1));
    STAGE(sFile2Path);
    COMMIT("S1");

    // Merge with dirty working directory is refused with a distinct code
    SWITCH_TO_BRANCH("trunk");
    QVERIFY(createFile(sFile1Path, sTrunk1));
    MERGE_BRANCH(sResult, "side");
    QVERIFY(sResult == expectedCode(Aze::CError::eDirtyWorkingFolder));
    QVERIFY(Aze::CError::eDirtyWorkingFolder != Aze::CError::eCouldNotMerge);
    QVERIFY(createFile(sFile1Path, sBase1));

    // T1 : trunk changes first file only, merge stays clean on both files
    QVERIFY(createFile(sFile1Path, sTrunk1));
    STAGE(sFile1Path);
    COMMIT("T1");

    MERGE_BRANCH(sResult, "side");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));
    COMMIT("M2");

    QVERIFY(readFile(sFile1Path, sFileContent));
    QVERIFY(sFileContent == sTrunk1);
    QVERIFY(readFile(sFile2Path, sFileContent));
    QVERIFY(sFileContent == sSide1);

    // Graph shows a fork below the merge commit, proving two parents
    LOG_GRAPH(sResult);
    QVERIFY(sResult.contains("\\  "));

    // T2 and S2 change the second file in diverging ways
    QVERIFY(createFile(sFile2Path, sTrunk2));
    STAGE(sFile2Path);
    COMMIT("T2");

    {
        Aze::CRepository oTrunkRepository(m_sRootPath);
        QVERIFY(oTrunkRepository.ok());
        sTrunkTipId = oTrunkRepository.tipCommit()->id();
        QVERIFY(not sTrunkTipId.isEmpty());
    }

    SWITCH_TO_BRANCH("side");
    QVERIFY(createFile(sFile2Path, sSide2));
    STAGE(sFile2Path);
    COMMIT("S2");

    {
        Aze::CRepository oSideRepository(m_sRootPath);
        QVERIFY(oSideRepository.ok());
        sSideTipId = oSideRepository.tipCommit()->id();
        QVERIFY(not sSideTipId.isEmpty());
        QVERIFY(sSideTipId != sTrunkTipId);
    }

    // Merge reports conflicts, clean file is merged, conflicting file is marked
    SWITCH_TO_BRANCH("trunk");
    MERGE_BRANCH(sResult, "side");
    QVERIFY(sResult == expectedCode(Aze::CError::eMergeHasConflicts));

    QVERIFY(readFile(sFile1Path, sFileContent));
    QVERIFY(sFileContent == sTrunk1);

    QVERIFY(readFile(sFile2Path, sFileContent));
    QVERIFY(sFileContent.contains("<<<<<<<"));
    QVERIFY(sFileContent.contains("|||||||"));
    QVERIFY(sFileContent.contains(">>>>>>>"));
    QVERIFY(sFileContent.contains("trunk2"));
    QVERIFY(sFileContent.contains("side2"));

    // Resolve conflict then commit
    QVERIFY(createFile(sFile2Path, sResolved));
    STAGE(sFile2Path);
    COMMIT("R1");

    QVERIFY(readFile(sFile2Path, sFileContent));
    QVERIFY(sFileContent == sResolved);

    // Resolution commit keeps both parents, proving a true merge
    {
        Aze::CRepository oResolvedRepository(m_sRootPath);
        QVERIFY(oResolvedRepository.ok());
        QVERIFY(oResolvedRepository.tipCommit()->isMerge());
        QCOMPARE(oResolvedRepository.tipCommit()->message(), QString("R1"));
        QStringList lParents = oResolvedRepository.tipCommit()->parents();
        QVERIFY(lParents.count() == 2);
        QVERIFY(lParents.contains(sTrunkTipId));
        QVERIFY(lParents.contains(sSideTipId));
    }

    // Graph still shows a fork below the resolution commit
    LOG_GRAPH(sResult);
    QVERIFY(sResult.contains("\\  "));

    // Nothing left to merge is refused with a distinct code
    MERGE_BRANCH(sResult, "side");
    QVERIFY(sResult == expectedCode(Aze::CError::eNothingToMerge));
    QVERIFY(Aze::CError::eNothingToMerge != Aze::CError::eCouldNotMerge);

    // Delete merged branch, trunk history stays readable
    DELETE_BRANCH(sResult, "side");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));

    SWITCH_TO_BRANCH("side");
    LOG(sResult);
    QVERIFY(sResult.contains("R1"));

    // Delete current branch is refused with a distinct code
    DELETE_BRANCH(sResult, "trunk");
    QVERIFY(sResult == expectedCode(Aze::CError::eCannotDeleteCurrentBranch));
    QVERIFY(Aze::CError::eCannotDeleteCurrentBranch != Aze::CError::eCouldNotDeleteBranch);

    // Delete unknown branch is refused with a distinct code
    DELETE_BRANCH(sResult, "no-such-branch");
    QVERIFY(sResult == expectedCode(Aze::CError::eBranchDoesNotExist));
    QVERIFY(Aze::CError::eBranchDoesNotExist != Aze::CError::eCouldNotDeleteBranch);
}

//-------------------------------------------------------------------------------------------------

void CTestAze::testRemotePush()
{
    QString sResult;

    QString sFilePath = "Files/RemotePush.txt";
    QString sContent("Pushed content.");

    INIT();

    QVERIFY(createFile(sFilePath, sContent));
    STAGE(sFilePath);
    COMMIT("P1");

    // Build a push request from this repository
    Aze::CRepository oSourceRepository(m_sRootPath);
    QVERIFY(oSourceRepository.ok());

    Aze::CRemoteRepository oSourceRemote(&oSourceRepository);
    CXMLNode xRequest = oSourceRemote.getPushRequest();
    QVERIFY(xRequest.attributes()[Aze::CStrings::s_sParamRequest] == Aze::CStrings::s_sParamPush);

    // Simulate the wire with a JSON round-trip
    CXMLNode xReceived = CXMLNode::parseJSON(xRequest.toJsonString());
    QVERIFY(xReceived.attributes()[Aze::CStrings::s_sParamTip] == oSourceRepository.tipCommit()->id());

    // Prepare an empty target repository outside of the source folder
    QDir oParent(m_sRootPath);
    QVERIFY(oParent.cdUp());
    QString sRemotePath = oParent.absoluteFilePath("aze-tests-remote");
    QDir oRemoteFolder(sRemotePath);

    if (oRemoteFolder.exists())
        QVERIFY(oRemoteFolder.removeRecursively());

    QVERIFY(QDir().mkpath(sRemotePath));

    Aze::CRepository oTargetRepository(sRemotePath);
    QVERIFY(oTargetRepository.init() == Aze::CError::eNone);

    // Apply the push request
    Aze::CRemoteRepository oTargetRemote(&oTargetRepository);
    QString sDetail;
    Aze::CError::EErrorCode ePushCode = oTargetRemote.applyPushRequest(xReceived, sDetail);
    QVERIFY2(ePushCode == Aze::CError::eNone, qPrintable(Aze::CError::message(ePushCode, sDetail)));

    // Check the pushed branch tip from a fresh repository instance
    Aze::CRepository oCheckRepository(sRemotePath);
    QVERIFY(oCheckRepository.ok());
    QCOMPARE(oCheckRepository.tipCommit()->id(), oSourceRepository.tipCommit()->id());

    // Check file content round-trips through transferred objects
    Aze::CCommit* pTipCommit = oCheckRepository.database()->getCommit(oCheckRepository.tipCommit()->id(), this);
    QCOMPARE(QString(pTipCommit->fileContent(oCheckRepository.database(), sFilePath)), sContent);
    delete pTipCommit;

    // Pushing the same tip again succeeds
    QVERIFY(oTargetRemote.applyPushRequest(xReceived, sDetail) == Aze::CError::eNone);

    // Pushing a rewritten tip that does not contain the target tip is refused
    Aze::CRepository oAmendedRepository(m_sRootPath);
    QVERIFY(oAmendedRepository.amend("CTestAze", "Rewritten") == Aze::CError::eNone);
    QVERIFY(oAmendedRepository.writeCurrentBranch() == Aze::CError::eNone);

    // Use a fresh instance so the rewritten tip is read back from the branch file
    Aze::CRepository oRewrittenRepository(m_sRootPath);
    Aze::CRemoteRepository oAmendedRemote(&oRewrittenRepository);
    CXMLNode xRewritten = oAmendedRemote.getPushRequest();
    QVERIFY(xRewritten.attributes()[Aze::CStrings::s_sParamTip] != oSourceRepository.tipCommit()->id());
    CXMLNode xRewrittenReceived = CXMLNode::parseJSON(xRewritten.toJsonString());
    QVERIFY(oTargetRemote.applyPushRequest(xRewrittenReceived, sDetail) == Aze::CError::ePushNotFastForward);

    // Push with an unknown tip is refused
    CXMLNode xBadRequest = CXMLNode::parseJSON(xRequest.toJsonString());
    xBadRequest.attributes()[Aze::CStrings::s_sParamTip] = "00";
    QVERIFY(oTargetRemote.applyPushRequest(xBadRequest, sDetail) == Aze::CError::ePushRequestInvalid);

    QVERIFY(QDir(sRemotePath).removeRecursively());
}

//-------------------------------------------------------------------------------------------------

void CTestAze::testClone()
{
    QString sTrunkFilePath = "Files/CloneTrunk.txt";
    QString sTrunkContent("Cloned trunk content.");
    QString sBranchFilePath = "Files/CloneBranch.txt";
    QString sBranchContent("Cloned branch content.");

    INIT();

    QVERIFY(createFile(sTrunkFilePath, sTrunkContent));
    STAGE(sTrunkFilePath);
    COMMIT("C1");

    CREATE_BRANCH(m_sBranch1);
    SWITCH_TO_BRANCH(m_sBranch1);
    QVERIFY(createFile(sBranchFilePath, sBranchContent));
    STAGE(sBranchFilePath);
    COMMIT("C2");
    SWITCH_TO_BRANCH(Aze::CStrings::s_sDefaultBranchName);

    // Read the source branch tips from a fresh repository instance
    Aze::CRepository oSourceRepository(m_sRootPath);
    QVERIFY(oSourceRepository.ok());
    QVERIFY(oSourceRepository.currentBranchName() == Aze::CStrings::s_sDefaultBranchName);

    QString sTrunkTip = oSourceRepository.tipCommit()->id();

    Aze::CBranch* pSourceBranch = oSourceRepository.database()->getBranch(m_sBranch1, this);
    QVERIFY(pSourceBranch != nullptr);
    QString sBranchTip = pSourceBranch->tipCommitId();
    delete pSourceBranch;

    QVERIFY(not sTrunkTip.isEmpty());
    QVERIFY(not sBranchTip.isEmpty());
    QVERIFY(sBranchTip != sTrunkTip);

    // Build a clone response and simulate the wire with a JSON round-trip
    Aze::CRemoteRepository oSourceRemote(&oSourceRepository);
    CXMLNode xResponse = oSourceRemote.getCloneResponse();
    QVERIFY(xResponse.attributes()[Aze::CStrings::s_sParamRequest] == Aze::CStrings::s_sParamClone);

    CXMLNode xReceived = CXMLNode::parseJSON(xResponse.toJsonString());
    QVERIFY(xReceived.attributes()[Aze::CStrings::s_sParamRequest] == Aze::CStrings::s_sParamClone);

    // Clone into a missing folder outside of the source folder
    QDir oParent(m_sRootPath);
    QVERIFY(oParent.cdUp());
    QString sClonePath = oParent.absoluteFilePath("aze-tests-clone");
    QDir oCloneFolder(sClonePath);

    if (oCloneFolder.exists())
        QVERIFY(oCloneFolder.removeRecursively());

    QVERIFY(not oCloneFolder.exists());

    QString sDetail;
    Aze::CError::EErrorCode eCloneCode = Aze::CRepository::clone(sClonePath, xReceived, sDetail, this);
    QVERIFY2(eCloneCode == Aze::CError::eNone, qPrintable(Aze::CError::message(eCloneCode, sDetail)));

    // Check branches, branch tips and file content from a fresh repository instance
    Aze::CRepository oCheckRepository(sClonePath);
    QVERIFY(oCheckRepository.ok());
    QCOMPARE(oCheckRepository.currentBranchName(), QString(Aze::CStrings::s_sDefaultBranchName));
    QCOMPARE(oCheckRepository.tipCommit()->id(), sTrunkTip);

    Aze::CBranch* pClonedBranch = oCheckRepository.database()->getBranch(m_sBranch1, this);
    QVERIFY(pClonedBranch != nullptr);
    QCOMPARE(pClonedBranch->tipCommitId(), sBranchTip);
    delete pClonedBranch;

    QCOMPARE(Aze::CUtils::getTextFileContent(QString("%1/%2").arg(sClonePath).arg(sTrunkFilePath)), sTrunkContent);
    QCOMPARE(Aze::CUtils::getTextFileContent(QString("%1/%2").arg(sClonePath).arg(sBranchFilePath)), QString());

    // Cloning again into the same folder is refused with a distinct code
    QVERIFY(Aze::CRepository::clone(sClonePath, xReceived, sDetail, this) == Aze::CError::eCouldNotClone);

    // Cloning without a target folder is refused with a distinct code
    QVERIFY(exec({ CConstants::s_sSwitchClone }) == expectedCode(Aze::CError::eNoFileNameGiven));

    QVERIFY(QDir(sClonePath).removeRecursively());
}

//-------------------------------------------------------------------------------------------------

void CTestAze::testDiff()
{
    QString sFile1Path = "Files/DiffTest1.txt";
    QString sFile2Path = "Files/DiffTest2.txt";
    QString sDiffNoArg;
    QString sDiffTip;
    QString sDiffTipPrev;
    QString sDiffFile1;
    QString sDiffFile2;

    QString sL1("L1\n--");
    QString sL2("L2\n--");
    QString sL3("L3\n--");
    QString sM1("M1\n--");
    QString sM2("M2\n--");

    QString sExpFile1 = Aze::CUtils::fileDiffHeader(sFile1Path, sFile1Path) + "@@ -1,2 +1,2 @@\n-L2\n+L3\n --\n";
    QString sExpFile2 = Aze::CUtils::fileDiffHeader(sFile2Path, sFile2Path) + "@@ -1,2 +1,2 @@\n-M1\n+M2\n --\n";
    QString sExpNoArg = sExpFile1 + sExpFile2;
    QString sExpTipPrevFile1 = Aze::CUtils::fileDiffHeader(sFile1Path, sFile1Path) + "@@ -1,2 +1,2 @@\n-L1\n+L3\n --\n";
    QString sExpTipPrev = sExpTipPrevFile1 + sExpFile2;

    INIT();

    // C1
    QVERIFY(createFile(sFile1Path, sL1));
    QVERIFY(createFile(sFile2Path, sM1));
    STAGE(sFile1Path);
    STAGE(sFile2Path);
    COMMIT("D1");

    // C2 : only first file changes
    QVERIFY(createFile(sFile1Path, sL2));
    STAGE(sFile1Path);
    COMMIT("D2");

    // Clean working folder : every diff variant is empty
    QVERIFY(exec({ CConstants::s_sSwitchDiff }) == expectedCode(Aze::CError::eNone));
    QVERIFY(exec({ CConstants::s_sSwitchDiff, "tip" }) == expectedCode(Aze::CError::eNone));
    QVERIFY(exec({ CConstants::s_sSwitchDiff, sFile1Path }) == expectedCode(Aze::CError::eNone));

    // Dirty working folder, without staging
    QVERIFY(createFile(sFile1Path, sL3));
    QVERIFY(createFile(sFile2Path, sM2));

    // No argument : tip against working folder
    sDiffNoArg = exec({ CConstants::s_sSwitchDiff });
    QVERIFY(sDiffNoArg == sExpNoArg);

    // One commit id : that commit against working folder
    sDiffTip = exec({ CConstants::s_sSwitchDiff, "tip" });
    QVERIFY(sDiffTip == sExpNoArg);

    sDiffTipPrev = exec({ CConstants::s_sSwitchDiff, "tip~1" });
    QVERIFY(sDiffTipPrev == sExpTipPrev);

    // One file name : tip version against working folder version of that file
    sDiffFile1 = exec({ CConstants::s_sSwitchDiff, sFile1Path });
    QVERIFY(sDiffFile1 == sExpFile1);

    sDiffFile2 = exec({ CConstants::s_sSwitchDiff, sFile2Path });
    QVERIFY(sDiffFile2 == sExpFile2);
}

//-------------------------------------------------------------------------------------------------

void CTestAze::testFileHashCache()
{
    QString sFilePath = "Files/HashCacheTest.txt";
    QString sAbsoluteFileName = QDir(m_sRootPath).absoluteFilePath(sFilePath);

    QVERIFY(QDir().mkpath("Files"));
    QFile::remove(sAbsoluteFileName);

    Aze::CFileHashCache oCache(this);

    QVERIFY(createFile(sFilePath, "alpha"));
    QString sIdentifier1 = oCache.identifierForFile(sAbsoluteFileName);
    QVERIFY(not sIdentifier1.isEmpty());
    QCOMPARE(sIdentifier1, Aze::CUtils::idFromFileContent(sAbsoluteFileName));
    QCOMPARE(oCache.missCount(), qint64(1));
    QCOMPARE(oCache.hitCount(), qint64(0));
    QVERIFY(oCache.containsFile(sAbsoluteFileName));

    // An unchanged file is served from the cache
    QString sIdentifier2 = oCache.identifierForFile(sAbsoluteFileName);
    QCOMPARE(sIdentifier2, sIdentifier1);
    QCOMPARE(oCache.hitCount(), qint64(1));
    QCOMPARE(oCache.missCount(), qint64(1));

    // A modified file misses the cache and yields a new identifier
    QVERIFY(createFile(sFilePath, "alpha-beta"));
    QVERIFY(not oCache.containsFile(sAbsoluteFileName));
    QString sIdentifier3 = oCache.identifierForFile(sAbsoluteFileName);
    QVERIFY(not sIdentifier3.isEmpty());
    QVERIFY(sIdentifier3 != sIdentifier1);
    QCOMPARE(oCache.missCount(), qint64(2));

    // Explicit invalidation forces a new read with the same result
    oCache.invalidateFile(sAbsoluteFileName);
    QVERIFY(not oCache.containsFile(sAbsoluteFileName));
    QString sIdentifier4 = oCache.identifierForFile(sAbsoluteFileName);
    QCOMPARE(sIdentifier4, sIdentifier3);
    QCOMPARE(oCache.missCount(), qint64(3));

    // Clearing drops entries but keeps counters
    oCache.clear();
    QCOMPARE(oCache.count(), qint64(0));
    QCOMPARE(oCache.missCount(), qint64(3));
    QVERIFY(not oCache.containsFile(sAbsoluteFileName));
    QCOMPARE(oCache.identifierForFile(sAbsoluteFileName), sIdentifier3);
    QCOMPARE(oCache.missCount(), qint64(4));

    // A missing file returns empty without being cached
    QVERIFY(deleteFile(sFilePath));
    QVERIFY(oCache.identifierForFile(sAbsoluteFileName).isEmpty());
    QVERIFY(not oCache.containsFile(sAbsoluteFileName));

    // An empty file name returns empty
    QVERIFY(oCache.identifierForFile("").isEmpty());
}

//-------------------------------------------------------------------------------------------------

void CTestAze::testHeavy()
{
//     INIT();

//    createManyFiles();

//    exec({ CConstants::s_sSwitchCreateBranch, m_sBranch1 });
//    exec({ CConstants::s_sSwitchSwitchToBranch, m_sBranch1 });

//    createManyFiles();

//    exec({ CConstants::s_sSwitchCreateBranch, m_sBranch2 });
//    exec({ CConstants::s_sSwitchSwitchToBranch, m_sBranch2 });

//    createManyFiles();
}

//-------------------------------------------------------------------------------------------------

void CTestAze::testFinalize()
{
    clearRepository();
}

//-------------------------------------------------------------------------------------------------
