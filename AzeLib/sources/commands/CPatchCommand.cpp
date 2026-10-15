
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CPatchCommand.h"
#include "../CRepository.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CPatchCommand::CPatchCommand(CRepository* pRepository, const QString& sFilename)
    : CBaseCommand(pRepository)
    , m_sFilename(sFilename)
{
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CPatchCommand::execute()
{
    if (m_sFilename.isEmpty())
        return CError::eNoFileNameGiven;

    QString sDiff = CUtils::getTextFileContent(m_sFilename);

    if (sDiff.isEmpty())
        return CError::eCouldNotPatch;

    // Apply the patchs
    return m_pRepository->commitFunctions()->applyDiff(sDiff);
}

//-------------------------------------------------------------------------------------------------

}
