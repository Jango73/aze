
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CMergeCommand.h"
#include "../CRepository.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CMergeCommand::CMergeCommand(CRepository* pRepository, const QString& sFromBranch, bool& bHasConflicts)
    : CBaseCommand(pRepository)
    , m_sFromBranch(sFromBranch)
    , m_bHasConflicts(bHasConflicts)
{
    m_bHasConflicts = false;
}

//-------------------------------------------------------------------------------------------------

/*!
    Merges the branch \a m_sFromBranch onto the current branch. \br\br
    Refuses a non empty stage to avoid overwriting merge parents. \br
    Returns the precise failure reason as a CError::EErrorCode. \br
*/
CError::EErrorCode CMergeCommand::execute()
{
    m_bHasConflicts = false;

    // Check validity of arguments
    if (m_sFromBranch.isEmpty())
    {
        m_pRepository->tellError(CError::eNoBranchNameGiven);
        return CError::eNoBranchNameGiven;
    }

    // Check presence of current branch
    if (m_pRepository->currentBranch().isNull())
    {
        m_pRepository->tellError(CError::eNoCurrentBranch);
        return CError::eNoCurrentBranch;
    }

    // Check presence of stage
    if (m_pRepository->stagingCommit().isNull())
    {
        m_pRepository->tellError(CError::eNoStagingCommit);
        return CError::eNoStagingCommit;
    }

    // Refuse a non empty stage, merge parents would overwrite staged files
    if (m_pRepository->stagingCommit()->files().count() > 0)
    {
        m_pRepository->tellError(CError::eStageNotEmpty);
        return CError::eStageNotEmpty;
    }

    // Check presence of current branch tip commit
    if (m_pRepository->tipCommit().isNull())
    {
        m_pRepository->tellError(CError::eNoTipCommit);
        return CError::eNoTipCommit;
    }

    CCommit* pToTipCommit = m_pRepository->tipCommit().get();
    CBranch* pFromBranch = m_pRepository->database()->getBranch(m_sFromBranch, this);

    // Check presence of 'from' branch tip commit
    if (IS_NULL(pFromBranch) || pFromBranch->tipCommitId().isEmpty())
    {
        m_pRepository->tellError(CError::eUnknownBranch, m_sFromBranch);
        return CError::eUnknownBranch;
    }

    // Check if trying to merge a commit on itself (yes it may happen...)
    if (pFromBranch->tipCommitId() == m_pRepository->tipCommit()->id())
    {
        m_pRepository->tellError(CError::eCannotMergeSelf);
        return CError::eCannotMergeSelf;
    }

    CCommit* pFromTipCommit = m_pRepository->commitFunctions()->loadCommit(pFromBranch->tipCommitId(), this);

    // Check validity of 'from' branch tip commit
    if (IS_NULL(pFromTipCommit))
    {
        m_pRepository->tellError(CError::eUnknownBranch, m_sFromBranch);
        return CError::eUnknownBranch;
    }

    // Get the common commit chains of the two commits
    // Lists are from root to tip
    QString sCommonAncestorId = m_pRepository->commitFunctions()->getCommonCommitChains(
                pToTipCommit->id(),
                pFromTipCommit->id(),
                nullptr,
                nullptr
                );

    // Check presence of common ancestor
    if (sCommonAncestorId.isEmpty())
    {
        m_pRepository->tellError(CError::eNoCommonAncestor);
        return CError::eNoCommonAncestor;
    }

    // Check relevancy of 'from' commit
    if (sCommonAncestorId == pFromTipCommit->id())
    {
        m_pRepository->tellError(CError::eNothingToMerge);
        return CError::eNothingToMerge;
    }

    // Check status of working directory
    m_pRepository->checkStatus(QStringList());

    if (m_pRepository->status() != CEnums::eClean)
    {
        m_pRepository->tellError(CError::eDirtyWorkingFolder);
        return CError::eDirtyWorkingFolder;
    }

    CCommit* pCommonAncestor = m_pRepository->commitFunctions()->loadCommit(sCommonAncestorId, this);
    if (IS_NULL(pCommonAncestor))
    {
        m_pRepository->tellError(CError::eNoCommonAncestor);
        return CError::eNoCommonAncestor;
    }

    // Do the actual 3-way merge
    CError::EErrorCode eMergeCode = m_pRepository->commitFunctions()->threeWayMerge(pCommonAncestor, pFromTipCommit, pToTipCommit, true, m_pRepository->stagingCommit().get());

    if (eMergeCode != CError::eNone && eMergeCode != CError::eMergeHasConflicts)
        return eMergeCode;

    m_bHasConflicts = (eMergeCode == CError::eMergeHasConflicts);

    // Set merge information in staging commit
    // Only the 'from' commit is added as a parent here, the other will be added by commit command
    m_pRepository->stagingCommit()->setIsMerge(true);
    m_pRepository->stagingCommit()->clearParents();
    m_pRepository->stagingCommit()->addParent(pFromTipCommit->id());
    m_pRepository->stagingCommit()->setMessage(
                QString(tr("Merge branch %1 into %2"))
                .arg(m_sFromBranch)
                .arg(m_pRepository->currentBranchName())
                );

    return eMergeCode;
}

//-------------------------------------------------------------------------------------------------

}
