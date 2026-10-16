
// Qt
#include <QDebug>
#include <QDir>
#include <QFileInfo>

// Application
#include "CMoveCommand.h"
#include "CStageCommand.h"
#include "../CRepository.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CMoveCommand. \br\br
    \a pRepository is the repository holding the working folder \br
    \a lFileNames holds the source files followed by the target, all relative
    to the repository root or absolute \br
*/
CMoveCommand::CMoveCommand(CRepository* pRepository, const QStringList& lFileNames)
    : CBaseCommand(pRepository)
    , m_lFileNames(lFileNames)
{
}

/************************************************************************/

/*!
    Moves the listed files to the target. \br\br
    The last entry of \a m_lFileNames is the target, previous entries are
    the sources. A folder target keeps the source file names, a file target
    requires a single source. Moved files are staged, source deletions of
    tracked files are staged as well. \br
*/
CError::EErrorCode CMoveCommand::execute()
{
    if (m_lFileNames.count() < 2)
    {
        m_pRepository->tellError(CError::eNoFileNameGiven);
        return CError::eNoFileNameGiven;
    }

    if (m_pRepository->stagingCommit().isNull())
    {
        m_pRepository->tellError(CError::eNoStagingCommit);
        return CError::eNoStagingCommit;
    }

    QString sTargetName = m_lFileNames.last();
    QString sTargetRelativeName = m_pRepository->database()->relativeFileName(sTargetName);
    QString sTargetAbsoluteName = m_pRepository->database()->absoluteFileName(sTargetRelativeName);
    bool bTargetIsFolder = QFileInfo(sTargetAbsoluteName).isDir();

    if (not bTargetIsFolder && m_lFileNames.count() > 2)
    {
        m_pRepository->tellError(CError::eCouldNotMoveFiles, sTargetRelativeName);
        return CError::eCouldNotMoveFiles;
    }

    for (int iIndex = 0; iIndex < m_lFileNames.count() - 1; ++iIndex)
    {
        QString sSourceRelativeName = m_pRepository->database()->relativeFileName(m_lFileNames[iIndex]);
        QString sSingleTargetRelativeName = sTargetRelativeName;

        if (bTargetIsFolder)
            sSingleTargetRelativeName += QStringLiteral("/") + QFileInfo(sSourceRelativeName).fileName();

        CError::EErrorCode eCode = moveSingleFile(sSourceRelativeName, sSingleTargetRelativeName);

        if (eCode != CError::eNone)
            return eCode;
    }

    return CError::eNone;
}

/************************************************************************/

/*!
    Moves \a sSourceRelativeName to \a sTargetRelativeName. \br\br
    The file is moved on disk, the target is staged and a tracked source
    is staged as deleted. Missing sources are refused. \br
*/
CError::EErrorCode CMoveCommand::moveSingleFile(const QString& sSourceRelativeName, const QString& sTargetRelativeName)
{
    QString sSourceAbsoluteName = m_pRepository->database()->absoluteFileName(sSourceRelativeName);
    QString sTargetAbsoluteName = m_pRepository->database()->absoluteFileName(sTargetRelativeName);

    if (not QFile(sSourceAbsoluteName).exists())
    {
        m_pRepository->tellError(CError::eNoSuchFile, sSourceRelativeName);
        return CError::eNoSuchFile;
    }

    CUtils::ensureFilePathExists(sTargetAbsoluteName);

    if (not CUtils::moveFile(sSourceAbsoluteName, sTargetAbsoluteName))
    {
        m_pRepository->tellError(CError::eCouldNotMoveFiles, sSourceRelativeName);
        return CError::eCouldNotMoveFiles;
    }

    bool bSourceTracked = false;

    if (not m_pRepository->tipCommit().isNull())
        bSourceTracked = m_pRepository->tipCommit()->files().contains(sSourceRelativeName);

    if (bSourceTracked)
        m_pRepository->stagingCommit()->addFile(m_pRepository->database(), sSourceRelativeName, "");

    CError::EErrorCode eStageCode = CStageCommand(m_pRepository, QStringList() << sTargetRelativeName).execute();

    if (eStageCode != CError::eNone)
        return eStageCode;

    return CError::eNone;
}

/************************************************************************/

} // namespace Aze
