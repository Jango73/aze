
// Qt
#include <QDebug>
#include <QDir>
#include <QDirIterator>

// qt-plus
#include <CXMLNode.h>
#include <File/CFileUtilities.h>

// Aze
#include "CUtils.h"

// Application
#include "CAzeServer.h"

/*!
    \class CAzeServer
    \inmodule Aze
    \section1 General
*/

//-------------------------------------------------------------------------------------------------

CAzeServer::CAzeServer(int argc, char *argv[])
    : QCoreApplication(argc, argv)
    , m_sRepositoriesBasePath(".")
    , m_tArguments(*this)
    , m_pProcessor(new CAzeServerProcessor(this, m_tArguments.m_tParser.value(m_tArguments.m_oHttpPort).toInt()))
    , m_eInitializationError(Aze::CError::eNone)
    , m_sInitializationDetail()
{
    const quint16 uiHttpsPort = static_cast<quint16>(m_tArguments.m_tParser.value(m_tArguments.m_oHttpsPort).toUInt());

    if (uiHttpsPort > 0)
    {
        const QString sCertPath = m_tArguments.m_tParser.value(m_tArguments.m_oSslCert);
        const QString sKeyPath = m_tArguments.m_tParser.value(m_tArguments.m_oSslKey);

        if (sCertPath.isEmpty() || sKeyPath.isEmpty())
        {
            m_eInitializationError = Aze::CError::eInvalidHttpsConfiguration;
            m_sInitializationDetail = QStringLiteral("--https-port requires --ssl-cert and --ssl-key");
            return;
        }

        if (not m_pProcessor->enableHttps(sCertPath, sKeyPath, uiHttpsPort))
        {
            m_eInitializationError = Aze::CError::eInvalidHttpsConfiguration;
            m_sInitializationDetail = QStringLiteral("Cannot enable HTTPS (invalid files or port already in use)");
        }
    }
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns the HTTPS initialization error stored by the constructor.
    Returns \c eNone when the server configuration is valid.
*/
Aze::CError::EErrorCode CAzeServer::initializationError() const
{
    return m_eInitializationError;
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns the detail of the HTTPS initialization error.
*/
QString CAzeServer::initializationErrorDetail() const
{
    return m_sInitializationDetail;
}

//-------------------------------------------------------------------------------------------------

/*!
    Runs the server event loop when the HTTPS configuration is valid.
    Returns a clean exit code without entering the event loop otherwise.
*/
int CAzeServer::run()
{
    if (m_eInitializationError != Aze::CError::eNone)
    {
        OUT_ERROR(Aze::CError::message(m_eInitializationError, m_sInitializationDetail));
        return Aze::CError::toExitCode(m_eInitializationError);
    }

    return exec();
}

//-------------------------------------------------------------------------------------------------

CAzeServer::~CAzeServer()
{
}

//-------------------------------------------------------------------------------------------------
