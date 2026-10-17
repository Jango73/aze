
// Qt
#include <QDebug>
#include <QDir>
#include <QFile>

// qt-plus
#include "File/CDirectoryListing.h"

// Application
#include "CCommitTreeList.h"
#include "CEnums.h"
#include "CStrings.h"
#include "CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CCommitTreeList::CCommitTreeList(CDatabase* pDatabase, QObject* parent, bool bSilent, bool bDebug, bool bTextCache)
    : QObject(parent)
    , m_pDatabase(pDatabase)
    , m_bSilent(bSilent)
    , m_bDebug(bDebug)
    , m_bDirty(false)
{
    m_sFileName = m_pDatabase->composeCacheFileName(metaObject()->className());

    QString sBinaryFileName = m_sFileName + CXMLNode::sExtension_BIN;
    QString sTextFileName = QString("%1.%2").arg(m_sFileName).arg(CStrings::s_sCompressedXMLExtension);

    // The text format is the former cache format, binary is the default
    if (bTextCache)
    {
        m_sFileName = sTextFileName;
    }
    else
    {
        m_sFileName = sBinaryFileName;
    }

    // One-way migration from the other format: load it once, then drop it.
    // The destructor persists the content in the selected format from now on.
    QString sOtherFileName = sTextFileName;

    if (m_sFileName == sTextFileName)
        sOtherFileName = sBinaryFileName;

    if (not QFile::exists(m_sFileName) and QFile::exists(sOtherFileName))
    {
        fromNode(CXMLNode::load(sOtherFileName));
        QFile::remove(sOtherFileName);
        m_bDirty = true;
    }
    else
    {
        fromNode(CXMLNode::load(m_sFileName));
    }
}

//-------------------------------------------------------------------------------------------------

CCommitTreeList::CCommitTreeList(const CCommitTreeList& target)
    : QObject(target.parent())
{
    *this = target;
}

//-------------------------------------------------------------------------------------------------

CCommitTreeList::~CCommitTreeList()
{
    if (m_bDirty)
    {
        toNode().save(m_sFileName);
    }
}

//-------------------------------------------------------------------------------------------------

CCommitTreeList& CCommitTreeList::operator = (const CCommitTreeList& target)
{
    if (this == &target)
        return *this;

    m_pDatabase = target.m_pDatabase;
    m_bSilent = target.m_bSilent;
    m_bDebug = target.m_bDebug;
    m_bDirty = target.m_bDirty;
    m_sFileName = target.m_sFileName;
    m_mCommitParentList = target.m_mCommitParentList;
    return *this;
}

//-------------------------------------------------------------------------------------------------

bool CCommitTreeList::operator == (const CCommitTreeList& target)
{
    return m_mCommitParentList == target.m_mCommitParentList;
}

//-------------------------------------------------------------------------------------------------

CXMLNode CCommitTreeList::toNode()
{
    CXMLNode xReturnValue(CStrings::s_sParamCommitTreeList);

    QStringList lCommitParentList;

    for (const QString& sKey : m_mCommitParentList.keys())
    {
        lCommitParentList << CUtils::packFileAndId(sKey, m_mCommitParentList[sKey].join(":"));
    }

    xReturnValue.attributes()[CStrings::s_sParamCommitParentList] = lCommitParentList.join("|");

    return xReturnValue;
}

//-------------------------------------------------------------------------------------------------

void CCommitTreeList::fromNode(const CXMLNode& xNode)
{
    m_mCommitParentList.clear();

    QString sPackedList = xNode.attributes()[CStrings::s_sParamCommitParentList];

    if (sPackedList.isEmpty())
        return;

    QStringList lCommitParentList = sPackedList.split("|");

    for (const QString& sItem : lCommitParentList)
    {
        if (sItem.isEmpty())
            continue;

        QString sId;
        QString sParents;
        CUtils::unpackFileAndId(sItem, sId, sParents);

        if (sId.isEmpty())
            continue;

        if (sParents.isEmpty())
            m_mCommitParentList[sId] = QStringList();
        else
            m_mCommitParentList[sId] = sParents.split(":");
    }
}

//-------------------------------------------------------------------------------------------------

/*!
    Tells if parent identifiers are cached for a commit. \br\br
    \a sCommitId is the commit identifier to look up \br
*/
bool CCommitTreeList::containsCommit(const QString& sCommitId) const
{
    return m_mCommitParentList.contains(sCommitId);
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

    if (m_mCommitParentList.value(sCommitId) == lParentIds)
        return;

    m_mCommitParentList[sCommitId] = lParentIds;
    m_bDirty = true;
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

    if (m_mCommitParentList.remove(sCommitId) > 0)
        m_bDirty = true;
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

    if (not m_mCommitParentList.contains(pCommit->id()))
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
    A cache hit performs no file access. On a cache miss the commit file is
    loaded once, its parents are cached, and later lookups are served from
    the cache. Missing commit files resolve to an empty list without caching. \br
    \a sCommitId is the commit identifier to look up \br
*/
QStringList CCommitTreeList::commitParentIds(const QString& sCommitId)
{
    if (sCommitId.isEmpty())
        return QStringList();

    if (m_mCommitParentList.contains(sCommitId))
        return m_mCommitParentList.value(sCommitId);

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
