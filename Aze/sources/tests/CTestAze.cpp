
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

#define STAGE(...) \
    exec({ \
    CConstants::s_sSwitchStage, \
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
    int iErrorCode = CAzeClient(argc, argv, &sOutput).run();

    for (index = 0; index < argc; index++)
        delete [] argv[index];

    delete [] argv;

    // If output is empty, return the error code
    if (baOutput.count() == 0)
    {
        return QString::number(iErrorCode);
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
    QVERIFY(sResult == "0");

    QVERIFY(readFile(sFile1Path, sFileContent));
    QVERIFY(sFileContent == sC2);
    QVERIFY(readFile(sFile2Path, sFileContent));
    QVERIFY(sFileContent == sC2);

    QVERIFY(createFile(sFile1Path, sC3));
    PATCH(sResult, sPatchPath);
    QVERIFY(sResult != "0");
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
    QVERIFY(sResult == "0");
    COMMIT("MA4");

    // Check merged file contents
    QVERIFY(readFile(sFile1Path, sFile1Content));
    QVERIFY(sFile1Content == sMA4);

    // Merge C on A
    MERGE_BRANCH(sResult, "C");
    QVERIFY(sResult == "0");
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
    QVERIFY(sResult == "0");
    COMMIT("MA6");

    // Check merged file contents
    QVERIFY(readFile(sFile1Path, sFile1Content));
    QVERIFY(sFile1Content == sMA6);

    // Switch to branch C
    SWITCH_TO_BRANCH("C");

    // Merge A on C
    MERGE_BRANCH(sResult, "A");
    QVERIFY(sResult == "0");
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
    QVERIFY(sResult == "0");
    COMMIT("MA7");

    // Check merged file contents
    QVERIFY(readFile(sFile1Path, sFile1Content));
    QVERIFY(sFile1Content == sMA7);

    // Merge C on A
    MERGE_BRANCH(sResult, "C");
    QVERIFY(sResult == "0");
    COMMIT("MA8");

    // Check merged file contents
    QVERIFY(readFile(sFile1Path, sFile1Content));
    QVERIFY(sFile1Content == sMA8);

    // Switch to branch B
    SWITCH_TO_BRANCH("B");

    // Merge A on B
    MERGE_BRANCH(sResult, "A");
    QVERIFY(sResult == "0");
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
    QVERIFY(sResult == QString::number(CConstants::s_iError_MergeHasConflicts));

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
    QVERIFY(sResult == CConstants::s_sTextCannotMergeBranchWithSelf + "\n");

    // Merge unknown branch is refused
    MERGE_BRANCH(sResult, "no-such-branch");
    QVERIFY(sResult == QString::number(CConstants::s_iError_CouldNotMerge));

    // S1 : side branch changes second file only
    CREATE_BRANCH("side");
    SWITCH_TO_BRANCH("side");
    QVERIFY(createFile(sFile2Path, sSide1));
    STAGE(sFile2Path);
    COMMIT("S1");

    // Merge with dirty working directory is refused
    SWITCH_TO_BRANCH("trunk");
    QVERIFY(createFile(sFile1Path, sTrunk1));
    MERGE_BRANCH(sResult, "side");
    QVERIFY(sResult == QString::number(CConstants::s_iError_CouldNotMerge));
    QVERIFY(createFile(sFile1Path, sBase1));

    // T1 : trunk changes first file only, merge stays clean on both files
    QVERIFY(createFile(sFile1Path, sTrunk1));
    STAGE(sFile1Path);
    COMMIT("T1");

    MERGE_BRANCH(sResult, "side");
    QVERIFY(sResult == "0");
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

    SWITCH_TO_BRANCH("side");
    QVERIFY(createFile(sFile2Path, sSide2));
    STAGE(sFile2Path);
    COMMIT("S2");

    // Merge reports conflicts, clean file is merged, conflicting file is marked
    SWITCH_TO_BRANCH("trunk");
    MERGE_BRANCH(sResult, "side");
    QVERIFY(sResult == QString::number(CConstants::s_iError_MergeHasConflicts));

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

    // Nothing left to merge
    MERGE_BRANCH(sResult, "side");
    QVERIFY(sResult == QString::number(CConstants::s_iError_CouldNotMerge));

    // Delete merged branch, trunk history stays readable
    DELETE_BRANCH(sResult, "side");
    QVERIFY(sResult == "0");

    SWITCH_TO_BRANCH("side");
    LOG(sResult);
    QVERIFY(sResult.contains("R1"));

    // Delete current branch is refused
    DELETE_BRANCH(sResult, "trunk");
    QVERIFY(sResult == QString::number(CConstants::s_iError_CouldNotDeleteBranch));

    // Delete unknown branch is refused
    DELETE_BRANCH(sResult, "no-such-branch");
    QVERIFY(sResult == QString::number(CConstants::s_iError_CouldNotDeleteBranch));
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
    QVERIFY(oTargetRepository.init());

    // Apply the push request
    Aze::CRemoteRepository oTargetRemote(&oTargetRepository);
    QString sErrorMessage;
    QVERIFY2(oTargetRemote.applyPushRequest(xReceived, sErrorMessage), qPrintable(sErrorMessage));

    // Check the pushed branch tip from a fresh repository instance
    Aze::CRepository oCheckRepository(sRemotePath);
    QVERIFY(oCheckRepository.ok());
    QCOMPARE(oCheckRepository.tipCommit()->id(), oSourceRepository.tipCommit()->id());

    // Check file content round-trips through transferred objects
    Aze::CCommit* pTipCommit = oCheckRepository.database()->getCommit(oCheckRepository.tipCommit()->id(), this);
    QCOMPARE(QString(pTipCommit->fileContent(oCheckRepository.database(), sFilePath)), sContent);
    delete pTipCommit;

    // Pushing the same tip again succeeds
    QVERIFY(oTargetRemote.applyPushRequest(xReceived, sErrorMessage));

    // Push with an unknown tip is refused
    CXMLNode xBadRequest = CXMLNode::parseJSON(xRequest.toJsonString());
    xBadRequest.attributes()[Aze::CStrings::s_sParamTip] = "00";
    QVERIFY(not oTargetRemote.applyPushRequest(xBadRequest, sErrorMessage));

    QVERIFY(QDir(sRemotePath).removeRecursively());
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
