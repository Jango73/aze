
// Qt
#include <QDir>
#include <QFile>
#include <QRegularExpression>

// AzeLib
#include "CRepository.h"
#include "CUtils.h"

// Application
#include "CTestMerge.h"
#include "CTestMacros.h"

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Constructor.
* \param sArgument0 Binary path, forwarded to the fixture.
*/
/***************************************************************************/
CTestMerge::CTestMerge(const QString& sArgument0)
    : CTestFixture(sArgument0)
{
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests merge functionality.
*/
/***************************************************************************/
void CTestMerge::testMerge()
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

/***************************************************************************/
/*!
* \brief Tests merge error paths and conflict resolution.
*/
/***************************************************************************/
void CTestMerge::testMergeErrorsAndResolution()
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

    // Merge branch with self is refused with a distinct code
    MERGE_BRANCH(sResult, "trunk");
    QVERIFY(sResult == expectedCode(Aze::CError::eCannotMergeSelf));
    QVERIFY(Aze::CError::eCannotMergeSelf != Aze::CError::eCouldNotMerge);

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

    // Successful deletion removes the branch file, trunk history stays readable
    DELETE_BRANCH(sResult, "side");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));

    {
        Aze::CRepository oCheckRepository(m_sRootPath);
        QVERIFY(oCheckRepository.ok());
        QVERIFY(not QFile(oCheckRepository.database()->composeBranchFileName("side")).exists());
    }

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

    // Delete unmerged branch is refused with a distinct code
    CREATE_BRANCH("doomed");
    SWITCH_TO_BRANCH("doomed");
    QVERIFY(createFile(sFile1Path, sTrunk1));
    STAGE(sFile1Path);
    COMMIT("D1");
    SWITCH_TO_BRANCH("trunk");
    DELETE_BRANCH(sResult, "doomed");
    QVERIFY(sResult == expectedCode(Aze::CError::eCannotDeleteUnmergedBranch));
    QVERIFY(Aze::CError::eCannotDeleteUnmergedBranch != Aze::CError::eBranchDoesNotExist);
    QVERIFY(Aze::CError::eCannotDeleteUnmergedBranch != Aze::CError::eCouldNotDeleteBranch);

    // Force deletion removes the unmerged branch and its branch file
    DELETE_BRANCH_FORCE(sResult, "doomed");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));

    {
        Aze::CRepository oCheckRepository(m_sRootPath);
        QVERIFY(oCheckRepository.ok());
        QVERIFY(not QFile(oCheckRepository.database()->composeBranchFileName("doomed")).exists());
    }
}

//-------------------------------------------------------------------------------------------------
