
#pragma once

// Global
#include "../Aze_global.h"

// Application
#include "CListBranchesCommand.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CListTagsCommand : public CListBranchesCommand
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------------------------------

    CListTagsCommand(CRepository* pRepository, QStringList* pNames);

    //-------------------------------------------------------------------------------------------------
    // Protected control methods
    //-------------------------------------------------------------------------------------------------

protected:

    //!
    virtual bool acceptsType(const CBranch* pBranch) const override;
};

}
