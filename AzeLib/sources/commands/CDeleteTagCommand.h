
#pragma once

// Global
#include "../Aze_global.h"

// Application
#include "CDeleteBranchCommand.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CDeleteTagCommand : public CDeleteBranchCommand
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------------------------------

    CDeleteTagCommand(CRepository* pRepository, const QString& sName, bool bForce = false);

    //-------------------------------------------------------------------------------------------------
    // Protected control methods
    //-------------------------------------------------------------------------------------------------

protected:

    //!
    virtual CError::EErrorCode missingCode() const override;

    //!
    virtual bool acceptsType(const CBranch* pBranch) const override;
};

}
