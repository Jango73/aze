
#pragma once

// Qt
#include <QObject>
#include <QtGlobal>

// AzeLib
#include "CError.h"

//-------------------------------------------------------------------------------------------------

#define RETURN_WHEN_FALSE(a, eCode) if ((a) == false) return (eCode)

#define RETURN_WHEN_ERROR(eCall, eCode) { Aze::CError::EErrorCode eErrorCode = (eCall); if (eErrorCode != Aze::CError::eNone) return (eCode); }

//-------------------------------------------------------------------------------------------------

class CConstants : public QObject
{
    Q_OBJECT

public:

    // Commands
    enum ECommand
    {
        eCommandNone,
        eCommandAmend,
        eCommandCleanUp,
        eCommandClone,
        eCommandCommit,
        eCommandCreateBranch,
        eCommandCreateTag,
        eCommandDeleteBranch,
        eCommandDeleteTag,
        eCommandDiff,
        eCommandDump,
        eCommandHelp,
        eCommandInitRepository,
        eCommandLog,
        eCommandMerge,
        eCommandMove,
        eCommandPatch,
        eCommandPopStash,
        eCommandPrune,
        eCommandPull,
        eCommandPullTag,
        eCommandPush,
        eCommandPushTag,
        eCommandRemove,
        eCommandRevert,
        eCommandSaveStash,
        eCommandSetRemoteHost,
        eCommandShowStatus,
        eCommandStage,
        eCommandSwitchToBranch,
        eCommandListBranches,
        eCommandListTags,
        eCommandRenameBranch,
        eCommandUnstage,
        eCommandUnknown,
    };

    static const char* s_sContextMain;

    // Command switches
    static const char* s_sSwitchAmend;
    static const char* s_sSwitchCleanUp;
    static const char* s_sSwitchClone;
    static const char* s_sSwitchCommit;
    static const char* s_sSwitchCreateBranch;
    static const char* s_sSwitchCreateTag;
    static const char* s_sSwitchDeleteBranch;
    static const char* s_sSwitchDeleteTag;
    static const char* s_sSwitchDiff;
    static const char* s_sSwitchDump;
    static const char* s_sSwitchHelp;
    static const char* s_sSwitchInitRepository;
    static const char* s_sSwitchLog;
    static const char* s_sSwitchMerge;
    static const char* s_sSwitchMove;
    static const char* s_sSwitchPatch;
    static const char* s_sSwitchPopStash;
    static const char* s_sSwitchPrune;
    static const char* s_sSwitchPull;
    static const char* s_sSwitchPullTag;
    static const char* s_sSwitchPush;
    static const char* s_sSwitchPushTag;
    static const char* s_sSwitchRemove;
    static const char* s_sSwitchRevert;
    static const char* s_sSwitchSaveStash;
    static const char* s_sSwitchSetRemoteHost;
    static const char* s_sSwitchShowStatus;
    static const char* s_sSwitchStage;
    static const char* s_sSwitchSwitchToBranch;
    static const char* s_sSwitchListBranches;
    static const char* s_sSwitchListTags;
    static const char* s_sSwitchRenameBranch;
    static const char* s_sSwitchUnstage;
    static const char* s_sSwitchVoid;

    // Client switches
    static const char* s_sSwitchAdded;
    static const char* s_sSwitchAll;
    static const char* s_sSwitchAllowFileDelete;
    static const char* s_sSwitchAuthor;
    static const char* s_sSwitchClean;
    static const char* s_sSwitchCount;
    static const char* s_sSwitchDeleted;
    static const char* s_sSwitchForce;
    static const char* s_sSwitchGraph;
    static const char* s_sSwitchIgnored;
    static const char* s_sSwitchLoose;
    static const char* s_sSwitchMessage;
    static const char* s_sSwitchMissing;
    static const char* s_sSwitchModified;
    static const char* s_sSwitchNoFastForward;
    static const char* s_sSwitchDryRun;
    static const char* s_sSwitchRetention;
    static const char* s_sSwitchStart;
    static const char* s_sSwitchSilent;
    static const char* s_sSwitchDebug;
    static const char* s_sSwitchTextCache;

    // Server switches
    static const char* s_sSwitchPort;
    static const char* s_sSwitchHttpsPort;
    static const char* s_sSwitchSslCert;
    static const char* s_sSwitchSslKey;
    static const char* s_sSwitchBindAddress;
    static const char* s_sSwitchBasePath;

    // Text strings
    static const QString s_sAllFilesAreClean;
    static const QString s_sStatusOfFiles;
    static const QString s_sTextCommands;
    static const QString s_sTextYouAreAlreadyOnBranch;
    static const QString s_sTextYouAreNowOnBranch;

    static const QStringList s_lCopyrights;

    static QMap<QString, CConstants::ECommand> s_mCommands;
    static QMap<QString, QString> s_mHelp;

    //!
    static void initCommandMap();
};
