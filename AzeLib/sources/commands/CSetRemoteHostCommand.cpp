
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CSetRemoteHostCommand.h"
#include "../CRepository.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CSetRemoteHostCommand. \br\br
    \a pRepository is the repository holding the remote host name \br
    \a sName is the new remote host name \br
*/
CSetRemoteHostCommand::CSetRemoteHostCommand(CRepository* pRepository, const QString& sName)
    : CBaseCommand(pRepository)
    , m_sName(sName)
{
}

/************************************************************************/

/*!
    Sets the remote host name to \a m_sName. \br\br
    An empty name is refused. The caller writes the general information file. \br
*/
CError::EErrorCode CSetRemoteHostCommand::execute()
{
    if (m_sName.isEmpty())
    {
        m_pRepository->tellError(CError::eNoRemoteHostNameGiven);
        return CError::eNoRemoteHostNameGiven;
    }

    m_pRepository->setRemoteHostName(m_sName);

    return CError::eNone;
}

/************************************************************************/

} // namespace Aze
