
#pragma once

// Global
#include "../Aze_global.h"

// Application
#include "CBaseCommand.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class CBranch;

class AZE_SHARED_EXPORT CDeleteBranchCommand : public CBaseCommand
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------------------------------

    CDeleteBranchCommand(CRepository* pRepository, const QString& sName, bool bForce = false);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    virtual CError::EErrorCode execute() override;

    //-------------------------------------------------------------------------------------------------
    // Protected control methods
    //-------------------------------------------------------------------------------------------------

protected:

    //! Refuses the current branch and, without force, a branch not merged into it
    CError::EErrorCode checkDeletable(const QString& sTargetTipId);

    //! Returns the missing object code, overridden by tags
    virtual CError::EErrorCode missingCode() const;

    //! Returns true for a deletable object kind, overridden by tags
    virtual bool acceptsType(const CBranch* pBranch) const;

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    QString m_sName;
    bool    m_bForce;
};

}
