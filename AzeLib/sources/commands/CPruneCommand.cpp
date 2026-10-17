
// Qt
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QVector>

// Application
#include "CPruneCommand.h"
#include "../CRepository.h"
#include "../CStrings.h"
#include "../objects/CBranch.h"
#include "../objects/CStash.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CPruneCommand. \br\br
    \a pRepository is the repository holding the commits and objects \br
    \a bDryRun lists unreachable files without deleting them \br
    \a iRetentionSeconds is the grace period in seconds protecting files by modification time \br
    \a pRemovedCommits receives deleted or dry-run commit identifiers when not null \br
    \a pRemovedObjects receives deleted or dry-run object identifiers when not null \br
*/
CPruneCommand::CPruneCommand(CRepository* pRepository, bool bDryRun, qint64 iRetentionSeconds, QStringList* pRemovedCommits, QStringList* pRemovedObjects)
    : CBaseCommand(pRepository)
    , m_bDryRun(bDryRun)
    , m_iRetentionSeconds(iRetentionSeconds)
    , m_pRemovedCommits(pRemovedCommits)
    , m_pRemovedObjects(pRemovedObjects)
{
}

/************************************************************************/

/*!
    Deletes unreachable commit and object files by mark-and-sweep. \br\br
    Reachable commits are collected from every branch tip through all parents,
    while reachable objects are collected from reachable commits plus the stage
    and the stored stashes. Unreachable files younger than the retention are
    kept as protection against payloads stored by worker threads serving push
    requests concurrent with the prune. Dry-run mode reports candidates without
    deletion. \br
    Returns eCouldNotPrune when a deletion fails, eNone otherwise. \br
*/
CError::EErrorCode CPruneCommand::execute()
{
    if (IS_NULL(m_pRepository) || IS_NULL(m_pRepository->database()))
    {
        return CError::eCouldNotPrune;
    }

    if (m_iRetentionSeconds < 0)
        m_iRetentionSeconds = 0;

    if (IS_NOT_NULL(m_pRemovedCommits))
        m_pRemovedCommits->clear();

    if (IS_NOT_NULL(m_pRemovedObjects))
        m_pRemovedObjects->clear();

    QSet<QString> sReachableCommits;
    QSet<QString> sReachableObjects;

    collectReachable(sReachableCommits, sReachableObjects);

    QDateTime tReferenceTime = QDateTime::currentDateTimeUtc();

    CError::EErrorCode eCommitCode = sweepCommits(sReachableCommits, tReferenceTime);
    CError::EErrorCode eObjectCode = sweepObjects(sReachableObjects, tReferenceTime);

    if (eCommitCode != CError::eNone)
        return eCommitCode;

    return eObjectCode;
}

/************************************************************************/

/*!
    Collects reachable commit and object identifiers. \br\br
    Branch tips and branch roots seed the ancestor walk, the stage commit and
    every stored stash contribute referenced objects. Each visited commit is
    loaded once to collect file object identifiers and parent identifiers. \br
    \a sReachableCommits receives identifiers of commits reachable from branches \br
    \a sReachableObjects receives identifiers of objects referenced by reachable commits, stage and stashes \br
*/
void CPruneCommand::collectReachable(QSet<QString>& sReachableCommits, QSet<QString>& sReachableObjects)
{
    CDatabase* pDatabase = m_pRepository->database();

    QSet<QString> sVisited;
    QStringList lQueue;

    QVector<CBranch*> lBranchList = pDatabase->getAllBranches(this);

    for (CBranch* pBranch : lBranchList)
    {
        if (IS_NOT_NULL(pBranch))
        {
            queueCommit(pBranch->tipCommitId(), lQueue, sVisited);
            queueCommit(pBranch->rootCommitId(), lQueue, sVisited);
            delete pBranch;
        }
    }

    if (not m_pRepository->stagingCommit().isNull())
    {
        QDictionary mStagedFiles = m_pRepository->stagingCommit()->files();

        for (const QString& sObjectId : mStagedFiles.values())
        {
            if (not sObjectId.isEmpty())
                sReachableObjects << sObjectId;
        }
    }

    for (const QString& sStashId : m_pRepository->stashList())
    {
        CStash* pStash = pDatabase->getStash(sStashId, this);

        if (IS_NOT_NULL(pStash))
        {
            if (IS_NOT_NULL(pStash->stage()))
            {
                for (const QString& sObjectId : pStash->stage()->files().values())
                {
                    if (not sObjectId.isEmpty())
                        sReachableObjects << sObjectId;
                }
            }

            delete pStash;
        }
    }

    while (not lQueue.isEmpty())
    {
        QString sCommitId = lQueue.takeFirst();

        CCommit* pCommit = m_pRepository->commitFunctions()->loadCommit(sCommitId, this);

        if (IS_NULL(pCommit))
            continue;

        sReachableCommits << sCommitId;

        for (const QString& sObjectId : pCommit->files().values())
        {
            if (not sObjectId.isEmpty())
                sReachableObjects << sObjectId;
        }

        for (const QString& sParentId : pCommit->parents())
        {
            queueCommit(sParentId, lQueue, sVisited);
        }

        delete pCommit;
    }
}

/************************************************************************/

/*!
    Queues \a sCommitId for the ancestor walk when not empty and not visited. \br\br
    \a lQueue receives the identifier to visit \br
    \a sVisited holds identifiers already queued or visited \br
*/
void CPruneCommand::queueCommit(const QString& sCommitId, QStringList& lQueue, QSet<QString>& sVisited)
{
    if (sCommitId.isEmpty())
        return;

    if (sVisited.contains(sCommitId))
        return;

    sVisited << sCommitId;
    lQueue << sCommitId;
}

/************************************************************************/

/*!
    Deletes unreachable commit files older than the grace period. \br\br
    The commit folder listing is snapshotted before deletion so files stored
    by worker threads during the sweep are left for a later prune through the
    modification time check. The stage file is never a deletion candidate. \br
    \a sReachableCommits holds identifiers excluded from deletion \br
    \a tReferenceTime is the grace period reference for modification time comparison \br
    Returns eCouldNotPrune when a deletion fails, eNone otherwise. \br
*/
CError::EErrorCode CPruneCommand::sweepCommits(const QSet<QString>& sReachableCommits, const QDateTime& tReferenceTime)
{
    CDatabase* pDatabase = m_pRepository->database();

    QStringList lFilters;
    lFilters << QStringLiteral("*.xmlc") << QStringLiteral("*.xml");

    QDir tCommitFolder(pDatabase->commitPath());
    QFileInfoList lFileDetailsList = tCommitFolder.entryInfoList(lFilters, QDir::Files | QDir::NoSymLinks, QDir::Name);

    bool bDeletionFailed = false;

    for (const QFileInfo& tFileDetails : lFileDetailsList)
    {
        QString sCommitId = tFileDetails.baseName();

        if (sCommitId == CStrings::s_sStagingCommitFileName)
            continue;

        if (sReachableCommits.contains(sCommitId))
            continue;

        if (isProtectedByGrace(tFileDetails, tReferenceTime))
            continue;

        if (m_bDryRun)
        {
            if (IS_NOT_NULL(m_pRemovedCommits))
                (*m_pRemovedCommits) << sCommitId;

            continue;
        }

        QFile tCommitFile(tFileDetails.absoluteFilePath());

        if (not tCommitFile.exists())
            continue;

        if (tCommitFile.remove())
        {
            if (IS_NOT_NULL(m_pRemovedCommits))
                (*m_pRemovedCommits) << sCommitId;

            m_pRepository->commitFunctions()->commitTreeList()->removeCommit(sCommitId);
        }
        else if (QFile::exists(tFileDetails.absoluteFilePath()))
        {
            bDeletionFailed = true;
            m_pRepository->tellError(CError::eCouldNotPrune, sCommitId);
        }
    }

    if (bDeletionFailed)
        return CError::eCouldNotPrune;

    return CError::eNone;
}

/************************************************************************/

/*!
    Deletes unreachable object files older than the grace period. \br\br
    The object folder listing is snapshotted before deletion so files stored
    by worker threads during the sweep are left for a later prune through the
    modification time check. \br
    \a sReachableObjects holds identifiers excluded from deletion \br
    \a tReferenceTime is the grace period reference for modification time comparison \br
    Returns eCouldNotPrune when a deletion fails, eNone otherwise. \br
*/
CError::EErrorCode CPruneCommand::sweepObjects(const QSet<QString>& sReachableObjects, const QDateTime& tReferenceTime)
{
    CDatabase* pDatabase = m_pRepository->database();

    QDir tObjectFolder(pDatabase->objectPath());
    QFileInfoList lFileDetailsList = tObjectFolder.entryInfoList(QDir::Files | QDir::NoSymLinks, QDir::Name);

    bool bDeletionFailed = false;

    for (const QFileInfo& tFileDetails : lFileDetailsList)
    {
        QString sObjectId = tFileDetails.fileName();

        if (sReachableObjects.contains(sObjectId))
            continue;

        if (isProtectedByGrace(tFileDetails, tReferenceTime))
            continue;

        if (m_bDryRun)
        {
            if (IS_NOT_NULL(m_pRemovedObjects))
                (*m_pRemovedObjects) << sObjectId;

            continue;
        }

        QFile tObjectFile(tFileDetails.absoluteFilePath());

        if (not tObjectFile.exists())
            continue;

        if (tObjectFile.remove())
        {
            if (IS_NOT_NULL(m_pRemovedObjects))
                (*m_pRemovedObjects) << sObjectId;
        }
        else if (QFile::exists(tFileDetails.absoluteFilePath()))
        {
            bDeletionFailed = true;
            m_pRepository->tellError(CError::eCouldNotPrune, sObjectId);
        }
    }

    if (bDeletionFailed)
        return CError::eCouldNotPrune;

    return CError::eNone;
}

/************************************************************************/

/*!
    Tells if \a tFileDetails is protected by the grace period. \br\br
    A file with an age below the retention is protected, which covers payloads
    stored by worker threads concurrent with the prune. A zero retention
    disables the protection. A timestamp ahead of the reference is treated as
    zero age and remains protected. \br
    \a tReferenceTime is the grace period reference for modification time comparison \br
*/
bool CPruneCommand::isProtectedByGrace(const QFileInfo& tFileDetails, const QDateTime& tReferenceTime) const
{
    if (m_iRetentionSeconds <= 0)
        return false;

    qint64 iAgeSeconds = tFileDetails.lastModified().secsTo(tReferenceTime);

    if (iAgeSeconds < 0)
        iAgeSeconds = 0;

    return iAgeSeconds < m_iRetentionSeconds;
}

/************************************************************************/

} // namespace Aze
