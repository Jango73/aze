
#pragma once

// Qt
#include <QObject>

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
};

//-------------------------------------------------------------------------------------------------
