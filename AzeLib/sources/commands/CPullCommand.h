
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

class AZE_SHARED_EXPORT CPullCommand : public CBaseCommand
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------------------------------

    CPullCommand(CRepository* pLocalRepository, const CXMLNode& xResponse, QString& sDetail);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    virtual CError::EErrorCode execute() override;

    //-------------------------------------------------------------------------------------------------
    // Protected control methods
    //-------------------------------------------------------------------------------------------------

protected:

    //! Refuses a response which is not a pull payload with a branch name
    CError::EErrorCode validateResponse();

    //! Stores the object nodes of the response
    CError::EErrorCode storeObjects();

    //! Stores the commit nodes of the response into m_lCommitIds
    CError::EErrorCode storeCommits();

    //! Accepts an attributes-only response on a converged branch
    CError::EErrorCode acceptEmptyPayload();

    //! Updates the pulled branch with fast forward or merge
    CError::EErrorCode updateBranch();

    //! Creates the branch file when the server tip is empty
    CError::EErrorCode handleEmptyServerTip();

    //! Creates a missing branch file and checks out the tip when current
    CError::EErrorCode createMissingBranch();

    //! Moves the tip of a branch which is not the current one
    CError::EErrorCode updateOtherBranch();

    //! Fast forwards or merges the current branch onto the server tip
    CError::EErrorCode updateCurrentBranch();

    //! Extracts the tip commit files into the working folder
    CError::EErrorCode checkoutTipFiles(const QString& sTipId);

    //! Merges the server tip through a temporary branch
    CError::EErrorCode mergeServerTip(const QString& sServerTipId);

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    CXMLNode    m_xResponse;
    QString&    m_sDetail;
    QString     m_sBranchName;
    QString     m_sServerTipId;
    QStringList m_lCommitIds;
};

}
