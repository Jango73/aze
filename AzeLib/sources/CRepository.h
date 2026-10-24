
#pragma once

// Global
#include "Aze_global.h"

// Qt
#include <QObject>
#include <QDateTime>

// qt-plus
#include "Macros.h"
#include "CLazyPointer.h"

// Application
#include "CRemoteHostInfo.h"
#include "commits/CCommitFunctions.h"
#include "CEnums.h"
#include "CError.h"
#include "CStrings.h"
#include "objects/CBranch.h"
#include "objects/CCommit.h"
#include "objects/CDatabase.h"
#include "objects/CFile.h"
#include "objects/CObject.h"
#include "objects/CStash.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CRepository : public CObject
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

    Q_FAST_PROPERTY(bool, b, ok, Ok)
    Q_FAST_PROPERTY(bool, b, silent, Silent)
    Q_FAST_PROPERTY(bool, b, debug, Debug)
    Q_FAST_PROPERTY(CEnums::EFileStatus, e, status, Status)
    Q_FAST_PROPERTY(CDatabase*, p, database, Database)
    Q_FAST_PROPERTY(CRemoteHostInfo*, p, remoteHostInfo, RemoteHostInfo)
    Q_FAST_PROPERTY(CCommitFunctions*, p, commitFunctions, CommitFunctions)

    Q_FAST_PROPERTY(QString, s, currentBranchName, CurrentBranchName)
    Q_FAST_PROPERTY(QString, s, stagingCommitFileName, StagingCommitFileName)

public:

    //! Lazy holders for branch and commits
    //! CLazyPointer is not copyable, so these properties expose reference accessors only (no setter)
    CLazyPointer<CBranch>& currentBranch() { return m_pCurrentBranch; }
    const CLazyPointer<CBranch>& currentBranch() const { return m_pCurrentBranch; }
    CLazyPointer<CCommit>& stagingCommit() { return m_pStagingCommit; }
    const CLazyPointer<CCommit>& stagingCommit() const { return m_pStagingCommit; }
    CLazyPointer<CCommit>& rootCommit() { return m_pRootCommit; }
    const CLazyPointer<CCommit>& rootCommit() const { return m_pRootCommit; }
    CLazyPointer<CCommit>& tipCommit() { return m_pTipCommit; }
    const CLazyPointer<CCommit>& tipCommit() const { return m_pTipCommit; }

protected:

    CLazyPointer<CBranch> m_pCurrentBranch;
    CLazyPointer<CCommit> m_pStagingCommit;
    CLazyPointer<CCommit> m_pRootCommit;
    CLazyPointer<CCommit> m_pTipCommit;

    Q_FAST_REFED_PROPERTY(QStringList, l, stashList, StashList)

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //!
    CRepository(const QString& sRootPath, QObject* parent = nullptr, bool bSilent = false, bool bDebug = false, bool bTextCache = false);

    //!
    ~CRepository();

    //-------------------------------------------------------------------------------------------------
    // Command methods
    //-------------------------------------------------------------------------------------------------

    //! Initializes a repository, creating mandatory folders and files
    CError::EErrorCode init();

    //! Clones a repository from a clone response into the target folder
    //! Creates the `.aze` folder and copies branches, commits and objects
    //! sDetail optionally receives message detail (branch name, object identifier)
    static CError::EErrorCode clone(const QString& sTargetPath, const CXMLNode& xResponse, QString& sDetail, QObject* parent = nullptr);

    //! Creates a branch as sName
    CError::EErrorCode createBranch(const QString& sName);

    //! Creates a tag as sName from the current branch tip
    CError::EErrorCode createTag(const QString& sName);

    //! Deletes the branch sName
    //! bForce deletes a branch not merged into the current branch
    CError::EErrorCode deleteBranch(const QString& sName, bool bForce = false);

    //! Deletes the tag sName
    //! bForce deletes a tag not merged into the current branch
    CError::EErrorCode deleteTag(const QString& sName, bool bForce = false);

    //! Renames the branch sOldName to sNewName, moving the current branch pointer when needed
    CError::EErrorCode renameBranch(const QString& sOldName, const QString& sNewName);

    //! Lists local branch names into pNames, tags excluded, sorted case-insensitively
    CError::EErrorCode listBranches(QStringList* pNames);

    //! Lists local tag names into pNames, branches excluded, sorted case-insensitively
    CError::EErrorCode listTags(QStringList* pNames);

    //! Sets the current branch to sName
    CError::EErrorCode switchToBranch(const QString& sName, bool bAllowFileDelete = false);

    //! Stages all files listed in lFileNames
    CError::EErrorCode stage(const QStringList& lFileNames);

    //! Unstages all files listed in lFileNames
    CError::EErrorCode unstage(const QStringList& lFileNames);

    //! Removes all files listed in lFileNames
    CError::EErrorCode remove(const QStringList& lFileNames);

    //! Commits all files contained in the staging commit
    CError::EErrorCode commit(const QString& sAuthor, const QString& sMessage);

    //! Rewrites the tip commit without creation of a new parent
    CError::EErrorCode amend(const QString& sAuthor, const QString& sMessage);

    //! Reverts all files listed in lFileNames
    CError::EErrorCode revert(const QStringList& lFileNames, bool bAllowFileDelete = false);

    //! Reverts all files listed in the commit
    CError::EErrorCode revert(CCommit* pWorkingDirectory, bool bAllowFileDelete = false);

    //! Reverts all files
    CError::EErrorCode revert(bool bAllowFileDelete = false);

    //! Returns a log
    QString log(const QStringList& lFileNames, bool bGraph = false, int iStart = 0, int iCount = 0);

    //! Returns a list of files and their status
    QList<CFile> fileStatus(const QStringList& lFileNames);

    //! Makes a diff betweeen commits or files
    QString diff(const QString& sFirst, const QString& sSecond);

    //! Merges the branch sName onto the current one
    //! bHasConflicts is set when the merge reports conflicts
    //! bAllowFastForward moves the current tip forward without a merge commit when it is an ancestor
    //! Returns eNone, eMergeHasConflicts or the precise failure reason
    CError::EErrorCode merge(const QString& sName, bool& bHasConflicts, bool bAllowFastForward = true);

    //! Stashes the working directory changes
    CError::EErrorCode saveStash(const QString& sMessage);

    //! Restores the contents of a stash
    CError::EErrorCode popStash(const QString& sId);

    //! Patches the repository with file
    CError::EErrorCode patch(const QString& sFilename);

    //! Clears the stage and reverts the listed working folder files
    CError::EErrorCode cleanUp(const QStringList& lFileNames, bool bAllowFileDelete = true);

    //! Moves the listed files to the target, last entry is the target
    CError::EErrorCode move(const QStringList& lFileNames);

    //! Deletes unreachable commit and object files by mark-and-sweep
    //! bDryRun lists candidates without deleting them
    //! iRetentionSeconds is the grace period in seconds protecting files by modification time
    //! pRemovedCommits receives deleted or dry-run commit identifiers when not null
    //! pRemovedObjects receives deleted or dry-run object identifiers when not null
    CError::EErrorCode prune(bool bDryRun = false, qint64 iRetentionSeconds = 86400, QStringList* pRemovedCommits = nullptr, QStringList* pRemovedObjects = nullptr);

    //! Returns the printable contents of the listed database objects
    QString dump(const QStringList& lIds);

    //! Sets the remote host name
    CError::EErrorCode setRemoteHost(const QString& sName);

    //! Returns the staging commit
    CCommit* getStagingCommit();

    //! Reads the stage commit
    CError::EErrorCode readGeneralInformation();

    //! Writes the general information
    CError::EErrorCode writeGeneralInformation();

    //! Forces a reload of the current branch
    //! Needed when switching branches
    void refreshCurrentBranch();

    //! Drops the cached file status so the next check reads tip, stage and working folder again
    void invalidateStatusCache();

    //! Drops the cached tip and root commits and the cached file status
    //! Needed after the branch tip changes (commit, amend, fast forward, received push)
    void invalidateCommitCache();

    //! Drops the working folder hash cache and the cached file status
    //! Needed after the working folder content is replaced (merge, revert, switch, pull checkout, stash, patch, move, remove)
    void invalidateWorkingFolderCache();

    //! Drops the cached tip and root commits, the working folder hash cache and the cached file status
    //! Needed after operations that replace both history and working folder (commit, merge, received push, branch switch)
    void invalidateCaches();

    //! Writes the current branch
    CError::EErrorCode writeCurrentBranch();

    //! Writes the stage commit
    CError::EErrorCode writeStage();

    //! Clears the stage commit (but does not write it)
    CError::EErrorCode clearStage();

    //! Checks the status of tracked files
    CEnums::EFileStatus checkStatus(const QStringList& lFileNames);

    //! Gets a list of loose files
    QStringList getLooseFiles();

    //! Adds a stash ID to the list of stash IDs
    void addStashToList(const QString& sId);

    //! Removes a stash ID from the list of stash IDs
    void removeStashFromList(const QString& sId);

    //!
    void setRemoteHostName(const QString& sName);

    //-------------------------------------------------------------------------------------------------
    // Helper methods
    //-------------------------------------------------------------------------------------------------

    //! Prints an error message
    void tellError(CError::EErrorCode eCode, const QString& sDetail = QString());

    //! Prints an info message
    void tellInfo(CError::EErrorCode eCode, const QString& sDetail = QString());

    //! Replaces symbols in sText with their actual value (if any)
    QString processKeywords(const QString& sText);

    //! Places in iDelta the value of delta symbols in sText (if any)
    QString processDeltas(const QString& sText, int& iDelta);

    //! Returns the diff between tip commit and working directory, if any
    QString diffWorkingDirectory();

    //-------------------------------------------------------------------------------------------------
    // Instantiators
    //-------------------------------------------------------------------------------------------------

    //! Reads the current branch
    static CBranch* readCurrentBranch(void* pContext);

    //! Reads the general information
    static CCommit* readStage(void* pContext);

    //! Reads the current branch's root commit
    static CCommit* readRootCommit(void* pContext);

    //! Reads the current branch's tip commit
    static CCommit* readTipCommit(void* pContext);

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

private:

    QList<CFile> m_lFiles;
};

}
