
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CUnstageCommand.h"
#include "../CRepository.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CUnstageCommand::CUnstageCommand(CRepository* pRepository, const QStringList& lFileNames)
    : CBaseCommand(pRepository)
    , m_lFileNames(lFileNames)
{
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CUnstageCommand::execute()
{
    for (QString sFileName : m_lFileNames)
    {
        QString sRelativeFileName = m_pRepository->database()->relativeFileName(sFileName);

        CError::EErrorCode eCode = unstageSingleFile(sRelativeFileName);

        if (eCode != CError::eNone)
            return eCode;
    }

    m_pRepository->invalidateStatusCache();

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CUnstageCommand::unstageSingleFile(QString sRelativeFileName)
{
    if (not m_pRepository->stagingCommit().isNull())
    {
        m_pRepository->stagingCommit()->removeFile(sRelativeFileName);
        return CError::eNone;
    }

    return CError::eNoStagingCommit;
}

//-------------------------------------------------------------------------------------------------

}
