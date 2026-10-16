
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CCreateBranchCommand.h"
#include "../CRepository.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CCreateBranchCommand. \br\br
    \a pRepository is the repository holding the branches \br
    \a sName is the name of the branch to create \br
*/
CCreateBranchCommand::CCreateBranchCommand(CRepository* pRepository, const QString& sName)
    : CBaseCommand(pRepository)
    , m_sName(sName)
{
}

/************************************************************************/

/*!
    Creates the branch \a m_sName from the current branch tip. \br\br
    An existing branch is kept, the call reports eNone with an
    informational message. A new branch inherits root and tip commit
    identifiers from the current branch when present. \br
*/
CError::EErrorCode CCreateBranchCommand::execute()
{
    QString sBranchFileName = m_pRepository->database()->composeBranchFileName(m_sName);

    if (QFile(sBranchFileName).exists())
    {
        m_pRepository->tellInfo(CError::eBranchAlreadyExists, m_sName);
    }
    else
    {
        CBranch* pNewBranch = new CBranch(this);

        if (not m_pRepository->currentBranch().isNull())
        {
            pNewBranch->setRootCommitId(m_pRepository->currentBranch()->tipCommitId());
            pNewBranch->setTipCommitId(m_pRepository->currentBranch()->tipCommitId());
        }

        pNewBranch->toNode().save(sBranchFileName);
        delete pNewBranch;
    }

    return CError::eNone;
}

/************************************************************************/

} // namespace Aze
