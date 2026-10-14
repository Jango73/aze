
// Application
#include "CStrings.h"

//-------------------------------------------------------------------------------------------------

// Use first line for human readable files
#ifdef QT_DEBUG
#define XMLC_EXTENSION  "xml"
#else
#define XMLC_EXTENSION  "xmlc"
#endif

//-------------------------------------------------------------------------------------------------

namespace Aze {

const QString CStrings::s_sPathSep                              = "/";
const QString CStrings::s_sNewLine                              = "\n";
const QString CStrings::s_sNewLineEscaped                       = "\\n";
const QString CStrings::s_sBase64Marker                         = "B64:";
const QString CStrings::s_sPathIdSeparator                      = "@";
const QString CStrings::s_sItemSeparator                        = ",";
const QString CStrings::s_sCompressedXMLExtension               = XMLC_EXTENSION;
const QString CStrings::s_sDiffChunkHeader                      = "diff --aze";

const QString CStrings::s_sParamAuthor                          = "author";
const QString CStrings::s_sParamBranch                          = "branch";
const QString CStrings::s_sParamBranches                        = "branches";
const QString CStrings::s_sParamCommit                          = "commit";
const QString CStrings::s_sParamCommitChain                     = "commitChain";
const QString CStrings::s_sParamCommitParentList                = "commitParentList";
const QString CStrings::s_sParamCommitTreeList                  = "commitTreeList";
const QString CStrings::s_sParamCurrent                         = "current";
const QString CStrings::s_sParamDate                            = "date";
const QString CStrings::s_sParamDiff                            = "diff";
const QString CStrings::s_sParamError                           = "error";
const QString CStrings::s_sParamFiles                           = "files";
const QString CStrings::s_sParamId                              = "id";
const QString CStrings::s_sParamInfo                            = "info";
const QString CStrings::s_sParamIsMerge                         = "isMerge";
const QString CStrings::s_sParamMessage                         = "message";
const QString CStrings::s_sParamName                            = "name";
const QString CStrings::s_sParamObject                          = "object";
const QString CStrings::s_sParamParents                         = "parents";
const QString CStrings::s_sParamPull                            = "pull";
const QString CStrings::s_sParamPush                            = "push";
const QString CStrings::s_sParamRemoteHost                      = "remoteHost";
const QString CStrings::s_sParamRepository                      = "repository";
const QString CStrings::s_sParamRequest                         = "request";
const QString CStrings::s_sParamResult                          = "result";
const QString CStrings::s_sParamRoot                            = "root";
const QString CStrings::s_sParamStage                           = "stage";
const QString CStrings::s_sParamStash                           = "stash";
const QString CStrings::s_sParamStashList                       = "stashList";
const QString CStrings::s_sParamSuccess                         = "success";
const QString CStrings::s_sParamTip                             = "tip";
const QString CStrings::s_sParamType                            = "type";
const QString CStrings::s_sParamUser                            = "user";

const QString CStrings::s_sPathAzeBranchPath                    = "branches";
const QString CStrings::s_sPathAzeCommitPath                    = "commits";
const QString CStrings::s_sPathAzeDataRootPath                  = ".aze";
const QString CStrings::s_sPathAzeMergePath                     = "merge";
const QString CStrings::s_sPathAzeObjectPath                    = "objects";
const QString CStrings::s_sPathAzeStashPath                     = "stash";
const QString CStrings::s_sPathAzeCachePath                     = "cache";

const QString CStrings::s_sGeneralInformationFileName           = "info." XMLC_EXTENSION;
const QString CStrings::s_sStagingCommitFileName                = "stage";

const QString CStrings::s_sDefaultBranchName                    = "trunk";

const QString CStrings::s_sTextBranchAlreadyExists              = tr("Branch already exists.");
const QString CStrings::s_sTextBranchDoesNotExist               = tr("Branch does not exist.");
const QString CStrings::s_sTextBranchNameEmpty                  = tr("No branch name specified.");
const QString CStrings::s_sTextCannotMergeSameCommits           = tr("You are trying to merge a commit onto itself, cannot do that.");
const QString CStrings::s_sTextCommitEmpty                      = tr("No files to commit.");
const QString CStrings::s_sTextCommonAncestor                   = tr("Common ancestor is %1 (%2).");
const QString CStrings::s_sTextCouldNotApplyPatch               = tr("Could not patch %1.");
const QString CStrings::s_sTextCouldNotRemoveFile               = tr("Could not remove file %1.");
const QString CStrings::s_sTextMergeFailed                      = tr("Merge failed.");
const QString CStrings::s_sTextMergeWorkingDirectoryNotClean    = tr("Please clean up the working directory before atempting a merge.");
const QString CStrings::s_sTextNoCommitToMerge                  = tr("There are no commits to merge.");
const QString CStrings::s_sTextNoCommonAncestor                 = tr("Cannot find common ancestors.");
const QString CStrings::s_sTextNoCurrentBranch                  = tr("No current branch.");
const QString CStrings::s_sTextNoFileSpecified                  = tr("No file name specified.");
const QString CStrings::s_sTextNoStagingCommit                  = tr("Stage information missing.");
const QString CStrings::s_sTextNoSuchBranch                     = tr("No such branch.");
const QString CStrings::s_sTextNoSuchFile                       = tr("No such file.");
const QString CStrings::s_sTextNotARepository                   = tr("This is not an Aze repository.");
const QString CStrings::s_sTextNothingToStash                   = tr("There is nothing to stash.");
const QString CStrings::s_sTextNoTipCommit                      = tr("No tip commit.");
const QString CStrings::s_sTextObjectStoreFailed                = tr("Could not store object %1.");
const QString CStrings::s_sTextPushNotFastForward               = tr("Push rejected: current tip is not an ancestor of the pushed tip.");
const QString CStrings::s_sTextPushRequestInvalid               = tr("Invalid push request.");
const QString CStrings::s_sTextStashPopFailed                   = tr("Stash pop failed.");
const QString CStrings::s_sTextStashSaveFailed                  = tr("Stash save failed.");

} // namespace Aze
