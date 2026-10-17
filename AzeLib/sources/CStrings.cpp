
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
const QString CStrings::s_sParamCode                            = "code";
const QString CStrings::s_sParamClone                          = "clone";
const QString CStrings::s_sParamCommit                          = "commit";
const QString CStrings::s_sParamCommitChain                     = "commitChain";
const QString CStrings::s_sParamCurrent                         = "current";
const QString CStrings::s_sParamDate                            = "date";
const QString CStrings::s_sParamDeleteBranch                   = "delete-branch";
const QString CStrings::s_sParamDiff                            = "diff";
const QString CStrings::s_sParamError                           = "error";
const QString CStrings::s_sParamFiles                           = "files";
const QString CStrings::s_sParamForce                           = "force";
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

} // namespace Aze
