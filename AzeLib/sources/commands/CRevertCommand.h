
#pragma once

// Global
#include "../Aze_global.h"

// Application
#include "CBaseCommand.h"
#include "../objects/CCommit.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CRevertCommand : public CBaseCommand
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------------------------------

    //!
    CRevertCommand(CRepository* pRepository, bool bAllowFileDelete = false);

    //!
    CRevertCommand(CRepository* pRepository, const QStringList& lFileNames, bool bAllowFileDelete = false);

    //!
    CRevertCommand(CRepository* pRepository, CCommit* pWorkingDirectory, bool bAllowFileDeleted = false);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    virtual CError::EErrorCode execute() override;

    //!
    CError::EErrorCode revertSingleFile(QString sRelativeFileName);

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    bool            m_bAllowFileDelete;
    CCommit*        m_pWorkingDirectory;
    QStringList     m_lFileNames;
};

}
