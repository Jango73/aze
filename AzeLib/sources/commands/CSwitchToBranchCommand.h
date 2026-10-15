
#pragma once

// Global
#include "../Aze_global.h"

// Application
#include "CBaseCommand.h"
#include "../objects/CCommit.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CSwitchToBranchCommand : public CBaseCommand
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------------------------------

    CSwitchToBranchCommand(CRepository* pRepository, const QString& sBranchName, bool bAllowFileDelete = false);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    virtual CError::EErrorCode execute() override;

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    bool        m_bAllowFileDelete;
    QString     m_sBranchName;
};

}
