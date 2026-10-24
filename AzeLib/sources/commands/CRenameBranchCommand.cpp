
// Qt
#include <QFile>

// Application
#include "CRenameBranchCommand.h"
#include "../CRepository.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CRenameBranchCommand. \br\br
    \a pRepository is the repository holding the branches \br
    \a sOldName is the branch to rename \br
    \a sNewName is the new branch name \br
*/
CRenameBranchCommand::CRenameBranchCommand(CRepository* pRepository, const QString& sOldName, const QString& sNewName)
    : CBaseCommand(pRepository)
    , m_sOldName(sOldName)
    , m_sNewName(sNewName)
{
}

/************************************************************************/

/*!
    Renames the branch \a m_sOldName to \a m_sNewName. \br\br
    Refuses an unknown source with eBranchDoesNotExist, tags are never
    renamed. Refuses an existing target with eBranchAlreadyExists for a
    branch and eTagAlreadyExists for a tag. Renaming the current branch
    moves the current branch pointer to the new name. \br
    Returns eCouldNotRenameBranch when the branch file cannot be moved. \br
*/
CError::EErrorCode CRenameBranchCommand::execute()
{
    if (m_sOldName.isEmpty() || m_sNewName.isEmpty())
    {
        m_pRepository->tellError(CError::eNoBranchNameGiven);
        return CError::eNoBranchNameGiven;
    }

    QString sOldFileName = m_pRepository->database()->composeBranchFileName(m_sOldName);
    QString sNewFileName = m_pRepository->database()->composeBranchFileName(m_sNewName);

    if (not QFile(sOldFileName).exists())
    {
        m_pRepository->tellError(CError::eBranchDoesNotExist, m_sOldName);
        return CError::eBranchDoesNotExist;
    }

    CBranch* pSourceBranch = m_pRepository->database()->getBranch(m_sOldName, nullptr);

    if (IS_NULL(pSourceBranch))
    {
        m_pRepository->tellError(CError::eCouldNotRenameBranch, m_sOldName);
        return CError::eCouldNotRenameBranch;
    }

    bool bSourceIsTag = pSourceBranch->isTag();
    delete pSourceBranch;

    // Tags share the branch namespace but are never renamed as branches
    if (bSourceIsTag)
    {
        m_pRepository->tellError(CError::eBranchDoesNotExist, m_sOldName);
        return CError::eBranchDoesNotExist;
    }

    if (QFile(sNewFileName).exists())
    {
        CBranch* pTargetBranch = m_pRepository->database()->getBranch(m_sNewName, nullptr);

        CError::EErrorCode eExistsCode = CError::eBranchAlreadyExists;

        if (IS_NOT_NULL(pTargetBranch) && pTargetBranch->isTag())
            eExistsCode = CError::eTagAlreadyExists;

        delete pTargetBranch;

        m_pRepository->tellError(eExistsCode, m_sNewName);
        return eExistsCode;
    }

    if (not QFile::rename(sOldFileName, sNewFileName))
    {
        m_pRepository->tellError(CError::eCouldNotRenameBranch, m_sOldName);
        return CError::eCouldNotRenameBranch;
    }

    if (m_sOldName == m_pRepository->currentBranchName())
    {
        m_pRepository->setCurrentBranchName(m_sNewName);
        m_pRepository->refreshCurrentBranch();
    }

    return CError::eNone;
}

/************************************************************************/

} // namespace Aze
