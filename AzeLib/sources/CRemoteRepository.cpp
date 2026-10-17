
// Qt
#include <QDebug>
#include <QDir>
#include <QFile>

// Application
#include "CRemoteRepository.h"
#include "commands/CPullCommand.h"
#include "commands/CPushCommand.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CRemoteRepository::CRemoteRepository(CRepository* pLocalRepository, QObject* parent)
    : CObject(parent)
    , m_pLocalRepository(pLocalRepository)
{
}

//-------------------------------------------------------------------------------------------------

CRemoteRepository::~CRemoteRepository()
{
}

//-------------------------------------------------------------------------------------------------

CXMLNode CRemoteRepository::getPullRequest()
{
    CXMLNode xRequest("");

    xRequest.attributes()[CStrings::s_sParamRequest] = CStrings::s_sParamPull;
    xRequest.attributes()[CStrings::s_sParamBranch] = m_pLocalRepository->currentBranchName();

    if (not m_pLocalRepository->tipCommit().isNull())
    {
        xRequest.attributes()[CStrings::s_sParamTip] = m_pLocalRepository->tipCommit()->id();
    }

    return xRequest;
}

//-------------------------------------------------------------------------------------------------

CXMLNode CRemoteRepository::getPushRequest()
{
    CXMLNode xRequest("");

    xRequest.attributes()[CStrings::s_sParamRequest] = CStrings::s_sParamPush;
    xRequest.attributes()[CStrings::s_sParamBranch] = m_pLocalRepository->currentBranchName();

    if (not m_pLocalRepository->currentBranch().isNull())
    {
        xRequest.attributes()[CStrings::s_sParamRoot] = m_pLocalRepository->currentBranch()->rootCommitId();
    }

    if (m_pLocalRepository->tipCommit().isNull())
        return xRequest;

    QString sTipId = m_pLocalRepository->tipCommit()->id();
    xRequest.attributes()[CStrings::s_sParamTip] = sTipId;

    appendPushCommitNodes(xRequest, pushCommitIds(sTipId, ""), QSet<QString>());

    return xRequest;
}

/************************************************************************/

/*!
    Returns the push announce for the local branch. \br\br
    The announce carries the branch name and the local tip only, the server
    answers its own tip so the data request carries missing commits only. \br
*/
CXMLNode CRemoteRepository::getPushAnnounce()
{
    CXMLNode xAnnounce("");

    xAnnounce.attributes()[CStrings::s_sParamRequest] = CStrings::s_sParamPushAnnounce;
    xAnnounce.attributes()[CStrings::s_sParamBranch] = m_pLocalRepository->currentBranchName();

    if (not m_pLocalRepository->tipCommit().isNull())
    {
        xAnnounce.attributes()[CStrings::s_sParamTip] = m_pLocalRepository->tipCommit()->id();
    }

    return xAnnounce;
}

/************************************************************************/

/*!
    Returns the push announce response for \a xAnnounce. \br\br
    The response carries the branch name and the server tip, empty when the
    branch is unknown so the data request falls back to the full history. \br
    Returns an error node when the announce carries no branch name. \br
*/
CXMLNode CRemoteRepository::getPushAnnounceResponse(const CXMLNode& xAnnounce)
{
    QString sBranchName = xAnnounce.attributes()[CStrings::s_sParamBranch];

    if (xAnnounce.attributes()[CStrings::s_sParamRequest] != CStrings::s_sParamPushAnnounce
            || sBranchName.isEmpty())
    {
        return CError::toResultNode(CError::ePushRequestInvalid);
    }

    QString sServerTipId;

    CBranch* pBranch = nullptr;

    if (QFile(m_pLocalRepository->database()->composeBranchFileName(sBranchName)).exists())
    {
        pBranch = m_pLocalRepository->database()->getBranch(sBranchName, nullptr);

        if (IS_NOT_NULL(pBranch))
        {
            sServerTipId = pBranch->tipCommitId();
            delete pBranch;
        }
    }

    CXMLNode xResponse("");

    xResponse.attributes()[CStrings::s_sParamRequest] = CStrings::s_sParamPushAnnounce;
    xResponse.attributes()[CStrings::s_sParamBranch] = sBranchName;
    xResponse.attributes()[CStrings::s_sParamTip] = sServerTipId;

    return xResponse;
}

/************************************************************************/

/*!
    Builds the push data request for \a sServerTipId into \a xRequest. \br\br
    Only commits past the server tip travel, with objects unreferenced by the
    shared history. An empty server tip sends everything, a server tip equal
    to the local tip sends attributes only as the branches already converge. \br
    Refuses without building when the locally known server tip is not an
    ancestor of the local tip, the server would refuse the same push. An
    unknown server tip falls back to the full history and lets the server
    decide. Sets \a sDetail and returns the failure reason when refused. \br
*/
CError::EErrorCode CRemoteRepository::buildPushDataRequest(const QString& sServerTipId, CXMLNode& xRequest, QString& sDetail)
{
    xRequest = CXMLNode("");

    xRequest.attributes()[CStrings::s_sParamRequest] = CStrings::s_sParamPush;
    xRequest.attributes()[CStrings::s_sParamBranch] = m_pLocalRepository->currentBranchName();

    if (not m_pLocalRepository->currentBranch().isNull())
    {
        xRequest.attributes()[CStrings::s_sParamRoot] = m_pLocalRepository->currentBranch()->rootCommitId();
    }

    if (m_pLocalRepository->tipCommit().isNull())
    {
        sDetail = CError::message(CError::ePushRequestInvalid);

        return CError::ePushRequestInvalid;
    }

    QString sTipId = m_pLocalRepository->tipCommit()->id();
    xRequest.attributes()[CStrings::s_sParamTip] = sTipId;

    if (xRequest.attributes()[CStrings::s_sParamBranch].isEmpty())
    {
        sDetail = CError::message(CError::ePushRequestInvalid);

        return CError::ePushRequestInvalid;
    }

    // Branches already converge, attributes only, the server accepts it as a no-op
    if (not sServerTipId.isEmpty() && sServerTipId == sTipId)
        return CError::eNone;

    QStringList lFullCommitIds = pushCommitIds(sTipId, "");

    bool bServerTipKnown = not sServerTipId.isEmpty()
        && QFile::exists(m_pLocalRepository->database()->composeCommitFileName(sServerTipId));

    // The server refuses the same push, save both payloads
    if (bServerTipKnown && not lFullCommitIds.contains(sServerTipId))
    {
        sDetail = CError::message(CError::ePushNotFastForward);

        return CError::ePushNotFastForward;
    }

    QStringList lMissingCommitIds = pushCommitIds(sTipId, bServerTipKnown ? sServerTipId : QString());

    QSet<QString> lKnownObjectIds;

    if (bServerTipKnown)
    {
        QStringList lKnownCommitIds = pushCommitIds(sServerTipId, "");
        lKnownCommitIds.prepend(sServerTipId);
        lKnownObjectIds = pushReferencedObjectIds(lKnownCommitIds);
    }

    appendPushCommitNodes(xRequest, lMissingCommitIds, lKnownObjectIds);

    return CError::eNone;
}

/************************************************************************/

QStringList CRemoteRepository::pushCommitIds(const QString& sTipId, const QString& sStopAtCommitId) const
{
    QStringList lCommitIds;

    if (sTipId.isEmpty())
        return lCommitIds;

    lCommitIds << sTipId;

    QList<QPair<int, QString>> lAncestors = m_pLocalRepository->commitFunctions()->getCommitAncestorList(
                sTipId,
                true,
                0,
                sStopAtCommitId
                );

    for (const QPair<int, QString>& tAncestor : lAncestors)
        if (not lCommitIds.contains(tAncestor.second))
            lCommitIds << tAncestor.second;

    return lCommitIds;
}

/************************************************************************/

QSet<QString> CRemoteRepository::pushReferencedObjectIds(const QStringList& lCommitIds) const
{
    QSet<QString> lObjectIds;

    QList<CCommit*> lCommits = m_pLocalRepository->commitFunctions()->loadCommitList(lCommitIds, nullptr);

    for (CCommit* pCommit : lCommits)
        for (QString sObjectId : pCommit->files().values())
            if (not sObjectId.isEmpty())
                lObjectIds << sObjectId;

    qDeleteAll(lCommits);

    return lObjectIds;
}

/************************************************************************/

void CRemoteRepository::appendPushCommitNodes(CXMLNode& xRequest, const QStringList& lCommitIds, const QSet<QString>& lKnownObjectIds)
{
    QList<CCommit*> lCommits = m_pLocalRepository->commitFunctions()->loadCommitList(lCommitIds, nullptr);

    QSet<QString> lSentObjectIds;

    for (CCommit* pCommit : lCommits)
    {
        CXMLNode xCommit = pCommit->toNode();
        xCommit.attributes()[CStrings::s_sParamId] = pCommit->id();
        xRequest << xCommit;

        for (QString sObjectId : pCommit->files().values())
        {
            if (not sObjectId.isEmpty()
                    and not lKnownObjectIds.contains(sObjectId)
                    and not lSentObjectIds.contains(sObjectId))
            {
                lSentObjectIds << sObjectId;

                CXMLNode xObject(CStrings::s_sParamObject);
                xObject.attributes()[CStrings::s_sParamId] = sObjectId;
                xObject.setValue(CStrings::s_sBase64Marker + QString(m_pLocalRepository->database()->getObject(sObjectId).toBase64()));
                xRequest << xObject;
            }
        }
    }

    qDeleteAll(lCommits);
}

/************************************************************************/

/*!
    Returns the request for a clone. \br\br
    The clone request carries no payload, the server answers
    with getCloneResponse built from its own repository. \br
*/
CXMLNode CRemoteRepository::getCloneRequest()
{
    CXMLNode xRequest("");

    xRequest.attributes()[CStrings::s_sParamRequest] = CStrings::s_sParamClone;

    return xRequest;
}

/************************************************************************/

/*!
    Returns the full clone payload of the local repository. \br\br
    The response carries the current branch name, one named node per branch
    plus every commit reachable from the branch tips and every object
    referenced by those commits. \br
*/
CXMLNode CRemoteRepository::getCloneResponse()
{
    CXMLNode xResponse("");

    xResponse.attributes()[CStrings::s_sParamRequest] = CStrings::s_sParamClone;
    xResponse.attributes()[CStrings::s_sParamCurrent] = m_pLocalRepository->currentBranchName();

    QStringList lBranchNames = cloneBranchNames();
    QStringList lCommitIds = cloneCommitIds(lBranchNames);

    appendCloneBranchNodes(xResponse, lBranchNames);
    appendCloneCommitNodes(xResponse, lCommitIds);

    return xResponse;
}

/************************************************************************/

/*!
    Returns the pull payload for \a xRequest. \br\br
    The request carries the requested branch name and the client tip,
    the response carries the branch name, the server tip plus every
    commit reachable from the server tip and every object referenced
    by those commits. Branch author and root travel as attributes so
    no child node collides with them in JSON. \br
    Returns an error node when the request is invalid or the branch
    is unknown. \br
*/
CXMLNode CRemoteRepository::getPullResponse(const CXMLNode& xRequest)
{
    QString sBranchName = xRequest.attributes()[CStrings::s_sParamBranch];

    if (xRequest.attributes()[CStrings::s_sParamRequest] != CStrings::s_sParamPull
            || sBranchName.isEmpty())
    {
        return CError::toResultNode(CError::ePullRequestInvalid);
    }

    CBranch* pBranch = nullptr;

    if (not QFile(m_pLocalRepository->database()->composeBranchFileName(sBranchName)).exists())
    {
        return CError::toResultNode(CError::eBranchDoesNotExist, sBranchName);
    }

    pBranch = m_pLocalRepository->database()->getBranch(sBranchName, nullptr);

    if (IS_NULL(pBranch))
    {
        return CError::toResultNode(CError::eBranchDoesNotExist, sBranchName);
    }

    QString sServerTipId = pBranch->tipCommitId();
    QString sAuthor = pBranch->author();
    QString sRootId = pBranch->rootCommitId();
    delete pBranch;

    CXMLNode xResponse("");

    xResponse.attributes()[CStrings::s_sParamRequest] = CStrings::s_sParamPull;
    xResponse.attributes()[CStrings::s_sParamBranch] = sBranchName;
    xResponse.attributes()[CStrings::s_sParamTip] = sServerTipId;
    xResponse.attributes()[CStrings::s_sParamAuthor] = sAuthor;
    xResponse.attributes()[CStrings::s_sParamRoot] = sRootId;

    if (sServerTipId.isEmpty())
        return xResponse;

    QStringList lCommitIds = pullCommitIds(sServerTipId);

    appendCloneCommitNodes(xResponse, lCommitIds);

    return xResponse;
}

/************************************************************************/

/*!
    Applies \a xResponse as a pull to the local repository. \br\br
    Stores received objects and commits, then updates the pulled branch.
    The current branch is fast forwarded when it is an ancestor of the
    received tip, merged otherwise through a temporary branch. \br
    A divergent merge leaves conflict markers in the working folder and
    the merge parents in the stage, like a local merge. \br
    Sets \a sDetail and returns the failure reason when invalid. \br
*/
CError::EErrorCode CRemoteRepository::applyPullResponse(const CXMLNode& xResponse, QString& sDetail)
{
    return CPullCommand(m_pLocalRepository, xResponse, sDetail).execute();
}

/************************************************************************/

QStringList CRemoteRepository::pullCommitIds(const QString& sTipId) const
{
    QStringList lCommitIds;

    if (sTipId.isEmpty())
        return lCommitIds;

    lCommitIds << sTipId;

    QList<QPair<int, QString>> lAncestors = m_pLocalRepository->commitFunctions()->getCommitAncestorList(
                sTipId,
                false,
                0
                );

    for (const QPair<int, QString>& tAncestor : lAncestors)
        if (not lCommitIds.contains(tAncestor.second))
            lCommitIds << tAncestor.second;

    return lCommitIds;
}

/************************************************************************/

QStringList CRemoteRepository::cloneBranchNames() const
{
    QStringList lBranchNames;

    QStringList lFilters;
    lFilters << QString("*.%1").arg(CStrings::s_sCompressedXMLExtension);

    QDir tBranchFolder(m_pLocalRepository->database()->branchPath());
    QFileInfoList lFiles = tBranchFolder.entryInfoList(lFilters, QDir::Files | QDir::NoSymLinks);

    for (const QFileInfo& tFile : lFiles)
        lBranchNames << tFile.baseName();

    return lBranchNames;
}

/************************************************************************/

QStringList CRemoteRepository::cloneCommitIds(const QStringList& lBranchNames) const
{
    QStringList lCommitIds;

    for (const QString& sBranchName : lBranchNames)
    {
        CBranch* pBranch = m_pLocalRepository->database()->getBranch(sBranchName, nullptr);

        if (IS_NOT_NULL(pBranch))
        {
            QString sTipId = pBranch->tipCommitId();
            delete pBranch;

            if (not sTipId.isEmpty() and not lCommitIds.contains(sTipId))
                lCommitIds << sTipId;

            QList<QPair<int, QString>> lAncestors = m_pLocalRepository->commitFunctions()->getCommitAncestorList(
                        sTipId,
                        true,
                        0
                        );

            for (const QPair<int, QString>& tAncestor : lAncestors)
                if (not lCommitIds.contains(tAncestor.second))
                    lCommitIds << tAncestor.second;
        }
    }

    return lCommitIds;
}

/************************************************************************/

void CRemoteRepository::appendCloneBranchNodes(CXMLNode& xResponse, const QStringList& lBranchNames)
{
    for (const QString& sBranchName : lBranchNames)
    {
        CBranch* pBranch = m_pLocalRepository->database()->getBranch(sBranchName, this);

        if (IS_NOT_NULL(pBranch))
        {
            CXMLNode xBranch = pBranch->toNode();
            xBranch.attributes()[CStrings::s_sParamName] = sBranchName;
            xResponse << xBranch;
            delete pBranch;
        }
    }
}

/************************************************************************/

void CRemoteRepository::appendCloneCommitNodes(CXMLNode& xResponse, const QStringList& lCommitIds)
{
    QList<CCommit*> lCommits = m_pLocalRepository->commitFunctions()->loadCommitList(lCommitIds, nullptr);

    QStringList lSentObjectIds;

    for (CCommit* pCommit : lCommits)
    {
        CXMLNode xCommit = pCommit->toNode();
        xCommit.attributes()[CStrings::s_sParamId] = pCommit->id();
        xResponse << xCommit;

        for (QString sObjectId : pCommit->files().values())
        {
            if (not sObjectId.isEmpty() and not lSentObjectIds.contains(sObjectId))
            {
                lSentObjectIds << sObjectId;

                CXMLNode xObject(CStrings::s_sParamObject);
                xObject.attributes()[CStrings::s_sParamId] = sObjectId;
                xObject.setValue(CStrings::s_sBase64Marker + QString(m_pLocalRepository->database()->getObject(sObjectId).toBase64()));
                xResponse << xObject;
            }
        }
    }

    qDeleteAll(lCommits);
}

//-------------------------------------------------------------------------------------------------

/*!
    Applies \a xRequest as a push to the local repository. \br\br
    Stores received objects and commits, then updates the pushed branch.
    Sets \a sDetail and returns the failure reason when the request is invalid. \br
*/
CError::EErrorCode CRemoteRepository::applyPushRequest(const CXMLNode& xRequest, QString& sDetail)
{
    return CPushCommand(m_pLocalRepository, xRequest, sDetail).execute();
}

/************************************************************************/

/*!
    Returns the request for a delete-branch. \br\br
    The request carries the branch name and the force flag, the server
    deletes the branch with CRepository::deleteBranch. \br
*/
CXMLNode CRemoteRepository::getDeleteBranchRequest(const QString& sBranchName, bool bForce)
{
    CXMLNode xRequest("");

    xRequest.attributes()[CStrings::s_sParamRequest] = CStrings::s_sParamDeleteBranch;
    xRequest.attributes()[CStrings::s_sParamBranch] = sBranchName;
    xRequest.attributes()[CStrings::s_sParamForce] = bForce ? QStringLiteral("true") : QStringLiteral("false");

    return xRequest;
}

/************************************************************************/

/*!
    Applies \a xRequest as a branch deletion to the local repository. \br\br
    The request carries the branch name and the force flag. \br
    Sets \a sDetail and returns the failure reason when the request is invalid. \br
*/
CError::EErrorCode CRemoteRepository::applyDeleteBranchRequest(const CXMLNode& xRequest, QString& sDetail)
{
    QString sBranchName = xRequest.attributes()[CStrings::s_sParamBranch];
    QString sForce = xRequest.attributes()[CStrings::s_sParamForce];

    if (xRequest.attributes()[CStrings::s_sParamRequest] != CStrings::s_sParamDeleteBranch
            || sBranchName.isEmpty())
    {
        sDetail = CError::message(CError::eDeleteBranchRequestInvalid);
        return CError::eDeleteBranchRequestInvalid;
    }

    bool bForce = (sForce == QStringLiteral("true") || sForce == QStringLiteral("1"));

    CError::EErrorCode eCode = m_pLocalRepository->deleteBranch(sBranchName, bForce);

    if (eCode != CError::eNone)
        sDetail = sBranchName;

    return eCode;
}

//-------------------------------------------------------------------------------------------------

}
