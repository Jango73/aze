
// Qt
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslError>
#include <QSslKey>
#include <QSslSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

// STL
#include <vector>

// AzeLib
#include "CRepository.h"
#include "CRemoteRepository.h"
#include "CError.h"
#include "CStrings.h"
#include "CUtils.h"
#include <CXMLNode.h>

// Application
#include "../server/CAzeServer.h"
#include "../server/CAzeServerProcessor.h"
#include "CTestServe.h"
#include "CTestMacros.h"

// Exposes the protected server handlers to the test without touching
// production headers: servePush and serveDeleteBranch are static,
// so no server instance is needed.
class CServePushProbe : public CAzeServerProcessor
{
public:
    using CAzeServerProcessor::servePush;
    using CAzeServerProcessor::servePushAnnounce;
    using CAzeServerProcessor::servePull;
    using CAzeServerProcessor::serveDeleteBranch;
    using CAzeServerProcessor::resolveRepositoryPath;
    using CAzeServerProcessor::checkPushPayloadLimits;
    using CAzeServerProcessor::pathStaysUnderBase;
};

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Returns a free local TCP port, 0 when none is available.
*/
/***************************************************************************/
static quint16 connectivityFreePort()
{
    QTcpServer tServer;

    if (not tServer.listen(QHostAddress::LocalHost, 0))
        return 0;

    quint16 uiPort = tServer.serverPort();
    tServer.close();

    return uiPort;
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Generates a self-signed certificate for local HTTPS tests.
* \param sCertPath Output certificate file in PEM format.
* \param sKeyPath Output private key file in PEM format.
* \return True when openssl wrote both files.
*/
/***************************************************************************/
static bool generateConnectivityCertificate(const QString& sCertPath, const QString& sKeyPath)
{
    QProcess tProcess;
    tProcess.start(QStringLiteral("openssl"), QStringList()
        << QStringLiteral("req")
        << QStringLiteral("-x509")
        << QStringLiteral("-newkey")
        << QStringLiteral("rsa:2048")
        << QStringLiteral("-nodes")
        << QStringLiteral("-keyout") << sKeyPath
        << QStringLiteral("-out") << sCertPath
        << QStringLiteral("-days") << QStringLiteral("2")
        << QStringLiteral("-subj") << QStringLiteral("/CN=localhost")
        << QStringLiteral("-addext") << QStringLiteral("subjectAltName=DNS:localhost,IP:127.0.0.1"));

    if (not tProcess.waitForFinished(30000))
        return false;

    return tProcess.exitCode() == 0 && QFile::exists(sCertPath) && QFile::exists(sKeyPath);
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Runs one openssl command in a folder, true when it exits with code 0.
* \param sWorkingFolder Folder the command runs in, keeps serial files next to the certificates.
* \param lArguments Command line arguments passed to openssl.
* \return True when openssl exits with code 0 before the timeout.
*/
/***************************************************************************/
static bool runOpensslCommand(const QString& sWorkingFolder, const QStringList& lArguments)
{
    QProcess tProcess;
    tProcess.setWorkingDirectory(sWorkingFolder);
    tProcess.start(QStringLiteral("openssl"), lArguments);

    if (not tProcess.waitForFinished(60000))
        return false;

    return tProcess.exitCode() == 0;
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Generates a portal CA with a server and a portal client certificate chained to it.
* \param sFolder Output folder, created when missing, holds every generated file.
* \param sCaPath Receives the portal CA certificate file in PEM format.
* \param sServerCertPath Receives the server certificate file in PEM format.
* \param sServerKeyPath Receives the server private key file in PEM format.
* \param sClientCertPath Receives the portal client certificate file in PEM format.
* \param sClientKeyPath Receives the portal client private key file in PEM format.
* \return True when openssl wrote every file.
*/
/***************************************************************************/
static bool generateMutualTlsCertificates(const QString& sFolder, QString& sCaPath, QString& sServerCertPath, QString& sServerKeyPath, QString& sClientCertPath, QString& sClientKeyPath)
{
    QDir oFolder(sFolder);

    if (oFolder.exists() && not oFolder.removeRecursively())
        return false;

    if (not QDir().mkpath(sFolder))
        return false;

    sCaPath = oFolder.absoluteFilePath("portal-ca.crt");
    sServerCertPath = oFolder.absoluteFilePath("server.crt");
    sServerKeyPath = oFolder.absoluteFilePath("server.key");
    sClientCertPath = oFolder.absoluteFilePath("portal-client.crt");
    sClientKeyPath = oFolder.absoluteFilePath("portal-client.key");

    const QString sCaKey = oFolder.absoluteFilePath("portal-ca.key");
    const QString sServerCsr = oFolder.absoluteFilePath("server.csr");
    const QString sClientCsr = oFolder.absoluteFilePath("portal-client.csr");
    const QString sSanPath = oFolder.absoluteFilePath("server-san.ext");

    QFile fSan(sSanPath);

    if (not fSan.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    fSan.write("subjectAltName=DNS:localhost,IP:127.0.0.1\n");
    fSan.close();

    if (not runOpensslCommand(sFolder, QStringList()
        << QStringLiteral("req") << QStringLiteral("-x509")
        << QStringLiteral("-newkey") << QStringLiteral("rsa:2048")
        << QStringLiteral("-nodes")
        << QStringLiteral("-keyout") << sCaKey
        << QStringLiteral("-out") << sCaPath
        << QStringLiteral("-days") << QStringLiteral("2")
        << QStringLiteral("-subj") << QStringLiteral("/CN=test-portal-ca")))
        return false;

    if (not runOpensslCommand(sFolder, QStringList()
        << QStringLiteral("req")
        << QStringLiteral("-newkey") << QStringLiteral("rsa:2048")
        << QStringLiteral("-nodes")
        << QStringLiteral("-keyout") << sServerKeyPath
        << QStringLiteral("-out") << sServerCsr
        << QStringLiteral("-subj") << QStringLiteral("/CN=localhost")))
        return false;

    if (not runOpensslCommand(sFolder, QStringList()
        << QStringLiteral("x509") << QStringLiteral("-req")
        << QStringLiteral("-in") << sServerCsr
        << QStringLiteral("-CA") << sCaPath
        << QStringLiteral("-CAkey") << sCaKey
        << QStringLiteral("-CAcreateserial")
        << QStringLiteral("-out") << sServerCertPath
        << QStringLiteral("-days") << QStringLiteral("2")
        << QStringLiteral("-extfile") << sSanPath))
        return false;

    if (not runOpensslCommand(sFolder, QStringList()
        << QStringLiteral("req")
        << QStringLiteral("-newkey") << QStringLiteral("rsa:2048")
        << QStringLiteral("-nodes")
        << QStringLiteral("-keyout") << sClientKeyPath
        << QStringLiteral("-out") << sClientCsr
        << QStringLiteral("-subj") << QStringLiteral("/CN=test-portal")))
        return false;

    if (not runOpensslCommand(sFolder, QStringList()
        << QStringLiteral("x509") << QStringLiteral("-req")
        << QStringLiteral("-in") << sClientCsr
        << QStringLiteral("-CA") << sCaPath
        << QStringLiteral("-CAkey") << sCaKey
        << QStringLiteral("-CAcreateserial")
        << QStringLiteral("-out") << sClientCertPath
        << QStringLiteral("-days") << QStringLiteral("2")))
        return false;

    return QFile::exists(sCaPath) && QFile::exists(sServerCertPath) && QFile::exists(sServerKeyPath)
        && QFile::exists(sClientCertPath) && QFile::exists(sClientKeyPath);
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Builds a mutable argv array from a string list for in-process server construction.
* \param lArguments Command line tokens, the first one is the binary path.
* \param lStorage Receives the token buffers, keeps the pointers valid.
* \param vArgv Receives the null-terminated pointer array entries.
* \return The argument count.
*/
/***************************************************************************/
static int buildServerArguments(const QStringList& lArguments, QList<QByteArray>& lStorage, std::vector<char*>& vArgv)
{
    lStorage.clear();
    vArgv.clear();

    for (const QString& sArgument : lArguments)
        lStorage.append(sArgument.toLocal8Bit());

    for (QByteArray& baArgument : lStorage)
        vArgv.push_back(baArgument.data());

    return static_cast<int>(vArgv.size());
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Performs a synchronous HTTP request against the test server.
* \param pManager Network access manager running in the test thread.
* \param tRequest Request holding the URL, headers and TLS configuration.
* \param baPostContent POST body, null for a GET request.
* \param baBody Receives the response body.
* \param sError Receives the failure reason.
* \return True when the reply completes without error nor timeout.
*/
/***************************************************************************/
static bool fetchConnectivityResponse(QNetworkAccessManager* pManager, const QNetworkRequest& tRequest, const QByteArray& baPostContent, QByteArray& baBody, QString& sError)
{
    QEventLoop tLoop;
    QTimer tTimer;
    tTimer.setSingleShot(true);
    QObject::connect(&tTimer, &QTimer::timeout, &tLoop, &QEventLoop::quit);

    QNetworkReply* pReply = baPostContent.isNull() ? pManager->get(tRequest) : pManager->post(tRequest, baPostContent);
    QObject::connect(pReply, &QNetworkReply::sslErrors, pReply, [pReply](const QList<QSslError>& lErrors) {
        Q_UNUSED(lErrors);
        pReply->ignoreSslErrors();
    });
    QObject::connect(pReply, &QNetworkReply::finished, &tLoop, &QEventLoop::quit);

    tTimer.start(10000);
    tLoop.exec();

    bool bTimedOut = not tTimer.isActive();
    tTimer.stop();

    if (bTimedOut)
    {
        sError = QStringLiteral("Request timed out");
        pReply->abort();
        pReply->deleteLater();
        return false;
    }

    if (pReply->error() != QNetworkReply::NoError)
    {
        sError = pReply->errorString();
        pReply->deleteLater();
        return false;
    }

    baBody = pReply->readAll();
    pReply->deleteLater();

    return true;
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tells if a response body holds the empty-request JSON result.
* \param baBody Response body.
* \return True when the body parses to eEmptyRequest.
*/
/***************************************************************************/
static bool isEmptyRequestResponse(const QByteArray& baBody)
{
    if (baBody.isEmpty())
        return false;

    CXMLNode xNode = CXMLNode::parseJSON(QString::fromUtf8(baBody));

    return Aze::CError::fromResultNode(xNode) == Aze::CError::eEmptyRequest;
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Constructor.
* \param sArgument0 Binary path, forwarded to the fixture.
*/
/***************************************************************************/
CTestServe::CTestServe(const QString& sArgument0)
    : CTestFixture(sArgument0)
{
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests push request applying through the server layer.
*/
/***************************************************************************/
void CTestServe::testServePush()
{
    QString sFilePath = "Files/ServePush.txt";
    QString sContent("Served push content.");

    INIT();

    QVERIFY(createFile(sFilePath, sContent));
    STAGE(sFilePath);
    COMMIT("P1");

    // Build a push request from this repository
    Aze::CRepository oSourceRepository(m_sRootPath);
    QVERIFY(oSourceRepository.ok());

    Aze::CRemoteRepository oSourceRemote(&oSourceRepository);
    CXMLNode xRequest = oSourceRemote.getPushRequest();
    QVERIFY(xRequest.attributes()[Aze::CStrings::s_sParamRequest] == Aze::CStrings::s_sParamPush);

    // Simulate the HTTP wire with a JSON round-trip of the POST body
    CXMLNode xReceived = CXMLNode::parseJSON(xRequest.toJsonString());
    QVERIFY(xReceived.attributes()[Aze::CStrings::s_sParamTip] == oSourceRepository.tipCommit()->id());

    // Prepare an empty target repository outside of the source folder
    QDir oParent(m_sRootPath);
    QVERIFY(oParent.cdUp());
    QString sRemotePath = oParent.absoluteFilePath("aze-tests-remote");
    QDir oRemoteFolder(sRemotePath);

    if (oRemoteFolder.exists())
        QVERIFY(oRemoteFolder.removeRecursively());

    QVERIFY(QDir().mkpath(sRemotePath));

    Aze::CRepository oTargetRepository(sRemotePath);
    QVERIFY(oTargetRepository.init() == Aze::CError::eNone);

    // Apply the push request through the server layer
    CWebContext tContext;
    CXMLNode xResponse = CServePushProbe::servePush(&oTargetRepository, tContext, xReceived);

    // Simulate the HTTP wire back with a JSON round-trip of the response body
    CXMLNode xDelivered = CXMLNode::parseJSON(xResponse.toJsonString());
    QVERIFY(Aze::CError::fromResultNode(xDelivered) == Aze::CError::eNone);

    // Check the pushed branch tip from a fresh repository instance
    Aze::CRepository oCheckRepository(sRemotePath);
    QVERIFY(oCheckRepository.ok());
    QCOMPARE(oCheckRepository.tipCommit()->id(), oSourceRepository.tipCommit()->id());

    // Check file content round-trips through transferred objects
    Aze::CCommit* pTipCommit = oCheckRepository.database()->getCommit(oCheckRepository.tipCommit()->id(), this);
    QCOMPARE(QString(pTipCommit->fileContent(oCheckRepository.database(), sFilePath)), sContent);
    delete pTipCommit;

    // Pushing the same tip again through the server succeeds
    {
        Aze::CRepository oFreshTarget(sRemotePath);
        QVERIFY(oFreshTarget.ok());
        CXMLNode xAgain = CServePushProbe::servePush(&oFreshTarget, tContext, xReceived);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xAgain.toJsonString())) == Aze::CError::eNone);
    }

    // Pushing a rewritten tip that does not contain the target tip is refused
    Aze::CRepository oAmendedRepository(m_sRootPath);
    QVERIFY(oAmendedRepository.amend("CTestAze", "Rewritten") == Aze::CError::eNone);
    QVERIFY(oAmendedRepository.writeCurrentBranch() == Aze::CError::eNone);

    // Use a fresh instance so the rewritten tip is read back from the branch file
    Aze::CRepository oRewrittenRepository(m_sRootPath);
    Aze::CRemoteRepository oAmendedRemote(&oRewrittenRepository);
    CXMLNode xRewritten = oAmendedRemote.getPushRequest();
    QVERIFY(xRewritten.attributes()[Aze::CStrings::s_sParamTip] != oSourceRepository.tipCommit()->id());
    CXMLNode xRewrittenReceived = CXMLNode::parseJSON(xRewritten.toJsonString());

    {
        Aze::CRepository oFreshTarget(sRemotePath);
        QVERIFY(oFreshTarget.ok());
        CXMLNode xRejected = CServePushProbe::servePush(&oFreshTarget, tContext, xRewrittenReceived);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xRejected.toJsonString())) == Aze::CError::ePushNotFastForward);
    }

    // Push with an unknown tip is refused
    CXMLNode xBadRequest = CXMLNode::parseJSON(xRequest.toJsonString());
    xBadRequest.attributes()[Aze::CStrings::s_sParamTip] = "00";

    {
        Aze::CRepository oFreshTarget(sRemotePath);
        QVERIFY(oFreshTarget.ok());
        CXMLNode xBadResponse = CServePushProbe::servePush(&oFreshTarget, tContext, xBadRequest);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xBadResponse.toJsonString())) == Aze::CError::ePushRequestInvalid);
    }

    QVERIFY(QDir(sRemotePath).removeRecursively());
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests push announce and delta data through the server layer.
*/
/***************************************************************************/
void CTestServe::testServePushAnnounce()
{
    QString sFilePath = "Files/ServePushAnnounce.txt";
    QString sDetail;

    INIT();

    QVERIFY(createFile(sFilePath, "Announced v1."));
    STAGE(sFilePath);
    COMMIT("A1");

    // Prepare an empty target repository outside of the source folder
    QDir oParent(m_sRootPath);
    QVERIFY(oParent.cdUp());
    QString sRemotePath = oParent.absoluteFilePath("aze-tests-remote");
    QDir oRemoteFolder(sRemotePath);

    if (oRemoteFolder.exists())
        QVERIFY(oRemoteFolder.removeRecursively());

    QVERIFY(QDir().mkpath(sRemotePath));

    {
        Aze::CRepository oTargetRepository(sRemotePath);
        QVERIFY(oTargetRepository.init() == Aze::CError::eNone);
    }

    CWebContext tContext;

    // Unknown branch announces an empty tip through the server layer
    {
        Aze::CRepository oSourceRepository(m_sRootPath);
        QVERIFY(oSourceRepository.ok());

        Aze::CRemoteRepository oSourceRemote(&oSourceRepository);
        CXMLNode xAnnounce = CXMLNode::parseJSON(oSourceRemote.getPushAnnounce().toJsonString());

        Aze::CRepository oTargetRepository(sRemotePath);
        QVERIFY(oTargetRepository.ok());
        CXMLNode xResponse = CServePushProbe::servePushAnnounce(&oTargetRepository, tContext, xAnnounce);
        CXMLNode xDelivered = CXMLNode::parseJSON(xResponse.toJsonString());
        QVERIFY(Aze::CError::fromResultNode(xDelivered) == Aze::CError::eInvalidResponse);
        QVERIFY(xDelivered.attributes()[Aze::CStrings::s_sParamTip].isEmpty());
    }

    // Invalid announce answers an error node
    {
        Aze::CRepository oTargetRepository(sRemotePath);
        QVERIFY(oTargetRepository.ok());
        CXMLNode xBad("");
        xBad.attributes()[Aze::CStrings::s_sParamRequest] = Aze::CStrings::s_sParamPushAnnounce;
        CXMLNode xError = CServePushProbe::servePushAnnounce(&oTargetRepository, tContext, xBad);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xError.toJsonString())) == Aze::CError::ePushRequestInvalid);
    }

    // First data carries the full history through the server layer
    QString sTip1;

    {
        Aze::CRepository oSourceRepository(m_sRootPath);
        QVERIFY(oSourceRepository.ok());
        sTip1 = oSourceRepository.tipCommit()->id();

        Aze::CRemoteRepository oSourceRemote(&oSourceRepository);
        CXMLNode xData;
        QVERIFY(oSourceRemote.buildPushDataRequest("", xData, sDetail) == Aze::CError::eNone);

        Aze::CRepository oTargetRepository(sRemotePath);
        QVERIFY(oTargetRepository.ok());
        CXMLNode xApplied = CServePushProbe::servePush(&oTargetRepository, tContext, CXMLNode::parseJSON(xData.toJsonString()));
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xApplied.toJsonString())) == Aze::CError::eNone);
    }

    // Second commit: delta data carries the new commit only
    QVERIFY(createFile(sFilePath, "Announced v2."));
    STAGE(sFilePath);
    COMMIT("A2");

    {
        Aze::CRepository oSourceRepository(m_sRootPath);
        QVERIFY(oSourceRepository.ok());

        Aze::CRemoteRepository oSourceRemote(&oSourceRepository);
        CXMLNode xData;
        QVERIFY(oSourceRemote.buildPushDataRequest(sTip1, xData, sDetail) == Aze::CError::eNone);
        QCOMPARE(xData.getNodesByTagName(Aze::CStrings::s_sParamCommit).count(), 1);

        Aze::CRepository oTargetRepository(sRemotePath);
        QVERIFY(oTargetRepository.ok());
        CXMLNode xApplied = CServePushProbe::servePush(&oTargetRepository, tContext, CXMLNode::parseJSON(xData.toJsonString()));
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xApplied.toJsonString())) == Aze::CError::eNone);
    }

    // Remote tip converges, announce answers the pushed tip
    {
        Aze::CRepository oSourceRepository(m_sRootPath);
        QVERIFY(oSourceRepository.ok());

        Aze::CRepository oCheckRepository(sRemotePath);
        QVERIFY(oCheckRepository.ok());
        QCOMPARE(oCheckRepository.tipCommit()->id(), oSourceRepository.tipCommit()->id());

        Aze::CRemoteRepository oSourceRemote(&oSourceRepository);
        CXMLNode xAnnounce = CXMLNode::parseJSON(oSourceRemote.getPushAnnounce().toJsonString());

        Aze::CRepository oTargetRepository(sRemotePath);
        QVERIFY(oTargetRepository.ok());
        CXMLNode xResponse = CServePushProbe::servePushAnnounce(&oTargetRepository, tContext, xAnnounce);
        CXMLNode xDelivered = CXMLNode::parseJSON(xResponse.toJsonString());
        QCOMPARE(xDelivered.attributes()[Aze::CStrings::s_sParamTip], oSourceRepository.tipCommit()->id());
    }

    QVERIFY(QDir(sRemotePath).removeRecursively());
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests delta pull carrying missing commits only through the server layer.
*/
/***************************************************************************/
void CTestServe::testServePullDelta()
{
    QString sFilePath = "Files/ServePullDelta.txt";
    QString sDetail;

    INIT();

    QVERIFY(createFile(sFilePath, "Served v1."));
    STAGE(sFilePath);
    COMMIT("P1");

    QString sTip1;

    {
        Aze::CRepository oSourceRepository(m_sRootPath);
        QVERIFY(oSourceRepository.ok());
        sTip1 = oSourceRepository.tipCommit()->id();
    }

    // Prepare a remote repository with the source tip pushed through the server layer
    QDir oParent(m_sRootPath);
    QVERIFY(oParent.cdUp());
    QString sRemotePath = oParent.absoluteFilePath("aze-tests-remote");
    QDir oRemoteFolder(sRemotePath);

    if (oRemoteFolder.exists())
        QVERIFY(oRemoteFolder.removeRecursively());

    QVERIFY(QDir().mkpath(sRemotePath));

    {
        Aze::CRepository oTargetRepository(sRemotePath);
        QVERIFY(oTargetRepository.init() == Aze::CError::eNone);
    }

    CWebContext tContext;

    {
        Aze::CRepository oSourceRepository(m_sRootPath);
        QVERIFY(oSourceRepository.ok());

        Aze::CRemoteRepository oSourceRemote(&oSourceRepository);
        CXMLNode xPush = CXMLNode::parseJSON(oSourceRemote.getPushRequest().toJsonString());

        Aze::CRepository oTargetRepository(sRemotePath);
        QVERIFY(oTargetRepository.ok());
        CXMLNode xApplied = CServePushProbe::servePush(&oTargetRepository, tContext, xPush);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xApplied.toJsonString())) == Aze::CError::eNone);
    }

    // Prepare an empty pull repository outside of the source folder
    QString sPullPath = oParent.absoluteFilePath("aze-tests-pull");
    QDir oPullFolder(sPullPath);

    if (oPullFolder.exists())
        QVERIFY(oPullFolder.removeRecursively());

    QVERIFY(QDir().mkpath(sPullPath));

    {
        Aze::CRepository oPullRepository(sPullPath);
        QVERIFY(oPullRepository.init() == Aze::CError::eNone);
    }

    // Empty client tip pulls the full history through the server layer
    {
        Aze::CRepository oPullRepository(sPullPath);
        QVERIFY(oPullRepository.ok());

        Aze::CRemoteRepository oPullRemote(&oPullRepository);
        CXMLNode xRequest = CXMLNode::parseJSON(oPullRemote.getPullRequest().toJsonString());

        Aze::CRepository oRemoteRepository(sRemotePath);
        QVERIFY(oRemoteRepository.ok());
        CXMLNode xResponse = CServePushProbe::servePull(&oRemoteRepository, tContext, xRequest);
        CXMLNode xDelivered = CXMLNode::parseJSON(xResponse.toJsonString());
        QVERIFY(Aze::CError::fromResultNode(xDelivered) == Aze::CError::eInvalidResponse);
        QCOMPARE(xDelivered.attributes()[Aze::CStrings::s_sParamTip], sTip1);
        QCOMPARE(xDelivered.getNodesByTagName(Aze::CStrings::s_sParamCommit).count(), 1);

        QVERIFY(oPullRemote.applyPullResponse(xDelivered, sDetail) == Aze::CError::eNone);
    }

    // Second commit: behind pull request answers the new commit only
    QVERIFY(createFile(sFilePath, "Served v2."));
    STAGE(sFilePath);
    COMMIT("P2");

    QString sTip2;

    {
        Aze::CRepository oSourceRepository(m_sRootPath);
        QVERIFY(oSourceRepository.ok());
        sTip2 = oSourceRepository.tipCommit()->id();

        Aze::CRemoteRepository oSourceRemote(&oSourceRepository);
        CXMLNode xPush = CXMLNode::parseJSON(oSourceRemote.getPushRequest().toJsonString());

        Aze::CRepository oTargetRepository(sRemotePath);
        QVERIFY(oTargetRepository.ok());
        CXMLNode xApplied = CServePushProbe::servePush(&oTargetRepository, tContext, xPush);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xApplied.toJsonString())) == Aze::CError::eNone);
    }

    {
        Aze::CRepository oPullRepository(sPullPath);
        QVERIFY(oPullRepository.ok());

        Aze::CRemoteRepository oPullRemote(&oPullRepository);
        CXMLNode xRequest = CXMLNode::parseJSON(oPullRemote.getPullRequest().toJsonString());
        QCOMPARE(xRequest.attributes()[Aze::CStrings::s_sParamTip], sTip1);

        Aze::CRepository oRemoteRepository(sRemotePath);
        QVERIFY(oRemoteRepository.ok());
        CXMLNode xResponse = CServePushProbe::servePull(&oRemoteRepository, tContext, xRequest);
        CXMLNode xDelivered = CXMLNode::parseJSON(xResponse.toJsonString());
        QVERIFY(Aze::CError::fromResultNode(xDelivered) == Aze::CError::eInvalidResponse);
        QCOMPARE(xDelivered.attributes()[Aze::CStrings::s_sParamTip], sTip2);
        QCOMPARE(xDelivered.getNodesByTagName(Aze::CStrings::s_sParamCommit).count(), 1);
        QCOMPARE(xDelivered.getNodesByTagName(Aze::CStrings::s_sParamObject).count(), 1);

        QVERIFY(oPullRemote.applyPullResponse(xDelivered, sDetail) == Aze::CError::eNone);
    }

    {
        Aze::CRepository oCheckRepository(sPullPath);
        QVERIFY(oCheckRepository.ok());
        QCOMPARE(oCheckRepository.tipCommit()->id(), sTip2);
        QCOMPARE(Aze::CUtils::getTextFileContent(QString("%1/%2").arg(sPullPath).arg(sFilePath)), QString("Served v2."));
    }

    // Converged branches: response carries attributes only and applies as a no-op
    {
        Aze::CRepository oPullRepository(sPullPath);
        QVERIFY(oPullRepository.ok());

        Aze::CRemoteRepository oPullRemote(&oPullRepository);
        CXMLNode xRequest = CXMLNode::parseJSON(oPullRemote.getPullRequest().toJsonString());

        Aze::CRepository oRemoteRepository(sRemotePath);
        QVERIFY(oRemoteRepository.ok());
        CXMLNode xResponse = CServePushProbe::servePull(&oRemoteRepository, tContext, xRequest);
        CXMLNode xDelivered = CXMLNode::parseJSON(xResponse.toJsonString());
        QVERIFY(Aze::CError::fromResultNode(xDelivered) == Aze::CError::eInvalidResponse);
        QVERIFY(xDelivered.getNodesByTagName(Aze::CStrings::s_sParamCommit).isEmpty());

        QVERIFY(oPullRemote.applyPullResponse(xDelivered, sDetail) == Aze::CError::eNone);
    }

    // Unknown branch answers an error node through the server layer
    {
        Aze::CRepository oPullRepository(sPullPath);
        QVERIFY(oPullRepository.ok());

        Aze::CRemoteRepository oPullRemote(&oPullRepository);
        CXMLNode xRequest = CXMLNode::parseJSON(oPullRemote.getPullRequest().toJsonString());
        xRequest.attributes()[Aze::CStrings::s_sParamBranch] = "no-such-branch";

        Aze::CRepository oRemoteRepository(sRemotePath);
        QVERIFY(oRemoteRepository.ok());
        CXMLNode xError = CServePushProbe::servePull(&oRemoteRepository, tContext, xRequest);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xError.toJsonString())) == Aze::CError::eBranchDoesNotExist);
    }

    QVERIFY(QDir(sRemotePath).removeRecursively());
    QVERIFY(QDir(sPullPath).removeRecursively());
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests delete-branch request applying through the server layer.
*/
/***************************************************************************/
void CTestServe::testServeDeleteBranch()
{
    QString sFilePath = "Files/ServeDeleteBranch.txt";
    QString sContent("Served delete branch content.");

    INIT();

    QVERIFY(createFile(sFilePath, sContent));
    STAGE(sFilePath);
    COMMIT("P1");

    CREATE_BRANCH("doomed");

    Aze::CRepository oServerRepository(m_sRootPath);
    QVERIFY(oServerRepository.ok());

    Aze::CRemoteRepository oServerRemote(&oServerRepository);
    CWebContext tContext;

    // The delete-branch request carries the branch name and the force flag
    CXMLNode xRequest = oServerRemote.getDeleteBranchRequest("doomed", false);
    QVERIFY(xRequest.attributes()[Aze::CStrings::s_sParamRequest] == Aze::CStrings::s_sParamDeleteBranch);
    QCOMPARE(xRequest.attributes()[Aze::CStrings::s_sParamBranch], QString("doomed"));

    // Simulate the HTTP wire with a JSON round-trip of the POST body
    CXMLNode xReceived = CXMLNode::parseJSON(xRequest.toJsonString());
    QVERIFY(xReceived.attributes()[Aze::CStrings::s_sParamBranch] == QString("doomed"));

    // Apply the delete-branch request through the server layer
    CXMLNode xResponse = CServePushProbe::serveDeleteBranch(&oServerRepository, tContext, xReceived);

    // Simulate the HTTP wire back with a JSON round-trip of the response body
    QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xResponse.toJsonString())) == Aze::CError::eNone);

    // Check the branch file is gone from a fresh repository instance
    {
        Aze::CRepository oCheckRepository(m_sRootPath);
        QVERIFY(oCheckRepository.ok());
        QVERIFY(not QFile(oCheckRepository.database()->composeBranchFileName("doomed")).exists());
    }

    // Deleting an unknown branch through the server is refused
    {
        Aze::CRepository oFreshServer(m_sRootPath);
        QVERIFY(oFreshServer.ok());
        Aze::CRemoteRepository oFreshRemote(&oFreshServer);
        CXMLNode xUnknown = CXMLNode::parseJSON(oFreshRemote.getDeleteBranchRequest("no-such-branch", false).toJsonString());
        CXMLNode xUnknownResponse = CServePushProbe::serveDeleteBranch(&oFreshServer, tContext, xUnknown);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xUnknownResponse.toJsonString())) == Aze::CError::eBranchDoesNotExist);
    }

    // Deleting without a branch name through the server is refused
    {
        Aze::CRepository oFreshServer(m_sRootPath);
        QVERIFY(oFreshServer.ok());
        CXMLNode xBad("");
        xBad.attributes()[Aze::CStrings::s_sParamRequest] = Aze::CStrings::s_sParamDeleteBranch;
        CXMLNode xBadReceived = CXMLNode::parseJSON(xBad.toJsonString());
        CXMLNode xBadResponse = CServePushProbe::serveDeleteBranch(&oFreshServer, tContext, xBadReceived);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xBadResponse.toJsonString())) == Aze::CError::eDeleteBranchRequestInvalid);
    }

    // Deleting the current branch through the server is refused
    {
        Aze::CRepository oFreshServer(m_sRootPath);
        QVERIFY(oFreshServer.ok());
        Aze::CRemoteRepository oFreshRemote(&oFreshServer);
        CXMLNode xCurrent = CXMLNode::parseJSON(oFreshRemote.getDeleteBranchRequest(oFreshServer.currentBranchName(), true).toJsonString());
        CXMLNode xCurrentResponse = CServePushProbe::serveDeleteBranch(&oFreshServer, tContext, xCurrent);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xCurrentResponse.toJsonString())) == Aze::CError::eCannotDeleteCurrentBranch);
    }

    // Deleting an unmerged branch without force is refused, with force it succeeds
    CREATE_BRANCH("unmerged");
    SWITCH_TO_BRANCH("unmerged");
    QVERIFY(createFile(sFilePath, QString("Diverged content.")));
    STAGE(sFilePath);
    COMMIT("U1");
    SWITCH_TO_BRANCH(Aze::CStrings::s_sDefaultBranchName);

    {
        Aze::CRepository oFreshServer(m_sRootPath);
        QVERIFY(oFreshServer.ok());
        Aze::CRemoteRepository oFreshRemote(&oFreshServer);
        CXMLNode xUnmerged = CXMLNode::parseJSON(oFreshRemote.getDeleteBranchRequest("unmerged", false).toJsonString());
        CXMLNode xUnmergedResponse = CServePushProbe::serveDeleteBranch(&oFreshServer, tContext, xUnmerged);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xUnmergedResponse.toJsonString())) == Aze::CError::eCannotDeleteUnmergedBranch);
    }

    {
        Aze::CRepository oFreshServer(m_sRootPath);
        QVERIFY(oFreshServer.ok());
        Aze::CRemoteRepository oFreshRemote(&oFreshServer);
        CXMLNode xForced = CXMLNode::parseJSON(oFreshRemote.getDeleteBranchRequest("unmerged", true).toJsonString());
        QVERIFY(xForced.attributes()[Aze::CStrings::s_sParamForce] == QString("true"));
        CXMLNode xForcedResponse = CServePushProbe::serveDeleteBranch(&oFreshServer, tContext, xForced);
        QVERIFY(Aze::CError::fromResultNode(CXMLNode::parseJSON(xForcedResponse.toJsonString())) == Aze::CError::eNone);
    }

    {
        Aze::CRepository oCheckRepository(m_sRootPath);
        QVERIFY(oCheckRepository.ok());
        QVERIFY(not QFile(oCheckRepository.database()->composeBranchFileName("unmerged")).exists());
    }
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests repository path confinement under the server base folder.
*/
/***************************************************************************/
void CTestServe::testServePathIsolation()
{
    const QString sBase = QDir(m_sRootPath).absolutePath();
    QString sResolved;

    // A plain repository name stays under the base folder
    QVERIFY(CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << "repo", sResolved));
    QCOMPARE(sResolved, QDir::cleanPath(sBase + "/repo"));

    // Nested folders under the base folder are accepted
    QVERIFY(CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << "group" << "repo", sResolved));
    QCOMPARE(sResolved, QDir::cleanPath(sBase + "/group/repo"));

    // Parent folder markers are refused wherever they appear
    QVERIFY(not CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << "..", sResolved));
    QVERIFY(not CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << "repo" << ".." << "other", sResolved));

    // Current folder markers and empty segments are refused
    QVERIFY(not CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << ".", sResolved));
    QVERIFY(not CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << "", sResolved));
    QVERIFY(not CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList(), sResolved));

    // Embedded separators and drive specifications are refused
    QVERIFY(not CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << "a/b", sResolved));
    QVERIFY(not CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << "a\\b", sResolved));
    QVERIFY(not CServePushProbe::resolveRepositoryPath(m_sRootPath, QStringList() << "C:", sResolved));
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests HTTP and HTTPS connectivity against a live server processor.
*
* Starts a CAzeServerProcessor without a parent server and checks that both
* listeners answer: requests without a repository path are refused with the
* empty-request JSON result, which proves the TCP, HTTP framing, TLS and
* JSON layers carry a request and its response. Request handling itself is
* covered by the direct servePush and serveDeleteBranch tests.
*/
/***************************************************************************/
void CTestServe::testServeConnectivity()
{
    quint16 uiHttpPort = connectivityFreePort();
    quint16 uiHttpsPort = connectivityFreePort();
    QVERIFY(uiHttpPort != 0);
    QVERIFY(uiHttpsPort != 0);
    QVERIFY(uiHttpPort != uiHttpsPort);

    // Generate a short-lived development certificate in scratch space
    QDir oParent(m_sRootPath);
    QVERIFY(oParent.cdUp());
    QString sCertFolder = oParent.absoluteFilePath("aze-tests-connectivity");
    QDir oCertFolder(sCertFolder);

    if (oCertFolder.exists())
        QVERIFY(oCertFolder.removeRecursively());

    QVERIFY(QDir().mkpath(sCertFolder));

    QString sCertPath = QString("%1/localhost.crt").arg(sCertFolder);
    QString sKeyPath = QString("%1/localhost.key").arg(sCertFolder);

    if (not generateConnectivityCertificate(sCertPath, sKeyPath))
    {
        QVERIFY(QDir(sCertFolder).removeRecursively());
        QSKIP("openssl is unavailable, cannot generate the HTTPS test certificate");
    }

    // Network stacks need an event dispatcher: the test process runs
    // without an application object, so a local one is created here.
    // Stack allocation restores the previous state on any return path.
    int iArgc = 1;
    QByteArray baBinaryPath = m_sArgument0.toLocal8Bit();
    char* sBinaryPath = baBinaryPath.data();
    char* lArgv[1] = { sBinaryPath };
    QCoreApplication tApplication(iArgc, lArgv);

    // A null parent server is enough: pathless requests are refused
    // before the repositories base path is resolved
    CAzeServerProcessor tServer(uiHttpPort);
    QVERIFY(tServer.isListening());
    QVERIFY(tServer.enableHttps(sCertPath, sKeyPath, uiHttpsPort));
    QVERIFY(tServer.isHttpsEnabled());

    QNetworkAccessManager tManager;
    QByteArray baBody;
    QString sError;

    // Plain HTTP GET reaches the processor
    QVERIFY2(fetchConnectivityResponse(&tManager,
        QNetworkRequest(QUrl(QString("http://127.0.0.1:%1/").arg(uiHttpPort))),
        QByteArray(), baBody, sError), qPrintable(sError));
    QVERIFY(isEmptyRequestResponse(baBody));

    // Plain HTTP POST with a JSON body reaches the processor
    QNetworkRequest tPostRequest(QUrl(QString("http://127.0.0.1:%1/").arg(uiHttpPort)));
    tPostRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QVERIFY2(fetchConnectivityResponse(&tManager, tPostRequest,
        QByteArray("{\"request\":\"connectivity\"}"), baBody, sError), qPrintable(sError));
    QVERIFY(isEmptyRequestResponse(baBody));

    // HTTPS GET reaches the processor, the self-signed certificate is accepted by the test client
    QNetworkRequest tSecureRequest(QUrl(QString("https://127.0.0.1:%1/").arg(uiHttpsPort)));
    QSslConfiguration tSslConfiguration = QSslConfiguration::defaultConfiguration();
    tSslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
    tSecureRequest.setSslConfiguration(tSslConfiguration);
    QVERIFY2(fetchConnectivityResponse(&tManager, tSecureRequest,
        QByteArray(), baBody, sError), qPrintable(sError));
    QVERIFY(isEmptyRequestResponse(baBody));

    tServer.close();

    QVERIFY(QDir(sCertFolder).removeRecursively());
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests loopback binding and invalid address refusal.
*
* A loopback-bound processor listens on 127.0.0.1 and answers loopback TCP
* connections, an invalid bind address refuses to listen.
*/
void CTestServe::testServeBindAddress()
{
    quint16 uiPort = connectivityFreePort();
    QVERIFY(uiPort != 0);

    CAzeServerProcessor tLoopback(uiPort, QHostAddress::LocalHost);
    QVERIFY(tLoopback.isListening());
    QCOMPARE(tLoopback.listenAddress(), QHostAddress(QHostAddress::LocalHost));

    QTcpSocket tSocket;
    tSocket.connectToHost(QHostAddress::LocalHost, uiPort);
    QVERIFY(tSocket.waitForConnected(5000));
    tSocket.disconnectFromHost();

    tLoopback.close();

    quint16 uiBadPort = connectivityFreePort();
    QVERIFY(uiBadPort != 0);
    QVERIFY(uiBadPort != uiPort);

    CAzeServerProcessor tInvalid(uiBadPort, QHostAddress(QStringLiteral("192.0.2.1")));
    QVERIFY(not tInvalid.isListening());
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests POST body and push payload limit enforcement.
*
* The static checker refuses commit counts and object sizes past the limits,
* a live processor refuses oversized bodies before JSON parsing and refuses
* push payloads past the commit count before opening any repository.
*/
void CTestServe::testServePayloadLimits()
{
    CXMLNode xPush("");
    xPush.attributes()[Aze::CStrings::s_sParamRequest] = Aze::CStrings::s_sParamPush;

    for (int iIndex = 0; iIndex < 3; iIndex++)
    {
        CXMLNode xCommit(Aze::CStrings::s_sParamCommit);
        xCommit.attributes()[Aze::CStrings::s_sParamId] = QString::number(iIndex);
        xPush << xCommit;
    }

    QString sDetail;
    QCOMPARE(CServePushProbe::checkPushPayloadLimits(xPush, 2, 1024 * 1024, sDetail), Aze::CError::eRequestTooLarge);
    QVERIFY(sDetail.contains(QStringLiteral("commit")));

    sDetail.clear();
    QCOMPARE(CServePushProbe::checkPushPayloadLimits(xPush, 3, 1024 * 1024, sDetail), Aze::CError::eNone);

    CXMLNode xPushObject("");
    xPushObject.attributes()[Aze::CStrings::s_sParamRequest] = Aze::CStrings::s_sParamPush;

    CXMLNode xObject(Aze::CStrings::s_sParamObject);
    xObject.attributes()[Aze::CStrings::s_sParamId] = QStringLiteral("object");
    xObject.setValue(Aze::CStrings::s_sBase64Marker + QString(100, QLatin1Char('x')));
    xPushObject << xObject;

    sDetail.clear();
    QCOMPARE(CServePushProbe::checkPushPayloadLimits(xPushObject, 100000, 10, sDetail), Aze::CError::eRequestTooLarge);
    QVERIFY(sDetail.contains(QStringLiteral("object")));

    sDetail.clear();
    QCOMPARE(CServePushProbe::checkPushPayloadLimits(xPushObject, 100000, 1024 * 1024, sDetail), Aze::CError::eNone);

    // Network stacks need an event dispatcher, see testServeConnectivity
    int iArgc = 1;
    QByteArray baBinaryPath = m_sArgument0.toLocal8Bit();
    char* sBinaryPath = baBinaryPath.data();
    char* lArgv[1] = { sBinaryPath };
    QCoreApplication tApplication(iArgc, lArgv);

    quint16 uiPort = connectivityFreePort();
    QVERIFY(uiPort != 0);

    CAzeServerProcessor tServer(uiPort, QHostAddress::LocalHost);
    tServer.setRepositoriesBasePath(m_sRootPath);
    tServer.setMaxPostBodyBytes(512);
    tServer.setMaxCommitsPerPush(1);
    QVERIFY(tServer.isListening());

    QNetworkAccessManager tManager;
    QByteArray baBody;
    QString sError;

    // An oversized body is refused before JSON parsing, whatever the path holds
    QNetworkRequest tBodyRequest(QUrl(QString("http://127.0.0.1:%1/repo").arg(uiPort)));
    tBodyRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QVERIFY2(fetchConnectivityResponse(&tManager, tBodyRequest,
        QByteArray(2048, 'a'), baBody, sError), qPrintable(sError));
    QCOMPARE(Aze::CError::fromResultNode(CXMLNode::parseJSON(QString::fromUtf8(baBody))), Aze::CError::eRequestTooLarge);

    // A push payload past the commit count is refused before any repository opens
    tServer.setMaxPostBodyBytes(32 * 1024 * 1024);

    QNetworkRequest tPushRequest(QUrl(QString("http://127.0.0.1:%1/repo").arg(uiPort)));
    tPushRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QVERIFY2(fetchConnectivityResponse(&tManager, tPushRequest,
        QByteArray("{\"request\":\"push\",\"branch\":\"branch\",\"tip\":\"tip\",\"commit\":[{\"id\":\"1\"},{\"id\":\"2\"}]}"), baBody, sError), qPrintable(sError));
    QCOMPARE(Aze::CError::fromResultNode(CXMLNode::parseJSON(QString::fromUtf8(baBody))), Aze::CError::eRequestTooLarge);

    tServer.close();
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests repository path resolution across symlink swaps.
*
* A symlink pointing inside the base folder resolves to the canonical target,
* swapping it to an outside folder makes the same segments resolve to a
* refusal.
*/
void CTestServe::testServeSymlinkSwap()
{
    QDir oRoot(m_sRootPath);

    const QString sBase = oRoot.absoluteFilePath("symlink-base");
    const QString sOutside = oRoot.absoluteFilePath("symlink-outside");

    QDir(sBase).removeRecursively();
    QDir(sOutside).removeRecursively();

    QVERIFY(QDir().mkpath(sBase + "/real"));
    QVERIFY(QDir().mkpath(sOutside));

    QVERIFY(QFile::link(sBase + "/real", sBase + "/link"));

    QString sResolved;
    QVERIFY(CServePushProbe::resolveRepositoryPath(sBase, QStringList() << "link", sResolved));
    QCOMPARE(sResolved, QFileInfo(sBase + "/real").canonicalFilePath());

    QVERIFY(QFile::remove(sBase + "/link"));
    QVERIFY(QFile::link(sOutside, sBase + "/link"));

    QVERIFY(not CServePushProbe::resolveRepositoryPath(sBase, QStringList() << "link", sResolved));

    QVERIFY(QDir(sBase).removeRecursively());
    QVERIFY(QDir(sOutside).removeRecursively());
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests portal client certificate enforcement on the HTTPS endpoint.
*
* A processor trusting a portal CA with peer verification answers a client
* presenting a chained certificate in both directions, refuses a client
* without a certificate and a client with a certificate outside the chain.
* A processor without peer verification answers the same uncertified client,
* which proves the verification mode is the gate.
*/
void CTestServe::testServeMutualTls()
{
    quint16 uiHttpPort = connectivityFreePort();
    quint16 uiHttpsPort = connectivityFreePort();
    QVERIFY(uiHttpPort != 0);
    QVERIFY(uiHttpsPort != 0);
    QVERIFY(uiHttpPort != uiHttpsPort);

    QDir oParent(m_sRootPath);
    QVERIFY(oParent.cdUp());
    QString sCertFolder = oParent.absoluteFilePath("aze-tests-mutual-tls");

    // Network stacks need an event dispatcher, see testServeConnectivity
    int iArgc = 1;
    QByteArray baBinaryPath = m_sArgument0.toLocal8Bit();
    char* sBinaryPath = baBinaryPath.data();
    char* lArgv[1] = { sBinaryPath };
    QCoreApplication tApplication(iArgc, lArgv);

    QString sCaPath;
    QString sServerCertPath;
    QString sServerKeyPath;
    QString sClientCertPath;
    QString sClientKeyPath;

    if (not generateMutualTlsCertificates(sCertFolder, sCaPath, sServerCertPath, sServerKeyPath, sClientCertPath, sClientKeyPath))
    {
        QVERIFY(QDir(sCertFolder).removeRecursively());
        QSKIP("openssl is unavailable, cannot generate the mutual TLS test certificates");
    }

    QList<QSslCertificate> lCaCertificates = QSslCertificate::fromPath(sCaPath);
    QVERIFY(not lCaCertificates.isEmpty());

    QList<QSslCertificate> lClientCertificates = QSslCertificate::fromPath(sClientCertPath);
    QVERIFY(not lClientCertificates.isEmpty());

    QFile fClientKey(sClientKeyPath);
    QVERIFY(fClientKey.open(QIODevice::ReadOnly));
    QSslKey tClientKey(fClientKey.readAll(), QSsl::Rsa, QSsl::Pem);
    fClientKey.close();
    QVERIFY(not tClientKey.isNull());

    // A rogue client certificate stays self-signed, outside the portal chain
    QString sRogueCertPath = QString("%1/rogue.crt").arg(sCertFolder);
    QString sRogueKeyPath = QString("%1/rogue.key").arg(sCertFolder);
    QVERIFY(generateConnectivityCertificate(sRogueCertPath, sRogueKeyPath));

    QList<QSslCertificate> lRogueCertificates = QSslCertificate::fromPath(sRogueCertPath);
    QVERIFY(not lRogueCertificates.isEmpty());

    QFile fRogueKey(sRogueKeyPath);
    QVERIFY(fRogueKey.open(QIODevice::ReadOnly));
    QSslKey tRogueKey(fRogueKey.readAll(), QSsl::Rsa, QSsl::Pem);
    fRogueKey.close();
    QVERIFY(not tRogueKey.isNull());

    CAzeServerProcessor tServer(uiHttpPort, QHostAddress::LocalHost);
    tServer.setSslCaCertificates(lCaCertificates);
    tServer.setSslPeerVerifyMode(QSslSocket::VerifyPeer);
    QCOMPARE(tServer.sslPeerVerifyMode(), QSslSocket::VerifyPeer);
    QCOMPARE(tServer.sslCaCertificates().size(), lCaCertificates.size());
    QVERIFY(tServer.enableHttps(sServerCertPath, sServerKeyPath, uiHttpsPort));
    QVERIFY(tServer.isHttpsEnabled());

    QSslConfiguration tCertifiedClient = QSslConfiguration::defaultConfiguration();
    tCertifiedClient.setCaCertificates(lCaCertificates);
    tCertifiedClient.setPeerVerifyMode(QSslSocket::VerifyPeer);
    tCertifiedClient.setLocalCertificate(lClientCertificates.first());
    tCertifiedClient.setPrivateKey(tClientKey);

    QSslConfiguration tUncertifiedClient = QSslConfiguration::defaultConfiguration();
    tUncertifiedClient.setCaCertificates(lCaCertificates);
    tUncertifiedClient.setPeerVerifyMode(QSslSocket::VerifyPeer);

    QSslConfiguration tRogueClient = QSslConfiguration::defaultConfiguration();
    tRogueClient.setCaCertificates(lCaCertificates);
    tRogueClient.setPeerVerifyMode(QSslSocket::VerifyPeer);
    tRogueClient.setLocalCertificate(lRogueCertificates.first());
    tRogueClient.setPrivateKey(tRogueKey);

    QNetworkAccessManager tCertifiedManager;
    QNetworkAccessManager tUncertifiedManager;
    QNetworkAccessManager tRogueManager;
    QByteArray baBody;
    QString sError;

    // A portal client with a chained certificate is served over verified TLS in both directions
    // Each identity uses its own manager so no keep-alive connection is shared between handshakes
    QNetworkRequest tCertifiedRequest(QUrl(QString("https://127.0.0.1:%1/").arg(uiHttpsPort)));
    tCertifiedRequest.setSslConfiguration(tCertifiedClient);
    QVERIFY2(fetchConnectivityResponse(&tCertifiedManager, tCertifiedRequest,
        QByteArray(), baBody, sError), qPrintable(sError));
    QVERIFY(isEmptyRequestResponse(baBody));

    // A client without a certificate fails the handshake
    QNetworkRequest tUncertifiedRequest(QUrl(QString("https://127.0.0.1:%1/").arg(uiHttpsPort)));
    tUncertifiedRequest.setSslConfiguration(tUncertifiedClient);
    QVERIFY(not fetchConnectivityResponse(&tUncertifiedManager, tUncertifiedRequest,
        QByteArray(), baBody, sError));

    // A client with a certificate outside the portal chain fails the handshake
    QNetworkRequest tRogueRequest(QUrl(QString("https://127.0.0.1:%1/").arg(uiHttpsPort)));
    tRogueRequest.setSslConfiguration(tRogueClient);
    QVERIFY(not fetchConnectivityResponse(&tRogueManager, tRogueRequest,
        QByteArray(), baBody, sError));

    tServer.close();

    // Without peer verification the same uncertified client is served, the mode is the gate
    quint16 uiPlainHttpPort = connectivityFreePort();
    quint16 uiPlainHttpsPort = connectivityFreePort();
    QVERIFY(uiPlainHttpPort != 0);
    QVERIFY(uiPlainHttpsPort != 0);
    QVERIFY(uiPlainHttpPort != uiPlainHttpsPort);

    CAzeServerProcessor tOpenServer(uiPlainHttpPort, QHostAddress::LocalHost);
    QVERIFY(tOpenServer.enableHttps(sServerCertPath, sServerKeyPath, uiPlainHttpsPort));

    QNetworkRequest tOpenRequest(QUrl(QString("https://127.0.0.1:%1/").arg(uiPlainHttpsPort)));
    tOpenRequest.setSslConfiguration(tUncertifiedClient);
    QVERIFY2(fetchConnectivityResponse(&tUncertifiedManager, tOpenRequest,
        QByteArray(), baBody, sError), qPrintable(sError));
    QVERIFY(isEmptyRequestResponse(baBody));

    tOpenServer.close();

    QVERIFY(QDir(sCertFolder).removeRecursively());
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests --ssl-verify-peer and --ssl-ca option validation with fail-closed errors.
*
* Peer verification without an HTTPS endpoint, a CA file without peer
* verification, and a CA file holding no valid certificate all refuse to
* initialize. A complete portal configuration initializes and serves a
* chained portal client while refusing an uncertified one, which proves the
* command line wiring reaches the TLS layer.
*/
void CTestServe::testServeVerifyPeerOptions()
{
    QDir oParent(m_sRootPath);
    QVERIFY(oParent.cdUp());
    QString sCertFolder = oParent.absoluteFilePath("aze-tests-verify-peer");

    // Certificate generation needs an event dispatcher, the servers below
    // bring their own application object and must be built after it is gone
    {
        int iArgc = 1;
        QByteArray baBinaryPath = m_sArgument0.toLocal8Bit();
        char* sBinaryPath = baBinaryPath.data();
        char* lArgv[1] = { sBinaryPath };
        QCoreApplication tApplication(iArgc, lArgv);

        QString sCaPath;
        QString sServerCertPath;
        QString sServerKeyPath;
        QString sClientCertPath;
        QString sClientKeyPath;

        if (not generateMutualTlsCertificates(sCertFolder, sCaPath, sServerCertPath, sServerKeyPath, sClientCertPath, sClientKeyPath))
        {
            QVERIFY(QDir(sCertFolder).removeRecursively());
            QSKIP("openssl is unavailable, cannot generate the peer verification test certificates");
        }
    }

    const QString sCaPath = QString("%1/portal-ca.crt").arg(sCertFolder);
    const QString sServerCertPath = QString("%1/server.crt").arg(sCertFolder);
    const QString sServerKeyPath = QString("%1/server.key").arg(sCertFolder);
    const QString sClientCertPath = QString("%1/portal-client.crt").arg(sCertFolder);
    const QString sClientKeyPath = QString("%1/portal-client.key").arg(sCertFolder);

    QList<QByteArray> lStorage;
    std::vector<char*> vArgv;

    // Peer verification without an HTTPS endpoint refuses to initialize
    {
        int iArgc = buildServerArguments(QStringList()
            << m_sArgument0 << QStringLiteral("serve")
            << QStringLiteral("--base-path") << m_sRootPath
            << QStringLiteral("--ssl-verify-peer"), lStorage, vArgv);
        CAzeServer tServer(iArgc, vArgv.data());
        QCOMPARE(tServer.initializationError(), Aze::CError::eInvalidServerConfiguration);
        QVERIFY(tServer.initializationErrorDetail().contains(QStringLiteral("--ssl-verify-peer")));
    }

    // A CA file without peer verification refuses to initialize
    {
        int iArgc = buildServerArguments(QStringList()
            << m_sArgument0 << QStringLiteral("serve")
            << QStringLiteral("--base-path") << m_sRootPath
            << QStringLiteral("--ssl-ca") << sCaPath, lStorage, vArgv);
        CAzeServer tServer(iArgc, vArgv.data());
        QCOMPARE(tServer.initializationError(), Aze::CError::eInvalidServerConfiguration);
        QVERIFY(tServer.initializationErrorDetail().contains(QStringLiteral("--ssl-ca")));
    }

    // A CA file holding no valid certificate refuses to initialize
    {
        const quint16 uiHttpsPort = connectivityFreePort();
        QVERIFY(uiHttpsPort != 0);

        int iArgc = buildServerArguments(QStringList()
            << m_sArgument0 << QStringLiteral("serve")
            << QStringLiteral("--base-path") << m_sRootPath
            << QStringLiteral("--https-port") << QString::number(uiHttpsPort)
            << QStringLiteral("--ssl-cert") << sServerCertPath
            << QStringLiteral("--ssl-key") << sServerKeyPath
            << QStringLiteral("--ssl-verify-peer")
            << QStringLiteral("--ssl-ca") << QString("%1/portal-ca.srl").arg(sCertFolder), lStorage, vArgv);
        CAzeServer tServer(iArgc, vArgv.data());
        QCOMPARE(tServer.initializationError(), Aze::CError::eInvalidServerConfiguration);
        QVERIFY(tServer.initializationErrorDetail().contains(QStringLiteral("--ssl-ca")));
    }

    // A complete portal configuration initializes and enforces the client certificate live
    {
        const quint16 uiHttpPort = connectivityFreePort();
        const quint16 uiHttpsPort = connectivityFreePort();
        QVERIFY(uiHttpPort != 0);
        QVERIFY(uiHttpsPort != 0);
        QVERIFY(uiHttpPort != uiHttpsPort);

        int iArgc = buildServerArguments(QStringList()
            << m_sArgument0 << QStringLiteral("serve")
            << QStringLiteral("--port") << QString::number(uiHttpPort)
            << QStringLiteral("--https-port") << QString::number(uiHttpsPort)
            << QStringLiteral("--ssl-cert") << sServerCertPath
            << QStringLiteral("--ssl-key") << sServerKeyPath
            << QStringLiteral("--ssl-verify-peer")
            << QStringLiteral("--ssl-ca") << sCaPath
            << QStringLiteral("--base-path") << m_sRootPath, lStorage, vArgv);
        CAzeServer tServer(iArgc, vArgv.data());
        QCOMPARE(tServer.initializationError(), Aze::CError::eNone);

        QList<QSslCertificate> lCaCertificates = QSslCertificate::fromPath(sCaPath);
        QVERIFY(not lCaCertificates.isEmpty());

        QList<QSslCertificate> lClientCertificates = QSslCertificate::fromPath(sClientCertPath);
        QVERIFY(not lClientCertificates.isEmpty());

        QFile fClientKey(sClientKeyPath);
        QVERIFY(fClientKey.open(QIODevice::ReadOnly));
        QSslKey tClientKey(fClientKey.readAll(), QSsl::Rsa, QSsl::Pem);
        fClientKey.close();
        QVERIFY(not tClientKey.isNull());

        QNetworkAccessManager tCertifiedManager;
        QNetworkAccessManager tUncertifiedManager;
        QByteArray baBody;
        QString sError;

        QSslConfiguration tCertifiedClient = QSslConfiguration::defaultConfiguration();
        tCertifiedClient.setCaCertificates(lCaCertificates);
        tCertifiedClient.setPeerVerifyMode(QSslSocket::VerifyPeer);
        tCertifiedClient.setLocalCertificate(lClientCertificates.first());
        tCertifiedClient.setPrivateKey(tClientKey);

        QNetworkRequest tCertifiedRequest(QUrl(QString("https://127.0.0.1:%1/").arg(uiHttpsPort)));
        tCertifiedRequest.setSslConfiguration(tCertifiedClient);
        QVERIFY2(fetchConnectivityResponse(&tCertifiedManager, tCertifiedRequest,
            QByteArray(), baBody, sError), qPrintable(sError));
        QVERIFY(isEmptyRequestResponse(baBody));

        QSslConfiguration tUncertifiedClient = QSslConfiguration::defaultConfiguration();
        tUncertifiedClient.setCaCertificates(lCaCertificates);
        tUncertifiedClient.setPeerVerifyMode(QSslSocket::VerifyPeer);

        QNetworkRequest tUncertifiedRequest(QUrl(QString("https://127.0.0.1:%1/").arg(uiHttpsPort)));
        tUncertifiedRequest.setSslConfiguration(tUncertifiedClient);
        QVERIFY(not fetchConnectivityResponse(&tUncertifiedManager, tUncertifiedRequest,
            QByteArray(), baBody, sError));
    }

    QVERIFY(QDir(sCertFolder).removeRecursively());
}

//-------------------------------------------------------------------------------------------------
