
// Qt
#include <QDebug>
#include <QDir>
#include <QEventLoop>

// qt-plus
#include <CXMLNode.h>

// Aze
#include "CRemoteRepository.h"
#include "objects/CBranch.h"

// Application
#include "CAzeClient.h"

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::pull()
{
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    CXMLNode xRequest = Aze::CRemoteRepository(m_pRepository).getPullRequest();

    postNetworkRequest(xRequest);

    if (m_eLastError == Aze::CError::eNetworkError)
        return m_eLastError;

    // A pull payload carries no result attribute, anything else is a server error
    Aze::CError::EErrorCode eResponseCode = Aze::CError::fromResultNode(m_xLastResponse);

    if (eResponseCode != Aze::CError::eInvalidResponse)
        return eResponseCode;

    QString sDetail;
    Aze::CError::EErrorCode ePullCode = Aze::CRemoteRepository(m_pRepository).applyPullResponse(m_xLastResponse, sDetail);

    if (ePullCode != Aze::CError::eNone && ePullCode != Aze::CError::eMergeHasConflicts)
    {
        OUT_ERROR(Aze::CError::message(ePullCode, sDetail));
    }

    return ePullCode;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::push()
{
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    Aze::CRemoteRepository oRemoteRepository(m_pRepository);

    // Phase 1: announce the branch tip so the server answers its own tip
    CXMLNode xAnnounce = oRemoteRepository.getPushAnnounce();

    postNetworkRequest(xAnnounce);

    if (m_eLastError == Aze::CError::eNetworkError)
        return m_eLastError;

    Aze::CError::EErrorCode eAnnounceCode = Aze::CError::fromResultNode(m_xLastResponse);

    // Old servers know push only, fall back to the full history request
    if (eAnnounceCode == Aze::CError::eUnknownAction)
    {
        CXMLNode xFallback = oRemoteRepository.getPushRequest();

        return postNetworkRequest(xFallback);
    }

    // Phase 2: send missing commits only, refused early without sending
    // when the known server tip is not an ancestor of the local tip
    CXMLNode xData;
    QString sDetail;
    Aze::CError::EErrorCode eBuildCode = oRemoteRepository.buildPushDataRequestFromAnnounceResponse(m_xLastResponse, xData, sDetail);

    if (eBuildCode != Aze::CError::eNone)
    {
        OUT_ERROR(Aze::CError::message(eBuildCode, sDetail));

        return eBuildCode;
    }

    // Branches already converge, nothing travels
    if (xData.getNodesByTagName(Aze::CStrings::s_sParamCommit).isEmpty())
        return Aze::CError::eNone;

    return postNetworkRequest(xData);
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::pullTag()
{
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    if (m_tArguments.m_lFilesAndIds.count() == 0)
    {
        OUT_ERROR(Aze::CError::message(Aze::CError::eNoTagNameGiven));
        return Aze::CError::eNoTagNameGiven;
    }

    QString sTagName = m_tArguments.m_lFilesAndIds[0];

    // The local tip travels when the tag exists, empty pulls everything
    QString sLocalTipId;
    Aze::CBranch* pLocalTag = m_pRepository->database()->getBranch(sTagName, this);

    if (IS_NOT_NULL(pLocalTag) && pLocalTag->isTag())
        sLocalTipId = pLocalTag->tipCommitId();

    delete pLocalTag;

    CXMLNode xRequest = Aze::CRemoteRepository(m_pRepository).getPullRequestFor(sTagName, sLocalTipId);

    postNetworkRequest(xRequest);

    if (m_eLastError == Aze::CError::eNetworkError)
        return m_eLastError;

    // A pull payload carries no result attribute, anything else is a server error
    Aze::CError::EErrorCode eResponseCode = Aze::CError::fromResultNode(m_xLastResponse);

    if (eResponseCode != Aze::CError::eInvalidResponse)
        return eResponseCode;

    QString sDetail;
    Aze::CError::EErrorCode ePullCode = Aze::CRemoteRepository(m_pRepository).applyPullResponse(m_xLastResponse, sDetail);

    if (ePullCode != Aze::CError::eNone && ePullCode != Aze::CError::eMergeHasConflicts)
    {
        OUT_ERROR(Aze::CError::message(ePullCode, sDetail));
    }

    return ePullCode;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::pushTag()
{
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    if (m_tArguments.m_lFilesAndIds.count() == 0)
    {
        OUT_ERROR(Aze::CError::message(Aze::CError::eNoTagNameGiven));
        return Aze::CError::eNoTagNameGiven;
    }

    QString sTagName = m_tArguments.m_lFilesAndIds[0];

    Aze::CBranch* pLocalTag = m_pRepository->database()->getBranch(sTagName, this);

    if (IS_NULL(pLocalTag) || not pLocalTag->isTag())
    {
        OUT_ERROR(Aze::CError::message(Aze::CError::eTagDoesNotExist, sTagName));
        delete pLocalTag;
        return Aze::CError::eTagDoesNotExist;
    }

    QString sRootId = pLocalTag->rootCommitId();
    QString sTipId = pLocalTag->tipCommitId();
    delete pLocalTag;

    Aze::CRemoteRepository oRemoteRepository(m_pRepository);

    // Phase 1: announce the tag tip so the server answers its own tip
    CXMLNode xAnnounce = oRemoteRepository.getPushAnnounceFor(sTagName, sTipId);

    postNetworkRequest(xAnnounce);

    if (m_eLastError == Aze::CError::eNetworkError)
        return m_eLastError;

    Aze::CError::EErrorCode eAnnounceCode = Aze::CError::fromResultNode(m_xLastResponse);

    // Old servers know push only, fall back to the full history request
    if (eAnnounceCode == Aze::CError::eUnknownAction)
    {
        CXMLNode xFallback = oRemoteRepository.getPushRequestFor(sTagName, sRootId, sTipId, Aze::CEnums::eTag);

        return postNetworkRequest(xFallback);
    }

    // Phase 2: send missing commits only
    CXMLNode xData;
    QString sDetail;
    Aze::CError::EErrorCode eBuildCode = oRemoteRepository.buildPushDataRequestForAnnounceResponse(
                sTagName, sRootId, sTipId, Aze::CEnums::eTag, m_xLastResponse, xData, sDetail);

    if (eBuildCode != Aze::CError::eNone)
    {
        OUT_ERROR(Aze::CError::message(eBuildCode, sDetail));

        return eBuildCode;
    }

    // Tags already converge, nothing travels
    if (xData.getNodesByTagName(Aze::CStrings::s_sParamCommit).isEmpty())
        return Aze::CError::eNone;

    return postNetworkRequest(xData);
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::clone()
{
    // A target folder is mandatory, it is created when missing
    if (m_tArguments.m_lFilesAndIds.count() == 0 || m_tArguments.m_lFilesAndIds[0].isEmpty())
    {
        OUT_ERROR(Aze::CError::message(Aze::CError::eNoFileNameGiven));
        return Aze::CError::eNoFileNameGiven;
    }

    QString sTargetPath = QDir::current().absoluteFilePath(m_tArguments.m_lFilesAndIds[0]);

    CXMLNode xRequest = Aze::CRemoteRepository(m_pRepository).getCloneRequest();

    postNetworkRequest(xRequest);

    if (m_eLastError == Aze::CError::eNetworkError)
        return m_eLastError;

    // A clone payload carries no result attribute, anything else is a server error
    Aze::CError::EErrorCode eResponseCode = Aze::CError::fromResultNode(m_xLastResponse);

    if (eResponseCode != Aze::CError::eInvalidResponse)
        return eResponseCode;

    QString sDetail;
    Aze::CError::EErrorCode eCloneCode = Aze::CRepository::clone(sTargetPath, m_xLastResponse, sDetail, this);

    if (eCloneCode != Aze::CError::eNone)
    {
        OUT_ERROR(Aze::CError::message(eCloneCode, sDetail));
    }

    return eCloneCode;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::postNetworkRequest(CXMLNode xRequest)
{
    m_eLastError = Aze::CError::eNone;

    if (IS_NOT_NULL(m_pRepository))
    {
        QString sRemoteHostName = m_pRepository->remoteHostInfo()->name();

        // Keep the default host when the repository sets none,
        // cloning outside of a repository leaves the name empty
        if (not sRemoteHostName.isEmpty())
            setupRemoteHost(sRemoteHostName);
    }

    // Authentication is delegated to the frontal outside Aze (see server confinement only)

    QNetworkReply* pReply = m_pNetworkAccessManager->post(m_tNetworkRequest, xRequest.toJsonString().simplified().toUtf8());

    // Wait for the matching manager response without busy wait, the quit
    // handler is connected after onNetworkAccessFinished so m_eLastError
    // is stored before the local event loop returns
    QEventLoop oWaitLoop;
    QObject::connect(m_pNetworkAccessManager, &QNetworkAccessManager::finished, &oWaitLoop, [&](QNetworkReply* pFinishedReply) {
        if (pFinishedReply == pReply)
            oWaitLoop.quit();
    });
    oWaitLoop.exec();

    return m_eLastError;
}

//-------------------------------------------------------------------------------------------------

void CAzeClient::onNetworkAccessFinished(QNetworkReply* pReply)
{
    if (pReply->error() == QNetworkReply::NoError)
    {
        m_xLastResponse = CXMLNode::parseJSON(QString(pReply->readAll()));

        m_eLastError = Aze::CError::fromResultNode(m_xLastResponse);
    }
    else
    {
        m_xLastResponse = CXMLNode();
        m_eLastError = Aze::CError::eNetworkError;
    }

    pReply->deleteLater();
}

//-------------------------------------------------------------------------------------------------
