
#pragma once

// Global
#include "../Aze_global.h"

// Application
#include "CBaseCommand.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CMergeCommand : public CBaseCommand
{
    Q_OBJECT

public:

    enum EMergeError
    {
        eMergeOk = 0,
        eMergeGeneric,
        eMergeUnknownBranch,
        eMergeDirtyWorkingFolder,
        eMergeNothingToMerge,
        eMergeStageNotEmpty
    };

    //-------------------------------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------------------------------

    CMergeCommand(CRepository* pRepository, const QString& sFromBranch, bool& bHasConflicts);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    virtual bool execute() override;

    //!
    int mergeError() const { return m_iMergeError; }

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    QString     m_sFromBranch;
    bool&       m_bHasConflicts;
    int         m_iMergeError;
};

}
