
// Qt
#include <QDebug>
#include <QDir>
#include <QFile>

// Application
#include "CRemoteRepository.h"
#include "CUtils.h"

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

    QList<QPair<int, QString>> lCommitList = m_pLocalRepository->commitFunctions()->getCommitAncestorList(
                sTipId,
                true,
                0
                );

    QStringList lCommitIds;
    lCommitIds << sTipId;

    for (QPair<int, QString> pCommit : lCommitList)
        if (not lCommitIds.contains(pCommit.second))
            lCommitIds << pCommit.second;

    QList<CCommit*> lCommits = m_pLocalRepository->commitFunctions()->loadCommitList(lCommitIds, nullptr);

    QStringList lSentObjectIds;

    for (CCommit* pCommit : lCommits)
    {
        CXMLNode xCommit = pCommit->toNode();
        xCommit.attributes()[CStrings::s_sParamId] = pCommit->id();
        xRequest << xCommit;

        for (QString sObjectId : pCommit->files().values())
        {
            if (not sObjectId.isEmpty() and not lSentObjectIds.contains(sObjectId))
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

    return xRequest;
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
    CError::EErrorCode eCheckCode = CError::fromResultNode(xResponse);

    if (eCheckCode != CError::eInvalidResponse)
        return eCheckCode;

    if (xResponse.attributes()[CStrings::s_sParamRequest] != CStrings::s_sParamPull)
    {
        sDetail = CError::message(CError::ePullRequestInvalid);
        return CError::ePullRequestInvalid;
    }

    QString sBranchName = xResponse.attributes()[CStrings::s_sParamBranch];
    QString sServerTipId = xResponse.attributes()[CStrings::s_sParamTip];

    if (sBranchName.isEmpty())
    {
        sDetail = CError::message(CError::ePullRequestInvalid);
        return CError::ePullRequestInvalid;
    }

    if (sServerTipId.isEmpty())
    {
        QString sBranchFileName = m_pLocalRepository->database()->composeBranchFileName(sBranchName);

        if (not QFile(sBranchFileName).exists())
        {
            CBranch* pNewBranch = new CBranch(m_pLocalRepository);
            pNewBranch->toNode().save(sBranchFileName);
            delete pNewBranch;
        }

        return CError::eNone;
    }

    CError::EErrorCode eCode = storeRequestObjects(xResponse, sDetail);

    if (eCode != CError::eNone)
        return eCode;

    QStringList lCommitIds;
    eCode = storeRequestCommits(xResponse, lCommitIds, sDetail);

    if (eCode != CError::eNone)
        return eCode;

    if (lCommitIds.isEmpty() || not lCommitIds.contains(sServerTipId))
    {
        sDetail = CError::message(CError::ePullRequestInvalid);
        return CError::ePullRequestInvalid;
    }

    CDatabase* pDatabase = m_pLocalRepository->database();
    QString sBranchFileName = pDatabase->composeBranchFileName(sBranchName);

    if (not QFile(sBranchFileName).exists())
    {
        CBranch* pNewBranch = new CBranch(m_pLocalRepository);

        pNewBranch->setAuthor(xResponse.attributes()[CStrings::s_sParamAuthor]);
        pNewBranch->setRootCommitId(xResponse.attributes()[CStrings::s_sParamRoot]);
        pNewBranch->setTipCommitId(sServerTipId);

        if (pNewBranch->rootCommitId().isEmpty())
            pNewBranch->setRootCommitId(sServerTipId);

        if (pNewBranch->author().isEmpty())
        {
            CCommit* pTipCommit = m_pLocalRepository->commitFunctions()->loadCommit(sServerTipId, m_pLocalRepository);

            if (IS_NOT_NULL(pTipCommit))
            {
                pNewBranch->setAuthor(pTipCommit->author());
                delete pTipCommit;
            }
        }

        pNewBranch->toNode().save(sBranchFileName);
        delete pNewBranch;

        if (sBranchName != m_pLocalRepository->currentBranchName())
            return CError::eNone;

        m_pLocalRepository->refreshCurrentBranch();

        CCommit* pTipCommit = m_pLocalRepository->commitFunctions()->loadCommit(sServerTipId, m_pLocalRepository);

        if (IS_NULL(pTipCommit))
        {
            sDetail = sServerTipId;
            return CError::ePullRequestInvalid;
        }

        QDictionary mFiles = pTipCommit->files();
        delete pTipCommit;

        for (const QString& sRelativeFileName : mFiles.keys())
        {
            QByteArray baData = pDatabase->getObject(mFiles[sRelativeFileName]);

            if (baData.isEmpty())
            {
                sDetail = sRelativeFileName;
                return CError::eObjectStoreFailed;
            }

            if (not CUtils::putBinaryFileContent(pDatabase->composeLocalFileName(sRelativeFileName), baData))
            {
                sDetail = sRelativeFileName;
                return CError::eObjectStoreFailed;
            }
        }

        return CError::eNone;
    }

    if (sBranchName != m_pLocalRepository->currentBranchName())
    {
        CBranch* pBranch = pDatabase->getBranch(sBranchName, m_pLocalRepository);

        if (IS_NULL(pBranch))
        {
            sDetail = sBranchName;
            return CError::eBranchDoesNotExist;
        }

        pBranch->setTipCommitId(sServerTipId);
        pBranch->toNode().save(sBranchFileName);
        delete pBranch;

        return CError::eNone;
    }

    m_pLocalRepository->refreshCurrentBranch();

    QString sLocalTipId;

    if (not m_pLocalRepository->currentBranch().isNull())
        sLocalTipId = m_pLocalRepository->currentBranch()->tipCommitId();

    if (sLocalTipId == sServerTipId)
        return CError::eNone;

    if (sLocalTipId.isEmpty())
    {
        m_pLocalRepository->currentBranch()->setTipCommitId(sServerTipId);
        m_pLocalRepository->writeCurrentBranch();

        CCommit* pTipCommit = m_pLocalRepository->commitFunctions()->loadCommit(sServerTipId, m_pLocalRepository);

        if (IS_NULL(pTipCommit))
        {
            sDetail = sServerTipId;
            return CError::ePullRequestInvalid;
        }

        QDictionary mFiles = pTipCommit->files();
        delete pTipCommit;

        for (const QString& sRelativeFileName : mFiles.keys())
        {
            QByteArray baData = pDatabase->getObject(mFiles[sRelativeFileName]);

            if (baData.isEmpty())
            {
                sDetail = sRelativeFileName;
                return CError::eObjectStoreFailed;
            }

            if (not CUtils::putBinaryFileContent(pDatabase->composeLocalFileName(sRelativeFileName), baData))
            {
                sDetail = sRelativeFileName;
                return CError::eObjectStoreFailed;
            }
        }

        return CError::eNone;
    }

    QList<QPair<int, QString>> lServerAncestors = m_pLocalRepository->commitFunctions()->getCommitAncestorList(
                sServerTipId,
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
        QList<QPair<int, QString>> lLocalAncestors = m_pLocalRepository->commitFunctions()->getCommitAncestorList(
                    sLocalTipId,
                    false,
                    0
                    );

        for (const QPair<int, QString>& tAncestor : lLocalAncestors)
        {
            if (tAncestor.second == sServerTipId)
                return CError::eNone;
        }
    }

    const QString sTemporaryBranchName("__aze_pull_tmp__");
    QString sTemporaryFileName = pDatabase->composeBranchFileName(sTemporaryBranchName);

    QFile::remove(sTemporaryFileName);

    CBranch* pTemporaryBranch = new CBranch(m_pLocalRepository);
    pTemporaryBranch->setTipCommitId(sServerTipId);
    pTemporaryBranch->toNode().save(sTemporaryFileName);
    delete pTemporaryBranch;

    bool bHasConflicts = false;
    CError::EErrorCode eMergeCode = m_pLocalRepository->merge(sTemporaryBranchName, bHasConflicts, true);

    QFile::remove(sTemporaryFileName);

    if (eMergeCode == CError::eNothingToMerge)
        return CError::eNone;

    if (eMergeCode != CError::eNone && eMergeCode != CError::eMergeHasConflicts)
        return eMergeCode;

    m_pLocalRepository->writeCurrentBranch();
    m_pLocalRepository->writeStage();

    if (bHasConflicts || eMergeCode == CError::eMergeHasConflicts)
        return CError::eMergeHasConflicts;

    return CError::eNone;
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
    QString sBranchName = xRequest.attributes()[CStrings::s_sParamBranch];
    QString sRootId = xRequest.attributes()[CStrings::s_sParamRoot];
    QString sTipId = xRequest.attributes()[CStrings::s_sParamTip];

    if (xRequest.attributes()[CStrings::s_sParamRequest] != CStrings::s_sParamPush
            || sBranchName.isEmpty()
            || sTipId.isEmpty())
    {
        sDetail = CError::message(CError::ePushRequestInvalid);
        return CError::ePushRequestInvalid;
    }

    CError::EErrorCode eCode = storeRequestObjects(xRequest, sDetail);

    if (eCode != CError::eNone)
        return eCode;

    QStringList lCommitIds;
    eCode = storeRequestCommits(xRequest, lCommitIds, sDetail);

    if (eCode != CError::eNone)
        return eCode;

    if (lCommitIds.isEmpty() || not lCommitIds.contains(sTipId))
    {
        sDetail = CError::message(CError::ePushRequestInvalid);
        return CError::ePushRequestInvalid;
    }

    return updateBranchFromPush(sBranchName, sRootId, sTipId, lCommitIds, sDetail);
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

CError::EErrorCode CRemoteRepository::storeRequestObjects(const CXMLNode& xRequest, QString& sDetail)
{
    CXMLNodeList lObjects = xRequest.getNodesByTagName(CStrings::s_sParamObject);

    for (CXMLNode xObject : lObjects)
    {
        QString sObjectId = xObject.attributes()[CStrings::s_sParamId];
        QString sContent = xObject.value();

        if (sObjectId.isEmpty() || not sContent.startsWith(CStrings::s_sBase64Marker))
        {
            sDetail = CError::message(CError::ePushRequestInvalid);
            return CError::ePushRequestInvalid;
        }

        sContent.remove(0, CStrings::s_sBase64Marker.length());
        QByteArray baData = QByteArray::fromBase64(sContent.toLatin1());

        if (not m_pLocalRepository->database()->storeObject(sObjectId, baData))
        {
            sDetail = sObjectId;
            return CError::eObjectStoreFailed;
        }
    }

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CRemoteRepository::storeRequestCommits(const CXMLNode& xRequest, QStringList& lCommitIds, QString& sDetail)
{
    CXMLNodeList lCommits = xRequest.getNodesByTagName(CStrings::s_sParamCommit);

    for (CXMLNode xCommit : lCommits)
    {
        QString sCommitId = xCommit.attributes()[CStrings::s_sParamId];

        if (sCommitId.isEmpty())
        {
            sDetail = CError::message(CError::ePushRequestInvalid);
            return CError::ePushRequestInvalid;
        }

        CCommit* pCommit = CCommit::fromNode(xCommit, this, sCommitId);
        QString sStoredId = m_pLocalRepository->commitFunctions()->storeCommit(pCommit, sCommitId);
        delete pCommit;

        if (sStoredId.isEmpty())
        {
            sDetail = CError::message(CError::ePushRequestInvalid);
            return CError::ePushRequestInvalid;
        }

        if (not lCommitIds.contains(sStoredId))
            lCommitIds << sStoredId;
    }

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CRemoteRepository::updateBranchFromPush(const QString& sBranchName, const QString& sRequestedRootId, const QString& sTipId, const QStringList& lCommitIds, QString& sDetail)
{
    CDatabase* pDatabase = m_pLocalRepository->database();
    QString sBranchFileName = pDatabase->composeBranchFileName(sBranchName);

    CBranch* pBranch = nullptr;

    if (QFile(sBranchFileName).exists())
    {
        pBranch = pDatabase->getBranch(sBranchName, this);

        // Refuse to overwrite history that the pushed commits do not contain
        if (not pBranch->tipCommitId().isEmpty()
                && pBranch->tipCommitId() != sTipId
                && not lCommitIds.contains(pBranch->tipCommitId()))
        {
            sDetail = CError::message(CError::ePushNotFastForward);
            delete pBranch;
            return CError::ePushNotFastForward;
        }

        if (pBranch->rootCommitId().isEmpty() && not sRequestedRootId.isEmpty())
        {
            pBranch->setRootCommitId(sRequestedRootId);
        }
    }
    else
    {
        pBranch = new CBranch(this);

        CCommit* pTipCommit = m_pLocalRepository->commitFunctions()->loadCommit(sTipId, this);
        pBranch->setAuthor(pTipCommit->author());
        delete pTipCommit;

        if (not sRequestedRootId.isEmpty())
        {
            pBranch->setRootCommitId(sRequestedRootId);
        }
        else
        {
            // Fallback for requests without a root, oldest pushed commit without parents
            QString sRootId = sTipId;

            for (int iIndex = lCommitIds.count() - 1; iIndex >= 0; --iIndex)
            {
                CCommit* pCommit = m_pLocalRepository->commitFunctions()->loadCommit(lCommitIds[iIndex], this);

                if (IS_NOT_NULL(pCommit))
                {
                    if (pCommit->parents().isEmpty())
                        sRootId = pCommit->id();

                    delete pCommit;
                }
            }

            pBranch->setRootCommitId(sRootId);
        }
    }

    pBranch->setTipCommitId(sTipId);
    pBranch->toNode().save(sBranchFileName);
    delete pBranch;

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

}
