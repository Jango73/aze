
// Qt
#include <QDebug>
#include <QDir>
#include <QFile>

// Application
#include "CPushCommand.h"
#include "../CRepository.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CPushCommand. \br\br
    \a pTargetRepository is the repository receiving the push, usually owned
    by the server serving the request \br
    \a xRequest is the push payload built by CRemoteRepository::getPushRequest \br
    \a sDetail receives message detail (object identifier, branch name) on failure \br
*/
CPushCommand::CPushCommand(CRepository* pTargetRepository, const CXMLNode& xRequest, QString& sDetail)
    : CBaseCommand(pTargetRepository)
    , m_xRequest(xRequest)
    , m_sDetail(sDetail)
{
}

/************************************************************************/

/*!
    Applies the push request to the target repository. \br\br
    Stores received objects and commits, then updates the pushed branch.
    History loss is refused when the current tip is missing from the
    pushed commits. \br
    Returns eNone on success, the precise failure reason otherwise. \br
*/
CError::EErrorCode CPushCommand::execute()
{
    CError::EErrorCode eCode = validateRequest();

    if (eCode != CError::eNone)
        return eCode;

    eCode = storeObjects();

    if (eCode != CError::eNone)
        return eCode;

    eCode = storeCommits();

    if (eCode != CError::eNone)
        return eCode;

    if (m_lCommitIds.isEmpty() || not m_lCommitIds.contains(m_sTipId))
    {
        m_sDetail = CError::message(CError::ePushRequestInvalid);
        return CError::ePushRequestInvalid;
    }

    return updateBranch();
}

/************************************************************************/

/*!
    Refuses a request which is not a push payload with branch and tip. \br\br
    Caches the branch name, the requested root identifier and the tip
    identifier into the command properties. \br
*/
CError::EErrorCode CPushCommand::validateRequest()
{
    m_sBranchName = m_xRequest.attributes()[CStrings::s_sParamBranch];
    m_sRequestedRootId = m_xRequest.attributes()[CStrings::s_sParamRoot];
    m_sTipId = m_xRequest.attributes()[CStrings::s_sParamTip];

    if (m_xRequest.attributes()[CStrings::s_sParamRequest] != CStrings::s_sParamPush
            || m_sBranchName.isEmpty()
            || m_sTipId.isEmpty())
    {
        m_sDetail = CError::message(CError::ePushRequestInvalid);
        return CError::ePushRequestInvalid;
    }

    return CError::eNone;
}

/************************************************************************/

/*!
    Stores the object nodes of the push request. \br\br
    Sets \a m_sDetail and returns the failure reason when a node is invalid. \br
*/
CError::EErrorCode CPushCommand::storeObjects()
{
    return storePayloadObjects(m_xRequest, m_sDetail, CError::ePushRequestInvalid);
}

/************************************************************************/

/*!
    Stores the commit nodes of the push request into m_lCommitIds. \br\br
    Sets \a m_sDetail and returns the failure reason when a node is invalid. \br
*/
CError::EErrorCode CPushCommand::storeCommits()
{
    m_lCommitIds.clear();

    return storePayloadCommits(m_xRequest, m_lCommitIds, m_sDetail, CError::ePushRequestInvalid);
}

/************************************************************************/

/*!
    Updates the pushed branch from the stored commits. \br\br
    Refuses to overwrite history that the pushed commits do not contain.
    A created branch keeps the requested root so the branch point is
    preserved. \br
    Sets \a m_sDetail and returns the failure reason when refused. \br
*/
CError::EErrorCode CPushCommand::updateBranch()
{
    CDatabase* pDatabase = m_pRepository->database();
    QString sBranchFileName = pDatabase->composeBranchFileName(m_sBranchName);

    CBranch* pBranch = nullptr;

    if (QFile(sBranchFileName).exists())
    {
        pBranch = pDatabase->getBranch(m_sBranchName, this);

        // Refuse to overwrite history that the pushed commits do not contain
        if (not pBranch->tipCommitId().isEmpty()
                && pBranch->tipCommitId() != m_sTipId
                && not m_lCommitIds.contains(pBranch->tipCommitId()))
        {
            m_sDetail = CError::message(CError::ePushNotFastForward);
            delete pBranch;
            return CError::ePushNotFastForward;
        }

        if (pBranch->rootCommitId().isEmpty() && not m_sRequestedRootId.isEmpty())
        {
            pBranch->setRootCommitId(m_sRequestedRootId);
        }
    }
    else
    {
        pBranch = new CBranch(this);

        CCommit* pTipCommit = m_pRepository->commitFunctions()->loadCommit(m_sTipId, this);
        pBranch->setAuthor(pTipCommit->author());
        delete pTipCommit;

        if (not m_sRequestedRootId.isEmpty())
        {
            pBranch->setRootCommitId(m_sRequestedRootId);
        }
        else
        {
            pBranch->setRootCommitId(resolveRootCommitId());
        }
    }

    pBranch->setTipCommitId(m_sTipId);
    pBranch->toNode().save(sBranchFileName);
    delete pBranch;

    if (m_sBranchName == m_pRepository->currentBranchName())
    {
        m_pRepository->refreshCurrentBranch();
        m_pRepository->invalidateCaches();
    }

    return CError::eNone;
}

/************************************************************************/

/*!
    Returns the root identifier for a created branch. \br\br
    Uses the requested root when present, else the oldest pushed commit
    without parents, else the pushed tip. \br
*/
QString CPushCommand::resolveRootCommitId()
{
    if (not m_sRequestedRootId.isEmpty())
        return m_sRequestedRootId;

    // Fallback for requests without a root, oldest pushed commit without parents
    QString sRootId = m_sTipId;

    for (int iIndex = m_lCommitIds.count() - 1; iIndex >= 0; --iIndex)
    {
        CCommit* pCommit = m_pRepository->commitFunctions()->loadCommit(m_lCommitIds[iIndex], this);

        if (IS_NOT_NULL(pCommit))
        {
            if (pCommit->parents().isEmpty())
                sRootId = pCommit->id();

            delete pCommit;
        }
    }

    return sRootId;
}

/************************************************************************/

} // namespace Aze
