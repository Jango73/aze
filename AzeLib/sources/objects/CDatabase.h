
#pragma once

// Global
#include "../Aze_global.h"

// Qt
#include <QObject>

// qt-plus
#include "Macros.h"

// Application
#include "../CEnums.h"
#include "../CStrings.h"
#include "CBranch.h"
#include "CCommit.h"
#include "CStash.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CDatabase : public QObject
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

    Q_FAST_PROPERTY(QString, s, startPath, StartPath)
    Q_FAST_PROPERTY(QString, s, rootPath, RootPath)
    Q_FAST_PROPERTY(QString, s, dataPath, DataPath)
    Q_FAST_PROPERTY(QString, s, stashPath, StashPath)
    Q_FAST_PROPERTY(QString, s, mergePath, MergePath)
    Q_FAST_PROPERTY(QString, s, branchPath, BranchPath)
    Q_FAST_PROPERTY(QString, s, commitPath, CommitPath)
    Q_FAST_PROPERTY(QString, s, objectPath, ObjectPath)
    Q_FAST_PROPERTY(QString, s, cachePath, CachePath)

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //!
    CDatabase(const QString& sRootPath, QObject* parent = nullptr);

    //-------------------------------------------------------------------------------------------------
    // Getters
    //-------------------------------------------------------------------------------------------------

    //!
    CBranch* getBranch(const QString& sName, QObject* parent);

    //!
    CCommit* getCommit(const QString& sId, QObject* parent);

    //!
    CStash* getStash(const QString& sId, QObject* parent);

    //!
    QByteArray getObject(const QString& sId);

    //!
    QVector<CBranch*> getAllBranches(QObject* parent);

    //-------------------------------------------------------------------------------------------------
    // Setters
    //-------------------------------------------------------------------------------------------------

    //!
    QString storeCommit(const CCommit* pCommit, const QString& sId = "");

    //!
    QString storeStash(const CStash* pStash, const QString& sId = "");

    //!
    QString storeFileAsObject(const QString& sFileName);

    //! Stores raw data as the object sId, returns true on success
    bool storeObject(const QString& sId, const QByteArray& baData);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    bool init();

    //!
    bool removeStash(const QString& sId);

    //! Returns a relative file name given a root path and an absolute path to a file
    QString relativeFileName(const QString& sFileName);

    //!
    QString absoluteFileName(const QString& sFileName);

    //!
    QString startRelativeFileName(const QString& sFileName);

    //!
    QString generalInformationFileName();

    //!
    QString composeLocalFileName(const QString& sFileName);

    //!
    QString composeMergeFileName(const QString& sFileName);

    //!
    QString composeBranchFileName(const QString& sBranchName);

    //!
    QString composeCommitFileName(const QString& sCommitId);

    //!
    QString composeObjectFileName(const QString& sId);

    //!
    QString composeStashFileName(const QString& sId);

    //!
    QString composeCacheFileName(const QString& sFileName);

    //!
    bool fileWithIdExists(const QString& sId);

    //!
    QString printableFileContentById(const QString& sId);

    //-------------------------------------------------------------------------------------------------
    // Protected control methods
    //-------------------------------------------------------------------------------------------------

protected:

    QString getActualRoot(const QString& sPath);
};

}
