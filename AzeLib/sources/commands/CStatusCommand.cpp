
// Qt
#include <QDebug>
#include <QDir>
#include <QHash>

// Application
#include "CStatusCommand.h"
#include "../CRepository.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CStatusCommand::CStatusCommand(CRepository* pRepository, const QStringList& lFileNames, QList<CFile>* pFileStatus, CEnums::EFileStatus* pGeneralStatus)
    : CBaseCommand(pRepository)
    , m_lFileNames(lFileNames)
    , m_pFileStatus(pFileStatus)
    , m_pGeneralStatus(pGeneralStatus)
{
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CStatusCommand::execute()
{
    QHash<QString, int> lProcessedFiles;

    if (IS_NULL(m_pFileStatus))
        return CError::eGeneric;

    if (IS_NULL(m_pGeneralStatus))
        return CError::eGeneric;

    // Sanity check on stage
    if (m_pRepository->stagingCommit().isNull())
    {
        m_pRepository->tellError(CError::eNoStagingCommit);
        return CError::eNoStagingCommit;
    }

    // Set default status
    (*m_pGeneralStatus) = CEnums::eClean;

    CCommit* pFromCommit = nullptr;

    // Get tip commit if it exists
    if (not m_pRepository->tipCommit().isNull())
    {
        pFromCommit = m_pRepository->tipCommit().get();
    }
    else
    {
        pFromCommit = new CCommit(this);
    }

    // Get the working directory
    CCommit* pWorkCommit = m_pRepository->commitFunctions()->folderAsCommit(this);

    QStringList lWorkFiles = pWorkCommit->files().keys();
    lWorkFiles.sort();

    // Iterate through working directory files
    // Check how each file differs from the tip commit
    for (QString sRelativeName : lWorkFiles)
    {
        QString sAbsoluteName = m_pRepository->database()->absoluteFileName(sRelativeName);
        QString sStartPathRelativeName = m_pRepository->database()->startRelativeFileName(sAbsoluteName);

        QString sIdInFrom = pFromCommit->files()[sRelativeName];
        QString sIdInWork = pWorkCommit->files()[sRelativeName];
        QString sIdInStage = m_pRepository->stagingCommit()->files()[sRelativeName];

        CFile file;
        file.setRelativeName(sStartPathRelativeName);

        if (not CUtils::idValid(sIdInFrom) && not CUtils::idValid(sIdInWork))
        {
            file.setStatus(CEnums::eLoose);
        }
        else if (not CUtils::idValid(sIdInFrom) && CUtils::idValid(sIdInWork))
        {
            if (CUtils::idValid(sIdInStage))
            {
                file.setStatus(CEnums::eAdded);

                (*m_pGeneralStatus) = CEnums::eModified;
            }
        }
        else if (CUtils::idValid(sIdInFrom) && not CUtils::idValid(sIdInWork))
        {
            file.setStatus(CEnums::eDeleted);

            (*m_pGeneralStatus) = CEnums::eModified;
        }
        else
        {
            if (sIdInWork != sIdInFrom)
            {
                file.setStatus(CEnums::eModified);

                (*m_pGeneralStatus) = CEnums::eModified;
            }
            else
            {
                file.setStatus(CEnums::eClean);
            }
        }

        lProcessedFiles[sRelativeName] = 0;
        (*m_pFileStatus) << file;
    }

    // Traverse tip commit files
    // Every file in the tip not existing in working directory is considered missing,
    // unless present in the staging commit, which means it is being deleted
    if (not m_pRepository->tipCommit().isNull())
    {
        for (QString sRelativeName : m_pRepository->tipCommit()->files().keys())
        {
            if (not lProcessedFiles.contains(sRelativeName))
            {
                QString sAbsoluteName = m_pRepository->database()->absoluteFileName(sRelativeName);
                QString sStartPathRelativeName = m_pRepository->database()->startRelativeFileName(sAbsoluteName);

                CFile file;
                file.setRelativeName(sStartPathRelativeName);
                file.setStatus(CEnums::eMissing);

                if( m_pRepository->stagingCommit()->files().contains(sRelativeName))
                {
                    file.setStatus(CEnums::eDeleted);
                }

                (*m_pGeneralStatus) = CEnums::eModified;

                lProcessedFiles[sRelativeName] = 0;
                (*m_pFileStatus) << file;
            }
        }
    }

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

}
