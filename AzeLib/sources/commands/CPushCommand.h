
#pragma once

// Global
#include "../Aze_global.h"

// Qt
#include <QStringList>

// Application
#include "CBaseCommand.h"
#include "../CEnums.h"

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

    //! Accepts an empty payload when the pushed tip matches the branch tip
    CError::EErrorCode acceptEmptyPayload();

    //! Returns true when the pushed tip descends from the commit
    bool pushedTipContains(const QString& sCommitId);

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
    CEnums::EBranchType m_eType;
    QStringList m_lCommitIds;
};

}
