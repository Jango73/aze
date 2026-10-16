
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CDumpCommand.h"
#include "../CRepository.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CDumpCommand. \br\br
    \a pRepository is the repository holding the database \br
    \a lIds holds the object identifiers to dump \br
    \a pResult receives the concatenated printable contents \br
*/
CDumpCommand::CDumpCommand(CRepository* pRepository, const QStringList& lIds, QString* pResult)
    : CBaseCommand(pRepository)
    , m_lIds(lIds)
    , m_pResult(pResult)
{
}

/************************************************************************/

/*!
    Dumps the database objects listed in \a m_lIds. \br\br
    Printable contents are appended to \a m_pResult when present. \br
*/
CError::EErrorCode CDumpCommand::execute()
{
    if (IS_NULL(m_pResult))
        return CError::eGeneric;

    for (const QString& sId : m_lIds)
        (*m_pResult) += m_pRepository->database()->printableFileContentById(sId);

    return CError::eNone;
}

/************************************************************************/

} // namespace Aze
