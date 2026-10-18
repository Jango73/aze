
// Qt
#include <QDir>

// AzeLib
#include "CRepository.h"
#include "CRemoteRepository.h"
#include "CError.h"
#include "CStrings.h"
#include <CXMLNode.h>

// Application
#include "CTestTag.h"
#include "CTestMacros.h"

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Constructor.
* \param sArgument0 Binary path, forwarded to the fixture.
*/
/***************************************************************************/
CTestTag::CTestTag(const QString& sArgument0)
    : CTestFixture(sArgument0)
{
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests tag creation from the current branch tip.
*/
/***************************************************************************/
void CTestTag::testCreateTag()
{
    QString sResult;

    QString sFilePath = "Files/TagCreate.txt";
    QString sContent("Tagged content.");

    INIT();

    QVERIFY(createFile(sFilePath, sContent));
    QVERIFY(STAGE(sFilePath) == expectedCode(Aze::CError::eNone));
    COMMIT("C1");

    CREATE_TAG("v1.0");

    {
        Aze::CRepository oRepository(m_sRootPath);
        QVERIFY(oRepository.ok());

        Aze::CBranch* pTag = oRepository.database()->getBranch("v1.0", this);
        QVERIFY((pTag != nullptr));
        QVERIFY(pTag->isTag());
        QCOMPARE(pTag->tipCommitId(), oRepository.tipCommit()->id());
        delete pTag;

        // Dotted names stay intact through the branch folder listing
        QVector<Aze::CBranch*> lAllBranches = oRepository.database()->getAllBranches(this);
        bool bTagFound = false;

        for (Aze::CBranch* pListedBranch : lAllBranches)
        {
            if (pListedBranch->isTag() && pListedBranch->tipCommitId() == oRepository.tipCommit()->id())
                bTagFound = true;

            delete pListedBranch;
        }

        QVERIFY(bTagFound);
    }

    LIST_TAGS(sResult);
    QVERIFY(sResult.contains("v1.0"));

    // Creating the same tag again keeps the stored tip
    CREATE_TAG("v1.0");

    {
        Aze::CRepository oRepository(m_sRootPath);
        QVERIFY(oRepository.ok());

        Aze::CBranch* pTag = oRepository.database()->getBranch("v1.0", this);
        QVERIFY((pTag != nullptr));
        QVERIFY(pTag->isTag());
        QCOMPARE(pTag->tipCommitId(), oRepository.tipCommit()->id());
        delete pTag;
    }
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests branch and tag namespace guards.
*/
/***************************************************************************/
void CTestTag::testTagGuards()
{
    QString sFilePath = "Files/TagGuards.txt";
    QString sContent("Guarded content.");

    INIT();

    QVERIFY(createFile(sFilePath, sContent));
    QVERIFY(STAGE(sFilePath) == expectedCode(Aze::CError::eNone));
    COMMIT("C1");

    Aze::CRepository oRepository(m_sRootPath);
    QVERIFY(oRepository.ok());
    QVERIFY(oRepository.createTag("v1.0") == Aze::CError::eNone);

    // Branch commands refuse the tag name
    QVERIFY(oRepository.deleteBranch("v1.0") == Aze::CError::eBranchDoesNotExist);
    QVERIFY(oRepository.switchToBranch("v1.0") == Aze::CError::eUnknownBranch);

    // Tag commands refuse the branch name
    QVERIFY(oRepository.createBranch("side") == Aze::CError::eNone);
    QVERIFY(oRepository.deleteTag("side") == Aze::CError::eTagDoesNotExist);
    QVERIFY(oRepository.deleteBranch("side", true) == Aze::CError::eNone);

    // Creating a branch over a tag keeps the stored tag
    QVERIFY(oRepository.createBranch("v1.0") == Aze::CError::eNone);

    Aze::CBranch* pTag = oRepository.database()->getBranch("v1.0", this);
    QVERIFY((pTag != nullptr));
    QVERIFY(pTag->isTag());
    delete pTag;
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests tag deletion.
*/
/***************************************************************************/
void CTestTag::testDeleteTag()
{
    QString sResult;

    QString sFilePath = "Files/TagDelete.txt";
    QString sContent("Deleted content.");

    INIT();

    QVERIFY(createFile(sFilePath, sContent));
    QVERIFY(STAGE(sFilePath) == expectedCode(Aze::CError::eNone));
    COMMIT("C1");

    CREATE_TAG("v1.0");

    DELETE_TAG(sResult, "v1.0");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));

    {
        Aze::CRepository oRepository(m_sRootPath);
        QVERIFY(oRepository.ok());
        QVERIFY(not QFile(oRepository.database()->composeBranchFileName("v1.0")).exists());
    }

    LIST_TAGS(sResult);
    QVERIFY(not sResult.contains("v1.0"));

    // Deleting an unknown tag reports the tag code
    DELETE_TAG(sResult, "v1.0");
    QVERIFY(sResult == expectedCode(Aze::CError::eTagDoesNotExist));
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests tag push and pull through the branch protocol.
*/
/***************************************************************************/
void CTestTag::testPushPullTag()
{
    QString sDetail;

    QString sFilePath = "Files/TagPushPull.txt";
    QString sContent("Pushed tag content.");

    INIT();

    QVERIFY(createFile(sFilePath, sContent));
    QVERIFY(STAGE(sFilePath) == expectedCode(Aze::CError::eNone));
    COMMIT("C1");
    CREATE_TAG("v1.0");

    Aze::CRepository oSourceRepository(m_sRootPath);
    QVERIFY(oSourceRepository.ok());

    Aze::CBranch* pSourceTag = oSourceRepository.database()->getBranch("v1.0", this);
    QVERIFY((pSourceTag != nullptr));
    QString sTagTipId = pSourceTag->tipCommitId();
    QString sTagRootId = pSourceTag->rootCommitId();
    delete pSourceTag;
    QVERIFY(not sTagTipId.isEmpty());

    // Prepare an empty target repository outside of the source folder
    QDir oParent(m_sRootPath);
    QVERIFY(oParent.cdUp());
    QString sRemotePath = oParent.absoluteFilePath("aze-tests-tag-remote");
    QDir oRemoteFolder(sRemotePath);

    if (oRemoteFolder.exists())
        QVERIFY(oRemoteFolder.removeRecursively());

    QVERIFY(QDir().mkpath(sRemotePath));

    Aze::CRepository oTargetRepository(sRemotePath);
    QVERIFY(oTargetRepository.init() == Aze::CError::eNone);

    // Push the tag through the branch protocol with a JSON round-trip
    Aze::CRemoteRepository oSourceRemote(&oSourceRepository);
    Aze::CRemoteRepository oTargetRemote(&oTargetRepository);

    CXMLNode xPush = oSourceRemote.getPushRequestFor(
                "v1.0", sTagRootId, sTagTipId, Aze::CEnums::eTag);
    CXMLNode xReceived = CXMLNode::parseJSON(xPush.toJsonString());
    QVERIFY(oTargetRemote.applyPushRequest(xReceived, sDetail) == Aze::CError::eNone);

    {
        Aze::CRepository oCheckRepository(sRemotePath);
        QVERIFY(oCheckRepository.ok());

        Aze::CBranch* pPushedTag = oCheckRepository.database()->getBranch("v1.0", this);
        QVERIFY((pPushedTag != nullptr));
        QVERIFY(pPushedTag->isTag());
        QCOMPARE(pPushedTag->tipCommitId(), sTagTipId);
        delete pPushedTag;
    }

    // Pushing the same tip again converges
    QVERIFY(oTargetRemote.applyPushRequest(xReceived, sDetail) == Aze::CError::eNone);

    // Pull the tag into a fresh repository holding no tag
    QString sPullPath = oParent.absoluteFilePath("aze-tests-tag-pull");
    QDir oPullFolder(sPullPath);

    if (oPullFolder.exists())
        QVERIFY(oPullFolder.removeRecursively());

    QVERIFY(QDir().mkpath(sPullPath));

    Aze::CRepository oPullRepository(sPullPath);
    QVERIFY(oPullRepository.init() == Aze::CError::eNone);

    Aze::CRemoteRepository oPullRemote(&oPullRepository);
    CXMLNode xPullRequest = oPullRemote.getPullRequestFor("v1.0", "");
    CXMLNode xPullReceived = CXMLNode::parseJSON(xPullRequest.toJsonString());
    CXMLNode xPullResponse = oTargetRemote.getPullResponse(xPullReceived);
    CXMLNode xResponseReceived = CXMLNode::parseJSON(xPullResponse.toJsonString());
    QVERIFY(oPullRemote.applyPullResponse(xResponseReceived, sDetail) == Aze::CError::eNone);

    {
        Aze::CRepository oCheckRepository(sPullPath);
        QVERIFY(oCheckRepository.ok());

        Aze::CBranch* pPulledTag = oCheckRepository.database()->getBranch("v1.0", this);
        QVERIFY((pPulledTag != nullptr));
        QVERIFY(pPulledTag->isTag());
        QCOMPARE(pPulledTag->tipCommitId(), sTagTipId);
        delete pPulledTag;
    }

    // Pushing another tip over the tag is refused on the server
    QVERIFY(createFile(sFilePath, QString("Moved content.")));
    QVERIFY(STAGE(sFilePath) == expectedCode(Aze::CError::eNone));
    COMMIT("C2");

    Aze::CRepository oMovedRepository(m_sRootPath);
    QVERIFY(oMovedRepository.ok());

    Aze::CRemoteRepository oMovedRemote(&oMovedRepository);
    CXMLNode xMove = oMovedRemote.getPushRequestFor(
                "v1.0", sTagRootId, oMovedRepository.tipCommit()->id(), Aze::CEnums::eTag);
    CXMLNode xMoveReceived = CXMLNode::parseJSON(xMove.toJsonString());
    QVERIFY(oTargetRemote.applyPushRequest(xMoveReceived, sDetail) == Aze::CError::eTagMoveRefused);

    QVERIFY(QDir(sRemotePath).removeRecursively());
    QVERIFY(QDir(sPullPath).removeRecursively());
}

//-------------------------------------------------------------------------------------------------
