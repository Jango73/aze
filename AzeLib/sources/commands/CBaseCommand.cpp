
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CBaseCommand.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CBaseCommand::CBaseCommand(CRepository* pRepository)
    : m_bCalledInternally(false)
    , m_pRepository(pRepository)
{
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CBaseCommand::execute()
{
    return CError::eNone;
}

}
