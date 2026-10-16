
#pragma once

// Global
#include "Aze_global.h"

// Qt
#include <QObject>
#include <QtGlobal>

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CStrings : public QObject
{
    Q_OBJECT

public:

    static const QString s_sPathSep;
    static const QString s_sNewLine;
    static const QString s_sNewLineEscaped;
    static const QString s_sBase64Marker;
    static const QString s_sPathIdSeparator;
    static const QString s_sItemSeparator;
    static const QString s_sCompressedXMLExtension;
    static const QString s_sDiffChunkHeader;

    static const QString s_sParamAuthor;
    static const QString s_sParamBranch;
    static const QString s_sParamBranches;
    static const QString s_sParamCode;
    static const QString s_sParamClone;
    static const QString s_sParamCommit;
    static const QString s_sParamCommitChain;
    static const QString s_sParamCommitParentList;
    static const QString s_sParamCommitTreeList;
    static const QString s_sParamCurrent;
    static const QString s_sParamDate;
    static const QString s_sParamDiff;
    static const QString s_sParamError;
    static const QString s_sParamFiles;
    static const QString s_sParamId;
    static const QString s_sParamInfo;
    static const QString s_sParamIsMerge;
    static const QString s_sParamMessage;
    static const QString s_sParamName;
    static const QString s_sParamObject;
    static const QString s_sParamParents;
    static const QString s_sParamPull;
    static const QString s_sParamPush;
    static const QString s_sParamRemoteHost;
    static const QString s_sParamRepository;
    static const QString s_sParamRequest;
    static const QString s_sParamResult;
    static const QString s_sParamRoot;
    static const QString s_sParamStage;
    static const QString s_sParamStash;
    static const QString s_sParamStashList;
    static const QString s_sParamSuccess;
    static const QString s_sParamTip;
    static const QString s_sParamType;
    static const QString s_sParamUser;

    static const QString s_sPathAzeBranchPath;
    static const QString s_sPathAzeCommitPath;
    static const QString s_sPathAzeDataRootPath;
    static const QString s_sPathAzeMergePath;
    static const QString s_sPathAzeObjectPath;
    static const QString s_sPathAzeStashPath;
    static const QString s_sPathAzeCachePath;

    static const QString s_sGeneralInformationFileName;
    static const QString s_sStagingCommitFileName;

    static const QString s_sDefaultBranchName;
};

}
