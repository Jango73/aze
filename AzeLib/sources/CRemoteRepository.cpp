
// Qt
#include <QDebug>
#include <QDir>
#include <QFile>

// Application
#include "CRemoteRepository.h"

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

    QVector<CBranch*> lBranches = m_pLocalRepository->database()->getAllBranches(this);

    xRequest.attributes()[CStrings::s_sParamRequest] = CStrings::s_sParamPull;

    for (CBranch* pBranch : lBranches)
    {
        CXMLNode xBranch = pBranch->toNode();
        xRequest << xBranch;
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
