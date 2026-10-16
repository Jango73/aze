
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CStageCommand.h"
#include "../CRepository.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CStageCommand::CStageCommand(CRepository* pRepository, const QStringList& lFileNames)
    : CBaseCommand(pRepository)
    , m_lFileNames(lFileNames)
{
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CStageCommand::execute()
{
    for (QString sFileName : m_lFileNames)
    {
        QString sRelativeFileName = m_pRepository->database()->relativeFileName(sFileName);

        CError::EErrorCode eCode = addSingleFile(sRelativeFileName);

        if (eCode != CError::eNone)
            return eCode;
    }

    m_pRepository->invalidateStatusCache();

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CStageCommand::addSingleFile(QString sRelativeFileName)
{
    // Sanity check on stage
    if (m_pRepository->stagingCommit().isNull())
    {
        m_pRepository->tellError(CError::eNoStagingCommit);
        return CError::eNoStagingCommit;
    }

    if (CUtils::fileExists(m_pRepository->database()->rootPath(), sRelativeFileName))
    {
        QString sId = CUtils::idFromString(sRelativeFileName);
        m_pRepository->stagingCommit()->addFile(m_pRepository->database(), sRelativeFileName, sId);
        return CError::eNone;
    }
    else
    {
        // Check if this is a file deletion
        if (not m_pRepository->tipCommit().isNull() && m_pRepository->tipCommit()->files().contains(sRelativeFileName))
        {
            m_pRepository->stagingCommit()->addFile(m_pRepository->database(), sRelativeFileName, "");
            return CError::eNone;
        }

        m_pRepository->tellError(CError::eNoSuchFile, sRelativeFileName);
        return CError::eNoSuchFile;
    }
}

//-------------------------------------------------------------------------------------------------

}
