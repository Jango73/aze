
// Qt
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSet>

// Application
#include "CDeleteBranchCommand.h"
#include "../CRepository.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CDeleteBranchCommand. \br\br
    \a pRepository is the repository holding the branches \br
    \a sName is the name of the branch to delete \br
    \a bForce deletes a branch not merged into the current branch \br
*/
CDeleteBranchCommand::CDeleteBranchCommand(CRepository* pRepository, const QString& sName, bool bForce)
    : CBaseCommand(pRepository)
    , m_sName(sName)
    , m_bForce(bForce)
{
}

/************************************************************************/

/*!
    Deletes the branch \a m_sName. \br\br
    Refuses the current branch to avoid leaving the repository without a branch. \br
    Refuses a branch not merged into the current branch to avoid losing commits,
    unless \a m_bForce is true. \br
    A branch is merged when its tip is the current tip or is reachable from it. \br
    Returns eCannotDeleteCurrentBranch, eCannotDeleteUnmergedBranch,
    eBranchDoesNotExist or eCouldNotDeleteBranch on failure. \br
*/
CError::EErrorCode CDeleteBranchCommand::execute()
{
    if (m_sName == m_pRepository->currentBranchName())
    {
        m_pRepository->tellError(CError::eCannotDeleteCurrentBranch, m_sName);
        return CError::eCannotDeleteCurrentBranch;
    }

    QString sBranchFileName = m_pRepository->database()->composeBranchFileName(m_sName);

    if (not QFile(sBranchFileName).exists())
    {
        m_pRepository->tellError(missingCode(), m_sName);
        return missingCode();
    }

    CBranch* pBranchToDelete = m_pRepository->database()->getBranch(m_sName, nullptr);

    if (IS_NULL(pBranchToDelete))
    {
        m_pRepository->tellError(CError::eCouldNotDeleteBranch, m_sName);
        return CError::eCouldNotDeleteBranch;
    }

    if (not acceptsType(pBranchToDelete))
    {
        m_pRepository->tellError(missingCode(), m_sName);
        delete pBranchToDelete;
        return missingCode();
    }

    QString sTargetTipId = pBranchToDelete->tipCommitId();
    delete pBranchToDelete;

    CError::EErrorCode eCheckCode = checkDeletable(sTargetTipId);

    if (eCheckCode != CError::eNone)
        return eCheckCode;

    if (not QFile::remove(sBranchFileName))
    {
        m_pRepository->tellError(CError::eCouldNotDeleteBranch, m_sName);
        return CError::eCouldNotDeleteBranch;
    }

    return CError::eNone;
}

/************************************************************************/

/*!
    Returns the missing object code. \br\br
    Subclasses report their own object kind here. \br
*/
CError::EErrorCode CDeleteBranchCommand::missingCode() const
{
    return CError::eBranchDoesNotExist;
}

/************************************************************************/

/*!
    Returns true for a deletable object kind. \br\br
    Branches refuse tags sharing the name namespace and conversely. \br
*/
bool CDeleteBranchCommand::acceptsType(const CBranch* pBranch) const
{
    return IS_NOT_NULL(pBranch) && not pBranch->isTag();
}

/************************************************************************/

/*!
    Checks that the branch tip \a sTargetTipId may be deleted. \br\br
    A branch without tip holds no commit and loses nothing. Otherwise the
    tip must equal the current tip or be reachable from it, unless
    \a m_bForce is true. \br
*/
CError::EErrorCode CDeleteBranchCommand::checkDeletable(const QString& sTargetTipId)
{
    // A branch without tip holds no commit, deletion loses nothing
    if (not m_bForce and not sTargetTipId.isEmpty())
    {
        QString sCurrentTipId;

        if (not m_pRepository->currentBranch().isNull())
            sCurrentTipId = m_pRepository->currentBranch()->tipCommitId();

        bool bMerged = (sTargetTipId == sCurrentTipId);

        if (not bMerged and not sCurrentTipId.isEmpty())
        {
            QSet<QString> sVisited;
            QStringList lQueue;

            sVisited << sCurrentTipId;
            lQueue << sCurrentTipId;

            while (not lQueue.isEmpty() and not bMerged)
            {
                QString sCommitId = lQueue.takeFirst();

                if (sCommitId == sTargetTipId)
                {
                    bMerged = true;
                    break;
                }

                for (const QString& sParentId : m_pRepository->commitFunctions()->commitTreeList()->commitParentIds(sCommitId))
                {
                    if (not sVisited.contains(sParentId))
                    {
                        sVisited << sParentId;
                        lQueue << sParentId;
                    }
                }
            }
        }

        if (not bMerged)
        {
            m_pRepository->tellError(CError::eCannotDeleteUnmergedBranch, m_sName);
            return CError::eCannotDeleteUnmergedBranch;
        }
    }

    return CError::eNone;
}

/************************************************************************/

} // namespace Aze
