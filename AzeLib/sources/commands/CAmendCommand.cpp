
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CAmendCommand.h"
#include "../CRepository.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CAmendCommand. \br\br
    \a pRepository is the repository to work on \br
    \a sAuthor is the new author, kept from tip commit when empty \br
    \a sMessage is the new message, kept from tip commit when empty \br
*/
CAmendCommand::CAmendCommand(CRepository* pRepository, const QString& sAuthor, const QString& sMessage)
    : CBaseCommand(pRepository)
    , m_sAuthor(sAuthor)
    , m_sMessage(sMessage)
{
}

//-------------------------------------------------------------------------------------------------

/*!
    Rewrites the tip commit without creation of a new parent. \br\br
    Staged files are folded into a copy of the tip commit which keeps
    its parent identifiers, then the current branch tip is moved to the
    rewritten commit. The root identifier follows when the tip was the root.
*/
CError::EErrorCode CAmendCommand::execute()
{
    // Check presence of current branch
    if (m_pRepository->currentBranch().isNull())
    {
        m_pRepository->tellError(CError::eNoCurrentBranch);
        return CError::eNoCurrentBranch;
    }

    // Check presence of a tip commit to rewrite
    if (m_pRepository->tipCommit().isNull())
    {
        m_pRepository->tellError(CError::eNoTipCommit);
        return CError::eNoTipCommit;
    }

    QString sOldTipId = m_pRepository->tipCommit()->id();
    QStringList lParents = m_pRepository->tipCommit()->parents();

    // Copy the tip commit, keeping its parents
    CCommit* pNewCommit = m_pRepository->tipCommit()->clone(this);

    // Fold staged files into the copy, if any
    if (not m_pRepository->stagingCommit().isNull() && m_pRepository->stagingCommit()->files().count() > 0)
    {
        pNewCommit->addCommit(m_pRepository->database(), m_pRepository->stagingCommit().get());
        pNewCommit->setParents(lParents);
    }

    // Keep tip author and message when no override is given
    if (m_sAuthor.isEmpty())
        m_sAuthor = m_pRepository->tipCommit()->author();

    if (m_sMessage.isEmpty())
        m_sMessage = m_pRepository->tipCommit()->message();

    // Finalize and save the rewritten commit
    // We need all the info to generate the commit's id
    pNewCommit->setDateToNow();
    pNewCommit->setAuthor(m_sAuthor);
    pNewCommit->setMessage(m_sMessage);

    QString sNewCommitId = m_pRepository->commitFunctions()->storeCommit(pNewCommit);

    // Move the current branch tip to the rewritten commit
    m_pRepository->currentBranch()->setTipCommitId(sNewCommitId);

    if (m_pRepository->currentBranch()->rootCommitId().isEmpty()
            || m_pRepository->currentBranch()->rootCommitId() == sOldTipId)
    {
        m_pRepository->currentBranch()->setRootCommitId(sNewCommitId);
    }

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

}
