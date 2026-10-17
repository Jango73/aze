
#pragma once

// Global
#include "../Aze_global.h"

// Qt
#include <QDateTime>
#include <QFileInfo>
#include <QSet>
#include <QStringList>

// Application
#include "CBaseCommand.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CPruneCommand : public CBaseCommand
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

    static constexpr qint64 s_iDefaultRetentionSeconds = 86400;

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------------------------------------------

    CPruneCommand(CRepository* pRepository, bool bDryRun = false, qint64 iRetentionSeconds = s_iDefaultRetentionSeconds, QStringList* pRemovedCommits = nullptr, QStringList* pRemovedObjects = nullptr);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    virtual CError::EErrorCode execute() override;

    //-------------------------------------------------------------------------------------------------
    // Protected control methods
    //-------------------------------------------------------------------------------------------------

protected:

    //! Collects branch tips, stage and stash references into reachable commit and object sets
    void collectReachable(QSet<QString>& sReachableCommits, QSet<QString>& sReachableObjects);

    //! Queues a commit identifier for the ancestor walk when not visited
    void queueCommit(const QString& sCommitId, QStringList& lQueue, QSet<QString>& sVisited);

    //! Deletes unreachable commit files older than the grace period, reports dry-run candidates
    CError::EErrorCode sweepCommits(const QSet<QString>& sReachableCommits, const QDateTime& tReferenceTime);

    //! Deletes unreachable object files older than the grace period, reports dry-run candidates
    CError::EErrorCode sweepObjects(const QSet<QString>& sReachableObjects, const QDateTime& tReferenceTime);

    //! Tells if a file timestamp satisfies the grace period defined by the retention
    bool isProtectedByGrace(const QFileInfo& tFileDetails, const QDateTime& tReferenceTime) const;

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    bool         m_bDryRun;
    qint64       m_iRetentionSeconds;
    QStringList* m_pRemovedCommits;
    QStringList* m_pRemovedObjects;
};

}
