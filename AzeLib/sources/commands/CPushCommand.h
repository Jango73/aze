
#pragma once

// Global
#include "../Aze_global.h"

// Qt
#include <QStringList>

// Application
#include "CBaseCommand.h"

// qt-plus
#include "CXMLNode.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CPushCommand : public CBaseCommand
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------------------------------

    CPushCommand(CRepository* pTargetRepository, const CXMLNode& xRequest, QString& sDetail);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    virtual CError::EErrorCode execute() override;

    //-------------------------------------------------------------------------------------------------
    // Protected control methods
    //-------------------------------------------------------------------------------------------------

protected:

    //! Refuses a request which is not a push payload with branch and tip
    CError::EErrorCode validateRequest();

    //! Stores the object nodes of the request
    CError::EErrorCode storeObjects();

    //! Stores the commit nodes of the request into m_lCommitIds
    CError::EErrorCode storeCommits();

    //! Updates the pushed branch, refusing history loss without fast forward
    CError::EErrorCode updateBranch();

    //! Returns the root identifier for a created branch, with fallback on oldest parentless commit
    QString resolveRootCommitId();

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    CXMLNode    m_xRequest;
    QString&    m_sDetail;
    QString     m_sBranchName;
    QString     m_sRequestedRootId;
    QString     m_sTipId;
    QStringList m_lCommitIds;
};

}
