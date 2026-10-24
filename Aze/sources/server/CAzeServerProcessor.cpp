
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

/*!
    \class CAzeServerProcessor
    \inmodule Aze
    \section1 General
*/

//-------------------------------------------------------------------------------------------------

/*
    AZE Request format

    {
    "request": "<Type of request: pull, push, push-announce, clone or delete-branch>"
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
    A push announce carries the branch and the client tip, the response
    carries the branch and the server tip so the data request carries
    missing commits only.
    A pull request carries the requested branch and the client tip.
    A pull response carries the requested branch with its name, the server
    tip plus the commits missing on the client and the objects unreferenced
    by the shared history. Attributes only travel when both tips match,
    the full history travels on unknown or divergent client tip.
    A delete-branch request carries the branch name and the force flag.
*/

//-------------------------------------------------------------------------------------------------

CAzeServerProcessor::CAzeServerProcessor(quint16 iPort)
    : CAzeServerProcessor(iPort, QHostAddress::Any)
{
}

//-------------------------------------------------------------------------------------------------

CAzeServerProcessor::CAzeServerProcessor(quint16 iPort, const QHostAddress& tListenAddress)
    : CDynamicHTTPServer(iPort, tListenAddress)
    , m_sBasePath(QDir::currentPath())
    , m_iMaxPostBodyBytes(32 * 1024 * 1024)
    , m_iMaxCommitsPerPush(100000)
    , m_iMaxObjectBytes(64 * 1024 * 1024)
{
    setRepositoriesBasePath(QDir::currentPath());

    m_mRequestFunctionMap[Aze::CStrings::s_sParamPull] = servePull;
    m_mRequestFunctionMap[Aze::CStrings::s_sParamPush] = servePush;
    m_mRequestFunctionMap[Aze::CStrings::s_sParamPushAnnounce] = servePushAnnounce;
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

/*!
    Sets the canonical base folder served repositories resolve under. \br\br
    The folder is stored in canonical absolute form so later containment
    checks compare canonical paths only. An empty canonical form keeps the
    previous setting. \br
    \a sBasePath is the folder holding the served repositories \br
*/
void CAzeServerProcessor::setRepositoriesBasePath(const QString& sBasePath)
{
    const QString sAbsolute = QDir::cleanPath(QDir(sBasePath).absolutePath());
    const QString sCanonical = QFileInfo(sAbsolute).canonicalFilePath();

    if (sCanonical.isEmpty())
        return;

    m_sBasePath = sCanonical;
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns the canonical base folder served repositories resolve under.
*/
QString CAzeServerProcessor::repositoriesBasePath() const
{
    return m_sBasePath;
}

//-------------------------------------------------------------------------------------------------

/*!
    Sets the largest accepted POST body in bytes. \br\br
    Larger bodies are refused before JSON parsing. Non-positive values keep
    the previous setting. \br
    \a iMax is the body size limit in bytes \br
*/
void CAzeServerProcessor::setMaxPostBodyBytes(qint64 iMax)
{
    if (iMax > 0)
        m_iMaxPostBodyBytes = iMax;
}

//-------------------------------------------------------------------------------------------------

/*!
    Sets the largest accepted commit node count in a push payload. \br\br
    Non-positive values keep the previous setting. \br
    \a iMax is the commit node limit \br
*/
void CAzeServerProcessor::setMaxCommitsPerPush(int iMax)
{
    if (iMax > 0)
        m_iMaxCommitsPerPush = iMax;
}

//-------------------------------------------------------------------------------------------------

/*!
    Sets the largest accepted decoded object size in bytes in a push payload. \br\br
    Non-positive values keep the previous setting. \br
    \a iMax is the object size limit in bytes \br
*/
void CAzeServerProcessor::setMaxObjectBytes(qint64 iMax)
{
    if (iMax > 0)
        m_iMaxObjectBytes = iMax;
}

//-------------------------------------------------------------------------------------------------

/*!
    Checks push payload node counts and object sizes against the limits. \br\br
    Counts commit and object nodes, estimates decoded object sizes from the
    base64 text length, sets \a sDetail and returns eRequestTooLarge on the
    first exceeded limit, eNone when the payload fits. \br
    \a xRequest is the parsed push payload, \a iMaxCommits bounds commit nodes,
    \a iMaxObjectBytes bounds decoded object bytes \br
*/
Aze::CError::EErrorCode CAzeServerProcessor::checkPushPayloadLimits(const CXMLNode& xRequest, int iMaxCommits, qint64 iMaxObjectBytes, QString& sDetail)
{
    CXMLNodeList lCommits = xRequest.getNodesByTagName(Aze::CStrings::s_sParamCommit);

    if (lCommits.count() > iMaxCommits)
    {
        sDetail = QStringLiteral("push payload holds %1 commits, limit is %2").arg(lCommits.count()).arg(iMaxCommits);
        return Aze::CError::eRequestTooLarge;
    }

    CXMLNodeList lObjects = xRequest.getNodesByTagName(Aze::CStrings::s_sParamObject);

    for (const CXMLNode& xObject : lObjects)
    {
        const QString sContent = xObject.value();
        const qint64 iDecodedBytes = (sContent.length() * 3) / 4;

        if (iDecodedBytes > iMaxObjectBytes)
        {
            sDetail = QStringLiteral("push object exceeds %1 bytes").arg(iMaxObjectBytes);
            return Aze::CError::eRequestTooLarge;
        }
    }

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns true when \a sCandidate equals \a sBasePath or stays under it. \br\br
    Both paths must be canonical absolute forms sharing the same root. \br
*/
bool CAzeServerProcessor::pathStaysUnderBase(const QString& sBasePath, const QString& sCandidate)
{
    return sCandidate == sBasePath || sCandidate.startsWith(sBasePath + QLatin1Char('/'));
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
            // Refuse oversized bodies before JSON parsing and repository work
            if (tContext.m_baPostContent.size() > m_iMaxPostBodyBytes)
            {
                sCustomResponse = Aze::CError::toResultNode(Aze::CError::eRequestTooLarge,
                    QStringLiteral("POST body holds %1 bytes, limit is %2").arg(tContext.m_baPostContent.size()).arg(m_iMaxPostBodyBytes)).toJsonString();
                sCustomResponseMIME = MIME_Content_JSON;
                return;
            }

            // Confine the requested repository under the base folder
            QString sRepositoryPath;

            if (not resolveRepositoryPath(m_sBasePath, tContext.m_lPath, sRepositoryPath))
            {
                sCustomResponse = Aze::CError::toResultNode(Aze::CError::eNotARepository, tContext.m_lPath.join(Aze::CStrings::s_sPathSep)).toJsonString();
                sCustomResponseMIME = MIME_Content_JSON;
                return;
            }

            CXMLNode xRequest = CXMLNode::parseJSON(QString(tContext.m_baPostContent));

            // Bound push payloads before storing anything
            if (xRequest.attributes()[Aze::CStrings::s_sParamRequest] == Aze::CStrings::s_sParamPush)
            {
                QString sLimitDetail;

                if (checkPushPayloadLimits(xRequest, m_iMaxCommitsPerPush, m_iMaxObjectBytes, sLimitDetail) != Aze::CError::eNone)
                {
                    sCustomResponse = Aze::CError::toResultNode(Aze::CError::eRequestTooLarge, sLimitDetail).toJsonString();
                    sCustomResponseMIME = MIME_Content_JSON;
                    return;
                }
            }

            Aze::CRepository repository(sRepositoryPath);

            if (repository.ok())
            {
                // Reverify containment after open so a swapped symlink between
                // resolution and open cannot redirect the served folder
                const QString sCanonicalServed = QFileInfo(sRepositoryPath).canonicalFilePath();

                if (sCanonicalServed.isEmpty() || not pathStaysUnderBase(m_sBasePath, sCanonicalServed))
                {
                    sCustomResponse = Aze::CError::toResultNode(Aze::CError::eNotARepository, tContext.m_lPath.join(Aze::CStrings::s_sPathSep)).toJsonString();
                }
                else
                {
                    CXMLNode xResponse = serveRequest(tContext, xRequest, &repository);

                    sCustomResponse = xResponse.toJsonString();

                    if (m_tPostServeHook)
                        m_tPostServeHook(sRepositoryPath, xRequest.attributes()[Aze::CStrings::s_sParamRequest]);
                }
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

    if (not pathStaysUnderBase(sBaseAbsolute, sCandidate))
        return false;

    const QString sCanonicalBase = QFileInfo(sBaseAbsolute).canonicalFilePath();
    const QString sCanonicalCandidate = QFileInfo(sCandidate).canonicalFilePath();

    if (not sCanonicalBase.isEmpty() && not sCanonicalCandidate.isEmpty())
    {
        if (not pathStaysUnderBase(sCanonicalBase, sCanonicalCandidate))
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

CXMLNode CAzeServerProcessor::servePushAnnounce(Aze::CRepository* pRepository, CWebContext& tContext, const CXMLNode& xRequest)
{
    Q_UNUSED(tContext)

    Aze::CRemoteRepository oRemoteRepository(pRepository);

    return oRemoteRepository.getPushAnnounceResponse(xRequest);
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
