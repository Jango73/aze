
// Qt
#include <QDebug>

// Application
#include "CConstants.h"

//-------------------------------------------------------------------------------------------------

const char* CConstants::s_sContextMain                      = "main";

// Command switches

const char* CConstants::s_sSwitchAmend                      = "amend";
const char* CConstants::s_sSwitchCleanUp                    = "clean-up";
const char* CConstants::s_sSwitchClone                      = "clone";
const char* CConstants::s_sSwitchCommit                     = "commit";
const char* CConstants::s_sSwitchCreateBranch               = "create-branch";
const char* CConstants::s_sSwitchCreateTag                  = "create-tag";
const char* CConstants::s_sSwitchDeleteBranch               = "delete-branch";
const char* CConstants::s_sSwitchDeleteTag                  = "delete-tag";
const char* CConstants::s_sSwitchDiff                       = "diff";
const char* CConstants::s_sSwitchDump                       = "show";
const char* CConstants::s_sSwitchHelp                       = "help";
const char* CConstants::s_sSwitchInitRepository             = "init";
const char* CConstants::s_sSwitchLog                        = "log";
const char* CConstants::s_sSwitchMerge                      = "merge";
const char* CConstants::s_sSwitchMove                       = "move";
const char* CConstants::s_sSwitchPatch                      = "patch";
const char* CConstants::s_sSwitchPopStash                   = "pop-stash";
const char* CConstants::s_sSwitchPrune                      = "prune";
const char* CConstants::s_sSwitchPull                       = "pull";
const char* CConstants::s_sSwitchPullTag                    = "pull-tag";
const char* CConstants::s_sSwitchPush                       = "push";
const char* CConstants::s_sSwitchPushTag                    = "push-tag";
const char* CConstants::s_sSwitchRemove                     = "remove";
const char* CConstants::s_sSwitchRevert                     = "revert";
const char* CConstants::s_sSwitchSaveStash                  = "save-stash";
const char* CConstants::s_sSwitchSetRemoteHost              = "set-remote-host";
const char* CConstants::s_sSwitchShowStatus                 = "status";
const char* CConstants::s_sSwitchStage                      = "stage";
const char* CConstants::s_sSwitchSwitchToBranch             = "set-branch";
const char* CConstants::s_sSwitchListBranches              = "list-branches";
const char* CConstants::s_sSwitchListTags                   = "list-tags";
const char* CConstants::s_sSwitchRenameBranch              = "rename-branch";
const char* CConstants::s_sSwitchUnstage                    = "unstage";

//-------------------------------------------------------------------------------------------------
// Client switches

const char* CConstants::s_sSwitchAdded                      = "added";
const char* CConstants::s_sSwitchAll                        = "all";
const char* CConstants::s_sSwitchAllowFileDelete            = "allow-delete";
const char* CConstants::s_sSwitchAuthor                     = "author";
const char* CConstants::s_sSwitchClean                      = "clean";
const char* CConstants::s_sSwitchCount                      = "count";
const char* CConstants::s_sSwitchDeleted                    = "deleted";
const char* CConstants::s_sSwitchForce                      = "force";
const char* CConstants::s_sSwitchGraph                      = "graph";
const char* CConstants::s_sSwitchIgnored                    = "ignored";
const char* CConstants::s_sSwitchLoose                      = "loose";
const char* CConstants::s_sSwitchMessage                    = "message";
const char* CConstants::s_sSwitchMissing                    = "missing";
const char* CConstants::s_sSwitchModified                   = "modified";
const char* CConstants::s_sSwitchNoFastForward              = "no-fast-forward";
const char* CConstants::s_sSwitchDryRun                     = "dry-run";
const char* CConstants::s_sSwitchRetention                  = "retention";
const char* CConstants::s_sSwitchStart                      = "start";
const char* CConstants::s_sSwitchSilent                     = "silent";
const char* CConstants::s_sSwitchDebug                      = "debug";
const char* CConstants::s_sSwitchTextCache                  = "text-cache";
const char* CConstants::s_sSwitchVoid                       = "";

//-------------------------------------------------------------------------------------------------
// Server switches

const char* CConstants::s_sSwitchPort                       = "port";
const char* CConstants::s_sSwitchHttpsPort                 = "https-port";
const char* CConstants::s_sSwitchSslCert                   = "ssl-cert";
const char* CConstants::s_sSwitchSslKey                    = "ssl-key";

//-------------------------------------------------------------------------------------------------
// Text strings

const QString CConstants::s_sAllFilesAreClean               = tr("All files are clean on branch %1.");
const QString CConstants::s_sStatusOfFiles                  = tr("Status of working directory files, on branch %1:");
const QString CConstants::s_sTextCommands                   = tr("Commands");
const QString CConstants::s_sTextYouAreAlreadyOnBranch      = tr("You are already on branch %1.");
const QString CConstants::s_sTextYouAreNowOnBranch          = tr("You are now on branch %1.");

const QStringList CConstants::s_lCopyrights =
{
    "Aze Copyright (c) 2020 Jango73 (https://github.com/Jango73/Aze)",
    "Based on Qt 5.13.2 Copyright (c) 2020 The Qt Company",
    "Using Diff Template Library Copyright (c) 2015 Tatsuhiko Kubo <cubicdaiya@gmail.com> All rights reserved."
};

QMap<QString, CConstants::ECommand> CConstants::s_mCommands;
QMap<QString, QString> CConstants::s_mHelp;

//-------------------------------------------------------------------------------------------------

void CConstants::initCommandMap()
{
    s_mCommands[CConstants::s_sSwitchAmend]               = CConstants::eCommandAmend;
    s_mCommands[CConstants::s_sSwitchCleanUp]           = CConstants::eCommandCleanUp;
    s_mCommands[CConstants::s_sSwitchClone]             = CConstants::eCommandClone;
    s_mCommands[CConstants::s_sSwitchCommit]            = CConstants::eCommandCommit;
    s_mCommands[CConstants::s_sSwitchCreateBranch]      = CConstants::eCommandCreateBranch;
    s_mCommands[CConstants::s_sSwitchCreateTag]         = CConstants::eCommandCreateTag;
    s_mCommands[CConstants::s_sSwitchDeleteBranch]      = CConstants::eCommandDeleteBranch;
    s_mCommands[CConstants::s_sSwitchDeleteTag]         = CConstants::eCommandDeleteTag;
    s_mCommands[CConstants::s_sSwitchDiff]              = CConstants::eCommandDiff;
    s_mCommands[CConstants::s_sSwitchDump]              = CConstants::eCommandDump;
    s_mCommands[CConstants::s_sSwitchHelp]              = CConstants::eCommandHelp;
    s_mCommands[CConstants::s_sSwitchInitRepository]    = CConstants::eCommandInitRepository;
    s_mCommands[CConstants::s_sSwitchLog]               = CConstants::eCommandLog;
    s_mCommands[CConstants::s_sSwitchMerge]             = CConstants::eCommandMerge;
    s_mCommands[CConstants::s_sSwitchMove]              = CConstants::eCommandMove;
    s_mCommands[CConstants::s_sSwitchPatch]             = CConstants::eCommandPatch;
    s_mCommands[CConstants::s_sSwitchPopStash]          = CConstants::eCommandPopStash;
    s_mCommands[CConstants::s_sSwitchPrune]             = CConstants::eCommandPrune;
    s_mCommands[CConstants::s_sSwitchPull]              = CConstants::eCommandPull;
    s_mCommands[CConstants::s_sSwitchPullTag]           = CConstants::eCommandPullTag;
    s_mCommands[CConstants::s_sSwitchPush]              = CConstants::eCommandPush;
    s_mCommands[CConstants::s_sSwitchPushTag]           = CConstants::eCommandPushTag;
    s_mCommands[CConstants::s_sSwitchRemove]            = CConstants::eCommandRemove;
    s_mCommands[CConstants::s_sSwitchRevert]            = CConstants::eCommandRevert;
    s_mCommands[CConstants::s_sSwitchSaveStash]         = CConstants::eCommandSaveStash;
    s_mCommands[CConstants::s_sSwitchSetRemoteHost]     = CConstants::eCommandSetRemoteHost;
    s_mCommands[CConstants::s_sSwitchShowStatus]        = CConstants::eCommandShowStatus;
    s_mCommands[CConstants::s_sSwitchStage]             = CConstants::eCommandStage;
    s_mCommands[CConstants::s_sSwitchSwitchToBranch]    = CConstants::eCommandSwitchToBranch;
    s_mCommands[CConstants::s_sSwitchListBranches]       = CConstants::eCommandListBranches;
    s_mCommands[CConstants::s_sSwitchListTags]          = CConstants::eCommandListTags;
    s_mCommands[CConstants::s_sSwitchRenameBranch]      = CConstants::eCommandRenameBranch;
    s_mCommands[CConstants::s_sSwitchUnstage]           = CConstants::eCommandUnstage;
    s_mCommands[CConstants::s_sSwitchVoid]              = CConstants::eCommandNone;

    s_mHelp[CConstants::s_sSwitchAmend]                 = tr("Rewrites the tip commit with the staged files.");
    s_mHelp[CConstants::s_sSwitchCleanUp]               = tr("Clears the stage and reverts all files. Warning: loose files are deleted.");
    s_mHelp[CConstants::s_sSwitchClone]                 = tr("Clones a repository from a server into a new folder.");
    s_mHelp[CConstants::s_sSwitchCommit]                = tr("Creates a commit from the stage.");
    s_mHelp[CConstants::s_sSwitchCreateBranch]          = tr("Creates a branch.");
    s_mHelp[CConstants::s_sSwitchCreateTag]             = tr("Creates a tag from the current branch tip.");
    s_mHelp[CConstants::s_sSwitchDeleteBranch]          = tr("Deletes a branch. Use --force to delete a branch not merged into the current branch.");
    s_mHelp[CConstants::s_sSwitchDeleteTag]             = tr("Deletes a tag. Use --force to delete a tag not merged into the current branch.");
    s_mHelp[CConstants::s_sSwitchDiff]                  = tr("Shows a diff between two commits or files.");
    s_mHelp[CConstants::s_sSwitchDump]                  = tr("Dumps the content of a database object.");
    s_mHelp[CConstants::s_sSwitchHelp]                  = tr("Shows the help text.");
    s_mHelp[CConstants::s_sSwitchInitRepository]        = tr("Makes the current directory an Aze repository.");
    s_mHelp[CConstants::s_sSwitchLog]                   = tr("Shows a log of a branch.");
    s_mHelp[CConstants::s_sSwitchMerge]                 = tr("Merges a branch onto the current one.");
    s_mHelp[CConstants::s_sSwitchMove]                  = tr("Moves some files.");
    s_mHelp[CConstants::s_sSwitchPatch]                 = tr("Apply a patch (diff) to the repository.");
    s_mHelp[CConstants::s_sSwitchPopStash]              = tr("Restores the contents of a stash.");
    s_mHelp[CConstants::s_sSwitchPrune]                 = tr("Deletes unreachable commits and objects. Use --dry-run to list candidates and --retention to set the grace period in seconds.");
    s_mHelp[CConstants::s_sSwitchPush]                  = tr("Sends local changes to a server.");
    s_mHelp[CConstants::s_sSwitchPushTag]               = tr("Sends a tag to a server.");
    s_mHelp[CConstants::s_sSwitchPull]                  = tr("Gets remote changes from a server.");
    s_mHelp[CConstants::s_sSwitchPullTag]               = tr("Gets a tag from a server.");
    s_mHelp[CConstants::s_sSwitchRemove]                = tr("Removes some files from tracking.");
    s_mHelp[CConstants::s_sSwitchRevert]                = tr("Reverts some files using tip commit.");
    s_mHelp[CConstants::s_sSwitchSaveStash]             = tr("Saves changed files and cleans up the working directory.");
    s_mHelp[CConstants::s_sSwitchSetRemoteHost]         = tr("Sets the remote host name for this repository (e.g. http://myhost.com:8080).");
    s_mHelp[CConstants::s_sSwitchShowStatus]            = tr("Shows status of files.");
    s_mHelp[CConstants::s_sSwitchStage]                 = tr("Adds some files to the staged.");
    s_mHelp[CConstants::s_sSwitchSwitchToBranch]        = tr("Switches to a branch.");
    s_mHelp[CConstants::s_sSwitchListBranches]          = tr("Lists the branches, marking the current branch with an asterisk.");
    s_mHelp[CConstants::s_sSwitchListTags]              = tr("Lists the tags.");
    s_mHelp[CConstants::s_sSwitchRenameBranch]          = tr("Renames a branch.");
    s_mHelp[CConstants::s_sSwitchUnstage]               = tr("Removes some files from the stage.");
}
