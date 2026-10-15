
// Qt
#include <QDebug>

// Aze
#include "CUtils.h"
#include "CStrings.h"
#include "CRemoteRepository.h"

// Application
#include "CAzeServerProcessor.h"
#include "CAzeServer.h"

/*!
    \class CAzeServer
    \inmodule Aze
    \section1 General
*/

//-------------------------------------------------------------------------------------------------

/*
    AZE Request format

    {
    "request": "<Type of request: pull or push>"
    "branch": "<Name of branch>"
    "root": "<Root commit Id of the pushed branch>"
    "tip": "<Tip commit Id>"
    }
    "commit": [
        { "id": "<Commit Id>", ...commit nodes... }
    ]
    "object": [
        { "id": "<Object Id>", "value": "B64:<Base64 content>" }
    ]
*/

//-------------------------------------------------------------------------------------------------

CAzeServerProcessor::CAzeServerProcessor(CAzeServer* parent, quint16 iPort)
    : CDynamicHTTPServer(iPort)
    , m_pServer(parent)
{
    m_mRequestFunctionMap[Aze::CStrings::s_sParamPull] = servePull;
    m_mRequestFunctionMap[Aze::CStrings::s_sParamPush] = servePush;
}

//-------------------------------------------------------------------------------------------------

CAzeServerProcessor::~CAzeServerProcessor()
{
}

//-------------------------------------------------------------------------------------------------

void CAzeServerProcessor::getContent(CWebContext& tContext, QString& sHead, QString& sBody, QString& sCustomResponse, QString& sCustomResponseMIME)
{
    Q_UNUSED(sHead)
    Q_UNUSED(sBody)

    if (tContext.pathValid() && not tContext.m_baPostContent.isEmpty())
    {
        if (tContext.m_sContentType == MIME_Content_JSON)
        {
            CXMLNode xRequest = CXMLNode::parseJSON(QString(tContext.m_baPostContent));

            // Compose repository name
            QString sRepositoryPath = QString("%1/%2")
                    .arg(m_pServer->repositoriesBasePath())
                    .arg(tContext.m_lPath.join(Aze::CStrings::s_sPathSep));

            Aze::CRepository repository(sRepositoryPath);

            if (repository.ok())
            {
                sCustomResponse = serveRequest(tContext, xRequest, &repository).toJsonString();
            }
            else
            {
                sCustomResponse = Aze::CError::toResultNode(Aze::CError::eNotARepository, sRepositoryPath).toJsonString();
            }

            sCustomResponseMIME = MIME_Content_JSON;
        }
        else
        {
            sCustomResponse = Aze::CError::toResultNode(Aze::CError::eInvalidResponse).toJsonString();
            sCustomResponseMIME = MIME_Content_JSON;
        }
    }
    else
    {
        sCustomResponse = Aze::CError::toResultNode(Aze::CError::eEmptyRequest).toJsonString();
        sCustomResponseMIME = MIME_Content_JSON;
    }
}

//-------------------------------------------------------------------------------------------------

CXMLNode CAzeServerProcessor::serveRequest(CWebContext& tContext, const CXMLNode& xRequest, Aze::CRepository* pRepository)
{
    QString sAction = xRequest.attributes()[Aze::CStrings::s_sParamRequest];

    if (sAction.isEmpty())
        return Aze::CError::toResultNode(Aze::CError::eEmptyRequest);

    // Get the IP
//        QString sIP = tContext.m_sPeer;

    if (IS_NOT_NULL(m_mRequestFunctionMap[sAction]))
    {
        return m_mRequestFunctionMap[sAction](pRepository, tContext, xRequest);
    }

    return Aze::CError::toResultNode(Aze::CError::eUnknownAction, sAction);
}

//-------------------------------------------------------------------------------------------------

CXMLNode CAzeServerProcessor::servePull(Aze::CRepository* pRepository, CWebContext& tContext, const CXMLNode& xRequest)
{
    Q_UNUSED(pRepository)
    Q_UNUSED(tContext)
    Q_UNUSED(xRequest)

    // TODO: implement pull payload handling (TODO-018, TODO-019, TODO-020)
    return Aze::CError::toResultNode(Aze::CError::eNone);
}

//-------------------------------------------------------------------------------------------------

CXMLNode CAzeServerProcessor::servePush(Aze::CRepository* pRepository, CWebContext& tContext, const CXMLNode& xRequest)
{
    Q_UNUSED(tContext)

    QString sDetail;

    Aze::CRemoteRepository oRemoteRepository(pRepository);

    Aze::CError::EErrorCode eCode = oRemoteRepository.applyPushRequest(xRequest, sDetail);

    return Aze::CError::toResultNode(eCode, sDetail);
}

//-------------------------------------------------------------------------------------------------
