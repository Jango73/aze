
// Qt
#include <QFile>

// qt-plus
#include "CXMLNode.h"

// Application
#include "CCommitTreeList.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CCommitTreeList::CCommitTreeList(CDatabase* pDatabase, QObject* parent, bool bSilent, bool bDebug, bool bTextCache)
    : QObject(parent)
    , m_pDatabase(pDatabase)
    , m_bSilent(bSilent)
    , m_bDebug(bDebug)
    , m_pShardStore(new CShardFileStore(m_pDatabase->cachePath(), "Aze::CShardFileStore", this, 2000, not bTextCache))
{
}

//-------------------------------------------------------------------------------------------------

CCommitTreeList::~CCommitTreeList()
{
    m_pShardStore->save();
}

//-------------------------------------------------------------------------------------------------

/*!
    Tells if parent identifiers are cached for a commit. \br\br
    \a sCommitId is the commit identifier to look up \br
*/
bool CCommitTreeList::containsCommit(const QString& sCommitId) const
{
    return m_pShardStore->containsValue(sCommitId);
}

//-------------------------------------------------------------------------------------------------

/*!
    Caches the parent identifiers of a commit without loading any file. \br\br
    \a sCommitId is the commit identifier to register \br
    \a lParentIds are the parent identifiers to cache \br
*/
void CCommitTreeList::registerCommitParents(const QString& sCommitId, const QStringList& lParentIds)
{
    if (sCommitId.isEmpty())
        return;

    m_pShardStore->setValue(sCommitId, lParentIds);
}

//-------------------------------------------------------------------------------------------------

/*!
    Forgets the parent identifiers of a pruned commit. \br\br
    \a sCommitId is the commit identifier to forget \br
*/
void CCommitTreeList::removeCommit(const QString& sCommitId)
{
    if (sCommitId.isEmpty())
        return;

    m_pShardStore->removeValue(sCommitId);
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns the parent commits of an in-memory commit. \br\br
    The parent identifiers are resolved through the cache, then each parent
    commit file is loaded once. The source commit parents seed the cache so
    no file is loaded for the source commit itself. \br
    \a pCommit is the source commit \br
    \a parent is the owner of the returned commits \br
*/
QList<CCommit*> CCommitTreeList::commitParentList(const CCommit* pCommit, QObject* parent)
{
    if (IS_NULL(pCommit) || pCommit->id().isEmpty())
        return QList<CCommit*>();

    if (not m_pShardStore->containsValue(pCommit->id()))
        registerCommitParents(pCommit->id(), pCommit->parents());

    return commitParentList(pCommit->id(), parent);
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns the parent commits of a commit identifier. \br\br
    The parent identifiers are resolved through the cache, so commit files are
    loaded only for the direct parents, never for the whole ancestry. \br
    \a sCommitId is the source commit identifier \br
    \a parent is the owner of the returned commits \br
*/
QList<CCommit*> CCommitTreeList::commitParentList(const QString& sCommitId, QObject* parent)
{
    QList<CCommit*> lParents;

    if (sCommitId.isEmpty())
        return lParents;

    QStringList lParentIds = commitParentIds(sCommitId);

    for (const QString& sParentId : lParentIds)
    {
        if (sParentId.isEmpty())
            continue;

        QString sCommitFileName = m_pDatabase->composeCommitFileName(sParentId);
        lParents << CCommit::fromFile(sCommitFileName, parent, sParentId);
    }

    return lParents;
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns the parent identifiers of a commit through the parent cache. \br\br
    A cache hit performs no file access beyond the holding shard. On a cache
    miss the commit file is loaded once, its parents are cached, and later
    lookups are served from the cache. Missing commit files resolve to an
    empty list without caching. \br
    \a sCommitId is the commit identifier to look up \br
*/
QStringList CCommitTreeList::commitParentIds(const QString& sCommitId)
{
    if (sCommitId.isEmpty())
        return QStringList();

    if (m_pShardStore->containsValue(sCommitId))
        return m_pShardStore->value(sCommitId);

    QString sCommitFileName = m_pDatabase->composeCommitFileName(sCommitId);

    if (not QFile::exists(sCommitFileName))
        return QStringList();

    CCommit* pCommit = CCommit::fromNode(CXMLNode::load(sCommitFileName), nullptr, sCommitId);
    QStringList lParents = pCommit->parents();
    delete pCommit;

    registerCommitParents(sCommitId, lParents);

    return lParents;
}

//-------------------------------------------------------------------------------------------------

}
