
#pragma once

// Global
#include "../Aze_global.h"

// Application
#include "CBaseCommand.h"
#include "../objects/CCommit.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CDiffCommand : public CBaseCommand
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------------------------------

    CDiffCommand(CRepository* pRepository, const QString& sAuthor, const QString& sMessage, QString* pResult);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    virtual CError::EErrorCode execute() override;

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    QString     m_sFirst;
    QString     m_sSecond;
    QString*    m_pResult;
};

}
