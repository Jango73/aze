
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CRemoveCommand.h"
#include "../CRepository.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CRemoveCommand::CRemoveCommand(CRepository* pRepository, const QStringList& lFileNames)
    : CBaseCommand(pRepository)
    , m_lFileNames(lFileNames)
{
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CRemoveCommand::execute()
{
    for (QString sFileName : m_lFileNames)
    {
        QString sRelativeFileName = m_pRepository->database()->relativeFileName(sFileName);

        CError::EErrorCode eCode = removeSingleFile(sRelativeFileName);

        if (eCode != CError::eNone)
            return eCode;
    }

    m_pRepository->invalidateWorkingFolderCache();

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CRemoveCommand::removeSingleFile(QString sRelativeFileName)
{
    // Sanity check on stage
    if (not m_pRepository->stagingCommit().isNull())
    {
        // Check existence of given file
        if (CUtils::fileExists(m_pRepository->database()->rootPath(), sRelativeFileName))
        {
            // Get the absolute file name
            QString sAbsoluteFileName = m_pRepository->database()->absoluteFileName(sRelativeFileName);

            QFile file(sAbsoluteFileName);

            // If file removal OK, stage the file
            if (file.remove())
            {
                m_pRepository->stagingCommit()->addFile(m_pRepository->database(), sRelativeFileName, "");
                return CError::eNone;
            }
            else
            {
                m_pRepository->tellError(CError::eNoSuchFile, sRelativeFileName);
                return CError::eNoSuchFile;
            }
        }
        else
        {
            m_pRepository->tellError(CError::eNoSuchFile, sRelativeFileName);
            return CError::eNoSuchFile;
        }
    }

    return CError::eNoStagingCommit;
}

//-------------------------------------------------------------------------------------------------

}
