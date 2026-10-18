
#pragma once

// Global
#include "../Aze_global.h"

// Application
#include "CCreateBranchCommand.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CCreateTagCommand : public CCreateBranchCommand
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------------------------------

    CCreateTagCommand(CRepository* pRepository, const QString& sName);

    //-------------------------------------------------------------------------------------------------
    // Protected control methods
    //-------------------------------------------------------------------------------------------------

protected:

    //!
    virtual CBranch* createBranchObject() override;
};

}
