
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
#include "CAzeClient.h"

/*!
    \class CAzeClient
    \inmodule Aze
    \section1 General
*/

//-------------------------------------------------------------------------------------------------

CAzeClient::CAzeClient(int argc, char *argv[], QTextStream* pOutStream)
    : QCoreApplication(argc, argv)
    , m_tArguments(*this)
    , m_pOutStream(pOutStream)
    , m_pRepository(new Aze::CRepository(
                        QDir::currentPath(), this,
                        m_tArguments.m_tParser.isSet(m_tArguments.m_oSilent),
                        m_tArguments.m_tParser.isSet(m_tArguments.m_oDebug),
                        m_tArguments.m_tParser.isSet(m_tArguments.m_oTextCache)
                        ))
    , m_pNetworkAccessManager(new QNetworkAccessManager(this))
    , m_eLastError(Aze::CError::eNone)
    , m_bOutputStreamIsMine(false)
{
    if (IS_NULL(m_pOutStream))
    {
        m_bOutputStreamIsMine = true;
        m_pOutStream = new QTextStream(stdout);
    }

    // Setup network request
    setupRemoteHost("127.0.0.1:8080");

    // Listen to network manager responses
    connect(m_pNetworkAccessManager, &QNetworkAccessManager::finished, this, &CAzeClient::onNetworkAccessFinished);
}

//-------------------------------------------------------------------------------------------------

CAzeClient::~CAzeClient()
{
    if (m_bOutputStreamIsMine)
        delete m_pOutStream;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::run()
{
    switch (m_tArguments.m_eCommand)
    {
    case CConstants::eCommandNone:
    case CConstants::eCommandHelp:
    {
        m_tArguments.showHelp(m_pOutStream);
        return Aze::CError::eNone;
    }

    case CConstants::eCommandUnknown:
    {
        OUT_ERROR(Aze::CError::message(Aze::CError::eUnknownCommand));
        return Aze::CError::eUnknownCommand;
    }

    // Local commands
    case CConstants::eCommandAmend:               return amend();
    case CConstants::eCommandCleanUp:           return cleanUp();
    case CConstants::eCommandCommit:            return commit();
    case CConstants::eCommandCreateBranch:      return createBranch();
    case CConstants::eCommandDeleteBranch:      return deleteBranch();
    case CConstants::eCommandDiff:              return diff();
    case CConstants::eCommandDump:              return dump();
    case CConstants::eCommandInitRepository:    return init();
    case CConstants::eCommandLog:               return log();
    case CConstants::eCommandMerge:             return merge();
    case CConstants::eCommandMove:              return move();
    case CConstants::eCommandPatch:             return patch();
    case CConstants::eCommandPopStash:          return popStash();
    case CConstants::eCommandRemove:            return remove();
    case CConstants::eCommandRevert:            return revert();
    case CConstants::eCommandSaveStash:         return saveStash();
    case CConstants::eCommandSetRemoteHost:     return setRemoteHost();
    case CConstants::eCommandShowStatus:        return status();
    case CConstants::eCommandStage:             return stage();
    case CConstants::eCommandSwitchToBranch:    return switchToBranch();
    case CConstants::eCommandUnstage:           return unstage();

    // Remote commands
    case CConstants::eCommandClone:          return clone();
    case CConstants::eCommandPull:           return pull();
    case CConstants::eCommandPush:           return push();
    }

    OUT_ERROR(Aze::CError::message(Aze::CError::eUnknownCommand));

    return Aze::CError::eUnknownCommand;
}

//-------------------------------------------------------------------------------------------------

bool CAzeClient::isASainRepository()
{
    if (not m_pRepository->ok())
    {
        OUT_ERROR(Aze::CError::message(Aze::CError::eNotARepository));

        return false;
    }

    return true;
}

//-------------------------------------------------------------------------------------------------

void CAzeClient::processWildCards()
{
    m_tArguments.m_lFilesAndIds = CFileUtilities::getInstance()->concernedFiles(
                m_pRepository->database()->startPath(),
                m_tArguments.m_lFilesAndIds
                );
}

//-------------------------------------------------------------------------------------------------

void CAzeClient::setupRemoteHost(const QString& sHostName)
{
    QString sUrl = QString("http://%1").arg(sHostName);
    m_tNetworkRequest.setUrl(QUrl(sUrl));
    m_tNetworkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
}

//-------------------------------------------------------------------------------------------------
