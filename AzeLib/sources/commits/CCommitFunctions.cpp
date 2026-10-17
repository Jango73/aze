
// Qt
#include <QDebug>
#include <QFile>

// qt-plus
#include "File/CDirectoryListing.h"

// Application
#include "CCommitFunctions.h"
#include "../CStrings.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CCommitFunctions::CCommitFunctions(CDatabase* pDatabase, QObject* parent, bool bSilent, bool bDebug, bool bTextCache)
    : QObject(parent)
    , m_pDatabase(pDatabase)
    , m_bSilent(bSilent)
    , m_bDebug(bDebug)
    , m_pCommitTreeList(new CCommitTreeList(pDatabase, parent, bSilent, bDebug, bTextCache))
    , m_pFileHashCache(new CFileHashCache(this))
    , m_pDiffFunctions(new CCommitDiffFunctions(pDatabase, m_pFileHashCache, this, bSilent, bDebug))
    , m_pMergeFunctions(new CCommitMergeFunctions(pDatabase, m_pFileHashCache, this, bSilent, bDebug))
{
}

//-------------------------------------------------------------------------------------------------

CCommitFunctions::~CCommitFunctions()
{
    delete m_pMergeFunctions;
    delete m_pDiffFunctions;
    delete m_pFileHashCache;
    delete m_pCommitTreeList;
}

//-------------------------------------------------------------------------------------------------

CCommit* CCommitFunctions::getCommitAncestor(CDatabase* pDatabase, CCommit* pCommit, QObject* owner, int iDelta)
{
    if (IS_NULL(pCommit) || IS_NULL(pDatabase))
        return nullptr;

    QString sAncestorId = commitAncestorId(pCommit->id(), iDelta);

    if (sAncestorId.isEmpty())
        return nullptr;

    CCommit* pAncestor = CCommit::fromId(pDatabase, sAncestorId, owner);

    if (IS_NOT_NULL(pAncestor) && QFile::exists(pDatabase->composeCommitFileName(sAncestorId)))
        m_pCommitTreeList->registerCommitParents(sAncestorId, pAncestor->parents());

    return pAncestor;
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns the identifier of the nth ancestor of a commit. \br\br
    The ancestry walk follows first parents through the parent cache, so no
    commit file is loaded during the walk. Only the caller decides whether
    the resulting identifier needs a commit object loaded. \br
    \a sCommitId is the starting commit identifier \br
    \a iDelta is the number of first-parent steps to walk \br
*/
QString CCommitFunctions::commitAncestorId(const QString& sCommitId, int iDelta)
{
    if (sCommitId.isEmpty() || iDelta <= 0)
        return "";

    QString sAncestorId = sCommitId;
    int iGuard = 999999;

    while (true)
    {
        QStringList lParents = m_pCommitTreeList->commitParentIds(sAncestorId);

        if (lParents.count() == 0)
            return "";

        // The first parent is the one to follow in order to stay on branch of pCommit
        sAncestorId = lParents[0];

        iDelta--;
        iGuard--;

        if (iDelta == 0 || iGuard == 0)
            break;
    }

    return sAncestorId;
}

//-------------------------------------------------------------------------------------------------

/*!
    Caches the parent identifiers of a commit without loading any file. \br\br
    Call this right after storing a new commit so later ancestor walks and
    log traversals hit the cache instead of reloading the commit file. \br
    \a sCommitId is the commit identifier to register \br
    \a lParentIds are the parent identifiers to cache \br
*/
void CCommitFunctions::registerCommitParents(const QString& sCommitId, const QStringList& lParentIds)
{
    m_pCommitTreeList->registerCommitParents(sCommitId, lParentIds);
}

//-------------------------------------------------------------------------------------------------

/*!
    Stores a commit and caches its parents in one step. \br\br
    This is the single write path for versioned commits: callers store
    through here instead of CDatabase so the parent cache never misses on
    freshly stored commits. \br
    \a pCommit is the commit to store \br
    \a sId forces the stored identifier, empty generates it from content \br
*/
QString CCommitFunctions::storeCommit(CCommit* pCommit, const QString& sId)
{
    if (IS_NULL(pCommit))
        return "";

    QString sStoredId = m_pDatabase->storeCommit(pCommit, sId);

    if (not sStoredId.isEmpty())
        m_pCommitTreeList->registerCommitParents(sStoredId, pCommit->parents());

    return sStoredId;
}

//-------------------------------------------------------------------------------------------------

/*!
    Loads a commit and seeds the parent cache in one step. \br\br
    This is the single read path for versioned commits: callers load through
    here instead of CDatabase so every loaded commit warms the cache for
    later ancestor walks and log traversals. Missing files resolve to null
    without polluting the cache. \br
    \a sCommitId is the commit identifier to load \br
    \a parent is the owner of the returned commit \br
*/
CCommit* CCommitFunctions::loadCommit(const QString& sCommitId, QObject* parent)
{
    if (sCommitId.isEmpty())
        return nullptr;

    if (not QFile::exists(m_pDatabase->composeCommitFileName(sCommitId)))
        return nullptr;

    CCommit* pCommit = m_pDatabase->getCommit(sCommitId, parent);

    if (IS_NOT_NULL(pCommit))
        m_pCommitTreeList->registerCommitParents(sCommitId, pCommit->parents());

    return pCommit;
}

//-------------------------------------------------------------------------------------------------

/*!
    Loads commits and seeds the parent cache in one step. \br\br
    Empty identifiers resolve to no object, existing ones are loaded through
    loadCommit so each warms the parent cache. \br
    \a lCommitIds are the commit identifiers to load \br
    \a parent is the owner of the returned commits \br
*/
QList<CCommit*> CCommitFunctions::loadCommitList(const QStringList& lCommitIds, QObject* parent)
{
    QList<CCommit*> lCommits;

    for (const QString& sCommitId : lCommitIds)
    {
        CCommit* pCommit = loadCommit(sCommitId, parent);

        if (IS_NOT_NULL(pCommit))
            lCommits << pCommit;
    }

    return lCommits;
}

//-------------------------------------------------------------------------------------------------

QList<QPair<int, QString>> CCommitFunctions::getCommitAncestorList(
        const QString& sCommitId,
        bool bStayOnBranch,
        int iMaxCount,
        const QString& sStopAtCommitId
        )
{
    int iGuard = iMaxCount == 0 ? 999999 : iMaxCount;
    int iDepth = 1;
    QList<QPair<int, QString>> lReturnValue;

    getCommitAncestorListRecurse(lReturnValue, sCommitId, iDepth, bStayOnBranch, iGuard, sStopAtCommitId);

    return lReturnValue;
}

//-------------------------------------------------------------------------------------------------

void CCommitFunctions::getCommitAncestorListRecurse(
        QList<QPair<int, QString>>& lCommitList,
        const QString& sCommitId,
        int iDepth,
        bool bStayOnBranch,
        int iGuard,
        const QString& sStopAtCommitId
        )
{
    if (iGuard <= 0)
        return;

    iGuard--;

    QStringList lParents = m_pCommitTreeList->commitParentIds(sCommitId);

    if (bStayOnBranch)
    {
        if (lParents.count() > 0)
        {
            if (lParents[0] != sStopAtCommitId)
            {
                lCommitList << QPair<int, QString>(iDepth, lParents[0]);

                getCommitAncestorListRecurse(
                            lCommitList,
                            lParents[0],
                        iDepth + 1,
                        bStayOnBranch,
                        iGuard - 1,
                        sStopAtCommitId
                        );
            }
        }
    }
    else
    {
        int iParentIndex = 0;

        for (QString pParent : lParents)
        {
            if (pParent != sStopAtCommitId)
            {
                lCommitList << QPair<int, QString>(iDepth, pParent);

                getCommitAncestorListRecurse(
                            lCommitList,
                            pParent,
                            iDepth + 1,
                            bStayOnBranch,
                            iGuard - 1,
                            sStopAtCommitId
                            );
            }

            iParentIndex++;
        }
    }
}

//-------------------------------------------------------------------------------------------------

QStringList CCommitFunctions::getShortestCommitChain(const QString& sTipCommitId, const QString& sAncestorCommitId)
{
    if (sTipCommitId != sAncestorCommitId)
    {
        // Initialize commit chains with sTipCommitId
        QList<QStringList> lCommitList;
        lCommitList << QStringList();
        lCommitList.last() << sTipCommitId;

        // Call the recursive part to get all commit chains
        getShortestCommitChainRecurse(
                    lCommitList,
                    lCommitList.last(),
                    sTipCommitId,
                    sAncestorCommitId,
                    999999
                    );

        // Remove the chains that do not contain sAncestorCommitId
        for (int iCommitIndex = 0; iCommitIndex < lCommitList.count(); iCommitIndex++)
        {
            if (not lCommitList[iCommitIndex].contains(sAncestorCommitId))
            {
                lCommitList.removeAt(iCommitIndex);
                iCommitIndex--;
            }
        }

        if (m_bDebug)
        {
            OUT_DEBUG("----------------------------------------");
            OUT_DEBUG(QString("%1").arg(lCommitList.count()));
            for (QStringList lSomeCommits : lCommitList)
            {
                OUT_DEBUG("--------------------");
                OUT_DEBUG(QString("%1").arg(lSomeCommits.count()));
                for (QString pSomeCommit : lSomeCommits)
                {
                    OUT_DEBUG(QString("%1").arg(pSomeCommit));
                }
            }
        }

        // Find the shortest commit chain
        if (not lCommitList.isEmpty())
        {
            QStringList* lReturnedList = &(lCommitList[0]);

            for (int iCommitIndex = 1; iCommitIndex < lCommitList.count(); iCommitIndex++)
            {
                if (lCommitList[iCommitIndex].count() < lReturnedList->count())
                {
                    lReturnedList = &(lCommitList[iCommitIndex]);
                }
            }

            return *lReturnedList;
        }
    }

    return QStringList();
}

//-------------------------------------------------------------------------------------------------

void CCommitFunctions::getShortestCommitChainRecurse(
        QList<QStringList>& lCommitListList,
        QStringList lCurrentCommitList,
        const QString& sCommiId,
        const QString& sStopAtCommitId,
        int iGuard
        )
{
    if (iGuard <= 0)
        return;

    if (CUtils::sameIds(sCommiId, sStopAtCommitId))
        return;

    QStringList lParentIds = m_pCommitTreeList->commitParentIds(sCommiId);

    for (QString sParentId : lParentIds)
    {
        QStringList lNewList = lCurrentCommitList;
        lNewList << sParentId;

        getShortestCommitChainRecurse(
                    lCommitListList,
                    lNewList,
                    sParentId,
                    sStopAtCommitId,
                    iGuard - 1
                    );

        lCommitListList << lNewList;
    }
}

//-------------------------------------------------------------------------------------------------

QString CCommitFunctions::getCommonCommitChains(
        const QString& sCommitId1,
        const QString &sCommitId2,
        QStringList* lCommitChainIds1,
        QStringList* lCommitChainIds2
        )
{
    QList<QPair<int, QString>> lAncestors1 = getCommitAncestorList(sCommitId1, false);
    QList<QPair<int, QString>> lAncestors2 = getCommitAncestorList(sCommitId2, false);
    QMap<int, QPair<QString, QString>> commonAncestors;

    // Add commits themselves to lists
    lAncestors1.prepend(QPair<int, QString>(0, sCommitId1));
    lAncestors2.prepend(QPair<int, QString>(0, sCommitId2));

    if (m_bDebug)
    {
        OUT_DEBUG("--------------------");
        OUT_DEBUG("pAncestors1:");
        for (QPair<int, QString> pCommit : lAncestors1)
            OUT_DEBUG(QString("%1 %2").arg(pCommit.first).arg(pCommit.second));
        OUT_DEBUG("--------------------");
        OUT_DEBUG("pAncestors2:");
        for (QPair<int, QString> pCommit : lAncestors2)
            OUT_DEBUG(QString("%1 %2").arg(pCommit.first).arg(pCommit.second));
        OUT_DEBUG("--------------------");
    }

    // Find the common ancestor
    for (QPair<int, QString> pSubCommit1 : lAncestors1)
    {
        for (QPair<int, QString> pSubCommit2 : lAncestors2)
        {
            if (CUtils::sameIds(pSubCommit1.second, pSubCommit2.second))
            {
                int iDistance = pSubCommit1.first + pSubCommit2.first;
                commonAncestors[iDistance] = { pSubCommit1.second, pSubCommit2.second };
            }
        }
    }

    // Bail ou if not common ancestor found
    if (commonAncestors.count() == 0)
        return nullptr;

    // Get the common ancestors
    int iKey = commonAncestors.keys()[0];
    QString sAncestorId1 = commonAncestors[iKey].first;
    QString sAncestorId2 = commonAncestors[iKey].second;

    if (m_bDebug)
    {
        for (int iTempKey : commonAncestors.keys())
        {
            CCommit* pTempCommit = loadCommit(commonAncestors[iTempKey].first, nullptr);

            if (IS_NOT_NULL(pTempCommit))
            {
                OUT_DEBUG(QString("Common ancestor at %1: %2")
                          .arg(iTempKey)
                          .arg(pTempCommit->message())
                          );
                delete pTempCommit;
            }
        }
    }

    // Populate the commit lists if required
    if (IS_NOT_NULL(lCommitChainIds1))
    {
        *lCommitChainIds1 = getShortestCommitChain(sCommitId1, sAncestorId1);
        reverseList(*lCommitChainIds1);
    }

    if (IS_NOT_NULL(lCommitChainIds2))
    {
        *lCommitChainIds2 = getShortestCommitChain(sCommitId2, sAncestorId2);
        reverseList(*lCommitChainIds2);
    }

    if (m_bDebug)
    {
        if (IS_NOT_NULL(lCommitChainIds1) && IS_NOT_NULL(lCommitChainIds2))
        {
            OUT_DEBUG("--------------------");
            OUT_DEBUG("lCommit1Chain:");
            for (QString pCommit : *lCommitChainIds1)
                OUT_DEBUG(QString("%1").arg(pCommit));
            OUT_DEBUG("--------------------");
            OUT_DEBUG("lCommit2Chain:");
            for (QString pCommit : *lCommitChainIds2)
                OUT_DEBUG(QString("%1").arg(pCommit));
            OUT_DEBUG("--------------------");
        }
    }

    return sAncestorId1;
}

//-------------------------------------------------------------------------------------------------

/*!
    Appends the unified diff between two commits to sOutput, resolving ancestor deltas first. \br\br
    \a sOutput receives the diff text \br
    \a pCommit1 is the first commit \br
    \a pCommit2 is the second commit \br
    \a lIgnoreFiles are the file names excluded from the diff \br
    \a iDelta1 walks pCommit1 back by first parents before diffing \br
    \a iDelta2 walks pCommit2 back by first parents before diffing \br
*/
void CCommitFunctions::diffCommits(QString& sOutput, CCommit* pCommit1, CCommit* pCommit2, const QStringList& lIgnoreFiles, int iDelta1, int iDelta2)
{
    CCommit* pResolvedCommit1 = pCommit1;
    CCommit* pResolvedCommit2 = pCommit2;

    if (iDelta1 != 0)
        pResolvedCommit1 = getCommitAncestor(m_pDatabase, pCommit1, this, iDelta1);

    if (iDelta2 != 0)
        pResolvedCommit2 = getCommitAncestor(m_pDatabase, pCommit2, this, iDelta2);

    m_pDiffFunctions->diffCommits(sOutput, pResolvedCommit1, pResolvedCommit2, lIgnoreFiles);

    if (iDelta1 != 0)
        delete pResolvedCommit1;

    if (iDelta2 != 0)
        delete pResolvedCommit2;
}

//-------------------------------------------------------------------------------------------------

/*!
    Appends the unified diff of commits in lToList missing from lFromList to sOutput. \br\br
    Delegates to CCommitDiffFunctions. \br
*/
void CCommitFunctions::diffCommitLists(QString& sOutput, const QList<CCommit*>& lToList, const QList<CCommit*>& lFromList)
{
    m_pDiffFunctions->diffCommitLists(sOutput, lToList, lFromList);
}

//-------------------------------------------------------------------------------------------------

/*!
    Appends the unified diff between two files on the file system to sOutput. \br\br
    Delegates to CCommitDiffFunctions. \br
*/
void CCommitFunctions::diffFiles(QString& sOutput, const QString& sFile1, const QString& sFile2)
{
    m_pDiffFunctions->diffFiles(sOutput, sFile1, sFile2);
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns the difference between two text files. \br\br
    Delegates to CCommitDiffFunctions. \br
*/
void CCommitFunctions::diffText(QString& sOutput, const QString& sFileName, const QString& sText1, const QString& sText2)
{
    m_pDiffFunctions->diffText(sOutput, sFileName, sText1, sText2);
}

//-------------------------------------------------------------------------------------------------

/*!
    Applies a diff to the working directory. \br\br
    Delegates to CCommitDiffFunctions. \br
*/
CError::EErrorCode CCommitFunctions::applyDiff(const QString& sFullDiff, bool bAddToStage, CCommit* pStagingCommit)
{
    return m_pDiffFunctions->applyDiff(sFullDiff, bAddToStage, pStagingCommit);
}

//-------------------------------------------------------------------------------------------------

/*!
    Do a 3-way merge and add (if requested) the modified files to the stage. \br\br
    Delegates to CCommitMergeFunctions. \br
*/
CError::EErrorCode CCommitFunctions::threeWayMerge(CCommit* pBaseCommit, CCommit* pFromTipCommit, CCommit* pToTipCommit, bool bAddToStage, CCommit* pStagingCommit)
{
    return m_pMergeFunctions->threeWayMerge(pBaseCommit, pFromTipCommit, pToTipCommit, bAddToStage, pStagingCommit);
}

//-------------------------------------------------------------------------------------------------

CCommit* CCommitFunctions::folderAsCommit(QObject* owner, QString sRootPath)
{
    CCommit* pNewCommit = new CCommit(owner);

    if (sRootPath.isEmpty())
        sRootPath = m_pDatabase->rootPath();

    // List contents of folder
    CDirectoryListing tDirectory;
    tDirectory.addIgnoreName(CStrings::s_sPathAzeDataRootPath);
    tDirectory.setRootPath(sRootPath);

    // Add listed files to the commit
    // The identifier is read from the working folder hash cache when file
    // size and modification time are unchanged, and is computed from file
    // content only on cache miss by CFileHashCache::identifierForFile
    // Other modules reuse the cache the same way: resolve the identifier
    // through the cache, then pass it to CCommit::addFile so no second
    // computation happens
    for (const CDirectoryListingItem& sFile : tDirectory.files())
    {
        if (not sFile.isFolder())
        {
            QString sAbsoluteFileName = CUtils::absoluteFileName(sRootPath, sFile.relativeName());
            QString sIdentifier = m_pFileHashCache->identifierForFile(sAbsoluteFileName);
            pNewCommit->addFile(m_pDatabase, sFile.relativeName(), sIdentifier);
        }
    }

    return pNewCommit;
}

//-------------------------------------------------------------------------------------------------

/*!
    Drops all entries of the working folder hash cache. \br\br
    Call this after the working folder content is replaced by commit, merge,
    received push or branch switch. \br
*/
void CCommitFunctions::clearFileHashCache()
{
    m_pFileHashCache->clear();
}

//-------------------------------------------------------------------------------------------------

/*!
    Drops the hash cache entry of a single absolute file name. \br\br
    \a sAbsoluteFileName is the absolute file name to drop from the cache \br
*/
void CCommitFunctions::invalidateFileInHashCache(const QString& sAbsoluteFileName)
{
    m_pFileHashCache->invalidateFile(sAbsoluteFileName);
}

//-------------------------------------------------------------------------------------------------

}
