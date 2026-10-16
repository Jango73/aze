
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CCleanUpCommand.h"
#include "CRevertCommand.h"
#include "../CRepository.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CCleanUpCommand. \br\br
    \a pRepository is the repository to clean up \br
    \a lFileNames holds the expanded working folder files to revert \br
    \a bAllowFileDelete allows deletion of files missing from the tip commit \br
*/
CCleanUpCommand::CCleanUpCommand(CRepository* pRepository, const QStringList& lFileNames, bool bAllowFileDelete)
    : CBaseCommand(pRepository)
    , m_lFileNames(lFileNames)
    , m_bAllowFileDelete(bAllowFileDelete)
{
}

/************************************************************************/

/*!
    Clears the stage and reverts the working folder files. \br\br
    The stage is cleared in memory, the caller writes it. Files listed in
    \a m_lFileNames are reverted to the tip commit state. \br
*/
CError::EErrorCode CCleanUpCommand::execute()
{
    CError::EErrorCode eCode = m_pRepository->clearStage();

    if (eCode != CError::eNone)
        return eCode;

    return CRevertCommand(m_pRepository, m_lFileNames, m_bAllowFileDelete).execute();
}

/************************************************************************/

} // namespace Aze
