
#pragma once

// Qt
#include <QObject>
#include <QHostAddress>

// Standard
#include <functional>

// qt-plus
#include "Web/CDynamicHTTPServer.h"

// AzeLib
#include "CRepository.h"

// Application
#include "../common/CConstants.h"

//-------------------------------------------------------------------------------------------------

class CAzeServerProcessor : public CDynamicHTTPServer
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Type definitions
    //-------------------------------------------------------------------------------------------------

    //! Generic post-serve hook, receives the served repository folder path and the request action
    //! Set once before serving, invoked best-effort after the response is built, never altering it
    typedef std::function<void(const QString&, const QString&)> PostServeHook;

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //! Constructor, binds all interfaces
    explicit CAzeServerProcessor(quint16 iPort = DEFAULT_HTTP_PORT);

    //! Constructor, binds tListenAddress only so portal-only deployments never listen on all interfaces
    CAzeServerProcessor(quint16 iPort, const QHostAddress& tListenAddress);

    //! Destructor
    virtual ~CAzeServerProcessor() override;

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    virtual void getContent(CWebContext& tContext, QString& sHead, QString& sBody, QString& sCustomResponse, QString& sCustomResponseMIME) override;

    //! Sets the post-serve hook invoked after each served repository request
    void setPostServeHook(const PostServeHook& tHook);

    //! Sets the canonical base folder served repositories resolve under
    void setRepositoriesBasePath(const QString& sBasePath);

    //! Returns the canonical base folder served repositories resolve under
    QString repositoriesBasePath() const;

    //! Sets the largest accepted POST body in bytes, rejected before parsing
    void setMaxPostBodyBytes(qint64 iMax);

    //! Sets the largest accepted commit node count in a push payload
    void setMaxCommitsPerPush(int iMax);

    //! Sets the largest accepted decoded object size in bytes in a push payload
    void setMaxObjectBytes(qint64 iMax);

protected:

    //-------------------------------------------------------------------------------------------------
    // Protected control methods
    //-------------------------------------------------------------------------------------------------

    //!
    CXMLNode serveRequest(CWebContext& tContext, const CXMLNode& xRequest, Aze::CRepository* pRepository);

    //! Resolves request path segments under sBasePath, returns false on traversal attempt
    static bool resolveRepositoryPath(const QString& sBasePath, const QStringList& lSegments, QString& sResolvedPath);

    //! Checks push payload node counts and object sizes against the limits, returns eNone when accepted
    static Aze::CError::EErrorCode checkPushPayloadLimits(const CXMLNode& xRequest, int iMaxCommits, qint64 iMaxObjectBytes, QString& sDetail);

    //! Returns true when sCandidate equals sBasePath or stays under it
    static bool pathStaysUnderBase(const QString& sBasePath, const QString& sCandidate);

    //!
    static CXMLNode servePull(Aze::CRepository* pRepository, CWebContext& tContext, const CXMLNode& xRequest);

    //!
    static CXMLNode servePush(Aze::CRepository* pRepository, CWebContext& tContext, const CXMLNode& xRequest);

    //!
    static CXMLNode servePushAnnounce(Aze::CRepository* pRepository, CWebContext& tContext, const CXMLNode& xRequest);

    //!
    static CXMLNode serveClone(Aze::CRepository* pRepository, CWebContext& tContext, const CXMLNode& xRequest);

    //!
    static CXMLNode serveDeleteBranch(Aze::CRepository* pRepository, CWebContext& tContext, const CXMLNode& xRequest);

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    typedef CXMLNode (*RequestServing) (Aze::CRepository* pRepository, CWebContext& tContext, const CXMLNode& xRequest);

    QMap<QString, RequestServing>   m_mRequestFunctionMap;
    PostServeHook                   m_tPostServeHook;
    QString                         m_sBasePath;
    qint64                          m_iMaxPostBodyBytes;
    int                             m_iMaxCommitsPerPush;
    qint64                          m_iMaxObjectBytes;
};

//-------------------------------------------------------------------------------------------------
