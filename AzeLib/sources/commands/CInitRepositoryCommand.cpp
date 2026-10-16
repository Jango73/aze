
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CInitRepositoryCommand.h"
#include "CCreateBranchCommand.h"
#include "../CRepository.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CInitRepositoryCommand. \br\br
    \a pRepository is the repository to initialize \br
*/
CInitRepositoryCommand::CInitRepositoryCommand(CRepository* pRepository)
    : CBaseCommand(pRepository)
{
}

/************************************************************************/

/*!
    Initializes the repository. \br\br
    Creates the database structure, the default branch and the general
    information file, then marks the repository as valid. \br
*/
CError::EErrorCode CInitRepositoryCommand::execute()
{
    if (not m_pRepository->database()->init())
        return CError::eGeneric;

    CError::EErrorCode eCode = CCreateBranchCommand(m_pRepository, CStrings::s_sDefaultBranchName).execute();

    if (eCode != CError::eNone)
        return eCode;

    m_pRepository->setCurrentBranchName(CStrings::s_sDefaultBranchName);

    eCode = m_pRepository->writeGeneralInformation();

    if (eCode != CError::eNone)
        return eCode;

    m_pRepository->setOk(true);

    return CError::eNone;
}

/************************************************************************/

} // namespace Aze
