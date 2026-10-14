
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
                sCustomResponseMIME = MIME_Content_JSON;
            }
            else
            {
                CXMLNode xResult;
                xResult.attributes()[Aze::CStrings::s_sParamResult] = Aze::CStrings::s_sParamError;
            }
        }
    }
    else
    {
        // CDynamicHTTPServer::getContent(tContext, sHead, sBody, sCustomResponse, sCustomResponseMIME);
    }
}

//-------------------------------------------------------------------------------------------------

CXMLNode CAzeServerProcessor::serveRequest(CWebContext& tContext, const CXMLNode& xRequest, Aze::CRepository* pRepository)
{
    CXMLNode xResult;
    QString sAction = xRequest.attributes()[Aze::CStrings::s_sParamRequest];

    if (not sAction.isEmpty())
    {
        // Get the IP
//        QString sIP = tContext.m_sPeer;

        if (IS_NOT_NULL(m_mRequestFunctionMap[sAction]))
        {
            return m_mRequestFunctionMap[sAction](pRepository, tContext, xRequest);
        }
    }

    return xResult;
}

//-------------------------------------------------------------------------------------------------

CXMLNode CAzeServerProcessor::servePull(Aze::CRepository* pRepository, CWebContext& tContext, const CXMLNode& xRequest)
{
    Q_UNUSED(pRepository)
    Q_UNUSED(tContext)
    Q_UNUSED(xRequest)

    CXMLNode xResult;

    xResult.attributes()[Aze::CStrings::s_sParamResult] = Aze::CStrings::s_sParamSuccess;

    // TODO: implement

    return xResult;
}

//-------------------------------------------------------------------------------------------------

CXMLNode CAzeServerProcessor::servePush(Aze::CRepository* pRepository, CWebContext& tContext, const CXMLNode& xRequest)
{
    Q_UNUSED(tContext)

    CXMLNode xResult;
    QString sErrorMessage;

    Aze::CRemoteRepository oRemoteRepository(pRepository);

    if (oRemoteRepository.applyPushRequest(xRequest, sErrorMessage))
    {
        xResult.attributes()[Aze::CStrings::s_sParamResult] = Aze::CStrings::s_sParamSuccess;
    }
    else
    {
        xResult.attributes()[Aze::CStrings::s_sParamResult] = Aze::CStrings::s_sParamError;
        xResult.attributes()[Aze::CStrings::s_sParamMessage] = sErrorMessage;
    }

    return xResult;
}

//-------------------------------------------------------------------------------------------------
