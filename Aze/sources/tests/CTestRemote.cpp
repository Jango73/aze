
// Qt
#include <QDir>

// AzeLib
#include "CRepository.h"
#include "CRemoteRepository.h"
#include "CError.h"
#include "CUtils.h"
#include "CStrings.h"
#include <CXMLNode.h>

// Application
#include "../common/CConstants.h"
#include "CTestRemote.h"
#include "CTestMacros.h"

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Constructor.
* \param sArgument0 Binary path, forwarded to the fixture.
*/
/***************************************************************************/
CTestRemote::CTestRemote(const QString& sArgument0)
    : CTestFixture(sArgument0)
{
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests push request building and applying to another repository.
*/
/***************************************************************************/
void CTestRemote::testRemotePush()
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

/***************************************************************************/
/*!
* \brief Tests pull convergence after push then pull.
*/
/***************************************************************************/
void CTestRemote::testPull()
{
    QString sFilePath = "Files/PullTest.txt";
    QString sContent1("Pulled line1.\nPulled content v1.\n");
    QString sContent2("Pulled line1.\nPulled content v2.\n");
    QString sLocalContent("Pulled line1.\nLocal divergent content.\n");
    QString sRemoteContent("Pulled line1.\nRemote divergent content.\n");
    QString sDetail;

    INIT();

    QVERIFY(createFile(sFilePath, sContent1));
    STAGE(sFilePath);
    COMMIT("P1");

    // The pull request carries the requested branch and the local tip
    Aze::CRepository oSourceRepository(m_sRootPath);
    QVERIFY(oSourceRepository.ok());

    Aze::CRemoteRepository oSourceRemote(&oSourceRepository);
    CXMLNode xPullRequest = oSourceRemote.getPullRequest();
    QVERIFY(xPullRequest.attributes()[Aze::CStrings::s_sParamRequest] == Aze::CStrings::s_sParamPull);
    QCOMPARE(xPullRequest.attributes()[Aze::CStrings::s_sParamBranch], oSourceRepository.currentBranchName());
    QCOMPARE(xPullRequest.attributes()[Aze::CStrings::s_sParamTip], oSourceRepository.tipCommit()->id());

    // Prepare an empty remote repository outside of the source folder
    QDir oParent(m_sRootPath);
    QVERIFY(oParent.cdUp());
    QString sRemotePath = oParent.absoluteFilePath("aze-tests-remote");
    QDir oRemoteFolder(sRemotePath);

    if (oRemoteFolder.exists())
        QVERIFY(oRemoteFolder.removeRecursively());

    QVERIFY(QDir().mkpath(sRemotePath));

    Aze::CRepository oRemoteRepository(sRemotePath);
    QVERIFY(oRemoteRepository.init() == Aze::CError::eNone);

    // Push the source tip to the remote repository
    Aze::CRemoteRepository oPushRemote(&oSourceRepository);
    CXMLNode xPushRequest = oPushRemote.getPushRequest();
    CXMLNode xPushReceived = CXMLNode::parseJSON(xPushRequest.toJsonString());

    Aze::CRemoteRepository oRemoteTarget(&oRemoteRepository);
    QVERIFY(oRemoteTarget.applyPushRequest(xPushReceived, sDetail) == Aze::CError::eNone);

    // Prepare an empty pull repository and pull the remote tip
    QString sPullPath = oParent.absoluteFilePath("aze-tests-pull");
    QDir oPullFolder(sPullPath);

    if (oPullFolder.exists())
        QVERIFY(oPullFolder.removeRecursively());

    QVERIFY(QDir().mkpath(sPullPath));

    Aze::CRepository oPullRepository(sPullPath);
    QVERIFY(oPullRepository.init() == Aze::CError::eNone);

    Aze::CRemoteRepository oPullRemote(&oPullRepository);
    CXMLNode xFirstPullRequest = CXMLNode::parseJSON(oPullRemote.getPullRequest().toJsonString());

    CXMLNode xFirstResponse;

    {
        Aze::CRepository oFreshRemote(sRemotePath);
        QVERIFY(oFreshRemote.ok());
        Aze::CRemoteRepository oFreshSource(&oFreshRemote);
        xFirstResponse = CXMLNode::parseJSON(oFreshSource.getPullResponse(xFirstPullRequest).toJsonString());
    }

    QVERIFY(xFirstResponse.attributes()[Aze::CStrings::s_sParamRequest] == Aze::CStrings::s_sParamPull);
    QVERIFY(oPullRemote.applyPullResponse(xFirstResponse, sDetail) == Aze::CError::eNone);

    // Content converges after push then pull
    Aze::CRepository oCheckPulledRepository(sPullPath);
    QVERIFY(oCheckPulledRepository.ok());
    QCOMPARE(oCheckPulledRepository.tipCommit()->id(), oSourceRepository.tipCommit()->id());
    QCOMPARE(Aze::CUtils::getTextFileContent(QString("%1/%2").arg(sPullPath).arg(sFilePath)), sContent1);

    // Advance the source, push, then pull again with fast forward
    QVERIFY(createFile(sFilePath, sContent2));
    STAGE(sFilePath);
    COMMIT("P2");

    Aze::CRepository oAdvancedRepository(m_sRootPath);
    QVERIFY(oAdvancedRepository.ok());

    Aze::CRemoteRepository oAdvancedRemote(&oAdvancedRepository);
    CXMLNode xSecondPush = CXMLNode::parseJSON(oAdvancedRemote.getPushRequest().toJsonString());
    QVERIFY(oRemoteTarget.applyPushRequest(xSecondPush, sDetail) == Aze::CError::eNone);

    Aze::CRepository oBehindRepository(sPullPath);
    Aze::CRemoteRepository oBehindRemote(&oBehindRepository);
    CXMLNode xSecondPullRequest = CXMLNode::parseJSON(oBehindRemote.getPullRequest().toJsonString());
    CXMLNode xSecondResponse;

    {
        Aze::CRepository oFreshRemote(sRemotePath);
        QVERIFY(oFreshRemote.ok());
        Aze::CRemoteRepository oFreshSource(&oFreshRemote);
        xSecondResponse = CXMLNode::parseJSON(oFreshSource.getPullResponse(xSecondPullRequest).toJsonString());
    }

    QVERIFY(oBehindRemote.applyPullResponse(xSecondResponse, sDetail) == Aze::CError::eNone);

    Aze::CRepository oConvergedRepository(sPullPath);
    QVERIFY(oConvergedRepository.ok());
    QCOMPARE(oConvergedRepository.tipCommit()->id(), oAdvancedRepository.tipCommit()->id());
    QCOMPARE(Aze::CUtils::getTextFileContent(QString("%1/%2").arg(sPullPath).arg(sFilePath)), sContent2);

    // Pulling again without new commits stays converged
    Aze::CRemoteRepository oUpToDateRemote(&oConvergedRepository);
    CXMLNode xUpToDateRequest = CXMLNode::parseJSON(oUpToDateRemote.getPullRequest().toJsonString());
    CXMLNode xUpToDateResponse;

    {
        Aze::CRepository oFreshRemote(sRemotePath);
        QVERIFY(oFreshRemote.ok());
        Aze::CRemoteRepository oFreshSource(&oFreshRemote);
        xUpToDateResponse = CXMLNode::parseJSON(oFreshSource.getPullResponse(xUpToDateRequest).toJsonString());
    }

    QVERIFY(oUpToDateRemote.applyPullResponse(xUpToDateResponse, sDetail) == Aze::CError::eNone);

    // Divergent histories report conflicts with markers, like a local merge
    QVERIFY(createFile(QString("%1/%2").arg(sPullPath).arg(sFilePath), sLocalContent));

        {
        Aze::CRepository oLocalRepository(sPullPath);
        QVERIFY(oLocalRepository.ok());
        QString sAbsolutePullFile = QString("%1/%2").arg(sPullPath).arg(sFilePath);
        QVERIFY(oLocalRepository.stage(QStringList() << sAbsolutePullFile) == Aze::CError::eNone);
        QVERIFY(oLocalRepository.writeStage() == Aze::CError::eNone);
        QVERIFY(oLocalRepository.commit(m_sAuthor, "Local") == Aze::CError::eNone);
        QVERIFY(oLocalRepository.writeCurrentBranch() == Aze::CError::eNone);
        QVERIFY(oLocalRepository.clearStage() == Aze::CError::eNone);
        QVERIFY(oLocalRepository.writeStage() == Aze::CError::eNone);
    }

    QVERIFY(createFile(sFilePath, sRemoteContent));
    STAGE(sFilePath);
    COMMIT("Remote");

    Aze::CRepository oDivergedSource(m_sRootPath);
    CXMLNode xDivergedPush = CXMLNode::parseJSON(Aze::CRemoteRepository(&oDivergedSource).getPushRequest().toJsonString());
    QVERIFY(oRemoteTarget.applyPushRequest(xDivergedPush, sDetail) == Aze::CError::eNone);

    Aze::CRepository oDivergedPull(sPullPath);
    Aze::CRemoteRepository oDivergedRemote(&oDivergedPull);
    CXMLNode xDivergedRequest = CXMLNode::parseJSON(oDivergedRemote.getPullRequest().toJsonString());
    CXMLNode xDivergedResponse;

    {
        Aze::CRepository oFreshRemote(sRemotePath);
        QVERIFY(oFreshRemote.ok());
        Aze::CRemoteRepository oFreshSource(&oFreshRemote);
        xDivergedResponse = CXMLNode::parseJSON(oFreshSource.getPullResponse(xDivergedRequest).toJsonString());
    }

    QVERIFY(oDivergedRemote.applyPullResponse(xDivergedResponse, sDetail) == Aze::CError::eMergeHasConflicts);

    QString sConflictedContent = Aze::CUtils::getTextFileContent(QString("%1/%2").arg(sPullPath).arg(sFilePath));
    QVERIFY(sConflictedContent.contains("<<<<<<<"));
    QVERIFY(sConflictedContent.contains(">>>>>>>"));
    QVERIFY(sConflictedContent.contains("Local divergent") && sConflictedContent.contains("Remote divergent"));

    // Resolve the conflict with the local content before pulling again
    QVERIFY(createFile(QString("%1/%2").arg(sPullPath).arg(sFilePath), sLocalContent));

    {
        Aze::CRepository oResolveRepository(sPullPath);
        QVERIFY(oResolveRepository.ok());
        QString sAbsoluteFile = QString("%1/%2").arg(sPullPath).arg(sFilePath);
        QVERIFY(oResolveRepository.stage(QStringList() << sAbsoluteFile) == Aze::CError::eNone);
        QVERIFY(oResolveRepository.writeStage() == Aze::CError::eNone);
        QVERIFY(oResolveRepository.commit(m_sAuthor, "Resolve") == Aze::CError::eNone);
        QVERIFY(oResolveRepository.writeCurrentBranch() == Aze::CError::eNone);
        QVERIFY(oResolveRepository.clearStage() == Aze::CError::eNone);
        QVERIFY(oResolveRepository.writeStage() == Aze::CError::eNone);
    }

    // Single line files without trailing newline keep conflict markers too
    QString sSinglePath = "Files/PullSingleLine.txt";
    QVERIFY(createFile(sSinglePath, "single"));
    STAGE(sSinglePath);
    COMMIT("Single base");

    Aze::CRepository oSingleSource(m_sRootPath);
    CXMLNode xSinglePush = CXMLNode::parseJSON(Aze::CRemoteRepository(&oSingleSource).getPushRequest().toJsonString());
    QVERIFY(oRemoteTarget.applyPushRequest(xSinglePush, sDetail) == Aze::CError::eNone);

    {
        Aze::CRepository oSinglePull(sPullPath);
        Aze::CRemoteRepository oSingleRemote(&oSinglePull);
        CXMLNode xSingleRequest = CXMLNode::parseJSON(oSingleRemote.getPullRequest().toJsonString());

        Aze::CRepository oFreshRemote(sRemotePath);
        QVERIFY(oFreshRemote.ok());
        Aze::CRemoteRepository oFreshSource(&oFreshRemote);
        CXMLNode xSingleResponse = CXMLNode::parseJSON(oFreshSource.getPullResponse(xSingleRequest).toJsonString());
        QVERIFY(oSingleRemote.applyPullResponse(xSingleResponse, sDetail) == Aze::CError::eNone);
    }

    QVERIFY(createFile(QString("%1/%2").arg(sPullPath).arg(sSinglePath), "local-single"));

    {
        Aze::CRepository oLocalRepository(sPullPath);
        QVERIFY(oLocalRepository.ok());
        QString sAbsoluteSingleFile = QString("%1/%2").arg(sPullPath).arg(sSinglePath);
        QVERIFY(oLocalRepository.stage(QStringList() << sAbsoluteSingleFile) == Aze::CError::eNone);
        QVERIFY(oLocalRepository.writeStage() == Aze::CError::eNone);
        QVERIFY(oLocalRepository.commit(m_sAuthor, "Local single") == Aze::CError::eNone);
        QVERIFY(oLocalRepository.writeCurrentBranch() == Aze::CError::eNone);
        QVERIFY(oLocalRepository.clearStage() == Aze::CError::eNone);
        QVERIFY(oLocalRepository.writeStage() == Aze::CError::eNone);
    }

    QVERIFY(createFile(sSinglePath, "remote-single"));
    STAGE(sSinglePath);
    COMMIT("Remote single");

    Aze::CRepository oSingleDiverged(m_sRootPath);
    CXMLNode xSingleDivergedPush = CXMLNode::parseJSON(Aze::CRemoteRepository(&oSingleDiverged).getPushRequest().toJsonString());
    QVERIFY(oRemoteTarget.applyPushRequest(xSingleDivergedPush, sDetail) == Aze::CError::eNone);

    Aze::CRepository oSinglePull(sPullPath);
    Aze::CRemoteRepository oSingleRemote(&oSinglePull);
    CXMLNode xSingleDivergedRequest = CXMLNode::parseJSON(oSingleRemote.getPullRequest().toJsonString());
    CXMLNode xSingleDivergedResponse;

    {
        Aze::CRepository oFreshRemote(sRemotePath);
        QVERIFY(oFreshRemote.ok());
        Aze::CRemoteRepository oFreshSource(&oFreshRemote);
        xSingleDivergedResponse = CXMLNode::parseJSON(oFreshSource.getPullResponse(xSingleDivergedRequest).toJsonString());
    }

    QVERIFY(oSingleRemote.applyPullResponse(xSingleDivergedResponse, sDetail) == Aze::CError::eMergeHasConflicts);

    QString sSingleConflicted = Aze::CUtils::getTextFileContent(QString("%1/%2").arg(sPullPath).arg(sSinglePath));
    QVERIFY(sSingleConflicted.contains("<<<<<<<"));
    QVERIFY(sSingleConflicted.contains(">>>>>>>"));
    QVERIFY(sSingleConflicted.contains("local-single") && sSingleConflicted.contains("remote-single"));

    // Unknown branches are refused with a distinct code
    CXMLNode xUnknownRequest("");
    xUnknownRequest.attributes()[Aze::CStrings::s_sParamRequest] = Aze::CStrings::s_sParamPull;
    xUnknownRequest.attributes()[Aze::CStrings::s_sParamBranch] = "no-such-branch";

    {
        Aze::CRepository oFreshRemote(sRemotePath);
        QVERIFY(oFreshRemote.ok());
        Aze::CRemoteRepository oFreshSource(&oFreshRemote);
        CXMLNode xUnknownResponse = CXMLNode::parseJSON(oFreshSource.getPullResponse(xUnknownRequest).toJsonString());
        QVERIFY(Aze::CError::fromResultNode(xUnknownResponse) == Aze::CError::eBranchDoesNotExist);
    }

    QVERIFY(QDir(sPullPath).removeRecursively());
    QVERIFY(QDir(sRemotePath).removeRecursively());
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests cloning of branches, branch tips and file contents.
*/
/***************************************************************************/
void CTestRemote::testClone()
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
