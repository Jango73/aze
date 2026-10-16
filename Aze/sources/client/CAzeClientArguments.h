
#pragma once

// Qt
#include <QObject>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

// Application
#include "../common/CConstants.h"

//-------------------------------------------------------------------------------------------------

class CAzeClientArguments : public QObject
{
public:

    CAzeClientArguments(QCoreApplication& app);

    void showHelp(QTextStream* pOutStream);

    QCommandLineParser      m_tParser;
    QCommandLineOption      m_oAll;
    QCommandLineOption      m_oLoose;
    QCommandLineOption      m_oClean;
    QCommandLineOption      m_oModified;
    QCommandLineOption      m_oAdded;
    QCommandLineOption      m_oDeleted;
    QCommandLineOption      m_oForce;
    QCommandLineOption      m_oMissing;
    QCommandLineOption      m_oIgnored;
    QCommandLineOption      m_oAuthor;
    QCommandLineOption      m_oMessage;
    QCommandLineOption      m_oStart;
    QCommandLineOption      m_oCount;
    QCommandLineOption      m_oAllowFileDelete;
    QCommandLineOption      m_oGraph;
    QCommandLineOption      m_oNoFastForward;
    QCommandLineOption      m_oSilent;
    QCommandLineOption      m_oDebug;
    QCommandLineOption      m_oTextCache;
    QCommandLineOption      m_oAuthToken;
    CConstants::ECommand    m_eCommand;
    QStringList             m_lFilesAndIds;
};

//-------------------------------------------------------------------------------------------------
