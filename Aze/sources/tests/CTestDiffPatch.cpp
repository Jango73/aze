
// Qt
#include <QDir>

// AzeLib
#include "CRepository.h"
#include "CUtils.h"

// Application
#include "../common/CConstants.h"
#include "CTestDiffPatch.h"
#include "CTestMacros.h"

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Constructor.
* \param sArgument0 Binary path, forwarded to the fixture.
*/
/***************************************************************************/
CTestDiffPatch::CTestDiffPatch(const QString& sArgument0)
    : CTestFixture(sArgument0)
{
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests patch functionality.
*/
/***************************************************************************/
void CTestDiffPatch::testPatch()
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

/***************************************************************************/
/*!
* \brief Tests diff variants (no argument, one id, file name).
*/
/***************************************************************************/
void CTestDiffPatch::testDiff()
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

/***************************************************************************/
/*!
* \brief Tests show variants (commit id, file object id, unknown id).
*/
/***************************************************************************/
void CTestDiffPatch::testShow()
{
    QString sFilePath = "Files/ShowTest.txt";
    QString sContent("S1\n--");
    QString sMessage("ShowMessage");

    INIT();

    QVERIFY(createFile(sFilePath, sContent));
    QVERIFY(STAGE(sFilePath) == expectedCode(Aze::CError::eNone));
    COMMIT(sMessage);

    QString sTipId;
    QString sObjectId;

    {
        Aze::CRepository oRepository(m_sRootPath);
        QVERIFY(oRepository.ok());
        sTipId = oRepository.tipCommit()->id();
        QVERIFY(not sTipId.isEmpty());
        QVERIFY(oRepository.tipCommit()->files().contains(sFilePath));
        sObjectId = oRepository.tipCommit()->files()[sFilePath];
        QVERIFY(not sObjectId.isEmpty());
    }

    // Commit identifier dumps the stored commit document
    QString sCommitDump = exec({ CConstants::s_sSwitchDump, sTipId });
    QVERIFY(sCommitDump.contains(sMessage));
    QVERIFY(sCommitDump.contains(m_sAuthor));
    QVERIFY(sCommitDump.contains(sFilePath));
    QVERIFY(sCommitDump.contains(sObjectId));

    // File object identifier dumps the stored file content
    QString sObjectDump = exec({ CConstants::s_sSwitchDump, sObjectId });
    QVERIFY(sObjectDump == sContent);

    // Several identifiers are concatenated in order
    QString sBothDump = exec({ CConstants::s_sSwitchDump, sTipId, sObjectId });
    QVERIFY(sBothDump == sCommitDump + sObjectDump);

    // Unknown identifier yields empty output
    QVERIFY(exec({ CConstants::s_sSwitchDump, "no-such-id" }) == expectedCode(Aze::CError::eNone));
}

//-------------------------------------------------------------------------------------------------
