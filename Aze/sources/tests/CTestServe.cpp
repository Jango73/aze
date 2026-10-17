
// Qt
#include <QDir>
#include <QFile>

// AzeLib
#include "CRepository.h"
#include "CRemoteRepository.h"
#include "CError.h"
#include "CStrings.h"
#include <CXMLNode.h>

// Application
#include "../server/CAzeServerProcessor.h"
#include "CTestServe.h"
#include "CTestMacros.h"

// Exposes the protected server handlers to the test without touching
// production headers: servePush and serveDeleteBranch are static,
// so no server instance is needed.
class CServePushProbe : public CAzeServerProcessor
{
public:
    using CAzeServerProcessor::servePush;
    using CAzeServerProcessor::serveDeleteBranch;
    using CAzeServerProcessor::resolveRepositoryPath;
};

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Constructor.
* \param sArgument0 Binary path, forwarded to the fixture.
*/
/***************************************************************************/
CTestServe::CTestServe(const QString& sArgument0)
    : CTestFixture(sArgument0)
{
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests push request applying through the server layer.
*/
/***************************************************************************/
void CTestServe::testServePush()
{
    QString sFilePath = "Files/ServePush.txt";
    QString sContent("Served push content.");

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

    // Simulate the HTTP wire with a JSON round-trip of the POST body
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

    // Apply the push request through the server layer
    CWebContext tContext;
    CXMLNode xResponse = CServePushProbe::servePush(&oTargetRepository, tContext, xReceived);

    // Simulate the HTTP wire back with a JSON round-trip of the response body
    CXMLNode xDelivered = CXMLNode::parseJSON(xResponse.toJsonString());
    QVERIFY(Aze::CError::fromResultNode(xDelivered) == Aze::CError::eNone);

    // Check the pushed branch tip from a fresh repository instance
    Aze::CRepository oCheckRepository(sRemotePath);
    QVERIFY(oCheckRepository.ok());
    QCOMPARE(oCheckRepository.tipCommit()->id(), oSourceRepository.tipCommit()->id());

    // Check file content round-trips through transferred objects
    Aze::CCommit* pTipCommit = oCheckRepository.database()->getCommit(oCheckRepository.tipCommit()->id(), this);
    QCOMPARE(QString(pTipCommit->fileContent(oCheckRepository.database(), sFilePath)), sContent);
    delete pTipCommit;

    // Pushing the same tip again through the server succeeds
    {
        Aze::CRepository oFreshTarget(sRemotePath);
        QVERIFY(oFreshTarget.ok());
        CXMLNode xAgain = CServePushProbe::servePush(&oFreshTarget, tContext, xReceived);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xAgain.toJsonString())) == Aze::CError::eNone);
    }

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

    {
        Aze::CRepository oFreshTarget(sRemotePath);
        QVERIFY(oFreshTarget.ok());
        CXMLNode xRejected = CServePushProbe::servePush(&oFreshTarget, tContext, xRewrittenReceived);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xRejected.toJsonString())) == Aze::CError::ePushNotFastForward);
    }

    // Push with an unknown tip is refused
    CXMLNode xBadRequest = CXMLNode::parseJSON(xRequest.toJsonString());
    xBadRequest.attributes()[Aze::CStrings::s_sParamTip] = "00";

    {
        Aze::CRepository oFreshTarget(sRemotePath);
        QVERIFY(oFreshTarget.ok());
        CXMLNode xBadResponse = CServePushProbe::servePush(&oFreshTarget, tContext, xBadRequest);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xBadResponse.toJsonString())) == Aze::CError::ePushRequestInvalid);
    }

    QVERIFY(QDir(sRemotePath).removeRecursively());
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests delete-branch request applying through the server layer.
*/
/***************************************************************************/
void CTestServe::testServeDeleteBranch()
{
    QString sFilePath = "Files/ServeDeleteBranch.txt";
    QString sContent("Served delete branch content.");

    INIT();

    QVERIFY(createFile(sFilePath, sContent));
    STAGE(sFilePath);
    COMMIT("P1");

    CREATE_BRANCH("doomed");

    Aze::CRepository oServerRepository(m_sRootPath);
    QVERIFY(oServerRepository.ok());

    Aze::CRemoteRepository oServerRemote(&oServerRepository);
    CWebContext tContext;

    // The delete-branch request carries the branch name and the force flag
    CXMLNode xRequest = oServerRemote.getDeleteBranchRequest("doomed", false);
    QVERIFY(xRequest.attributes()[Aze::CStrings::s_sParamRequest] == Aze::CStrings::s_sParamDeleteBranch);
    QCOMPARE(xRequest.attributes()[Aze::CStrings::s_sParamBranch], QString("doomed"));

    // Simulate the HTTP wire with a JSON round-trip of the POST body
    CXMLNode xReceived = CXMLNode::parseJSON(xRequest.toJsonString());
    QVERIFY(xReceived.attributes()[Aze::CStrings::s_sParamBranch] == QString("doomed"));

    // Apply the delete-branch request through the server layer
    CXMLNode xResponse = CServePushProbe::serveDeleteBranch(&oServerRepository, tContext, xReceived);

    // Simulate the HTTP wire back with a JSON round-trip of the response body
    QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xResponse.toJsonString())) == Aze::CError::eNone);

    // Check the branch file is gone from a fresh repository instance
    {
        Aze::CRepository oCheckRepository(m_sRootPath);
        QVERIFY(oCheckRepository.ok());
        QVERIFY(not QFile(oCheckRepository.database()->composeBranchFileName("doomed")).exists());
    }

    // Deleting an unknown branch through the server is refused
    {
        Aze::CRepository oFreshServer(m_sRootPath);
        QVERIFY(oFreshServer.ok());
        Aze::CRemoteRepository oFreshRemote(&oFreshServer);
        CXMLNode xUnknown = CXMLNode::parseJSON(oFreshRemote.getDeleteBranchRequest("no-such-branch", false).toJsonString());
        CXMLNode xUnknownResponse = CServePushProbe::serveDeleteBranch(&oFreshServer, tContext, xUnknown);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xUnknownResponse.toJsonString())) == Aze::CError::eBranchDoesNotExist);
    }

    // Deleting without a branch name through the server is refused
    {
        Aze::CRepository oFreshServer(m_sRootPath);
        QVERIFY(oFreshServer.ok());
        CXMLNode xBad("");
        xBad.attributes()[Aze::CStrings::s_sParamRequest] = Aze::CStrings::s_sParamDeleteBranch;
        CXMLNode xBadReceived = CXMLNode::parseJSON(xBad.toJsonString());
        CXMLNode xBadResponse = CServePushProbe::serveDeleteBranch(&oFreshServer, tContext, xBadReceived);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xBadResponse.toJsonString())) == Aze::CError::eDeleteBranchRequestInvalid);
    }

    // Deleting the current branch through the server is refused
    {
        Aze::CRepository oFreshServer(m_sRootPath);
        QVERIFY(oFreshServer.ok());
        Aze::CRemoteRepository oFreshRemote(&oFreshServer);
        CXMLNode xCurrent = CXMLNode::parseJSON(oFreshRemote.getDeleteBranchRequest(oFreshServer.currentBranchName(), true).toJsonString());
        CXMLNode xCurrentResponse = CServePushProbe::serveDeleteBranch(&oFreshServer, tContext, xCurrent);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xCurrentResponse.toJsonString())) == Aze::CError::eCannotDeleteCurrentBranch);
    }

    // Deleting an unmerged branch without force is refused, with force it succeeds
    CREATE_BRANCH("unmerged");
    SWITCH_TO_BRANCH("unmerged");
    QVERIFY(createFile(sFilePath, QString("Diverged content.")));
    STAGE(sFilePath);
    COMMIT("U1");
    SWITCH_TO_BRANCH(Aze::CStrings::s_sDefaultBranchName);

    {
        Aze::CRepository oFreshServer(m_sRootPath);
        QVERIFY(oFreshServer.ok());
        Aze::CRemoteRepository oFreshRemote(&oFreshServer);
        CXMLNode xUnmerged = CXMLNode::parseJSON(oFreshRemote.getDeleteBranchRequest("unmerged", false).toJsonString());
        CXMLNode xUnmergedResponse = CServePushProbe::serveDeleteBranch(&oFreshServer, tContext, xUnmerged);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xUnmergedResponse.toJsonString())) == Aze::CError::eCannotDeleteUnmergedBranch);
    }

    {
        Aze::CRepository oFreshServer(m_sRootPath);
        QVERIFY(oFreshServer.ok());
        Aze::CRemoteRepository oFreshRemote(&oFreshServer);
        CXMLNode xForced = CXMLNode::parseJSON(oFreshRemote.getDeleteBranchRequest("unmerged", true).toJsonString());
        QVERIFY(xForced.attributes()[Aze::CStrings::s_sParamForce] == QString("true"));
        CXMLNode xForcedResponse = CServePushProbe::serveDeleteBranch(&oFreshServer, tContext, xForced);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xForcedResponse.toJsonString())) == Aze::CError::eNone);
    }

    {
        Aze::CRepository oCheckRepository(m_sRootPath);
        QVERIFY(oCheckRepository.ok());
        QVERIFY(not QFile(oCheckRepository.database()->composeBranchFileName("unmerged")).exists());
    }
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests repository path confinement under the server base folder.
*/
/***************************************************************************/
void CTestServe::testServePathIsolation()
{
    const QString sBase = QDir(m_sRootPath).absolutePath();
    QString sResolved;

    // A plain repository name stays under the base folder
    QVERIFY(CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << "repo", sResolved));
    QCOMPARE(sResolved, QDir::cleanPath(sBase + "/repo"));

    // Nested folders under the base folder are accepted
    QVERIFY(CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << "group" << "repo", sResolved));
    QCOMPARE(sResolved, QDir::cleanPath(sBase + "/group/repo"));

    // Parent folder markers are refused wherever they appear
    QVERIFY(not CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << "..", sResolved));
    QVERIFY(not CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << "repo" << ".." << "other", sResolved));

    // Current folder markers and empty segments are refused
    QVERIFY(not CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << ".", sResolved));
    QVERIFY(not CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << "", sResolved));
    QVERIFY(not CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList(), sResolved));

    // Embedded separators and drive specifications are refused
    QVERIFY(not CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << "a/b", sResolved));
    QVERIFY(not CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << "a\\b", sResolved));
    QVERIFY(not CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << "C:", sResolved));
}

//-------------------------------------------------------------------------------------------------
