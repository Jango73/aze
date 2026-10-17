
// Qt
#include <QDir>
#include <QFile>

// AzeLib
#include "CRepository.h"
#include "CUtils.h"

// Application
#include "CTestWorkflow.h"
#include "CTestMacros.h"

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Constructor.
* \param sArgument0 Binary path, forwarded to the fixture.
*/
/***************************************************************************/
CTestWorkflow::CTestWorkflow(const QString& sArgument0)
    : CTestFixture(sArgument0)
{
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Test stage and commit functionalities.
*/
/***************************************************************************/
void CTestWorkflow::testCommit()
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

/***************************************************************************/
/*!
* \brief Test unstage functionality.
*/
/***************************************************************************/
void CTestWorkflow::testUnstage()
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

/***************************************************************************/
/*!
* \brief Test amend functionality.
*/
/***************************************************************************/
void CTestWorkflow::testAmend()
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

/***************************************************************************/
/*!
* \brief Test file deletion functionality.
*/
/***************************************************************************/
void CTestWorkflow::testDelete()
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

/***************************************************************************/
/*!
* \brief Test file move with history preservation.
*/
/***************************************************************************/
void CTestWorkflow::testMove()
{
    QString sSourcePath = "Files/MoveTest.txt";
    QString sTargetPath = "Files/MoveTestRenamed.txt";
    QString sOtherPath = "Files/MoveOther.txt";
    QString sResult, sFileContent;

    QString sContent("M1\n--");

    INIT();

    QVERIFY(createFile(sSourcePath, sContent));
    QVERIFY(STAGE(sSourcePath) == expectedCode(Aze::CError::eNone));
    COMMIT("M1");

    QString sTipBefore;
    QString sObjectBefore;

    {
        Aze::CRepository oBeforeRepository(m_sRootPath);
        QVERIFY(oBeforeRepository.ok());
        sTipBefore = oBeforeRepository.tipCommit()->id();
        QVERIFY(not sTipBefore.isEmpty());
        QVERIFY(oBeforeRepository.tipCommit()->files().contains(sSourcePath));
        sObjectBefore = oBeforeRepository.tipCommit()->files()[sSourcePath];
        QVERIFY(not sObjectBefore.isEmpty());
    }

    QVERIFY(MOVE(sSourcePath, sTargetPath) == expectedCode(Aze::CError::eNone));

    QVERIFY(not QFile(sSourcePath).exists());
    QVERIFY(QFile(sTargetPath).exists());
    QVERIFY(readFile(sTargetPath, sFileContent));
    QVERIFY(sFileContent == sContent);

    {
        Aze::CRepository oStagedRepository(m_sRootPath);
        QVERIFY(oStagedRepository.ok());
        QVERIFY(oStagedRepository.stagingCommit()->files().contains(sTargetPath));
        QCOMPARE(oStagedRepository.stagingCommit()->files()[sTargetPath], sObjectBefore);
    }

    COMMIT("M2");

    {
        Aze::CRepository oAfterRepository(m_sRootPath);
        QVERIFY(oAfterRepository.ok());
        QVERIFY(not oAfterRepository.tipCommit()->files().contains(sSourcePath));
        QVERIFY(oAfterRepository.tipCommit()->files().contains(sTargetPath));
        QCOMPARE(oAfterRepository.tipCommit()->files()[sTargetPath], sObjectBefore);
        QVERIFY(oAfterRepository.tipCommit()->fileContent(oAfterRepository.database(), sTargetPath) == sContent.toUtf8());

        QStringList lParents = oAfterRepository.tipCommit()->parents();
        QVERIFY(lParents.count() == 1);
        QCOMPARE(lParents.first(), sTipBefore);

        Aze::CCommit* pParent = Aze::CCommit::fromId(oAfterRepository.database(), sTipBefore, nullptr);
        QVERIFY(pParent != nullptr);
        QVERIFY(pParent->files().contains(sSourcePath));
        QCOMPARE(pParent->files()[sSourcePath], sObjectBefore);
        delete pParent;
    }

    QVERIFY(MOVE("Files/NoSuchFile.txt", sOtherPath) == expectedCode(Aze::CError::eCouldNotMoveFiles));
    QVERIFY(MOVE(sTargetPath, sTargetPath) == expectedCode(Aze::CError::eCouldNotMoveFiles));
    QVERIFY(createFile(sOtherPath, sContent));
    QVERIFY(MOVE(sTargetPath, sOtherPath) == expectedCode(Aze::CError::eCouldNotMoveFiles));
    QVERIFY(readFile(sTargetPath, sFileContent));
    QVERIFY(sFileContent == sContent);
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests functionalities under heavy load.
*/
/***************************************************************************/
void CTestWorkflow::testHeavy()
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
