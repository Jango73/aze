
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
#include "CCommitFunctions.h"
#include "CEnums.h"
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
    CRepository(const QString& sRootPath, QObject* parent = nullptr, bool bSilent = false, bool bDebug = false);

    //!
    ~CRepository();

    //-------------------------------------------------------------------------------------------------
    // Command methods
    //-------------------------------------------------------------------------------------------------

    //! Initializes a repository, creating mandatory folders and files
    bool init();

    //! Creates a branch as sName
    bool createBranch(const QString& sName);

    //! Deletes the branch sName
    bool deleteBranch(const QString& sName);

    //! Sets the current branch to sName
    bool switchToBranch(const QString& sName, bool bAllowFileDelete = false);

    //! Stages all files listed in lFileNames
    bool stage(const QStringList& lFileNames);

    //! Unstages all files listed in lFileNames
    bool unstage(const QStringList& lFileNames);

    //! Removes all files listed in lFileNames
    bool remove(const QStringList& lFileNames);

    //! Commits all files contained in the staging commit
    bool commit(const QString& sAuthor, const QString& sMessage);

    //! Rewrites the tip commit without creation of a new parent
    bool amend(const QString& sAuthor, const QString& sMessage);

    //! Reverts all files listed in lFileNames
    bool revert(const QStringList& lFileNames, bool bAllowFileDelete = false);

    //! Reverts all files listed in the commit
    bool revert(CCommit* pWorkingDirectory, bool bAllowFileDelete = false);

    //! Reverts all files
    bool revert(bool bAllowFileDelete = false);

    //! Returns a log
    QString log(const QStringList& lFileNames, bool bGraph = false, int iStart = 0, int iCount = 0);

    //! Returns a list of files and their status
    QList<CFile> fileStatus(const QStringList& lFileNames);

    //! Makes a diff betweeen commits or files
    QString diff(const QString& sFirst, const QString& sSecond);

    //! Merges the branch sName onto the current one
    //! bHasConflicts is set when the merge reports conflicts
    //! pMergeError optionally receives a CMergeCommand::EMergeError reason on failure
    bool merge(const QString& sName, bool& bHasConflicts, int* pMergeError = nullptr);

    //! Stashes the working directory changes
    bool saveStash(const QString& sMessage);

    //! Restores the contents of a stash
    bool popStash(const QString& sId);

    //! Patches the repository with file
    bool patch(const QString& sFilename);

    //! Returns the staging commit
    CCommit* getStagingCommit();

    //! Reads the stage commit
    bool readGeneralInformation();

    //! Writes the general information
    bool writeGeneralInformation();

    //! Forces a reload of the current branch
    //! Needed when switching branches
    void refreshCurrentBranch();

    //! Writes the current branch
    bool writeCurrentBranch();

    //! Writes the stage commit
    bool writeStage();

    //! Clears the stage commit (but does not write it)
    bool clearStage();

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
    void tellError(const QString& sText);

    //! Prints an info message
    void tellInfo(const QString& sText);

    //! Replaces symbols in sText with their actual value (if any)
    QString processKeywords(const QString& sText);

    //! Places in iDelta the value of delta symbols in sText (if any)
    QString processDeltas(const QString& sText, int& iDelta);

    //! Returns the diff between tip commit and working directory, if any
    QString diffWorkingDirectory();

    //!
    QString getPushRequest();

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
