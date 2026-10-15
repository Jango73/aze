
#pragma once

// Global
#include "../Aze_global.h"

// Application
#include "CBaseCommand.h"
#include "../objects/CCommit.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CLogEntry : public CBaseCommand
{
};

class AZE_SHARED_EXPORT CLogCommand : public CBaseCommand
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------------------------------

    CLogCommand(CRepository* pRepository, const QStringList& lFileNames, QString* pResult, bool bGraph = false, int iStart = 0, int iCount = 0);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    virtual CError::EErrorCode execute() override;

    //-------------------------------------------------------------------------------------------------
    // Protected control methods
    //-------------------------------------------------------------------------------------------------

protected:

    //!
    void outputBranches(int iCurrentBranch, bool bFirstLine);

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    QStringList                 m_lFileNames;
    QList<QMap<int, CCommit*>>  m_vBranches;
    QString*                    m_pResult;
    bool                        m_bGraph;
    int                         m_iStart;
    int                         m_iCount;
};

}
