
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
{
    const quint16 uiHttpsPort = static_cast<quint16>(m_tArguments.m_tParser.value(m_tArguments.m_oHttpsPort).toUInt());

    if (uiHttpsPort > 0)
    {
        const QString sCertPath = m_tArguments.m_tParser.value(m_tArguments.m_oSslCert);
        const QString sKeyPath = m_tArguments.m_tParser.value(m_tArguments.m_oSslKey);

        if (sCertPath.isEmpty() || sKeyPath.isEmpty())
        {
            qFatal("CAzeServer: --https-port requires --ssl-cert and --ssl-key.");
        }

        if (not m_pProcessor->enableHttps(sCertPath, sKeyPath, uiHttpsPort))
        {
            qFatal("CAzeServer: cannot enable HTTPS (invalid files or port already in use).");
        }
    }
}

//-------------------------------------------------------------------------------------------------

CAzeServer::~CAzeServer()
{
}

//-------------------------------------------------------------------------------------------------
