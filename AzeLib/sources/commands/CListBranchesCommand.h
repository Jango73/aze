
#pragma once

// Global
#include "../Aze_global.h"

// Qt
#include <QStringList>

// Application
#include "CBaseCommand.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class CBranch;

class AZE_SHARED_EXPORT CListBranchesCommand : public CBaseCommand
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------------------------------

    CListBranchesCommand(CRepository* pRepository, QStringList* pNames);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    virtual CError::EErrorCode execute() override;

    //-------------------------------------------------------------------------------------------------
    // Protected control methods
    //-------------------------------------------------------------------------------------------------

protected:

    //! Returns true for a listed object kind, overridden by tags
    virtual bool acceptsType(const CBranch* pBranch) const;

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    QStringList* m_pNames;
};

}
