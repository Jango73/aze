
#pragma once

// Global
#include "Aze_global.h"

// Qt
#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>

// qt-plus
#include "Macros.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

//-------------------------------------------------------------------------------------------------
// Type definitions
//-------------------------------------------------------------------------------------------------

//! Single entry of the working folder hash cache
struct FILE_HASH_CACHE_ENTRY
{
    qint64 FileSize;
    qint64 ModificationTime;
    QString Identifier;
};

//-------------------------------------------------------------------------------------------------

class AZE_SHARED_EXPORT CFileHashCache : public QObject
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //!
    explicit CFileHashCache(QObject* parent = nullptr);

    //!
    virtual ~CFileHashCache() override;

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //! Returns the content identifier of sAbsoluteFileName, reading the cache when size and modification time are unchanged
    QString identifierForFile(const QString& sAbsoluteFileName);

    //! Returns true when a fresh entry is stored for sAbsoluteFileName
    bool containsFile(const QString& sAbsoluteFileName) const;

    //! Drops the entry stored for sAbsoluteFileName, if any
    void invalidateFile(const QString& sAbsoluteFileName);

    //! Drops all stored entries, keeping hit and miss counters unchanged
    void clear();

    //! Returns the number of calls served from stored entries
    qint64 hitCount() const;

    //! Returns the number of calls that required reading file content
    qint64 missCount() const;

    //! Returns the number of stored entries
    qint64 count() const;

    //! Returns the absolute file names with a stored entry
    QStringList cachedFileNames() const;

protected:

    QMap<QString, FILE_HASH_CACHE_ENTRY> m_mEntryByFilePath;
    qint64 m_iHitCount;
    qint64 m_iMissCount;
};

}
