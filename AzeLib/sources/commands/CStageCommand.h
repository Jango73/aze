
#pragma once

// Global
#include "../Aze_global.h"

// Application
#include "CBaseCommand.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CStageCommand : public CBaseCommand
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------------------------------

    CStageCommand(CRepository* pRepository, const QStringList& lFileNames);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    virtual CError::EErrorCode execute() override;

    //!
    CError::EErrorCode addSingleFile(QString sRelativeFileName);

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    QStringList     m_lFileNames;
};

}
