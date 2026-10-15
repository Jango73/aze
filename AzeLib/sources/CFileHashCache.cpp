
// Qt
#include <QFileInfo>

// Application
#include "CFileHashCache.h"
#include "CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CFileHashCache. \br\br
    \a parent is the cache owner \br
*/
CFileHashCache::CFileHashCache(QObject* parent)
    : QObject(parent)
    , m_iHitCount(0)
    , m_iMissCount(0)
{
}

//-------------------------------------------------------------------------------------------------

/*!
    Destroys a CFileHashCache.
*/
CFileHashCache::~CFileHashCache()
{
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns the content identifier of \a sAbsoluteFileName. \br\br
    When a stored entry matches the file size and modification time, the stored
    identifier is returned without reading file content. \br
    Otherwise the identifier is computed from file content with
    CUtils::idFromFileContent and the stored entry is replaced. \br
    Returns an empty string when the file is missing or unreadable. \br
*/
QString CFileHashCache::identifierForFile(const QString& sAbsoluteFileName)
{
    if (sAbsoluteFileName.isEmpty())
        return "";

    QFileInfo tFileInfo(sAbsoluteFileName);
    QString sKey = tFileInfo.absoluteFilePath();

    if (not tFileInfo.exists() || not tFileInfo.isFile())
    {
        m_mEntryByFilePath.remove(sKey);
        return "";
    }

    qint64 iFileSize = tFileInfo.size();
    qint64 iModificationTime = tFileInfo.lastModified().toMSecsSinceEpoch();

    if (m_mEntryByFilePath.contains(sKey))
    {
        const FILE_HASH_CACHE_ENTRY& tEntry = m_mEntryByFilePath[sKey];

        if (tEntry.FileSize == iFileSize
                && tEntry.ModificationTime == iModificationTime
                && CUtils::idValid(tEntry.Identifier))
        {
            m_iHitCount++;
            return tEntry.Identifier;
        }
    }

    m_iMissCount++;

    QString sIdentifier = CUtils::idFromFileContent(sAbsoluteFileName);

    if (sIdentifier.isEmpty())
    {
        m_mEntryByFilePath.remove(sKey);
        return "";
    }

    FILE_HASH_CACHE_ENTRY tEntry;
    tEntry.FileSize = iFileSize;
    tEntry.ModificationTime = iModificationTime;
    tEntry.Identifier = sIdentifier;
    m_mEntryByFilePath[sKey] = tEntry;

    return sIdentifier;
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns true when a fresh entry is stored for \a sAbsoluteFileName. \br\br
    Fresh means that the stored file size and modification time match the file
    on the file system. \br
*/
bool CFileHashCache::containsFile(const QString& sAbsoluteFileName) const
{
    if (sAbsoluteFileName.isEmpty())
        return false;

    QFileInfo tFileInfo(sAbsoluteFileName);
    QString sKey = tFileInfo.absoluteFilePath();

    if (not tFileInfo.exists() || not tFileInfo.isFile())
        return false;

    if (not m_mEntryByFilePath.contains(sKey))
        return false;

    const FILE_HASH_CACHE_ENTRY& tEntry = m_mEntryByFilePath[sKey];

    return tEntry.FileSize == tFileInfo.size()
            && tEntry.ModificationTime == tFileInfo.lastModified().toMSecsSinceEpoch()
            && CUtils::idValid(tEntry.Identifier);
}

//-------------------------------------------------------------------------------------------------

/*!
    Drops the entry stored for \a sAbsoluteFileName, if any. \br\br
    The next call to identifierForFile reads file content again. \br
*/
void CFileHashCache::invalidateFile(const QString& sAbsoluteFileName)
{
    if (sAbsoluteFileName.isEmpty())
        return;

    m_mEntryByFilePath.remove(QFileInfo(sAbsoluteFileName).absoluteFilePath());
}

//-------------------------------------------------------------------------------------------------

/*!
    Drops all stored entries, keeping hit and miss counters unchanged. \br\br
    Call this after the working folder content is replaced by commit, merge,
    received push or branch switch (see TODO-037). \br
*/
void CFileHashCache::clear()
{
    m_mEntryByFilePath.clear();
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns the number of calls served from stored entries. \br
*/
qint64 CFileHashCache::hitCount() const
{
    return m_iHitCount;
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns the number of calls that required reading file content. \br\br
    Calls for missing or unreadable files are not counted. \br
*/
qint64 CFileHashCache::missCount() const
{
    return m_iMissCount;
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns the number of stored entries. \br
*/
qint64 CFileHashCache::count() const
{
    return qint64(m_mEntryByFilePath.count());
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns the absolute file names with a stored entry. \br
*/
QStringList CFileHashCache::cachedFileNames() const
{
    return m_mEntryByFilePath.keys();
}

//-------------------------------------------------------------------------------------------------

} // namespace Aze
