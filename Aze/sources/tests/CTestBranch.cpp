
// Qt
#include <QDir>

// AzeLib
#include "CRepository.h"
#include "CError.h"
#include "CStrings.h"

// Application
#include "CTestBranch.h"
#include "CTestMacros.h"

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Constructor.
* \param sArgument0 Binary path, forwarded to the fixture.
*/
/***************************************************************************/
CTestBranch::CTestBranch(const QString& sArgument0)
    : CTestFixture(sArgument0)
{
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests branch inventory with the current branch marked.
*/
/***************************************************************************/
void CTestBranch::testListBranches()
{
    QString sResult;

    QString sFilePath = "Files/BranchList.txt";
    QString sContent("Listed content.");

    INIT();

    QVERIFY(createFile(sFilePath, sContent));
    QVERIFY(STAGE(sFilePath) == expectedCode(Aze::CError::eNone));
    COMMIT("C1");
    CREATE_BRANCH("side-b");
    CREATE_BRANCH("side-a");
    CREATE_TAG("v1.0");

    LIST_BRANCHES(sResult);

    QString sTrunk = Aze::CStrings::s_sDefaultBranchName;

    // Current branch is marked, other branches are listed, tags are excluded
    QVERIFY(sResult.contains(QString("* %1").arg(sTrunk)));
    QVERIFY(sResult.contains("side-a"));
    QVERIFY(sResult.contains("side-b"));
    QVERIFY(not sResult.contains("v1.0"));

    // Switching the branch moves the mark
    SWITCH_TO_BRANCH("side-a");
    LIST_BRANCHES(sResult);

    QVERIFY(sResult.contains("* side-a"));
    QVERIFY(not sResult.contains(QString("* %1").arg(sTrunk)));
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests branch renaming with current pointer move and error guards.
*/
/***************************************************************************/
void CTestBranch::testRenameBranch()
{
    QString sResult;

    QString sFilePath = "Files/BranchRename.txt";
    QString sContent("Renamed content.");

    INIT();

    QVERIFY(createFile(sFilePath, sContent));
    QVERIFY(STAGE(sFilePath) == expectedCode(Aze::CError::eNone));
    COMMIT("C1");
    CREATE_BRANCH("side");
    CREATE_TAG("v1.0");

    {
        Aze::CRepository oRepository(m_sRootPath);
        QVERIFY(oRepository.ok());

        Aze::CBranch* pBranch = oRepository.database()->getBranch("side", this);
        QVERIFY(pBranch != nullptr);
        QCOMPARE(pBranch->tipCommitId(), oRepository.tipCommit()->id());
        delete pBranch;
    }

    // Renaming a branch keeps its tip
    RENAME_BRANCH(sResult, "side", "side2");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));

    {
        Aze::CRepository oRepository(m_sRootPath);
        QVERIFY(oRepository.ok());
        QVERIFY(not QFile(oRepository.database()->composeBranchFileName("side")).exists());
        QVERIFY(QFile(oRepository.database()->composeBranchFileName("side2")).exists());

        Aze::CBranch* pBranch = oRepository.database()->getBranch("side2", this);
        QVERIFY(pBranch != nullptr);
        QCOMPARE(pBranch->tipCommitId(), oRepository.tipCommit()->id());
        delete pBranch;
    }

    LIST_BRANCHES(sResult);
    QVERIFY(sResult.contains("side2"));
    QVERIFY(not sResult.contains("side\n") && not sResult.contains("  side\n"));

    // Renaming the current branch moves the current pointer
    SWITCH_TO_BRANCH("side2");
    RENAME_BRANCH(sResult, "side2", "side3");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));

    {
        Aze::CRepository oRepository(m_sRootPath);
        QVERIFY(oRepository.ok());
        QCOMPARE(oRepository.currentBranchName(), QString("side3"));
        QVERIFY(not QFile(oRepository.database()->composeBranchFileName("side2")).exists());
        QVERIFY(QFile(oRepository.database()->composeBranchFileName("side3")).exists());
    }

    LIST_BRANCHES(sResult);
    QVERIFY(sResult.contains("* side3"));

    // Error guards: unknown source, tag source, existing target, missing names
    RENAME_BRANCH(sResult, "unknown", "other");
    QVERIFY(sResult == expectedCode(Aze::CError::eBranchDoesNotExist));

    RENAME_BRANCH(sResult, "v1.0", "other");
    QVERIFY(sResult == expectedCode(Aze::CError::eBranchDoesNotExist));

    RENAME_BRANCH(sResult, "side3", "v1.0");
    QVERIFY(sResult == expectedCode(Aze::CError::eTagAlreadyExists));

    CREATE_BRANCH("other");
    RENAME_BRANCH(sResult, "side3", "other");
    QVERIFY(sResult == expectedCode(Aze::CError::eBranchAlreadyExists));

    sResult = exec({ CConstants::s_sSwitchRenameBranch, QString("--%1").arg(SILENT_OR_DEBUG), "side3" });
    QVERIFY(sResult == expectedCode(Aze::CError::eNoBranchNameGiven));
}

//-------------------------------------------------------------------------------------------------
