
#pragma once

// Qt
#include <QObject>
#include <QtGlobal>

//-------------------------------------------------------------------------------------------------

#define ERROR_WHEN_FALSE(a,b)           if ((a) == false) return b

#define ERROR_WHEN_FALSE_PRINT(a,b,c)   if ((a) == false) { OUT_ERROR(b); return c; }

//-------------------------------------------------------------------------------------------------

class CConstants : public QObject
{
    Q_OBJECT

public:

    // Commands
    enum ECommand
    {
        eCommandNone,
        eCommandCleanUp,
        eCommandCommit,
        eCommandCreateBranch,
        eCommandDeleteBranch,
        eCommandDiff,
        eCommandDump,
        eCommandHelp,
        eCommandInitRepository,
        eCommandLog,
        eCommandMerge,
        eCommandMove,
        eCommandPatch,
        eCommandPopStash,
        eCommandPull,
        eCommandPush,
        eCommandRemove,
        eCommandRevert,
        eCommandSaveStash,
        eCommandSetRemoteHost,
        eCommandShowStatus,
        eCommandStage,
        eCommandSwitchToBranch,
        eCommandUnstage,
    };

    static const char* s_sContextMain;

    // Command switches
    static const char* s_sSwitchCleanUp;
    static const char* s_sSwitchCommit;
    static const char* s_sSwitchCreateBranch;
    static const char* s_sSwitchDeleteBranch;
    static const char* s_sSwitchDiff;
    static const char* s_sSwitchDump;
    static const char* s_sSwitchHelp;
    static const char* s_sSwitchInitRepository;
    static const char* s_sSwitchLog;
    static const char* s_sSwitchMerge;
    static const char* s_sSwitchMove;
    static const char* s_sSwitchPatch;
    static const char* s_sSwitchPopStash;
    static const char* s_sSwitchPull;
    static const char* s_sSwitchPush;
    static const char* s_sSwitchRemove;
    static const char* s_sSwitchRevert;
    static const char* s_sSwitchSaveStash;
    static const char* s_sSwitchSetRemoteHost;
    static const char* s_sSwitchShowStatus;
    static const char* s_sSwitchStage;
    static const char* s_sSwitchSwitchToBranch;
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
    static const char* s_sSwitchGraph;
    static const char* s_sSwitchIgnored;
    static const char* s_sSwitchLoose;
    static const char* s_sSwitchMessage;
    static const char* s_sSwitchMissing;
    static const char* s_sSwitchModified;
    static const char* s_sSwitchStart;
    static const char* s_sSwitchSilent;
    static const char* s_sSwitchDebug;

    // Server switches
    static const char* s_sSwitchPort;
    static const char* s_sSwitchHttpsPort;
    static const char* s_sSwitchSslCert;
    static const char* s_sSwitchSslKey;

    // Error codes
    static const int s_iError_None;
    static const int s_iError_CouldNotAddFiles;
    static const int s_iError_CouldNotCreateBranch;
    static const int s_iError_CouldNotDeleteBranch;
    static const int s_iError_CouldNotMerge;
    static const int s_iError_CouldNotMoveFiles;
    static const int s_iError_CouldNotPatch;
    static const int s_iError_CouldNotPopStash;
    static const int s_iError_CouldNotReadStage;
    static const int s_iError_CouldNotRemoveFiles;
    static const int s_iError_CouldNotRestoreStash;
    static const int s_iError_CouldNotRevertFiles;
    static const int s_iError_CouldNotSaveStash;
    static const int s_iError_CouldNotSetCurrentBranch;
    static const int s_iError_CouldNotUnstageFiles;
    static const int s_iError_CouldNotWriteCurrentBranch;
    static const int s_iError_CouldNotWriteGeneralInformation;
    static const int s_iError_CouldNotWriteStage;
    static const int s_iError_NoBranchNameGiven;
    static const int s_iError_NoFileNameGiven;
    static const int s_iError_NoRemoteHostNameGiven;
    static const int s_iError_NotARepository;
    static const int s_iError_MergeHasConflicts;
    static const int s_iError_UnknownCommand;
    static const int s_iError_UnknownSwitch;

    // Text strings
    static const QString s_sAllFilesAreClean;
    static const QString s_sStatusOfFiles;
    static const QString s_sTextCannotDeleteCurrentBranch;
    static const QString s_sTextCannotMergeBranchWithSelf;
    static const QString s_sTextCommands;
    static const QString s_sTextUnknownCommand;
    static const QString s_sTextYouAreAlreadyOnBranch;
    static const QString s_sTextYouAreNowOnBranch;

    static const QStringList s_lCopyrights;

    static QMap<QString, CConstants::ECommand> s_mCommands;
    static QMap<QString, QString> s_mHelp;

    //!
    static void initCommandMap();
};
