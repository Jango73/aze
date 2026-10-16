
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CCommitCommand.h"
#include "../CRepository.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CCommitCommand::CCommitCommand(CRepository* pRepository, const QString& sAuthor, const QString& sMessage)
    : CBaseCommand(pRepository)
    , m_sAuthor(sAuthor)
    , m_sMessage(sMessage)
{
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CCommitCommand::execute()
{
    // Check presence of current branch
    if (m_pRepository->currentBranch().isNull())
    {
        m_pRepository->tellError(CError::eNoCurrentBranch);
        return CError::eNoCurrentBranch;
    }

    // Check presence and relevancy of staging commmit
    if (m_pRepository->stagingCommit().isNull() || m_pRepository->stagingCommit()->files().count() == 0)
    {
        m_pRepository->tellError(CError::eCommitEmpty);
        return CError::eCommitEmpty;
    }

    // Create a new commit object
    CCommit* pNewCommit = nullptr;

    // Read the tip commit of the current branch
    if (not m_pRepository->tipCommit().isNull())
    {
        pNewCommit = m_pRepository->tipCommit()->clone(this);
        pNewCommit->clearParents();
        pNewCommit->addParent(m_pRepository->tipCommit()->id());
    }
    else
    {
        pNewCommit = new CCommit(this);
    }

    // Add the staged commmit to this new commit
    pNewCommit->addCommit(m_pRepository->database(), m_pRepository->stagingCommit().get());

    if (m_sMessage.isEmpty())
        m_sMessage = m_pRepository->stagingCommit()->message();

    // Finalize and save the commit
    // We need all the info to generate the commit's id
    pNewCommit->setIsMerge(m_pRepository->stagingCommit()->isMerge());
    pNewCommit->setDateToNow();
    pNewCommit->setAuthor(m_sAuthor);
    pNewCommit->setMessage(m_sMessage);

    QString sNewCommitId = m_pRepository->commitFunctions()->storeCommit(pNewCommit);

    // Update the current branch
    m_pRepository->currentBranch()->setTipCommitId(sNewCommitId);

    if (m_pRepository->currentBranch()->rootCommitId().isEmpty())
    {
        m_pRepository->currentBranch()->setRootCommitId(sNewCommitId);
    }

    m_pRepository->invalidateCaches();

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

}
