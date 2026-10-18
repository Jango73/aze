
#pragma once

// Global
#include "Aze_global.h"

// Qt
#include <QObject>
#include <QDateTime>
#include <QSet>

// qt-plus
#include "Macros.h"

// Application
#include "CRepository.h"
#include "CEnums.h"

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

    //! Returns the pull request for a named branch or tag
    CXMLNode getPullRequestFor(const QString& sName, const QString& sTipId);

    //! Returns the request for push
    CXMLNode getPushRequest();

    //! Returns the push request for a named branch or tag
    CXMLNode getPushRequestFor(const QString& sName, const QString& sRootId, const QString& sTipId, CEnums::EBranchType eType);

    //! Returns the push announce carrying the branch name and the local tip
    //! The server answers with getPushAnnounceResponse so the data request
    //! carries missing commits only
    CXMLNode getPushAnnounce();

    //! Returns the push announce for a named branch or tag
    CXMLNode getPushAnnounceFor(const QString& sName, const QString& sTipId);

    //! Returns the push announce response carrying the branch name and the server tip
    //! An unknown branch answers an empty tip so the data request carries everything
    //! Returns an error node when the announce is invalid
    CXMLNode getPushAnnounceResponse(const CXMLNode& xAnnounce);

    //! Builds the push data request carrying missing commits only
    //! sServerTipId is the tip from the announce response, empty for unknown branches
    //! Commits reachable from sServerTipId and their objects are excluded
    //! Returns ePushNotFastForward without building when the known server tip
    //! is not an ancestor of the local tip, ePushRequestInvalid without a branch or tip
    //! sDetail optionally receives message detail (branch name, tip identifier)
    CError::EErrorCode buildPushDataRequest(const QString& sServerTipId, CXMLNode& xRequest, QString& sDetail);

    //! Builds the push data request for a named branch or tag
    CError::EErrorCode buildPushDataRequestFor(const QString& sName, const QString& sRootId, const QString& sTipId, CEnums::EBranchType eType, const QString& sServerTipId, CXMLNode& xRequest, QString& sDetail);

    //! Builds the push data request from a push announce response
    //! Validates the announce response and extracts the server tip before delegating
    //! to buildPushDataRequest so client and helper paths share one sequence
    //! Returns the announce error when the response carries a result node
    //! sDetail optionally receives message detail (branch name, tip identifier)
    CError::EErrorCode buildPushDataRequestFromAnnounceResponse(const CXMLNode& xAnnounceResponse, CXMLNode& xDataRequest, QString& sDetail);

    //! Builds the push data request for a named branch or tag from an announce response
    CError::EErrorCode buildPushDataRequestForAnnounceResponse(const QString& sName, const QString& sRootId, const QString& sTipId, CEnums::EBranchType eType, const CXMLNode& xAnnounceResponse, CXMLNode& xDataRequest, QString& sDetail);

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
    CXMLNode getDeleteBranchRequest(const QString& sBranchName, bool bForce = false, CEnums::EBranchType eType = CEnums::eBranch);

    //! Applies a delete-branch request to the local repository
    //! Returns eNone on success, the precise failure reason otherwise
    //! sDetail optionally receives message detail (branch name)
    CError::EErrorCode applyDeleteBranchRequest(const CXMLNode& xRequest, QString& sDetail);

protected:

    //-------------------------------------------------------------------------------------------------
    // Protected payload methods
    //-------------------------------------------------------------------------------------------------

    //! Collects the tip and ancestor identifiers down to a stop commit
    //! Follows every parent so merge history is included
    //! sStopAtCommitId ends the walk before the stop commit, empty walks everything
    QStringList missingCommitIds(const QString& sTipId, const QString& sStopAtCommitId) const;

    //! Collects the object identifiers referenced by the commits
    //! Missing commits resolve to nothing so unknown tips stay safe
    QSet<QString> referencedObjectIds(const QStringList& lCommitIds) const;

    //! Appends commit nodes for lCommitIds and object nodes for their content
    //! Objects in lKnownObjectIds are excluded as the receiver holds them already
    void appendCommitAndObjectNodes(CXMLNode& xResponse, const QStringList& lCommitIds, const QSet<QString>& lKnownObjectIds);

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
