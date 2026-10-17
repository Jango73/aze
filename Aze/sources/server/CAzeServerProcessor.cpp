
// Qt
#include <QDebug>
#include <QDir>
#include <QFileInfo>

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
    "request": "<Type of request: pull, push, clone or delete-branch>"
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

    A clone request carries no payload besides "request": "clone".
    A clone response carries every branch with its name plus every
    commit and object of the served repository.
    A pull request carries the requested branch and the client tip.
    A pull response carries the requested branch with its name plus
    every commit reachable from the server tip and every object
    referenced by those commits.
    A delete-branch request carries the branch name and the force flag.
*/

//-------------------------------------------------------------------------------------------------

CAzeServerProcessor::CAzeServerProcessor(CAzeServer* parent, quint16 iPort)
    : CDynamicHTTPServer(iPort)
    , m_pServer(parent)
{
    m_mRequestFunctionMap[Aze::CStrings::s_sParamPull] = servePull;
    m_mRequestFunctionMap[Aze::CStrings::s_sParamPush] = servePush;
    m_mRequestFunctionMap[Aze::CStrings::s_sParamClone] = serveClone;
    m_mRequestFunctionMap[Aze::CStrings::s_sParamDeleteBranch] = serveDeleteBranch;
}

//-------------------------------------------------------------------------------------------------

/************************************************************************/

/*!
    Sets the post-serve hook invoked after each served repository request. \br\br
    The hook receives the served repository folder path and the request
    action once the response is built. It runs best-effort and never alters
    the response. Set once before serving. \br
    \a tHook is the hook to invoke, empty clears it \br
*/
void CAzeServerProcessor::setPostServeHook(const PostServeHook& tHook)
{
    m_tPostServeHook = tHook;
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
            // Confine the requested repository under the base folder
            QString sRepositoryPath;

            if (not resolveRepositoryPath(m_pServer->repositoriesBasePath(), tContext.m_lPath, sRepositoryPath))
            {
                sCustomResponse = Aze::CError::toResultNode(Aze::CError::eNotARepository, tContext.m_lPath.join(Aze::CStrings::s_sPathSep)).toJsonString();
                sCustomResponseMIME = MIME_Content_JSON;
                return;
            }

            CXMLNode xRequest = CXMLNode::parseJSON(QString(tContext.m_baPostContent));

            Aze::CRepository repository(sRepositoryPath);

            if (repository.ok())
            {
                CXMLNode xResponse = serveRequest(tContext, xRequest, &repository);

                sCustomResponse = xResponse.toJsonString();

                if (m_tPostServeHook)
                    m_tPostServeHook(sRepositoryPath, xRequest.attributes()[Aze::CStrings::s_sParamRequest]);
            }
            else
            {
                sCustomResponse = Aze::CError::toResultNode(Aze::CError::eNotARepository, tContext.m_lPath.join(Aze::CStrings::s_sPathSep)).toJsonString();
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

/************************************************************************/

/*!
    Resolves \a lSegments under \a sBasePath into \a sResolvedPath. \br\br
    Rejects empty segments, current and parent folder markers and drive
    specifications, then verifies the cleaned path stays under the base
    folder, including canonical resolution against symbolic links when the
    folders exist on disk. Returns false on any traversal attempt.
*/
bool CAzeServerProcessor::resolveRepositoryPath(const QString& sBasePath, const QStringList& lSegments, QString& sResolvedPath)
{
    if (lSegments.isEmpty())
        return false;

    for (const QString& sSegment : lSegments)
    {
        if (sSegment.isEmpty() || sSegment == QStringLiteral(".") || sSegment == QStringLiteral(".."))
            return false;

        if (sSegment.contains(QLatin1Char('/')) || sSegment.contains(QLatin1Char('\\')) || sSegment.contains(QLatin1Char(':')))
            return false;
    }

    const QString sBaseAbsolute = QDir::cleanPath(QDir(sBasePath).absolutePath());
    const QString sCandidate = QDir::cleanPath(sBaseAbsolute + QLatin1Char('/') + lSegments.join(QLatin1Char('/')));

    if (sCandidate != sBaseAbsolute && not sCandidate.startsWith(sBaseAbsolute + QLatin1Char('/')))
        return false;

    const QString sCanonicalBase = QFileInfo(sBaseAbsolute).canonicalFilePath();
    const QString sCanonicalCandidate = QFileInfo(sCandidate).canonicalFilePath();

    if (not sCanonicalBase.isEmpty() && not sCanonicalCandidate.isEmpty())
    {
        if (sCanonicalCandidate != sCanonicalBase && not sCanonicalCandidate.startsWith(sCanonicalBase + QLatin1Char('/')))
            return false;

        sResolvedPath = sCanonicalCandidate;
        return true;
    }

    sResolvedPath = sCandidate;
    return true;
}

/************************************************************************/

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
    Q_UNUSED(tContext)

    Aze::CRemoteRepository oRemoteRepository(pRepository);

    return oRemoteRepository.getPullResponse(xRequest);
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

CXMLNode CAzeServerProcessor::serveClone(Aze::CRepository* pRepository, CWebContext& tContext, const CXMLNode& xRequest)
{
    Q_UNUSED(tContext)
    Q_UNUSED(xRequest)

    Aze::CRemoteRepository oRemoteRepository(pRepository);

    return oRemoteRepository.getCloneResponse();
}

//-------------------------------------------------------------------------------------------------

CXMLNode CAzeServerProcessor::serveDeleteBranch(Aze::CRepository* pRepository, CWebContext& tContext, const CXMLNode& xRequest)
{
    Q_UNUSED(tContext)

    QString sDetail;

    Aze::CRemoteRepository oRemoteRepository(pRepository);

    Aze::CError::EErrorCode eCode = oRemoteRepository.applyDeleteBranchRequest(xRequest, sDetail);

    return Aze::CError::toResultNode(eCode, sDetail);
}

//-------------------------------------------------------------------------------------------------
