
// Qt
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

// Application
#include "CMoveCommand.h"
#include "../CRepository.h"
#include "../CStrings.h"
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
    requires a single source. Moved files keep their content identifier so
    the target reuses the source object after commit, source deletions of
    tracked files are staged as well. Existing targets and self moves are
    refused so no tracked history is overwritten. \br
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

    m_pRepository->invalidateWorkingFolderCache();

    return CError::eNone;
}

/************************************************************************/

/*!
    Moves \a sSourceRelativeName to \a sTargetRelativeName. \br\br
    The file is moved on disk, the target is staged with the moved content
    identifier so the commit reuses the source object, and the source is
    staged as deleted when tracked in the tip commit or dropped from the
    stage when staged only. Missing sources, folders, repository internals,
    self moves and existing targets are refused. \br
*/
CError::EErrorCode CMoveCommand::moveSingleFile(const QString& sSourceRelativeName, const QString& sTargetRelativeName)
{
    if (sSourceRelativeName.isEmpty() || sTargetRelativeName.isEmpty())
    {
        m_pRepository->tellError(CError::eNoFileNameGiven);
        return CError::eNoFileNameGiven;
    }

    if (sSourceRelativeName == sTargetRelativeName)
    {
        m_pRepository->tellError(CError::eCouldNotMoveFiles, sSourceRelativeName);
        return CError::eCouldNotMoveFiles;
    }

    if (isRepositoryInternalPath(sSourceRelativeName) || isRepositoryInternalPath(sTargetRelativeName))
    {
        m_pRepository->tellError(CError::eCouldNotMoveFiles, sTargetRelativeName);
        return CError::eCouldNotMoveFiles;
    }

    QString sSourceAbsoluteName = m_pRepository->database()->absoluteFileName(sSourceRelativeName);
    QString sTargetAbsoluteName = m_pRepository->database()->absoluteFileName(sTargetRelativeName);

    if (QFileInfo(sSourceAbsoluteName).isDir())
    {
        m_pRepository->tellError(CError::eCouldNotMoveFiles, sSourceRelativeName);
        return CError::eCouldNotMoveFiles;
    }

    if (not QFile(sSourceAbsoluteName).exists())
    {
        m_pRepository->tellError(CError::eNoSuchFile, sSourceRelativeName);
        return CError::eNoSuchFile;
    }

    if (QFileInfo(sTargetAbsoluteName).exists())
    {
        m_pRepository->tellError(CError::eCouldNotMoveFiles, sTargetRelativeName);
        return CError::eCouldNotMoveFiles;
    }

    CUtils::ensureFilePathExists(sTargetAbsoluteName);

    if (not CUtils::moveFile(sSourceAbsoluteName, sTargetAbsoluteName))
    {
        m_pRepository->tellError(CError::eCouldNotMoveFiles, sSourceRelativeName);
        return CError::eCouldNotMoveFiles;
    }

    QString sContentId = CUtils::idFromFileContent(sTargetAbsoluteName);

    if (sContentId.isEmpty())
    {
        m_pRepository->tellError(CError::eCouldNotMoveFiles, sTargetRelativeName);
        return CError::eCouldNotMoveFiles;
    }

    bool bSourceInTip = false;
    bool bSourceInStage = false;

    if (not m_pRepository->tipCommit().isNull())
        bSourceInTip = m_pRepository->tipCommit()->files().contains(sSourceRelativeName);

    if (not m_pRepository->stagingCommit().isNull())
        bSourceInStage = m_pRepository->stagingCommit()->files().contains(sSourceRelativeName);

    if (bSourceInTip)
        m_pRepository->stagingCommit()->addFile(m_pRepository->database(), sSourceRelativeName, "", true);
    else if (bSourceInStage)
        m_pRepository->stagingCommit()->removeFile(sSourceRelativeName);

    // Reuse the moved content identifier so the target keeps the source history
    m_pRepository->stagingCommit()->addFile(m_pRepository->database(), sTargetRelativeName, sContentId, true);

    return CError::eNone;
}

/************************************************************************/

/*!
    Returns true when \a sRelativeName designates repository internals. \br\br
    Moves into or out of the `.aze` folder are refused so history and
    objects are never relocated through the working folder. \br
*/
bool CMoveCommand::isRepositoryInternalPath(const QString& sRelativeName) const
{
    return sRelativeName == CStrings::s_sPathAzeDataRootPath
        || sRelativeName.startsWith(CStrings::s_sPathAzeDataRootPath + QStringLiteral("/"));
}

/************************************************************************/

} // namespace Aze
