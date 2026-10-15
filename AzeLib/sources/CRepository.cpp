
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CRepository.h"
#include "CUtils.h"

#include "commands/CAmendCommand.h"
#include "commands/CCommitCommand.h"
#include "commands/CDiffCommand.h"
#include "commands/CLogCommand.h"
#include "commands/CMergeCommand.h"
#include "commands/CPatchCommand.h"
#include "commands/CPopStashCommand.h"
#include "commands/CRemoveCommand.h"
#include "commands/CRevertCommand.h"
#include "commands/CSaveStashCommand.h"
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
*/
CRepository::CRepository(const QString& sRootPath, QObject* parent, bool bSilent, bool bDebug)
    : CObject(parent)
    , m_bOk(false)
    , m_bSilent(bSilent)
    , m_bDebug(bDebug)
    , m_eStatus(CEnums::eUnknown)
    , m_pDatabase(new CDatabase(sRootPath, this))
    , m_pRemoteHostInfo(new CRemoteHostInfo(this))
    , m_pCommitFunctions(new CCommitFunctions(m_pDatabase, this, bSilent, bDebug))
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
bool CRepository::init()
{
    if (m_pDatabase->init())
    {
        createBranch(CStrings::s_sDefaultBranchName);
        setCurrentBranchName(CStrings::s_sDefaultBranchName);

        writeGeneralInformation();

        setOk(true);

        return true;
    }

    return false;
}

//-------------------------------------------------------------------------------------------------

/*!
    Creates a branch named \a sName.
*/
bool CRepository::createBranch(const QString& sName)
{
    QString sBranchFileName = m_pDatabase->composeBranchFileName(sName);

    if (QFile(sBranchFileName).exists())
    {
        tellInfo(CStrings::s_sTextBranchAlreadyExists);
    }
    else
    {
        CBranch* pNewBranch = new CBranch(this);

        if (not m_pCurrentBranch.isNull())
        {
            pNewBranch->setRootCommitId(m_pCurrentBranch->tipCommitId());
            pNewBranch->setTipCommitId(m_pCurrentBranch->tipCommitId());
        }

        pNewBranch->toNode().save(sBranchFileName);
    }

    return true;
}

//-------------------------------------------------------------------------------------------------

/*!
    Deletes, if it exists, the branch named \a sName.
*/
bool CRepository::deleteBranch(const QString& sName)
{
    QString sBranchFileName = m_pDatabase->composeBranchFileName(sName);

    if (not QFile(sBranchFileName).exists())
    {
        tellInfo(CStrings::s_sTextBranchDoesNotExist);
        return false;
    }

    QFile::remove(sBranchFileName);
    return true;
}

//-------------------------------------------------------------------------------------------------

/*!
    Switches to the branch named \a sName. \br\br
    \a bAllowFileDelete is passed to the underlying revert command
*/
bool CRepository::switchToBranch(const QString& sName, bool bAllowFileDelete)
{
    return CSwitchToBranchCommand(this, sName, bAllowFileDelete).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Adds files to the stage. \br\br
    \a lFileNames contains the file names to stage
*/
bool CRepository::stage(const QStringList& lFileNames)
{
    return CStageCommand(this, lFileNames).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Removes files from the stage. \br\br
    \a lFileNames contains the file names to unstage
*/
bool CRepository::unstage(const QStringList& lFileNames)
{
    return CUnstageCommand(this, lFileNames).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Adds a file deletion to the stage. \br\br
    \a lFileNames contains the file names to delete
*/
bool CRepository::remove(const QStringList& lFileNames)
{
    return CRemoveCommand(this, lFileNames).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Saves the state of the repository to a new commit. \br\br
    \a sAuthor tells who is committing
    \a sMessage tells what we are comitting
*/
bool CRepository::commit(const QString& sAuthor, const QString& sMessage)
{
    return CCommitCommand(this, sAuthor, sMessage).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Rewrites the tip commit without creation of a new parent. \br\br
    \a sAuthor replaces the tip author when not empty
    \a sMessage replaces the tip message when not empty
*/
bool CRepository::amend(const QString& sAuthor, const QString& sMessage)
{
    return CAmendCommand(this, sAuthor, sMessage).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Reverts files to the state they were in the last commit. \br\br
    \a lFileNames contains the file names to revert
    \a bAllowFileDelete allows to delete files that were not present in the last commit
*/
bool CRepository::revert(const QStringList& lFileNames, bool bAllowFileDelete)
{
    return CRevertCommand(this, lFileNames, bAllowFileDelete).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Reverts files to the state they were in the last commit. \br\br
    \a pWorkingDirectory contains the files to revert
    \a bAllowFileDelete allows to delete files that were not present in the last commit
*/
bool CRepository::revert(CCommit* pWorkingDirectory, bool bAllowFileDelete)
{
    return CRevertCommand(this, pWorkingDirectory, bAllowFileDelete).execute();
}

//-------------------------------------------------------------------------------------------------

/*!
    Reverts files to the state they were in the last commit. \br\br
    \a bAllowFileDelete allows to delete files that were not present in the last commit
*/
bool CRepository::revert(bool bAllowFileDelete)
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

    if (CLogCommand(this, lFileNames, &sReturnValue, bGraph, iStart, iCount).execute())
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

    if (CDiffCommand(this, sFirst, sSecond, &sReturnValue).execute())
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
    \a pMergeError optionally receives a CMergeCommand::EMergeError reason on failure
*/
bool CRepository::merge(const QString& sName, bool& bHasConflicts, int* pMergeError)
{
    CMergeCommand oCommand(this, sName, bHasConflicts);
    bool bOk = oCommand.execute();

    if (not bOk && IS_NOT_NULL(pMergeError))
        *pMergeError = oCommand.mergeError();

    if (bOk && IS_NOT_NULL(pMergeError))
        *pMergeError = CMergeCommand::eMergeOk;

    return bOk;
}

//-------------------------------------------------------------------------------------------------

bool CRepository::saveStash(const QString& sMessage)
{
    return CSaveStashCommand(this, sMessage).execute();
}

//-------------------------------------------------------------------------------------------------

bool CRepository::popStash(const QString& sId)
{
    return CPopStashCommand(this, sId).execute();
}

//-------------------------------------------------------------------------------------------------

bool CRepository::patch(const QString& sFilename)
{
    return CPatchCommand(this, sFilename).execute();
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
bool CRepository::readGeneralInformation()
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

    return true;
}

//-------------------------------------------------------------------------------------------------

/*!
    Writes the general information file.
*/
bool CRepository::writeGeneralInformation()
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

    return true;
}

//-------------------------------------------------------------------------------------------------

void CRepository::refreshCurrentBranch()
{
    m_pCurrentBranch.clear();
    m_pCurrentBranch.get();
}

//-------------------------------------------------------------------------------------------------

bool CRepository::writeCurrentBranch()
{
    if (not m_pCurrentBranch.isNull())
        m_pCurrentBranch->toNode().save(m_pDatabase->composeBranchFileName(m_sCurrentBranchName));

    return true;
}

//-------------------------------------------------------------------------------------------------

bool CRepository::writeStage()
{
    if (not m_pStagingCommit.isNull())
    {
        m_pStagingCommit->toFile(m_sStagingCommitFileName);
        return true;
    }

    return false;
}

//-------------------------------------------------------------------------------------------------

bool CRepository::clearStage()
{
    if (not m_pStagingCommit.isNull())
    {
        m_pStagingCommit.set(new CCommit(this));

        return true;
    }

    return false;
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

void CRepository::tellError(const QString& sText)
{
    if (!m_bSilent)
    {
        OUT_ERROR(sText);
    }
}

//-------------------------------------------------------------------------------------------------

void CRepository::tellInfo(const QString& sText)
{
    if (!m_bSilent)
    {
        OUT_INFO(sText);
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
        pFromCommit = m_pDatabase->getCommit(m_pCurrentBranch->tipCommitId(), this);

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
*/
CBranch* CRepository::readCurrentBranch(void* pContext)
{
    CRepository* pRepository = static_cast<CRepository*>(pContext);

    pRepository->m_pStagingCommit.clear();
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
        return pRepository->m_pDatabase->getCommit(pRepository->m_pCurrentBranch->rootCommitId(), nullptr);

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
        return pRepository->m_pDatabase->getCommit(pRepository->m_pCurrentBranch->tipCommitId(), nullptr);

    return nullptr;
}

//-------------------------------------------------------------------------------------------------

}
