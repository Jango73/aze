
// Qt
#include <QDir>
#include <QFile>

// AzeLib
#include "CRepository.h"
#include "CStrings.h"

// Application
#include "CTestPrune.h"
#include "CTestMacros.h"

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Constructor.
* \param sArgument0 Binary path, forwarded to the fixture.
*/
/***************************************************************************/
CTestPrune::CTestPrune(const QString& sArgument0)
    : CTestFixture(sArgument0)
{
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests prune mark-and-sweep over dangling commits and objects.
*/
/***************************************************************************/
void CTestPrune::testPrune()
{
    QString sKeepPath = "Files/PruneKeep.txt";
    QString sDanglingPath = "Files/PruneDangling.txt";
    QString sAmendPath = "Files/PruneAmend.txt";
    QString sGracePath = "Files/PruneGrace.txt";
    QString sResult;

    QString sKeepContent("prune-keep-v1\n--");
    QString sDanglingContent("prune-dangling-v1\n--");
    QString sAmendBefore("prune-amend-before\n--");
    QString sAmendAfter("prune-amend-after\n--");
    QString sGraceContent("prune-grace-v1\n--");

    INIT();

    // K1 on trunk holds the reachable file
    QVERIFY(createFile(sKeepPath, sKeepContent));
    QVERIFY(STAGE(sKeepPath) == expectedCode(Aze::CError::eNone));
    COMMIT("K1");

    QString sKeepCommitId;
    QString sKeepObjectId;

    {
        Aze::CRepository oRepository(m_sRootPath);
        QVERIFY(oRepository.ok());
        sKeepCommitId = oRepository.tipCommit()->id();
        QVERIFY(not sKeepCommitId.isEmpty());
        QVERIFY(oRepository.tipCommit()->files().contains(sKeepPath));
        sKeepObjectId = oRepository.tipCommit()->files()[sKeepPath];
        QVERIFY(not sKeepObjectId.isEmpty());
    }

    // D1 on a side branch holds an exclusive commit and an exclusive object
    CREATE_BRANCH("prune-doomed");
    SWITCH_TO_BRANCH("prune-doomed");
    QVERIFY(createFile(sDanglingPath, sDanglingContent));
    QVERIFY(STAGE(sDanglingPath) == expectedCode(Aze::CError::eNone));
    COMMIT("D1");

    QString sDanglingCommitId;
    QString sDanglingObjectId;

    {
        Aze::CRepository oRepository(m_sRootPath);
        QVERIFY(oRepository.ok());
        sDanglingCommitId = oRepository.tipCommit()->id();
        QVERIFY(not sDanglingCommitId.isEmpty());
        QVERIFY(sDanglingCommitId != sKeepCommitId);
        QVERIFY(oRepository.tipCommit()->files().contains(sDanglingPath));
        sDanglingObjectId = oRepository.tipCommit()->files()[sDanglingPath];
        QVERIFY(not sDanglingObjectId.isEmpty());
        QVERIFY(sDanglingObjectId != sKeepObjectId);
    }

    // Force deletion leaves D1 and its object unreachable
    SWITCH_TO_BRANCH("trunk");
    DELETE_BRANCH_FORCE(sResult, "prune-doomed");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));

    QString sDanglingCommitFile;
    QString sDanglingObjectFile;

    {
        Aze::CRepository oRepository(m_sRootPath);
        QVERIFY(oRepository.ok());
        QVERIFY(not QFile(oRepository.database()->composeBranchFileName("prune-doomed")).exists());
        sDanglingCommitFile = oRepository.database()->composeCommitFileName(sDanglingCommitId);
        sDanglingObjectFile = oRepository.database()->composeObjectFileName(sDanglingObjectId);
        QVERIFY(QFile(sDanglingCommitFile).exists());
        QVERIFY(QFile(sDanglingObjectFile).exists());
    }

    // A1 then amend leaves the first revision unreachable with its object
    QVERIFY(createFile(sAmendPath, sAmendBefore));
    QVERIFY(STAGE(sAmendPath) == expectedCode(Aze::CError::eNone));
    COMMIT("A1");

    QString sAmendedOldTipId;
    QString sAmendedOldObjectId;
    QStringList lAmendedParents;

    {
        Aze::CRepository oRepository(m_sRootPath);
        QVERIFY(oRepository.ok());
        sAmendedOldTipId = oRepository.tipCommit()->id();
        QVERIFY(not sAmendedOldTipId.isEmpty());
        lAmendedParents = oRepository.tipCommit()->parents();
        QVERIFY(oRepository.tipCommit()->files().contains(sAmendPath));
        sAmendedOldObjectId = oRepository.tipCommit()->files()[sAmendPath];
        QVERIFY(not sAmendedOldObjectId.isEmpty());
        QVERIFY(sAmendedOldObjectId != sKeepObjectId);
        QVERIFY(sAmendedOldObjectId != sDanglingObjectId);
    }

    QVERIFY(createFile(sAmendPath, sAmendAfter));
    QVERIFY(STAGE(sAmendPath) == expectedCode(Aze::CError::eNone));
    QVERIFY(AMEND("A1-amended", m_sAuthor) == expectedCode(Aze::CError::eNone));

    QString sTrunkTipId;
    QString sAmendedNewObjectId;

    {
        Aze::CRepository oRepository(m_sRootPath);
        QVERIFY(oRepository.ok());
        sTrunkTipId = oRepository.tipCommit()->id();
        QVERIFY(not sTrunkTipId.isEmpty());
        QVERIFY(sTrunkTipId != sAmendedOldTipId);
        QVERIFY(oRepository.tipCommit()->parents() == lAmendedParents);
        QCOMPARE(oRepository.tipCommit()->message(), QString("A1-amended"));
        QVERIFY(oRepository.tipCommit()->files().contains(sAmendPath));
        sAmendedNewObjectId = oRepository.tipCommit()->files()[sAmendPath];
        QVERIFY(not sAmendedNewObjectId.isEmpty());
        QVERIFY(sAmendedNewObjectId != sAmendedOldObjectId);
    }

    QString sAmendedOldCommitFile;
    QString sAmendedOldObjectFile;
    QString sTrunkTipFile;
    QString sKeepObjectFile;
    QString sAmendedNewObjectFile;

    {
        Aze::CRepository oRepository(m_sRootPath);
        QVERIFY(oRepository.ok());
        sAmendedOldCommitFile = oRepository.database()->composeCommitFileName(sAmendedOldTipId);
        sAmendedOldObjectFile = oRepository.database()->composeObjectFileName(sAmendedOldObjectId);
        sTrunkTipFile = oRepository.database()->composeCommitFileName(sTrunkTipId);
        sKeepObjectFile = oRepository.database()->composeObjectFileName(sKeepObjectId);
        sAmendedNewObjectFile = oRepository.database()->composeObjectFileName(sAmendedNewObjectId);
        QVERIFY(QFile(sAmendedOldCommitFile).exists());
        QVERIFY(QFile(sAmendedOldObjectFile).exists());
        QVERIFY(QFile(sTrunkTipFile).exists());
        QVERIFY(QFile(sKeepObjectFile).exists());
        QVERIFY(QFile(sAmendedNewObjectFile).exists());
    }

    // Dry run lists unreachable files without deleting them
    PRUNE_DRY_RUN(sResult);
    QVERIFY(sResult.contains("Pruned 2 commits and 2 objects."));
    QVERIFY(sResult.contains(sDanglingCommitId));
    QVERIFY(sResult.contains(sAmendedOldTipId));
    QVERIFY(sResult.contains(sDanglingObjectId));
    QVERIFY(sResult.contains(sAmendedOldObjectId));
    QVERIFY(QFile(sDanglingCommitFile).exists());
    QVERIFY(QFile(sAmendedOldCommitFile).exists());
    QVERIFY(QFile(sDanglingObjectFile).exists());
    QVERIFY(QFile(sAmendedOldObjectFile).exists());

    // Real prune removes only unreachable files
    PRUNE(sResult);
    QVERIFY(sResult.contains("Pruned 2 commits and 2 objects."));
    QVERIFY(sResult.contains(sDanglingCommitId));
    QVERIFY(sResult.contains(sAmendedOldTipId));
    QVERIFY(not QFile(sDanglingCommitFile).exists());
    QVERIFY(not QFile(sAmendedOldCommitFile).exists());
    QVERIFY(not QFile(sDanglingObjectFile).exists());
    QVERIFY(not QFile(sAmendedOldObjectFile).exists());

    // Reachable tips and referenced objects are preserved
    {
        Aze::CRepository oRepository(m_sRootPath);
        QVERIFY(oRepository.ok());
        QVERIFY(QFile(sTrunkTipFile).exists());
        QVERIFY(QFile(sKeepObjectFile).exists());
        QVERIFY(QFile(sAmendedNewObjectFile).exists());
        QCOMPARE(oRepository.tipCommit()->id(), sTrunkTipId);
        QVERIFY(oRepository.tipCommit()->files().contains(sKeepPath));
        QCOMPARE(oRepository.tipCommit()->files()[sKeepPath], sKeepObjectId);
        QVERIFY(oRepository.tipCommit()->files().contains(sAmendPath));
        QCOMPARE(oRepository.tipCommit()->files()[sAmendPath], sAmendedNewObjectId);
    }

    LOG(sResult);
    QVERIFY(sResult.contains("K1"));
    QVERIFY(sResult.contains("A1-amended"));

    // A fresh dangling commit is protected by the default grace period
    CREATE_BRANCH("prune-grace");
    SWITCH_TO_BRANCH("prune-grace");
    QVERIFY(createFile(sGracePath, sGraceContent));
    QVERIFY(STAGE(sGracePath) == expectedCode(Aze::CError::eNone));
    COMMIT("G1");

    QString sGraceCommitId;
    QString sGraceObjectId;

    {
        Aze::CRepository oRepository(m_sRootPath);
        QVERIFY(oRepository.ok());
        sGraceCommitId = oRepository.tipCommit()->id();
        QVERIFY(oRepository.tipCommit()->files().contains(sGracePath));
        sGraceObjectId = oRepository.tipCommit()->files()[sGracePath];
    }

    SWITCH_TO_BRANCH("trunk");
    DELETE_BRANCH_FORCE(sResult, "prune-grace");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));

    QString sGraceCommitFile;
    QString sGraceObjectFile;

    {
        Aze::CRepository oRepository(m_sRootPath);
        QVERIFY(oRepository.ok());
        sGraceCommitFile = oRepository.database()->composeCommitFileName(sGraceCommitId);
        sGraceObjectFile = oRepository.database()->composeObjectFileName(sGraceObjectId);
        QVERIFY(QFile(sGraceCommitFile).exists());
        QVERIFY(QFile(sGraceObjectFile).exists());
    }

    PRUNE_DEFAULT_RETENTION(sResult);
    QVERIFY(QFile(sGraceCommitFile).exists());
    QVERIFY(QFile(sGraceObjectFile).exists());

    PRUNE(sResult);
    QVERIFY(sResult.contains(sGraceCommitId));
    QVERIFY(sResult.contains(sGraceObjectId));
    QVERIFY(not QFile(sGraceCommitFile).exists());
    QVERIFY(not QFile(sGraceObjectFile).exists());
    QVERIFY(QFile(sTrunkTipFile).exists());
    QVERIFY(QFile(sKeepObjectFile).exists());
    QVERIFY(QFile(sAmendedNewObjectFile).exists());
}

//-------------------------------------------------------------------------------------------------
