
#pragma once

// Qt
#include <QObject>

// Standard
#include <functional>

// qt-plus
#include "Web/CDynamicHTTPServer.h"

// AzeLib
#include "CRepository.h"

// Application
#include "../common/CConstants.h"

//-------------------------------------------------------------------------------------------------

class CAzeServer;

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

    //! Constructor
    CAzeServerProcessor(CAzeServer* parent, quint16 iPort = DEFAULT_HTTP_PORT);

    //! Destructor
    virtual ~CAzeServerProcessor() override;

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    virtual void getContent(CWebContext& tContext, QString& sHead, QString& sBody, QString& sCustomResponse, QString& sCustomResponseMIME) override;

    //! Sets the post-serve hook invoked after each served repository request
    void setPostServeHook(const PostServeHook& tHook);

protected:

    //-------------------------------------------------------------------------------------------------
    // Protected control methods
    //-------------------------------------------------------------------------------------------------

    //!
    CXMLNode serveRequest(CWebContext& tContext, const CXMLNode& xRequest, Aze::CRepository* pRepository);

    //! Resolves request path segments under sBasePath, returns false on traversal attempt
    static bool resolveRepositoryPath(const QString& sBasePath, const QStringList& lSegments, QString& sResolvedPath);

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

    CAzeServer*                     m_pServer;
    QMap<QString, RequestServing>   m_mRequestFunctionMap;
    PostServeHook                   m_tPostServeHook;
};

//-------------------------------------------------------------------------------------------------
