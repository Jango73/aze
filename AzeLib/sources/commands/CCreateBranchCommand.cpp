
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
        CBranch* pExistingBranch = m_pRepository->database()->getBranch(m_sName, this);

        CError::EErrorCode eExistsCode = CError::eBranchAlreadyExists;

        if (IS_NOT_NULL(pExistingBranch) && pExistingBranch->isTag())
            eExistsCode = CError::eTagAlreadyExists;

        delete pExistingBranch;

        m_pRepository->tellInfo(eExistsCode, m_sName);
    }
    else
    {
        CBranch* pNewBranch = createBranchObject();

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

/*!
    Creates the branch object. \br\br
    Subclasses materialize their own branch kind here. \br
*/
CBranch* CCreateBranchCommand::createBranchObject()
{
    return new CBranch(this);
}

/************************************************************************/

} // namespace Aze
