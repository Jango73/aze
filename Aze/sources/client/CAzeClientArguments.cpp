
// Qt
#include <QDebug>

// Application
#include "CAzeClientArguments.h"

/*!
    \class CAzeClientArguments
    \inmodule Aze
    \section1 General
*/

#define TRT(a) QCoreApplication::translate(CConstants::s_sContextMain, a)

//-------------------------------------------------------------------------------------------------

CAzeClientArguments::CAzeClientArguments(QCoreApplication& app)
    : QObject(nullptr)
    , m_oAll(QStringList() << CConstants::s_sSwitchAll, TRT("Include all files."))
    , m_oLoose(QStringList() << CConstants::s_sSwitchLoose, TRT("Include loose files."))
    , m_oClean(QStringList() << CConstants::s_sSwitchClean, TRT("Include clean files."))
    , m_oModified(QStringList() << CConstants::s_sSwitchModified, TRT("Include modified files."))
    , m_oAdded(QStringList() << CConstants::s_sSwitchAdded, TRT("Include added files."))
    , m_oDeleted(QStringList() << CConstants::s_sSwitchDeleted, TRT("Include deleted files."))
    , m_oForce(QStringList() << "f" << CConstants::s_sSwitchForce, TRT("Force deletion of a branch not merged into the current branch."))
    , m_oMissing(QStringList() << CConstants::s_sSwitchMissing, TRT("Include missing files."))
    , m_oIgnored(QStringList() << CConstants::s_sSwitchIgnored, TRT("Include ignored files."))
    , m_oAuthor(QStringList() << "a" << CConstants::s_sSwitchAuthor, TRT("Set commit author to <author>."), TRT(CConstants::s_sSwitchAuthor))
    , m_oMessage(QStringList() << "m" << CConstants::s_sSwitchMessage, TRT("Set commit message to <message>."), TRT(CConstants::s_sSwitchMessage))
    , m_oStart(QStringList() << "s" << CConstants::s_sSwitchStart, TRT("Begin listing after <start> items."), TRT(CConstants::s_sSwitchStart))
    , m_oCount(QStringList() << "c" << CConstants::s_sSwitchCount, TRT("List <count> items."), TRT(CConstants::s_sSwitchCount))
    , m_oAllowFileDelete(QStringList() << "d" << CConstants::s_sSwitchAllowFileDelete, TRT("Allow Aze to delete files."))
    , m_oGraph(QStringList() << "g" << CConstants::s_sSwitchGraph, TRT("Show log as a graph."))
    , m_oNoFastForward(QStringList() << CConstants::s_sSwitchNoFastForward, TRT("Merge without fast forward, creating a merge commit."))
    , m_oDryRun(QStringList() << CConstants::s_sSwitchDryRun, TRT("List unreachable files without deleting them."))
    , m_oRetention(QStringList() << CConstants::s_sSwitchRetention, TRT("Grace period in seconds protecting recent files."), TRT(CConstants::s_sSwitchRetention))
    , m_oSilent(QStringList() << CConstants::s_sSwitchSilent, TRT("Silent mode (no messages)."))
    , m_oDebug(QStringList() << CConstants::s_sSwitchDebug, TRT("Debug mode (tons of messages)."))
    , m_oTextCache(QStringList() << CConstants::s_sSwitchTextCache, TRT("Use text format for the commit cache."))
    , m_eCommand(CConstants::eCommandNone)
{
    CConstants::initCommandMap();

    // Application name and version
    QCoreApplication::setApplicationName("Aze");
    QCoreApplication::setApplicationVersion(AZE_VERSION_STRING);

    // Set option array
    m_tParser.addOptions({
                             m_oAll,
                             m_oLoose,
                             m_oClean,
                             m_oModified,
                             m_oAdded,
                              m_oDeleted,
                              m_oForce,
                             m_oMissing,
                             m_oIgnored,
                             m_oAuthor,
                             m_oMessage,
                             m_oStart,
                             m_oCount,
                             m_oAllowFileDelete,
                              m_oGraph,
                              m_oNoFastForward,
                              m_oDryRun,
                              m_oRetention,
                               m_oSilent,
                               m_oDebug,
                               m_oTextCache
                           });

    // Finalize option array
    m_tParser.addVersionOption();
    m_tParser.process(app);

    // Get files and Ids
    QStringList lPosArgs = m_tParser.positionalArguments();

    // Get command
    if (lPosArgs.count() > 0)
    {
        QString sCommand = lPosArgs.takeFirst();

        if (CConstants::s_mCommands.contains(sCommand))
            m_eCommand = CConstants::s_mCommands[sCommand];
        else
            m_eCommand = CConstants::eCommandUnknown;

        for (QString sArg : lPosArgs)
        {
            m_lFilesAndIds << sArg.split(" ");
        }
    }
}

//-------------------------------------------------------------------------------------------------

void CAzeClientArguments::showHelp(QTextStream* pOutStream)
{
    (*pOutStream) << CConstants::s_sTextCommands << ":\n\n";

    for (QString sKey : CConstants::s_mHelp.keys())
    {
        (*pOutStream) << QString(" %1  %2\n").arg(sKey, -15).arg(CConstants::s_mHelp[sKey]);
    }

    (*pOutStream) << "\n";
    // (*pOutStream) << m_tParser.helpText();
    (*pOutStream) << "\n";
    for (const QString& sText : CConstants::s_lCopyrights)
        (*pOutStream) << sText << "\n";
    (*pOutStream) << "\n";
}

//-------------------------------------------------------------------------------------------------
