
#pragma once

// Global
#include "../Aze_global.h"

// Application
#include "CBaseCommand.h"
#include "../objects/CCommit.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CPopStashCommand : public CBaseCommand
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------------------------------

    //! Pops the stash with ID <sId>, uses head if <sId> is empty
    CPopStashCommand(CRepository* pRepository, const QString& sId);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    virtual CError::EErrorCode execute() override;

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    QString m_sId;
};

}
