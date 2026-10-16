
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
    CXMLNode getPullRequest();

    //! Returns the request for push
    CXMLNode getPushRequest();

    //! Returns the request for clone
    CXMLNode getCloneRequest();

    //! Returns the full clone payload of the local repository
    //! Branches carry their name, commits their identifier, objects their content
    CXMLNode getCloneResponse();

    //! Applies a push request to the local repository
    //! Returns eNone on success, the precise failure reason otherwise
    //! sDetail optionally receives message detail (object identifier, branch name)
    CError::EErrorCode applyPushRequest(const CXMLNode& xRequest, QString& sDetail);

protected:

    //-------------------------------------------------------------------------------------------------
    // Protected payload methods
    //-------------------------------------------------------------------------------------------------

    //! Lists the branch names stored in the local repository
    QStringList cloneBranchNames() const;

    //! Collects the tip and ancestor identifiers of the branches
    QStringList cloneCommitIds(const QStringList& lBranchNames) const;

    //! Appends one named node per branch to the response
    void appendCloneBranchNodes(CXMLNode& xResponse, const QStringList& lBranchNames);

    //! Appends commit and object nodes for the commits to the response
    void appendCloneCommitNodes(CXMLNode& xResponse, const QStringList& lCommitIds);

    //-------------------------------------------------------------------------------------------------
    // Protected control methods
    //-------------------------------------------------------------------------------------------------

    //! Stores the object nodes of a push request
    CError::EErrorCode storeRequestObjects(const CXMLNode& xRequest, QString& sDetail);

    //! Stores the commit nodes of a push request into lCommitIds
    CError::EErrorCode storeRequestCommits(const CXMLNode& xRequest, QStringList& lCommitIds, QString& sDetail);

    //! Updates a branch from pushed commits
    //! sRequestedRootId carries the source branch root so a created branch keeps the branch point
    CError::EErrorCode updateBranchFromPush(const QString& sBranchName, const QString& sRequestedRootId, const QString& sTipId, const QStringList& lCommitIds, QString& sDetail);
};

}
