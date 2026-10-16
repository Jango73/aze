
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

class AZE_SHARED_EXPORT CCloneCommand : public CBaseCommand
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------------------------------

    CCloneCommand(CRepository* pTargetRepository, const CXMLNode& xResponse, QString& sDetail);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    virtual CError::EErrorCode execute() override;

    //-------------------------------------------------------------------------------------------------
    // Protected control methods
    //-------------------------------------------------------------------------------------------------

protected:

    //! Refuses a response which is not a clone payload with named branches
    CError::EErrorCode validateResponse();

    //! Creates the target folder and refuses a folder which already holds a repository
    CError::EErrorCode createTargetFolder();

    //! Creates the `.aze` folder structure of the target repository
    CError::EErrorCode initDatabase();

    //! Stores the object nodes of the response
    CError::EErrorCode storeObjects();

    //! Stores the commit nodes of the response into m_lCommitIds
    CError::EErrorCode storeCommits();

    //! Stores the branch nodes of the response and resolves the current branch
    CError::EErrorCode storeBranches();

    //! Writes the general information file with the cloned current branch
    CError::EErrorCode writeGeneralInformation();

    //! Writes an empty stage commit for the cloned repository
    CError::EErrorCode writeEmptyStage();

    //! Extracts the tip commit files into the target working folder
    CError::EErrorCode checkoutWorkingFolder();

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    CXMLNode    m_xResponse;
    QString&    m_sDetail;
    QStringList m_lBranchNames;
    QString     m_sCurrentBranchName;
    QStringList m_lCommitIds;
};

}
