
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

    QList<CCommit*> lCommits = CCommit::fromIdList(m_pLocalRepository->database(), lCommitIds, nullptr);

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

//-------------------------------------------------------------------------------------------------

/*!
    Applies \a xRequest as a push to the local repository. \br\br
    Stores received objects and commits, then updates the pushed branch.
    Sets \a sDetail and returns the failure reason when the request is invalid. \br
*/
CError::EErrorCode CRemoteRepository::applyPushRequest(const CXMLNode& xRequest, QString& sDetail)
{
    QString sBranchName = xRequest.attributes()[CStrings::s_sParamBranch];
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

    return updateBranchFromPush(sBranchName, sTipId, lCommitIds, sDetail);
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
        QString sStoredId = m_pLocalRepository->database()->storeCommit(pCommit, sCommitId);
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

CError::EErrorCode CRemoteRepository::updateBranchFromPush(const QString& sBranchName, const QString& sTipId, const QStringList& lCommitIds, QString& sDetail)
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
    }
    else
    {
        pBranch = new CBranch(this);

        CCommit* pTipCommit = pDatabase->getCommit(sTipId, this);
        pBranch->setAuthor(pTipCommit->author());
        delete pTipCommit;

        // Find the root commit, oldest pushed commit without parents
        QString sRootId = sTipId;

        for (int iIndex = lCommitIds.count() - 1; iIndex >= 0; --iIndex)
        {
            CCommit* pCommit = pDatabase->getCommit(lCommitIds[iIndex], this);

            if (pCommit->parents().isEmpty())
                sRootId = pCommit->id();

            delete pCommit;
        }

        pBranch->setRootCommitId(sRootId);
    }

    pBranch->setTipCommitId(sTipId);
    pBranch->toNode().save(sBranchFileName);
    delete pBranch;

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

}
