
#pragma once

// Global
#include "../Aze_global.h"

// Qt
#include <QObject>
#include <QList>

// qt-plus
#include "Macros.h"

// Application
#include "../CError.h"
#include "../CFileHashCache.h"
#include "../objects/CCommit.h"
#include "../objects/CDatabase.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CCommitDiffFunctions : public QObject
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

    Q_FAST_PROPERTY(CDatabase*, p, database, Database)
    Q_FAST_PROPERTY(CFileHashCache*, p, fileHashCache, FileHashCache)
    Q_FAST_PROPERTY(bool, b, silent, Silent)
    Q_FAST_PROPERTY(bool, b, debug, Debug)

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //!
    CCommitDiffFunctions(CDatabase* pDatabase, CFileHashCache* pFileHashCache, QObject* parent = nullptr, bool bSilent = false, bool bDebug = false);

    //!
    ~CCommitDiffFunctions();

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    void diffCommits(QString& sOutput, CCommit* pCommit1, CCommit* pCommit2, const QStringList& lIgnoreFiles);

    //!
    void diffCommitLists(QString& sOutput, const QList<CCommit*>& lCommitList, const QList<CCommit*>& lIgnoreList);

    //!
    void diffFiles(QString& sOutput, const QString& sFile1, const QString& sFile2);

    //! Returns the difference between two text files
    void diffText(QString& sOutput, const QString& sFileName, const QString& sText1, const QString& sText2);

    //! Applies a diff to the working directory
    CError::EErrorCode applyDiff(const QString& sFullDiff, bool bAddToStage = false, CCommit* pStagingCommit = nullptr);
};

}
