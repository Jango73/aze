
#pragma once

// Global
#include "Aze_global.h"

// Qt
#include <QObject>
#include <QDateTime>

// qt-plus
#include "Macros.h"

// Application
#include "CRepository.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CRemoteRepository : public CObject
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

    Q_FAST_PROPERTY(CRepository*, p, localRepository, LocalRepository)

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //!
    CRemoteRepository(CRepository* pLocalRepository, QObject* parent = nullptr);

    //!
    ~CRemoteRepository();

    //-------------------------------------------------------------------------------------------------
    // Command methods
    //-------------------------------------------------------------------------------------------------

    //! Returns the request for pull
    //! Carries the requested branch name and the local tip identifier
    CXMLNode getPullRequest();

    //! Returns the request for push
    CXMLNode getPushRequest();

    //! Returns the pull response for a pull request
    //! Branches carry their name, commits their identifier, objects their content
    //! Returns an error node when the request is invalid or the branch is unknown
    CXMLNode getPullResponse(const CXMLNode& xRequest);

    //! Applies a pull response to the local repository
    //! Stores received objects and commits, updates the branch with fast forward or merge
    //! Returns eNone, eMergeHasConflicts or the precise failure reason
    //! sDetail optionally receives message detail (object identifier, branch name)
    CError::EErrorCode applyPullResponse(const CXMLNode& xResponse, QString& sDetail);

    //! Returns the request for clone
    CXMLNode getCloneRequest();

    //! Returns the full clone payload of the local repository
    //! Branches carry their name, commits their identifier, objects their content
    CXMLNode getCloneResponse();

    //! Applies a push request to the local repository
    //! Returns eNone on success, the precise failure reason otherwise
    //! sDetail optionally receives message detail (object identifier, branch name)
    CError::EErrorCode applyPushRequest(const CXMLNode& xRequest, QString& sDetail);

    //! Returns the request for delete-branch
    //! Carries the branch name and the force flag
    CXMLNode getDeleteBranchRequest(const QString& sBranchName, bool bForce = false);

    //! Applies a delete-branch request to the local repository
    //! Returns eNone on success, the precise failure reason otherwise
    //! sDetail optionally receives message detail (branch name)
    CError::EErrorCode applyDeleteBranchRequest(const CXMLNode& xRequest, QString& sDetail);

protected:

    //-------------------------------------------------------------------------------------------------
    // Protected payload methods
    //-------------------------------------------------------------------------------------------------

    //! Collects the tip and ancestor identifiers of a single branch tip
    //! Follows every parent so merge history is included
    QStringList pullCommitIds(const QString& sTipId) const;

    //! Lists the branch names stored in the local repository
    QStringList cloneBranchNames() const;

    //! Collects the tip and ancestor identifiers of the branches
    QStringList cloneCommitIds(const QStringList& lBranchNames) const;

    //! Appends one named node per branch to the response
    void appendCloneBranchNodes(CXMLNode& xResponse, const QStringList& lBranchNames);

    //! Appends commit and object nodes for the commits to the response
    void appendCloneCommitNodes(CXMLNode& xResponse, const QStringList& lCommitIds);
};

}
