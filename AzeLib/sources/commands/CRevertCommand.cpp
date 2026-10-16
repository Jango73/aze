
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CRevertCommand.h"
#include "../CRepository.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CRevertCommand::CRevertCommand(CRepository* pRepository, bool bAllowFileDelete)
    : CBaseCommand(pRepository)
    , m_bAllowFileDelete(bAllowFileDelete)
    , m_pWorkingDirectory(pRepository->commitFunctions()->folderAsCommit(this))
    , m_lFileNames(m_pWorkingDirectory->files().keys())
{
}

//-------------------------------------------------------------------------------------------------

CRevertCommand::CRevertCommand(CRepository* pRepository, const QStringList& lFileNames, bool bAllowFileDelete)
    : CBaseCommand(pRepository)
    , m_bAllowFileDelete(bAllowFileDelete)
    , m_pWorkingDirectory(pRepository->commitFunctions()->folderAsCommit(this))
    , m_lFileNames(lFileNames)
{
}

//-------------------------------------------------------------------------------------------------

CRevertCommand::CRevertCommand(CRepository* pRepository, CCommit* pWorkingDirectory, bool bAllowFileDeleted)
    : CBaseCommand(pRepository)
    , m_bAllowFileDelete(bAllowFileDeleted)
    , m_pWorkingDirectory(pWorkingDirectory)
    , m_lFileNames(pWorkingDirectory->files().keys())
{
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CRevertCommand::execute()
{
    for (QString sFileName : m_lFileNames)
    {
        QString sRelativeFileName = m_pRepository->database()->relativeFileName(sFileName);

        CError::EErrorCode eCode = revertSingleFile(sRelativeFileName);

        if (eCode != CError::eNone)
            return eCode;
    }

    m_pRepository->invalidateWorkingFolderCache();

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CRevertCommand::revertSingleFile(QString sRelativeFileName)
{
    bool bInStage = false;

    // Check presence of stage
    if (m_pRepository->stagingCommit().isNull())
    {
        m_pRepository->tellError(CError::eNoStagingCommit);
        return CError::eNoStagingCommit;
    }

    bInStage = m_pRepository->stagingCommit()->files().keys().contains(sRelativeFileName);

    // Proceed only when there is a tip commit
    if (not m_pRepository->tipCommit().isNull())
    {
        // Get the id of this file in the tip commit
        QString sIdInTip = m_pRepository->tipCommit()->files()[sRelativeFileName];

        // If file exists in tip, copy it from DB if required
        if (not sIdInTip.isEmpty())
        {
            // Get the id of this file in the working directory
            QString sIdInWorkingDir = m_pWorkingDirectory->files()[sRelativeFileName];

            // Copy the commited file back to the working directory when required
            if (sIdInTip != sIdInWorkingDir)
            {
                QByteArray baData = m_pRepository->database()->getObject(sIdInTip);

                if (not baData.isEmpty())
                {
                    // Write file
                    CUtils::putBinaryFileContent(
                                m_pRepository->database()->composeLocalFileName(sRelativeFileName),
                                baData
                                );
                }
            }
        }
        else
        {
            // Delete the commited file from the working directory if not present in the stage
            if (not bInStage && m_bAllowFileDelete)
            {
                QFile tFile(m_pRepository->database()->composeLocalFileName(sRelativeFileName));
                tFile.remove();
            }
        }
    }

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

}
