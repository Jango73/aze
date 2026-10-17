
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CRepository.h"
#include "CUtils.h"

#include "commands/CAmendCommand.h"
#include "commands/CCleanUpCommand.h"
#include "commands/CCloneCommand.h"
#include "commands/CCommitCommand.h"
#include "commands/CCreateBranchCommand.h"
#include "commands/CDeleteBranchCommand.h"
#include "commands/CDiffCommand.h"
#include "commands/CDumpCommand.h"
#include "commands/CInitRepositoryCommand.h"
#include "commands/CLogCommand.h"
#include "commands/CMergeCommand.h"
#include "commands/CMoveCommand.h"
#include "commands/CPatchCommand.h"
#include "commands/CPopStashCommand.h"
#include "commands/CPruneCommand.h"
#include "commands/CRemoveCommand.h"
#include "commands/CRevertCommand.h"
#include "commands/CSaveStashCommand.h"
#include "commands/CSetRemoteHostCommand.h"
#include "commands/CStageCommand.h"
#include "commands/CStatusCommand.h"
#include "commands/CSwitchToBranchCommand.h"
#include "commands/CUnstageCommand.h"

namespace Aze {

/*!
    \class CRepository
    \inmodule Aze
    \brief The top level class of a repository.
*/


//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CRepository. \br\br
    \a sRootPath is the repository's root folder \br
    \a parent is the repository's owner \br
    \a bSilent tells that we should not output any text \br
    \a bDebug tells that we should output debug information \br
    \a bTextCache tells that the commit cache uses the text format \br
*/
CRepository::CRepository(const QString& sRootPath, QObject* parent, bool bSilent, bool bDebug, bool bTextCache)
    : CObject(parent)
    , m_bOk(false)
    , m_bSilent(bSilent)
    , m_bDebug(bDebug)
    , m_eStatus(CEnums::eUnknown)
    , m_pDatabase(new CDatabase(sRootPath, this))
    , m_pRemoteHostInfo(new CRemoteHostInfo(this))
    , m_pCommitFunctions(new CCommitFunctions(m_pDatabase, this, bSilent, bDebug, bTextCache))
    , m_pCurrentBranch(readCurrentBranch, this)
    , m_pStagingCommit(readStage, this)
    , m_pRootCommit(readRootCommit, this)
    , m_pTipCommit(readTipCommit, this)
{
    // Get the path of the staging file
    m_sStagingCommitFileName = m_pDatabase->composeCommitFileName(CStrings::s_sStagingCommitFileName);

    // Check if repository is ok
    QDir dataPath(m_pDatabase->dataPath());
    if (dataPath.exists())
        setOk(true);

    if (m_bOk)
    {
        readGeneralInformation();
    }
}

//-------------------------------------------------------------------------------------------------

/*!
    Destroys a CRepository.
*/
CRepository::~CRepository()
{
    delete m_pCommitFunctions;
    delete m_pRemoteHostInfo;
    delete m_pDatabase;
}

//-------------------------------------------------------------------------------------------------

/*!
    Initializes the repository. \br\br
    \list
        \li Initializes the database
        \li Creates the default branch
        \li Creates the general information file
    \endlist
*/
CError::EErrorCode CRepository::init()
{
    return CInitRepositoryCommand(this).execute();
}

/************************************************************************/

/*!
    Clones a repository from \a xResponse into \a sTargetPath. \br\br
    The target folder is created with its `.aze` structure, then branches,
    commits and objects are copied from the server response and the cloned
    branch tip is extracted into the working folder. \br
    Sets \a sDetail and returns the failure reason when cloning fails. \br
*/
CError::EErrorCode CRepository::clone(const QString& sTargetPath, const CXMLNode& xResponse, QString& sDetail, QObject* parent)
{
    CRepository oTargetRepository(sTargetPath, parent, true);

    return CCloneCommand(&oTargetRepository, xResponse, sDetail).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Creates a branch named \a sName.
*/
CError::EErrorCode CRepository::createBranch(const QString& sName)
{
    return CCreateBranchCommand(this, sName).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Deletes, if it exists, the branch named \a sName. \br\br
    Refuses the current branch to avoid leaving the repository without a branch. \br
    Refuses a branch not merged into the current branch to avoid losing commits,
    unless \a bForce is true. \br
    A branch is merged when its tip is the current tip or is reachable from it. \br
    Returns eCannotDeleteCurrentBranch, eCannotDeleteUnmergedBranch, eBranchDoesNotExist or eCouldNotDeleteBranch on failure. \br
*/
CError::EErrorCode CRepository::deleteBranch(const QString& sName, bool bForce)
{
    return CDeleteBranchCommand(this, sName, bForce).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Switches to the branch named \a sName. \br\br
    \a bAllowFileDelete is passed to the underlying revert command
*/
CError::EErrorCode CRepository::switchToBranch(const QString& sName, bool bAllowFileDelete)
{
    return CSwitchToBranchCommand(this, sName, bAllowFileDelete).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Adds files to the stage. \br\br
    \a lFileNames contains the file names to stage
*/
CError::EErrorCode CRepository::stage(const QStringList& lFileNames)
{
    return CStageCommand(this, lFileNames).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Removes files from the stage. \br\br
    \a lFileNames contains the file names to unstage
*/
CError::EErrorCode CRepository::unstage(const QStringList& lFileNames)
{
    return CUnstageCommand(this, lFileNames).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Adds a file deletion to the stage. \br\br
    \a lFileNames contains the file names to delete
*/
CError::EErrorCode CRepository::remove(const QStringList& lFileNames)
{
    return CRemoveCommand(this, lFileNames).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Saves the state of the repository to a new commit. \br\br
    \a sAuthor tells who is committing
    \a sMessage tells what we are comitting
*/
CError::EErrorCode CRepository::commit(const QString& sAuthor, const QString& sMessage)
{
    return CCommitCommand(this, sAuthor, sMessage).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Rewrites the tip commit without creation of a new parent. \br\br
    \a sAuthor replaces the tip author when not empty
    \a sMessage replaces the tip message when not empty
*/
CError::EErrorCode CRepository::amend(const QString& sAuthor, const QString& sMessage)
{
    return CAmendCommand(this, sAuthor, sMessage).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Reverts files to the state they were in the last commit. \br\br
    \a lFileNames contains the file names to revert
    \a bAllowFileDelete allows to delete files that were not present in the last commit
*/
CError::EErrorCode CRepository::revert(const QStringList& lFileNames, bool bAllowFileDelete)
{
    return CRevertCommand(this, lFileNames, bAllowFileDelete).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Reverts files to the state they were in the last commit. \br\br
    \a pWorkingDirectory contains the files to revert
    \a bAllowFileDelete allows to delete files that were not present in the last commit
*/
CError::EErrorCode CRepository::revert(CCommit* pWorkingDirectory, bool bAllowFileDelete)
{
    return CRevertCommand(this, pWorkingDirectory, bAllowFileDelete).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Reverts files to the state they were in the last commit. \br\br
    \a bAllowFileDelete allows to delete files that were not present in the last commit
*/
CError::EErrorCode CRepository::revert(bool bAllowFileDelete)
{
    return CRevertCommand(this, bAllowFileDelete).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Shows a list of commits on the current branch. \br\br
    \a bGraph tells if the log is shown as a graph
    \a iStart is the index of the commit where we start outputting data
    \a iCount is the number of commits to show
*/
QString CRepository::log(const QStringList& lFileNames, bool bGraph, int iStart, int iCount)
{
    QString sReturnValue;

    if (CLogCommand(this, lFileNames, &sReturnValue, bGraph, iStart, iCount).execute() == CError::eNone)
    {
        return sReturnValue;
    }

    return "";
}

//-------------------------------------------------------------------------------------------------

/*!
    Shows a diff between two commits. \br\br
    \a sFirst is the 'from' commit
    \a sSecond is the 'to' commit
*/
QString CRepository::diff(const QString& sFirst, const QString& sSecond)
{
    QString sReturnValue;

    if (CDiffCommand(this, sFirst, sSecond, &sReturnValue).execute() == CError::eNone)
    {
        return sReturnValue;
    }

    return "";
}

//-------------------------------------------------------------------------------------------------

/*!
    Merges a branch onto the current branch. \br\br
    \a sName is the branch to merge
    \a bHasConflicts will contain whether the merge has conflict files
    \a bAllowFastForward moves the current tip forward without a merge commit when it is an ancestor
*/
CError::EErrorCode CRepository::merge(const QString& sName, bool& bHasConflicts, bool bAllowFastForward)
{
    CMergeCommand oCommand(this, sName, bHasConflicts, bAllowFastForward);
    CError::EErrorCode eCode = oCommand.execute();

    bHasConflicts = (eCode == CError::eMergeHasConflicts);

    return eCode;
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CRepository::saveStash(const QString& sMessage)
{
    return CSaveStashCommand(this, sMessage).execute();
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CRepository::popStash(const QString& sId)
{
    return CPopStashCommand(this, sId).execute();
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CRepository::patch(const QString& sFilename)
{
    return CPatchCommand(this, sFilename).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Clears the stage and reverts the listed working folder files. \br\br
    \a lFileNames holds the expanded working folder files to revert
    \a bAllowFileDelete allows deletion of files missing from the tip commit
*/
CError::EErrorCode CRepository::cleanUp(const QStringList& lFileNames, bool bAllowFileDelete)
{
    return CCleanUpCommand(this, lFileNames, bAllowFileDelete).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Moves the listed files to the target. \br\br
    \a lFileNames holds the source files followed by the target
*/
CError::EErrorCode CRepository::move(const QStringList& lFileNames)
{
    return CMoveCommand(this, lFileNames).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Deletes unreachable commit and object files by mark-and-sweep. \br\br
    Reachable commits derive from every branch tip through all parents, reachable
    objects derive from reachable commits plus stage and stashes. Unreachable
    files younger than \a iRetentionSeconds are kept as protection against
    payloads stored by worker threads serving push requests concurrent with the
    prune. \a bDryRun lists candidates without deletion. \br
    \a pRemovedCommits receives deleted or dry-run commit identifiers when not null \br
    \a pRemovedObjects receives deleted or dry-run object identifiers when not null \br
*/
CError::EErrorCode CRepository::prune(bool bDryRun, qint64 iRetentionSeconds, QStringList* pRemovedCommits, QStringList* pRemovedObjects)
{
    return CPruneCommand(this, bDryRun, iRetentionSeconds, pRemovedCommits, pRemovedObjects).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns the printable contents of the listed database objects. \br\br
    \a lIds holds the object identifiers to dump
*/
QString CRepository::dump(const QStringList& lIds)
{
    QString sReturnValue;

    if (CDumpCommand(this, lIds, &sReturnValue).execute() == CError::eNone)
    {
        return sReturnValue;
    }

    return "";
}

//-------------------------------------------------------------------------------------------------

/*!
    Sets the remote host name to \a sName. \br\br
    The caller writes the general information file.
*/
CError::EErrorCode CRepository::setRemoteHost(const QString& sName)
{
    return CSetRemoteHostCommand(this, sName).execute();
}

//-------------------------------------------------------------------------------------------------

QList<CFile> CRepository::fileStatus(const QStringList& lFileNames)
{
    m_eStatus = checkStatus(lFileNames);
    return m_lFiles;
}

//-------------------------------------------------------------------------------------------------

/*!
    Reads the general information file.
*/
CError::EErrorCode CRepository::readGeneralInformation()
{
    CXMLNode xInfo = CXMLNode::load(m_pDatabase->generalInformationFileName());

    // Read remote host information
    CXMLNode xRemoteHost = xInfo.getNodeByTagName(CStrings::s_sParamRemoteHost);
    m_pRemoteHostInfo->setName(xRemoteHost.attributes()[CStrings::s_sParamName]);

    // Read branches
    CXMLNode xBranches = xInfo.getNodeByTagName(CStrings::s_sParamBranches);
    setCurrentBranchName(xBranches.attributes()[CStrings::s_sParamCurrent]);

    // Read stash list
    CXMLNode xStashListNode = xInfo.getNodeByTagName(CStrings::s_sParamStashList);
    CXMLNodeList xStashList = xStashListNode.getNodesByTagName(CStrings::s_sParamStash);

    for (CXMLNode& xStash : xStashList)
    {
        m_lStashList << xStash.attributes()[CStrings::s_sParamId];
    }

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

/*!
    Writes the general information file.
*/
CError::EErrorCode CRepository::writeGeneralInformation()
{
    CXMLNode xInfo(CStrings::s_sParamInfo);

    // Write remote host information
    CXMLNode xRemoteHost(CStrings::s_sParamRemoteHost);
    xRemoteHost.attributes()[CStrings::s_sParamName] = m_pRemoteHostInfo->name();
    xInfo << xRemoteHost;

    // Write branches
    CXMLNode xBranches(CStrings::s_sParamBranches);
    xBranches.attributes()[CStrings::s_sParamCurrent] = m_sCurrentBranchName;
    xInfo << xBranches;

    // Write stash list
    CXMLNode xStashListNode(CStrings::s_sParamStashList);
    for (const QString& sId : m_lStashList)
    {
        CXMLNode xStash(CStrings::s_sParamStash);
        xStash.attributes()[CStrings::s_sParamId] = sId;
        xStashListNode << xStash;
    }

    xInfo << xStashListNode;

    xInfo.save(m_pDatabase->generalInformationFileName());

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

void CRepository::refreshCurrentBranch()
{
    m_pCurrentBranch.clear();
    m_pCurrentBranch.get();
    invalidateStatusCache();
    m_pCommitFunctions->clearFileHashCache();
}

//-------------------------------------------------------------------------------------------------

/*!
    Drops the cached file status. \br\br
    The status depends on tip, stage and working folder, so every mutation
    of one of them invalidates it and the next checkStatus reads again. \br
*/
void CRepository::invalidateStatusCache()
{
    m_eStatus = CEnums::eUnknown;
    m_lFiles.clear();
}

//-------------------------------------------------------------------------------------------------

/*!
    Drops the cached tip and root commits and the cached file status. \br\br
    Both lazy commits derive from the current branch tip, so they are
    reloaded from the updated branch on next access. \br
*/
void CRepository::invalidateCommitCache()
{
    m_pTipCommit.clear();
    m_pRootCommit.clear();
    invalidateStatusCache();
}

//-------------------------------------------------------------------------------------------------

/*!
    Drops the working folder hash cache and the cached file status. \br\br
    Stored size and modification time entries no longer describe the
    replaced working folder content, so identifiers are recomputed from
    file content on next access. \br
*/
void CRepository::invalidateWorkingFolderCache()
{
    m_pCommitFunctions->clearFileHashCache();
    invalidateStatusCache();
}

//-------------------------------------------------------------------------------------------------

/*!
    Drops the cached tip and root commits, the working folder hash cache
    and the cached file status. \br\br
    Call this after commit, amend, merge, received push and branch switch,
    which replace history, working folder content, or both (see TODO-037). \br
*/
void CRepository::invalidateCaches()
{
    m_pCommitFunctions->clearFileHashCache();
    m_pTipCommit.clear();
    m_pRootCommit.clear();
    invalidateStatusCache();
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CRepository::writeCurrentBranch()
{
    if (not m_pCurrentBranch.isNull())
        m_pCurrentBranch->toNode().save(m_pDatabase->composeBranchFileName(m_sCurrentBranchName));

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CRepository::writeStage()
{
    if (not m_pStagingCommit.isNull())
    {
        m_pStagingCommit->toFile(m_sStagingCommitFileName);
        return CError::eNone;
    }

    return CError::eNoStagingCommit;
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CRepository::clearStage()
{
    if (not m_pStagingCommit.isNull())
    {
        m_pStagingCommit.set(new CCommit(this));
        invalidateStatusCache();

        return CError::eNone;
    }

    return CError::eNoStagingCommit;
}

//-------------------------------------------------------------------------------------------------

CEnums::EFileStatus CRepository::checkStatus(const QStringList& lFileNames)
{
    if (m_eStatus == CEnums::eUnknown)
    {
        CStatusCommand(this, lFileNames, &m_lFiles, &m_eStatus).execute();
    }

    return m_eStatus;
}

//-------------------------------------------------------------------------------------------------

QStringList CRepository::getLooseFiles()
{
    QStringList lReturnValue;

    // Check status of working directory
    checkStatus(QStringList());

    for (CFile& tFile : m_lFiles)
        if (tFile.status() == CEnums::eLoose)
            lReturnValue << tFile.relativeName();

    return lReturnValue;
}

//-------------------------------------------------------------------------------------------------

void CRepository::addStashToList(const QString& sId)
{
    if (not sId.isEmpty())
        m_lStashList << sId;
}

//-------------------------------------------------------------------------------------------------

void CRepository::removeStashFromList(const QString& sId)
{
    m_lStashList.removeAll(sId);
}

//-------------------------------------------------------------------------------------------------

void CRepository::setRemoteHostName(const QString& sName)
{
    m_pRemoteHostInfo->setName(sName);
}

//-------------------------------------------------------------------------------------------------

void CRepository::tellError(CError::EErrorCode eCode, const QString& sDetail)
{
    if (!m_bSilent)
    {
        OUT_ERROR(CError::message(eCode, sDetail));
    }
}

//-------------------------------------------------------------------------------------------------

void CRepository::tellInfo(CError::EErrorCode eCode, const QString& sDetail)
{
    if (!m_bSilent)
    {
        OUT_INFO(CError::message(eCode, sDetail));
    }
}

//-------------------------------------------------------------------------------------------------

QString CRepository::processKeywords(const QString& sText)
{
    if (sText == CStrings::s_sParamTip && not m_pCurrentBranch.isNull())
        return m_pCurrentBranch->tipCommitId();

    if (sText == CStrings::s_sParamRoot && not m_pCurrentBranch.isNull())
        return m_pCurrentBranch->rootCommitId();

    return sText;
}

//-------------------------------------------------------------------------------------------------

QString CRepository::processDeltas(const QString& sText, int& iDelta)
{
    iDelta = 0;

    if (sText.contains("~"))
    {
        QStringList lValues = sText.split("~");

        if (lValues.count() > 1)
            iDelta = lValues[1].toInt();

        return lValues[0];
    }

    return sText;
}

//-------------------------------------------------------------------------------------------------

QString CRepository::diffWorkingDirectory()
{
    QStringList lLooseFiles = getLooseFiles();

    QString sDiff;
    CCommit* pFromCommit = nullptr;
    CCommit* pToCommit = nullptr;

    if (not m_pCurrentBranch->tipCommitId().isEmpty())
    {
        // Get the current branch tip commit
        pFromCommit = m_pCommitFunctions->loadCommit(m_pCurrentBranch->tipCommitId(), this);

        // Get the working directory as commit
        pToCommit = m_pCommitFunctions->folderAsCommit(this);

        // Diff the current tip commit and the working directory
        m_pCommitFunctions->diffCommits(sDiff, pFromCommit, pToCommit, lLooseFiles);
    }

    return sDiff;
}

//-------------------------------------------------------------------------------------------------

/*!
    Reads the current branch file.
    Drops the cached tip and root commits, which derive from the branch tip.
    The staging commit is working state and never derives from the branch,
    so it is preserved: dropping it here would resurrect stale stage content
    from disk on next access whenever the stage was cleared without write.
*/
CBranch* CRepository::readCurrentBranch(void* pContext)
{
    CRepository* pRepository = static_cast<CRepository*>(pContext);

    pRepository->m_pRootCommit.clear();
    pRepository->m_pTipCommit.clear();

    return  pRepository->m_pDatabase->getBranch(pRepository->m_sCurrentBranchName, nullptr);
}

//-------------------------------------------------------------------------------------------------

/*!
    Reads the stage commit.
*/
CCommit* CRepository::readStage(void* pContext)
{
    CRepository* pRepository = static_cast<CRepository*>(pContext);

    return CCommit::fromFile(pRepository->m_sStagingCommitFileName, nullptr, "");
}

//-------------------------------------------------------------------------------------------------

/*!
    Reads the root commit.
*/
CCommit* CRepository::readRootCommit(void* pContext)
{
    CRepository* pRepository = static_cast<CRepository*>(pContext);

    if (not pRepository->m_pCurrentBranch->rootCommitId().isEmpty())
        return pRepository->m_pCommitFunctions->loadCommit(pRepository->m_pCurrentBranch->rootCommitId(), nullptr);

    return nullptr;
}

//-------------------------------------------------------------------------------------------------

/*!
    Reads the tip commit.
*/
CCommit* CRepository::readTipCommit(void* pContext)
{
    CRepository* pRepository = static_cast<CRepository*>(pContext);

    if (not pRepository->m_pCurrentBranch->tipCommitId().isEmpty())
        return pRepository->m_pCommitFunctions->loadCommit(pRepository->m_pCurrentBranch->tipCommitId(), nullptr);

    return nullptr;
}

//-------------------------------------------------------------------------------------------------

}
