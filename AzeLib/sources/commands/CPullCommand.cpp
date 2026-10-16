
// Qt
#include <QDebug>
#include <QDir>
#include <QFile>

// Application
#include "CPullCommand.h"
#include "../CRepository.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CPullCommand. \br\br
    \a pLocalRepository is the repository receiving the pull \br
    \a xResponse is the pull payload built by CRemoteRepository::getPullResponse \br
    \a sDetail receives message detail (object identifier, branch name) on failure \br
*/
CPullCommand::CPullCommand(CRepository* pLocalRepository, const CXMLNode& xResponse, QString& sDetail)
    : CBaseCommand(pLocalRepository)
    , m_xResponse(xResponse)
    , m_sDetail(sDetail)
{
}

/************************************************************************/

/*!
    Applies the pull response to the local repository. \br\br
    Stores received objects and commits, then updates the pulled branch.
    The current branch is fast forwarded when it is an ancestor of the
    received tip, merged otherwise through a temporary branch. \br
    A divergent merge leaves conflict markers in the working folder and
    the merge parents in the stage, like a local merge. \br
    Returns eNone, eMergeHasConflicts or the precise failure reason. \br
*/
CError::EErrorCode CPullCommand::execute()
{
    CError::EErrorCode eCode = validateResponse();

    if (eCode != CError::eNone)
        return eCode;

    if (m_sServerTipId.isEmpty())
        return handleEmptyServerTip();

    eCode = storeObjects();

    if (eCode != CError::eNone)
        return eCode;

    eCode = storeCommits();

    if (eCode != CError::eNone)
        return eCode;

    if (m_lCommitIds.isEmpty() || not m_lCommitIds.contains(m_sServerTipId))
    {
        m_sDetail = CError::message(CError::ePullRequestInvalid);
        return CError::ePullRequestInvalid;
    }

    return updateBranch();
}

/************************************************************************/

/*!
    Refuses a response which is not a pull payload with a branch name. \br\br
    Error nodes are forwarded, then the branch name and the server tip
    identifier are cached into the command properties. \br
*/
CError::EErrorCode CPullCommand::validateResponse()
{
    CError::EErrorCode eCheckCode = CError::fromResultNode(m_xResponse);

    if (eCheckCode != CError::eInvalidResponse)
        return eCheckCode;

    if (m_xResponse.attributes()[CStrings::s_sParamRequest] != CStrings::s_sParamPull)
    {
        m_sDetail = CError::message(CError::ePullRequestInvalid);
        return CError::ePullRequestInvalid;
    }

    m_sBranchName = m_xResponse.attributes()[CStrings::s_sParamBranch];
    m_sServerTipId = m_xResponse.attributes()[CStrings::s_sParamTip];

    if (m_sBranchName.isEmpty())
    {
        m_sDetail = CError::message(CError::ePullRequestInvalid);
        return CError::ePullRequestInvalid;
    }

    return CError::eNone;
}

/************************************************************************/

/*!
    Stores the object nodes of the pull response. \br\br
    Sets \a m_sDetail and returns the failure reason when a node is invalid. \br
*/
CError::EErrorCode CPullCommand::storeObjects()
{
    return storePayloadObjects(m_xResponse, m_sDetail, CError::ePullRequestInvalid);
}

/************************************************************************/

/*!
    Stores the commit nodes of the pull response into m_lCommitIds. \br\br
    Sets \a m_sDetail and returns the failure reason when a node is invalid. \br
*/
CError::EErrorCode CPullCommand::storeCommits()
{
    m_lCommitIds.clear();

    return storePayloadCommits(m_xResponse, m_lCommitIds, m_sDetail, CError::ePullRequestInvalid);
}

/************************************************************************/

/*!
    Updates the pulled branch with fast forward or merge. \br\br
    A missing branch file is created, another branch is moved directly,
    the current branch is fast forwarded when possible and merged otherwise. \br
*/
CError::EErrorCode CPullCommand::updateBranch()
{
    QString sBranchFileName = m_pRepository->database()->composeBranchFileName(m_sBranchName);

    if (not QFile(sBranchFileName).exists())
        return createMissingBranch();

    if (m_sBranchName != m_pRepository->currentBranchName())
        return updateOtherBranch();

    return updateCurrentBranch();
}

/************************************************************************/

/*!
    Creates the branch file when the server tip is empty. \br\br
    An empty tip carries no commit, the branch is created empty when missing. \br
*/
CError::EErrorCode CPullCommand::handleEmptyServerTip()
{
    QString sBranchFileName = m_pRepository->database()->composeBranchFileName(m_sBranchName);

    if (not QFile(sBranchFileName).exists())
    {
        CBranch* pNewBranch = new CBranch(m_pRepository);
        pNewBranch->toNode().save(sBranchFileName);
        delete pNewBranch;
    }

    return CError::eNone;
}

/************************************************************************/

/*!
    Creates a missing branch file and checks out the tip when current. \br\br
    Author and root travel with the response, the tip files are extracted
    into the working folder when the pulled branch is the current one. \br
*/
CError::EErrorCode CPullCommand::createMissingBranch()
{
    CDatabase* pDatabase = m_pRepository->database();
    QString sBranchFileName = pDatabase->composeBranchFileName(m_sBranchName);

    CBranch* pNewBranch = new CBranch(m_pRepository);

    pNewBranch->setAuthor(m_xResponse.attributes()[CStrings::s_sParamAuthor]);
    pNewBranch->setRootCommitId(m_xResponse.attributes()[CStrings::s_sParamRoot]);
    pNewBranch->setTipCommitId(m_sServerTipId);

    if (pNewBranch->rootCommitId().isEmpty())
        pNewBranch->setRootCommitId(m_sServerTipId);

    if (pNewBranch->author().isEmpty())
    {
        CCommit* pTipCommit = m_pRepository->commitFunctions()->loadCommit(m_sServerTipId, m_pRepository);

        if (IS_NOT_NULL(pTipCommit))
        {
            pNewBranch->setAuthor(pTipCommit->author());
            delete pTipCommit;
        }
    }

    pNewBranch->toNode().save(sBranchFileName);
    delete pNewBranch;

    if (m_sBranchName != m_pRepository->currentBranchName())
        return CError::eNone;

    m_pRepository->refreshCurrentBranch();

    return checkoutTipFiles(m_sServerTipId);
}

/************************************************************************/

/*!
    Moves the tip of a branch which is not the current one. \br\br
    No working folder update is needed when the pulled branch is not checked out. \br
*/
CError::EErrorCode CPullCommand::updateOtherBranch()
{
    CDatabase* pDatabase = m_pRepository->database();
    QString sBranchFileName = pDatabase->composeBranchFileName(m_sBranchName);

    CBranch* pBranch = pDatabase->getBranch(m_sBranchName, m_pRepository);

    if (IS_NULL(pBranch))
    {
        m_sDetail = m_sBranchName;
        return CError::eBranchDoesNotExist;
    }

    pBranch->setTipCommitId(m_sServerTipId);
    pBranch->toNode().save(sBranchFileName);
    delete pBranch;

    return CError::eNone;
}

/************************************************************************/

/*!
    Fast forwards or merges the current branch onto the server tip. \br\br
    An empty local tip is fast forwarded with a working folder checkout,
    a local tip contained in the server history is fast forwarded as well,
    a server tip already contained locally needs no update, other cases
    are merged through a temporary branch. \br
*/
CError::EErrorCode CPullCommand::updateCurrentBranch()
{
    m_pRepository->refreshCurrentBranch();

    QString sLocalTipId;

    if (not m_pRepository->currentBranch().isNull())
        sLocalTipId = m_pRepository->currentBranch()->tipCommitId();

    if (sLocalTipId == m_sServerTipId)
        return CError::eNone;

    if (sLocalTipId.isEmpty())
    {
        m_pRepository->currentBranch()->setTipCommitId(m_sServerTipId);
        m_pRepository->writeCurrentBranch();

        return checkoutTipFiles(m_sServerTipId);
    }

    QList<QPair<int, QString>> lServerAncestors = m_pRepository->commitFunctions()->getCommitAncestorList(
                m_sServerTipId,
                false,
                0
                );

    bool bServerContainsLocal = false;

    for (const QPair<int, QString>& tAncestor : lServerAncestors)
    {
        if (tAncestor.second == sLocalTipId)
        {
            bServerContainsLocal = true;
            break;
        }
    }

    if (not bServerContainsLocal)
    {
        QList<QPair<int, QString>> lLocalAncestors = m_pRepository->commitFunctions()->getCommitAncestorList(
                    sLocalTipId,
                    false,
                    0
                    );

        for (const QPair<int, QString>& tAncestor : lLocalAncestors)
        {
            if (tAncestor.second == m_sServerTipId)
                return CError::eNone;
        }
    }

    return mergeServerTip(m_sServerTipId);
}

/************************************************************************/

/*!
    Extracts the tip commit files into the working folder. \br\br
    Every file of \a sTipId is read from the object database and written
    to the working folder. \br
    Sets \a m_sDetail and returns the failure reason when missing. \br
*/
CError::EErrorCode CPullCommand::checkoutTipFiles(const QString& sTipId)
{
    CDatabase* pDatabase = m_pRepository->database();

    CCommit* pTipCommit = m_pRepository->commitFunctions()->loadCommit(sTipId, m_pRepository);

    if (IS_NULL(pTipCommit))
    {
        m_sDetail = sTipId;
        return CError::ePullRequestInvalid;
    }

    QDictionary mFiles = pTipCommit->files();
    delete pTipCommit;

    for (const QString& sRelativeFileName : mFiles.keys())
    {
        QByteArray baData = pDatabase->getObject(mFiles[sRelativeFileName]);

        if (baData.isEmpty())
        {
            m_sDetail = sRelativeFileName;
            return CError::eObjectStoreFailed;
        }

        if (not CUtils::putBinaryFileContent(pDatabase->composeLocalFileName(sRelativeFileName), baData))
        {
            m_sDetail = sRelativeFileName;
            return CError::eObjectStoreFailed;
        }
    }

    return CError::eNone;
}

/************************************************************************/

/*!
    Merges the server tip through a temporary branch. \br\br
    The server tip is exposed as a temporary branch, merged with
    CRepository::merge, then the temporary branch file is removed.
    The current branch and the stage are written on success. \br
    Returns eNone, eMergeHasConflicts or the precise failure reason. \br
*/
CError::EErrorCode CPullCommand::mergeServerTip(const QString& sServerTipId)
{
    CDatabase* pDatabase = m_pRepository->database();

    const QString sTemporaryBranchName("__aze_pull_tmp__");
    QString sTemporaryFileName = pDatabase->composeBranchFileName(sTemporaryBranchName);

    QFile::remove(sTemporaryFileName);

    CBranch* pTemporaryBranch = new CBranch(m_pRepository);
    pTemporaryBranch->setTipCommitId(sServerTipId);
    pTemporaryBranch->toNode().save(sTemporaryFileName);
    delete pTemporaryBranch;

    bool bHasConflicts = false;
    CError::EErrorCode eMergeCode = m_pRepository->merge(sTemporaryBranchName, bHasConflicts, true);

    QFile::remove(sTemporaryFileName);

    if (eMergeCode == CError::eNothingToMerge)
        return CError::eNone;

    if (eMergeCode != CError::eNone && eMergeCode != CError::eMergeHasConflicts)
        return eMergeCode;

    m_pRepository->writeCurrentBranch();
    m_pRepository->writeStage();

    if (bHasConflicts || eMergeCode == CError::eMergeHasConflicts)
        return CError::eMergeHasConflicts;

    return CError::eNone;
}

/************************************************************************/

} // namespace Aze
