
// Qt
#include <QDebug>
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

    return postNetworkRequest(xRequest);
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::push()
{
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    CXMLNode xRequest = Aze::CRemoteRepository(m_pRepository).getPushRequest();

    return postNetworkRequest(xRequest);
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::postNetworkRequest(CXMLNode xRequest)
{
    m_eLastError = Aze::CError::eNone;
    m_bNetworkAccessFinished = false;

    if (IS_NOT_NULL(m_pRepository))
    {
        setupRemoteHost(m_pRepository->remoteHostInfo()->name());
    }

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
        CXMLNode xResponse = CXMLNode::parseJSON(QString(pReply->readAll()));

        m_eLastError = Aze::CError::fromResultNode(xResponse);
    }
    else
    {
        m_eLastError = Aze::CError::eNetworkError;
    }

    m_bNetworkAccessFinished = true;
}

//-------------------------------------------------------------------------------------------------
