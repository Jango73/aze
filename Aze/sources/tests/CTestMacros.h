
#pragma once

// Application
#include "../common/CConstants.h"

// For debug info on tests, swap next two lines
#define SILENT_OR_DEBUG CConstants::s_sSwitchSilent
//#define SILENT_OR_DEBUG CConstants::s_sSwitchDebug

//-------------------------------------------------------------------------------------------------
// Some macros to hide boiler plate code

#define INIT() \
    clearRepository(); \
    QDir rootDir(m_sRootPath); \
    QVERIFY(rootDir.mkdir(m_sFilesFolderName)); \
    QDir filesDir(m_sFilesFolderPath); \
    QVERIFY(filesDir.mkdir(m_sFilesSubFolderName)); \
    exec({ CConstants::s_sSwitchInitRepository })

#define COMMIT(a) \
    exec({ \
    CConstants::s_sSwitchCommit, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    QString("--%1").arg(CConstants::s_sSwitchMessage), \
    a, \
    QString("--%1").arg(CConstants::s_sSwitchAuthor), \
    m_sAuthor \
    })

#define AMEND(a, b) \
    exec({ \
    CConstants::s_sSwitchAmend, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    QString("--%1").arg(CConstants::s_sSwitchMessage), \
    a, \
    QString("--%1").arg(CConstants::s_sSwitchAuthor), \
    b \
    })

#define AMEND_MESSAGE_ONLY(a) \
    exec({ \
    CConstants::s_sSwitchAmend, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    QString("--%1").arg(CConstants::s_sSwitchMessage), \
    a \
    })

#define STAGE(...) \
    exec({ \
    CConstants::s_sSwitchStage, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    __VA_ARGS__ \
    })

#define UNSTAGE(...) \
    exec({ \
    CConstants::s_sSwitchUnstage, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    __VA_ARGS__ \
    })

#define REMOVE(...) \
    exec({ \
    CConstants::s_sSwitchRemove, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    __VA_ARGS__ \
    })

#define MOVE(...) \
    exec({ \
    CConstants::s_sSwitchMove, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    __VA_ARGS__ \
    })

#define LOG(a) \
    a = exec({ \
    CConstants::s_sSwitchLog, \
    })

#define LOG_GRAPH(a) \
    a = exec({ \
    CConstants::s_sSwitchLog, \
    QString("--%1").arg(CConstants::s_sSwitchGraph), \
    })

#define DELETE_BRANCH(a, b) \
    a = exec({ \
    CConstants::s_sSwitchDeleteBranch, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    b \
    })

#define DELETE_BRANCH_FORCE(a, b) \
    a = exec({ \
    CConstants::s_sSwitchDeleteBranch, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    QString("--%1").arg(CConstants::s_sSwitchForce), \
    b \
    })

#define DIFF_LAST(a) \
    a = exec({ \
    CConstants::s_sSwitchDiff, "tip~1 tip > " \
    })

#define DIFF_BEFORE_LAST(a) \
    a = exec({ \
    CConstants::s_sSwitchDiff, "tip~2 tip~1 > " \
    })

#define CREATE_BRANCH(a) \
    exec({ \
    CConstants::s_sSwitchCreateBranch, \
    QString("--%1").arg(SILENT_OR_DEBUG), a \
    })

#define SWITCH_TO_BRANCH(a) \
    exec({ \
    CConstants::s_sSwitchSwitchToBranch, \
    QString("--%1").arg(CConstants::s_sSwitchAllowFileDelete), \
    QString("--%1").arg(SILENT_OR_DEBUG), a \
    })

#define MERGE_BRANCH(a, b) \
    a = exec({ \
    CConstants::s_sSwitchMerge, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    b \
    })

#define SAVE_STASH(...) \
    exec({ \
    CConstants::s_sSwitchSaveStash, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    __VA_ARGS__ \
    })

#define POP_STASH(...) \
    exec({ \
    CConstants::s_sSwitchPopStash, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    __VA_ARGS__ \
    })

#define PATCH(a, b) \
    a = exec({ \
    CConstants::s_sSwitchPatch, \
    QString("--%1").arg(SILENT_OR_DEBUG), \
    b \
    })

#define STATUS(a) \
    a = exec({ \
    CConstants::s_sSwitchShowStatus, \
    QString("--%1").arg(SILENT_OR_DEBUG) \
    })

//-------------------------------------------------------------------------------------------------
