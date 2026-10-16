
#pragma once

// Global
#include "Aze_global.h"

// Qt
#include <QObject>
#include <QList>

// qt-plus
#include "Macros.h"

// Application
#include "CCommitTreeList.h"
#include "CError.h"
#include "CFileHashCache.h"
#include "objects/CCommit.h"
#include "objects/CFile.h"
#include "objects/CDatabase.h"

//-------------------------------------------------------------------------------------------------

namespace Aze {

class AZE_SHARED_EXPORT CCommitFunctions : public QObject
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

    Q_FAST_PROPERTY(CDatabase*, p, database, Database)
    Q_FAST_PROPERTY(bool, b, silent, Silent)
    Q_FAST_PROPERTY(bool, b, debug, Debug)
    Q_FAST_PROPERTY(CCommitTreeList* , p, commitTreeList, CommitTreeList)
    Q_FAST_PROPERTY(CFileHashCache*, p, fileHashCache, FileHashCache)

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //!
    CCommitFunctions(CDatabase* pDatabase, QObject* parent = nullptr, bool bSilent = false, bool bDebug = false);

    //!
    ~CCommitFunctions();

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //! Returns the nth ancestor of a commit
    CCommit* getCommitAncestor(CDatabase* pDatabase, CCommit* pCommit, QObject* owner = nullptr, int iDelta = 1);

    //! Returns the identifier of the nth ancestor of sCommitId through the parent cache
    QString commitAncestorId(const QString& sCommitId, int iDelta = 1);

    //! Caches parent identifiers of sCommitId without loading any commit file
    void registerCommitParents(const QString& sCommitId, const QStringList& lParentIds);

    //! Stores pCommit and caches its parents, single write path for commits
    QString storeCommit(CCommit* pCommit, const QString& sId = "");

    //! Loads a commit and seeds the parent cache, single read path for commits
    CCommit* loadCommit(const QString& sCommitId, QObject* parent);

    //! Loads commits and seeds the parent cache, single read path for commit lists
    QList<CCommit*> loadCommitList(const QStringList& lCommitIds, QObject* parent);

    //! Returns a list of pairs (distance, id) of ancestors of sCommitId
    QList<QPair<int, QString>> getCommitAncestorList(
            const QString& sCommitId,
            bool bStayOnBranch = true,
            int iMaxCount = 0,
            const QString& sStopAtCommitId = ""
            );

    //!
    QStringList getShortestCommitChain(
            const QString& sTipCommitId,
            const QString& sAncestorCommitId
            );

    //!
    QString getCommonCommitChains(
            const QString& sCommitId1,
            const QString& sCommitId2,
            QStringList* lCommitChainIds1 = nullptr,
            QStringList* lCommitChainIds2 = nullptr
            );

    //!
    void diffCommits(QString& sOutput, CCommit* pCommit1, CCommit* pCommit2, const QStringList& lIgnoreFiles, int iDelta1 = 0, int iDelta2 = 0);

    //!
    void diffCommitLists(QString& sOutput, const QList<CCommit*>& lCommitList, const QList<CCommit*>& lIgnoreList);

    //!
    void diffFiles(QString& sOutput, const QString& sFile1, const QString& sFile2);

    //! Returns the difference between two text files
    void diffText(QString& sOutput, const QString& sFileName, const QString& sText1, const QString& sText2);

    //! Applies a diff to the working directory
    CError::EErrorCode applyDiff(const QString& sFullDiff, bool bAddToStage = false, CCommit* pStagingCommit = nullptr);

    //! Do a 3-way merge and add (if requested) the modified files to the stage
    CError::EErrorCode threeWayMerge(CCommit* pBaseCommit, CCommit* pFromTipCommit, CCommit* pToTipCommit, bool bAddToStage, CCommit* pStagingCommit);

    //! Return a commit object that recflects the structure of a given file system folder (and its sub-folder)
    CCommit* folderAsCommit(QObject* owner = nullptr, QString sRootPath = "");

    //! Drops all entries of the working folder hash cache
    void clearFileHashCache();

    //! Drops the hash cache entry of a single absolute file name
    void invalidateFileInHashCache(const QString& sAbsoluteFileName);

    //-------------------------------------------------------------------------------------------------
    // Protected control methods
    //-------------------------------------------------------------------------------------------------

    //!
    void getCommitAncestorListRecurse(
            QList<QPair<int, QString>>& lCommitList,
            const QString& sCommitId,
            int iDepth = 0,
            bool bStayOnBranch = true,
            int iGuard = 0,
            const QString& sStopAtCommitId = ""
            );

    //!
    void getShortestCommitChainRecurse(
            QList<QStringList>& lCommitListList,
            QStringList lCurrentCommitList,
            const QString& pCommit,
            const QString& sStopAtCommitId,
            int iGuard
            );
};

}
