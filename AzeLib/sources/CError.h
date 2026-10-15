
#pragma once

// Global
#include "Aze_global.h"

// Qt
#include <QObject>
#include <QString>

// qt-plus
#include "CXMLNode.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CError : public QObject
{
    Q_OBJECT

public:

    // Error codes, values 0x00 to 0x1E preserve CConstants exit codes
    enum EErrorCode
    {
        eNone                               = 0x00,
        eCouldNotAddFiles                   = 0x01,
        eCouldNotAmend                      = 0x02,
        eCouldNotCommit                     = 0x03,
        eCouldNotCreateBranch               = 0x04,
        eCouldNotDeleteBranch               = 0x05,
        eCouldNotMerge                      = 0x06,
        eCouldNotMoveFiles                  = 0x07,
        eCouldNotPatch                      = 0x08,
        eCouldNotPopStash                   = 0x09,
        eCouldNotReadStage                  = 0x0A,
        eCouldNotRemoveFiles                = 0x0B,
        eCouldNotRestoreStash               = 0x0C,
        eCouldNotRevertFiles                = 0x0D,
        eCouldNotSaveStash                  = 0x0E,
        eCouldNotSetCurrentBranch           = 0x0F,
        eCouldNotUnstageFiles               = 0x10,
        eCouldNotWriteCurrentBranch         = 0x11,
        eCouldNotWriteGeneralInformation    = 0x12,
        eCouldNotWriteStage                 = 0x13,
        eNoBranchNameGiven                  = 0x14,
        eNoFileNameGiven                    = 0x15,
        eNoRemoteHostNameGiven              = 0x16,
        eNotARepository                     = 0x17,
        eMergeHasConflicts                  = 0x18,
        eUnknownCommand                     = 0x19,
        eUnknownSwitch                      = 0x1A,
        eUnknownBranch                      = 0x1B,
        eDirtyWorkingFolder                 = 0x1C,
        eNothingToMerge                     = 0x1D,
        eStageNotEmpty                      = 0x1E,
        eGeneric                             = 0x1F,
        eNoCurrentBranch                    = 0x20,
        eNoStagingCommit                    = 0x21,
        eNoTipCommit                        = 0x22,
        eNoSuchFile                         = 0x23,
        eBranchAlreadyExists                = 0x24,
        eBranchDoesNotExist                 = 0x25,
        eCannotDeleteCurrentBranch          = 0x26,
        eCannotMergeSelf                    = 0x27,
        eNoCommonAncestor                   = 0x28,
        eNoCommitToMerge                    = 0x29,
        eCommitEmpty                        = 0x2A,
        eCouldNotRemoveFile                 = 0x2B,
        eCouldNotApplyPatch                 = 0x2C,
        eObjectStoreFailed                  = 0x2D,
        ePushRequestInvalid                 = 0x2E,
        ePushNotFastForward                 = 0x2F,
        eStashSaveFailed                    = 0x30,
        eStashPopFailed                     = 0x31,
        eNothingToStash                     = 0x32,
        eNetworkError                       = 0x33,
        eInvalidResponse                    = 0x34,
        eUnknownAction                      = 0x35,
        eEmptyRequest                       = 0x36
    };

    Q_ENUM(EErrorCode)

    //-------------------------------------------------------------------------------------------------
    // Inline functions
    //-------------------------------------------------------------------------------------------------

    static bool isOk(EErrorCode eCode) { return eCode == eNone || eCode == eMergeHasConflicts; }
    static int toExitCode(EErrorCode eCode) { return static_cast<int>(eCode); }

    //-------------------------------------------------------------------------------------------------
    // External functions
    //-------------------------------------------------------------------------------------------------

    static QString message(EErrorCode eCode);
    static QString message(EErrorCode eCode, const QString& sDetail);
    static QString message(EErrorCode eCode, const QString& sDetailOne, const QString& sDetailTwo);
    static CXMLNode toResultNode(EErrorCode eCode, const QString& sDetail = QString());
    static EErrorCode fromResultNode(const CXMLNode& xNode);
};

} // namespace Aze
