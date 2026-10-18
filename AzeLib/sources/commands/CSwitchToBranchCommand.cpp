
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CSwitchToBranchCommand.h"
#include "CSaveStashCommand.h"
#include "CPopStashCommand.h"
#include "../CRepository.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CSwitchToBranchCommand::CSwitchToBranchCommand(CRepository* pRepository, const QString& sBranchName, bool bAllowFileDelete)
    : CBaseCommand(pRepository)
    , m_bAllowFileDelete(bAllowFileDelete)
    , m_sBranchName(sBranchName)
{
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CSwitchToBranchCommand::execute()
{
    // Check presence of current branch
    if (m_pRepository->currentBranch().isNull())
    {
        m_pRepository->tellError(CError::eNoCurrentBranch);
        return CError::eNoCurrentBranch;
    }

    QString sFileName = m_pRepository->database()->composeBranchFileName(m_sBranchName);

    if (not QFile(sFileName).exists())
        return CError::eUnknownBranch;

    CBranch* pTargetBranch = m_pRepository->database()->getBranch(m_sBranchName, this);

    // Tags share the branch namespace but are never checked out
    if (IS_NOT_NULL(pTargetBranch) && pTargetBranch->isTag())
    {
        delete pTargetBranch;
        return CError::eUnknownBranch;
    }

    delete pTargetBranch;

    // Save stash : diffs between last commit and working directory
    CSaveStashCommand stash(m_pRepository, "");
    stash.setCalledInternally(true);
    CError::EErrorCode eStashCode = stash.execute();

    if (eStashCode != CError::eNone)
    {
        m_pRepository->tellError(CError::eStashSaveFailed);
        return CError::eStashSaveFailed;
    }

    // Make the branch switch
    m_pRepository->setCurrentBranchName(m_sBranchName);
    m_pRepository->refreshCurrentBranch();

    // Get the working directory as commit
    CCommit* pToCommit = m_pRepository->commitFunctions()->folderAsCommit(this);

    // Revert local files to their last committed state
    if (IS_NOT_NULL(pToCommit))
    {
        m_pRepository->revert(pToCommit, m_bAllowFileDelete);
    }

    CError::EErrorCode ePopCode = CPopStashCommand(m_pRepository, "").execute();

    if (ePopCode != CError::eNone)
    {
        m_pRepository->tellError(CError::eStashPopFailed);
        return CError::eStashPopFailed;
    }

    m_pRepository->invalidateCaches();

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

}
