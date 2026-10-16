
// Qt
#include <QDebug>
#include <QDir>
#include <QMutex>
#include <QWaitCondition>

// qt-plus
#include <CXMLNode.h>

// Aze
#include "CRemoteRepository.h"

// Application
#include "CAzeClient.h"

//-------------------------------------------------------------------------------------------------

static void msleep(unsigned long msecs)
{
    QMutex mutex;
    mutex.lock();

    QWaitCondition waitCondition;
    waitCondition.wait(&mutex, msecs);

    mutex.unlock(); // Not necessary since new mutex will always be created,
    // but since destroying locked mutex
    // is bringing undefined behavior, let's follow some ethics
}

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

    CXMLNode xRequest = Aze::CRemoteRepository(m_pRepository).getPushRequest();

    return postNetworkRequest(xRequest);
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
    m_bNetworkAccessFinished = false;

    if (IS_NOT_NULL(m_pRepository))
    {
        QString sRemoteHostName = m_pRepository->remoteHostInfo()->name();

        // Keep the default host when the repository sets none,
        // cloning outside of a repository leaves the name empty
        if (not sRemoteHostName.isEmpty())
            setupRemoteHost(sRemoteHostName);
    }

    // Send the bearer token when one is configured, the command line
    // switch takes precedence over the AZE_AUTH_TOKEN environment variable
    QString sAuthToken = m_tArguments.m_tParser.value(m_tArguments.m_oAuthToken);

    if (sAuthToken.isEmpty())
        sAuthToken = QString::fromLocal8Bit(qgetenv("AZE_AUTH_TOKEN"));

    if (not sAuthToken.isEmpty())
        m_tNetworkRequest.setRawHeader("Authorization", QString("Bearer %1").arg(sAuthToken).toUtf8());

    m_pNetworkAccessManager->post(m_tNetworkRequest, xRequest.toJsonString().simplified().toUtf8());

    while (not m_bNetworkAccessFinished)
    {
        msleep(10);
        QCoreApplication::processEvents();
    }

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

    m_bNetworkAccessFinished = true;
}

//-------------------------------------------------------------------------------------------------
