
// Qt
#include <QDebug>

// qt-plus
#include "Web/CHTTPServer.h"

// Application
#include "CAzeServerArguments.h"

/*!
    \class CAzeServerArguments
    \inmodule Aze
    \section1 General
*/

#define TRT(a) QCoreApplication::translate(CConstants::s_sContextMain, a)

//-------------------------------------------------------------------------------------------------

CAzeServerArguments::CAzeServerArguments(QCoreApplication& app)
    : QObject(nullptr)
    , m_oHttpPort(QStringList() << CConstants::s_sSwitchPort << "p", TRT("HTTP listening port."), TRT(CConstants::s_sSwitchPort), QString::number(DEFAULT_HTTP_PORT))
    , m_oHttpsPort(QStringList() << CConstants::s_sSwitchHttpsPort, TRT("HTTPS listening port (0 disables HTTPS)."), TRT(CConstants::s_sSwitchHttpsPort), QString::number(0))
    , m_oSslCert(QStringList() << CConstants::s_sSwitchSslCert, TRT("SSL certificate file in PEM format."), TRT(CConstants::s_sSwitchSslCert), QString())
    , m_oSslKey(QStringList() << CConstants::s_sSwitchSslKey, TRT("SSL private key file in PEM format."), TRT(CConstants::s_sSwitchSslKey), QString())
    , m_oAuthToken(QStringList() << CConstants::s_sSwitchAuthToken, TRT("Global bearer token required by clients (empty disables authentication)."), TRT(CConstants::s_sSwitchAuthToken), QString())
{
    CConstants::initCommandMap();

    // Application name and version
    QCoreApplication::setApplicationName("Aze");
    QCoreApplication::setApplicationVersion(AZE_VERSION_STRING);

    // Set option array
    m_tParser.addOptions({ m_oHttpPort, m_oHttpsPort, m_oSslCert, m_oSslKey, m_oAuthToken });

    // Finalize option array
    m_tParser.addHelpOption();
    m_tParser.addVersionOption();
    m_tParser.process(app);
}

//-------------------------------------------------------------------------------------------------
