
// Qt
#include <QDebug>
#include <QDir>

// qt-plus
#include "File/CDirectoryListing.h"

// Application
#include "CCommitFunctions.h"
#include "CEnums.h"
#include "CStrings.h"
#include "CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CCommitFunctions::CCommitFunctions(CDatabase* pDatabase, QObject* parent, bool bSilent, bool bDebug)
    : QObject(parent)
    , m_pDatabase(pDatabase)
    , m_bSilent(bSilent)
    , m_bDebug(bDebug)
    , m_pCommitTreeList(new CCommitTreeList(pDatabase, parent, bSilent, bDebug))
    , m_pFileHashCache(new CFileHashCache(this))
{
}

//-------------------------------------------------------------------------------------------------

CCommitFunctions::~CCommitFunctions()
{
    delete m_pFileHashCache;
    delete m_pCommitTreeList;
}

//-------------------------------------------------------------------------------------------------

CCommit* CCommitFunctions::getCommitAncestor(CDatabase* pDatabase, CCommit* pCommit, QObject* owner, int iDelta)
{
    QString pAncestor = pCommit->id();
    int iGuard = 999999;

    while (true)
    {
        QStringList parents = m_pCommitTreeList->commitParentIds(pAncestor);

        if (parents.count() == 0)
        {
            pAncestor = "";
            return nullptr;
        }

        // The first parent is the one to follow in order to stay on branch of pCommit
        pAncestor = parents[0];

        iDelta--;
        iGuard--;

        if (iDelta == 0 || iGuard == 0)
            break;
    }

    return CCommit::fromId(pDatabase, pAncestor, owner);
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
            CCommit* pTempCommit = CCommit::fromId(m_pDatabase, commonAncestors[iTempKey].first, nullptr);
            OUT_DEBUG(QString("Common ancestor at %1: %2")
                      .arg(iTempKey)
                      .arg(pTempCommit->message())
                      );
            delete pTempCommit;
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

void CCommitFunctions::diffCommits(QString& sOutput, CCommit* pCommit1, CCommit* pCommit2, const QStringList& lIgnoreFiles, int iDelta1, int iDelta2)
{
    if (iDelta1 != 0)
        pCommit1 = getCommitAncestor(m_pDatabase, pCommit1, this, iDelta1);

    if (iDelta2 != 0)
        pCommit2 = getCommitAncestor(m_pDatabase, pCommit2, this, iDelta2);

    if (IS_NOT_NULL(pCommit1) && IS_NOT_NULL(pCommit2))
    {
        QHash<QString, int> lProcessedFiles;

        for (QString sName : pCommit2->files().keys())
        {
            if (not lIgnoreFiles.contains(sName) && not lProcessedFiles.contains(sName))
            {
                QByteArray baContent1 = pCommit1->fileContent(m_pDatabase, sName);
                QByteArray baContent2 = pCommit2->fileContent(m_pDatabase, sName);

                QString sDiffText = CUtils::unifiedDiff(QString(baContent1), QString(baContent2));

                if (not sDiffText.isEmpty())
                {
                    sOutput += CUtils::fileDiffHeader(sName, sName);
                    sOutput += sDiffText;
                }

                lProcessedFiles[sName] = 0;
            }
        }

        for (QString sName : pCommit1->files().keys())
        {
            if (not lIgnoreFiles.contains(sName) && not lProcessedFiles.contains(sName))
            {
                QByteArray baContent1 = pCommit1->fileContent(m_pDatabase, sName);
                QByteArray baContent2 = pCommit2->fileContent(m_pDatabase, sName);

                QString sDiffText = CUtils::unifiedDiff(QString(baContent1), QString(baContent2));

                if (not sDiffText.isEmpty())
                {
                    sOutput += CUtils::fileDiffHeader(sName, sName);
                    sOutput += sDiffText;
                }

                lProcessedFiles[sName] = 0;
            }
        }
    }

    if (iDelta1 != 0)
        delete pCommit1;

    if (iDelta2 != 0)
        delete pCommit2;
}

//-------------------------------------------------------------------------------------------------

void CCommitFunctions::diffCommitLists(QString& sOutput, const QList<CCommit*>& lToList, const QList<CCommit*>& lFromList)
{
    if (m_bDebug)
    {
        OUT_DEBUG("--------------------");
        OUT_DEBUG("To list:");
        for (CCommit* pCommit : lToList)
            OUT_DEBUG(QString("%1 %2").arg(pCommit->shortId()).arg(pCommit->message()));
        OUT_DEBUG("--------------------");
        OUT_DEBUG("From list:");
        for (CCommit* pCommit : lFromList)
            OUT_DEBUG(QString("%1 %2").arg(pCommit->shortId()).arg(pCommit->message()));
        OUT_DEBUG("--------------------");
    }

    for (int index = 1; index < lFromList.count(); index++)
    {
        CCommit* pCommit1 = lFromList[index - 1];
        CCommit* pCommit2 = lFromList[index];

        //        QList<CCommit*>::const_iterator foundCommit = std::find_if(
        //                    lIgnoreList.begin(),
        //                    lIgnoreList.end(),
        //                    [&](const CCommit*& c) { return c->id() == pCommit2->id(); }
        //        );

        bool bFound = false;

        for (CCommit* pCommit : lToList)
            if (CUtils::sameIds(pCommit->id(), pCommit2->id()))
            {
                bFound = true;
                break;
            }

        //        if (foundCommit == lIgnoreList.end())
        if (bFound == false)
        {
            for (QString sName : pCommit2->files().keys())
            {
                QByteArray baContent1 = pCommit1->fileContent(m_pDatabase, sName);
                QByteArray baContent2 = pCommit2->fileContent(m_pDatabase, sName);

                if (m_bDebug)
                {
                    OUT_DEBUG(QString("X (%1): %2").arg(pCommit1->message()).arg(QString(baContent1)));
                    OUT_DEBUG(QString("Y (%1): %2").arg(pCommit2->message()).arg(QString(baContent2)));
                }

                QString sDiffText = CUtils::unifiedDiff(QString(baContent1), QString(baContent2));

                if (m_bDebug)
                {
                    OUT_DEBUG(QString("Diff: %1").arg(CUtils::printableUnifiedDiff(sDiffText)));
                }

                if (not sDiffText.isEmpty())
                {
                    sOutput += CUtils::fileDiffHeader(sName, sName);
                    sOutput += sDiffText;
                }
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------

void CCommitFunctions::diffFiles(QString& sOutput, const QString& sFile1, const QString& sFile2)
{
    // Get text content of both files
    QString sText1 = CUtils::getTextFileContent(sFile1);
    QString sText2 = CUtils::getTextFileContent(sFile2);

    // Diff text contents
    QString sDiffText = CUtils::unifiedDiff(sText1, sText2);

    // Output diff
    if (not sDiffText.isEmpty())
    {
        sOutput += CUtils::fileDiffHeader(sFile1, sFile2);
        sOutput += sDiffText;
    }
}

//-------------------------------------------------------------------------------------------------

void CCommitFunctions::diffText(QString& sOutput, const QString& sFileName, const QString& sText1, const QString& sText2)
{
    QString sDiffText = CUtils::unifiedDiff(sText1, sText2);

    if (not sDiffText.isEmpty())
    {
        sOutput += CUtils::fileDiffHeader(sFileName, sFileName);
        sOutput += sDiffText;
    }
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CCommitFunctions::applyDiff(const QString& sFullDiff, bool bAddToStage, CCommit* pStagingCommit)
{
    // Keep track of merged files
    QDictionary mProcessedFiles;
    QStringList mMovedFiles;

    // Get a list of diffs per file
    QList<QPair<QString, QString> > mFileDiffs = CUtils::splitDiff(sFullDiff);

    for (QPair<QString, QString> tPair : mFileDiffs)
    {
        QString sFileName = tPair.first;
        QString sFileDiff = tPair.second;

        // Read the file, apply the diff and write to merge directory
        QString sFullSourceName = m_pDatabase->composeLocalFileName(sFileName);
        QString sFullTargetName = m_pDatabase->composeMergeFileName(sFileName);

        // Check if file is already present in merge directory
        bool bMergedFileExists = QFile(sFullTargetName).exists();

        if (bMergedFileExists)
            sFullSourceName = sFullTargetName;

        // Get the content of the file
        QString sPreviousContent = CUtils::getTextFileContent(sFullSourceName);
        QString sNewContent;

        bool bApplyOk = CUtils::applyUnifiedDiff(sPreviousContent, sFileDiff, sNewContent);

        if (m_bDebug)
        {
            OUT_DEBUG(QString("A: %1").arg(sPreviousContent));
            OUT_DEBUG(QString("B: %1").arg(CUtils::printableUnifiedDiff(sFileDiff)));
            OUT_DEBUG(QString("C: %1").arg(sNewContent));
            OUT_DEBUG("----------------------------------------");
        }

        if (not bApplyOk)
        {
            if (not m_bSilent)
            {
                OUT_ERROR(CError::message(CError::eCouldNotApplyPatch, sFileName));
            }
            return CError::eCouldNotApplyPatch;
        }

        CUtils::putTextFileContent(sFullTargetName, sNewContent);
        QString sId = CUtils::idFromByteArray(sNewContent.toUtf8());
        mProcessedFiles[sFileName] = sId;
    }

    // Stage the merged files
    if (bAddToStage && IS_NOT_NULL(pStagingCommit))
    {
        for (QString sFileName : mProcessedFiles.keys())
        {
            QString sId = mProcessedFiles[sFileName];
            pStagingCommit->addFile(m_pDatabase, sFileName, sId);
        }
    }

    // Move the merged files
    for (QString sFileName : mProcessedFiles.keys())
    {
        QString sFullSourceName = m_pDatabase->composeMergeFileName(sFileName);
        QString sFullTargetName = m_pDatabase->composeLocalFileName(sFileName);

        if (not mMovedFiles.contains(sFullSourceName))
        {
            if (not CUtils::moveFile(sFullSourceName, sFullTargetName))
            {
                if (not m_bSilent)
                {
                    OUT_ERROR(CError::message(CError::eCouldNotMoveFiles, sFullSourceName));
                }
                return CError::eCouldNotMoveFiles;
            }

            mMovedFiles << sFullSourceName;
        }
    }

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CCommitFunctions::threeWayMerge(CCommit* pBaseCommit, CCommit* pFromTipCommit, CCommit* pToTipCommit, bool bAddToStage, CCommit* pStagingCommit)
{
    bool bHasConflicts = false;
    QString sFromDiff;
    QString sToDiff;

    // TODO: Handle files missing left or right

    diffCommits(sFromDiff, pBaseCommit, pFromTipCommit, QStringList(), 0, 0);
    diffCommits(sToDiff, pBaseCommit, pToTipCommit, QStringList(), 0, 0);

    // Bail out if diff is empty
    // An empty merge is reported as conflicts to preserve the historical
    // bHasConflicts semantics of the caller
    if (sFromDiff.isEmpty() && sToDiff.isEmpty())
    {
        return CError::eMergeHasConflicts;
    }

    // Keep track of merged files
    QMap<QPair<QString, bool>, QString> mProcessedFiles;    // Maps relative file names to IDs
    QList<QPair<QString, QString>> mFromSideFiles;
    QList<QPair<QString, QString>> mToSideFiles;
    QStringList mBothSideFiles;

    // Get a list of diffs per file
    QList<QPair<QString, QString>> mFromFileDiffs = CUtils::splitDiff(sFromDiff);
    QList<QPair<QString, QString>> mToFileDiffs = CUtils::splitDiff(sToDiff);

    // Find files that need a 3-way merge
    for (QPair<QString, QString> tFromPair : mFromFileDiffs)
    {
        for (QPair<QString, QString> tToPair : mToFileDiffs)
            if (tToPair.first == tFromPair.first)
            {
                mBothSideFiles << tFromPair.first;
                break;
            }
    }

    for (QPair<QString, QString> tFromPair : mFromFileDiffs)
    {
        if (not mBothSideFiles.contains(tFromPair.first))
        {
            mFromSideFiles << tFromPair;
        }
    }

    for (QPair<QString, QString> tToPair : mToFileDiffs)
    {
        if (not mBothSideFiles.contains(tToPair.first))
        {
            mToSideFiles << tToPair;
        }
    }

    // Iterate through files that are modified in the from commit
    for (QPair<QString, QString> pFile : mFromSideFiles)
    {
        // Read the file, apply the diff and write back
        QString sFullSourceName = m_pDatabase->composeLocalFileName(pFile.first);

        // Copy the file from the commit
        CUtils::putTextFileContent(sFullSourceName, QString(pFromTipCommit->fileContent(m_pDatabase, pFile.first)));

        // Get the Id of the file from the commit
        QString sId = pFromTipCommit->files()[pFile.first];

        mProcessedFiles[QPair<QString, bool>(pFile.first, true)] = sId;
    }

    // Iterate through files that are modified in the to commit
    for (QPair<QString, QString> pFile : mToSideFiles)
    {
        // Read the file, apply the diff and write back
        QString sFullSourceName = m_pDatabase->composeLocalFileName(pFile.first);

        // Copy the file from the commit
        CUtils::putTextFileContent(sFullSourceName, QString(pToTipCommit->fileContent(m_pDatabase, pFile.first)));

        // Get the Id of the file from the commit
        QString sId = pToTipCommit->files()[pFile.first];

        mProcessedFiles[QPair<QString, bool>(pFile.first, true)] = sId;
    }

    // Iterate through files that need a 3-way merge
    for (QString sFileName : mBothSideFiles)
    {
        if (m_bDebug)
        {
            OUT_DEBUG(QString("3-way merging: %1").arg(sFileName));
        }

        QString sFromFileDiff;
        QString sToFileDiff;

        for (QPair<QString, QString> tFromPair : mFromFileDiffs)
            if (tFromPair.first == sFileName)
            {
                sFromFileDiff = tFromPair.second;
                break;
            }

        for (QPair<QString, QString> tToPair : mToFileDiffs)
            if (tToPair.first == sFileName)
            {
                sToFileDiff = tToPair.second;
                break;
            }

        // Read the file, apply the diff and write to merge directory
        QString sFullSourceName = m_pDatabase->composeLocalFileName(sFileName);

        // Get the content of the file
        QString sBaseContent = QString(pBaseCommit->fileContent(m_pDatabase, sFileName));
        QString sFromContent = QString(pFromTipCommit->fileContent(m_pDatabase, sFileName));
        QString sToContent = QString(pToTipCommit->fileContent(m_pDatabase, sFileName));
        QString sNewContent;

        QString sFromMarker = QString("%1 %2").arg(pFromTipCommit->shortId()).arg(pFromTipCommit->message().left(16));
        QString sToMarker = QString("%1 %2").arg(pToTipCommit->shortId()).arg(pToTipCommit->message().left(16));

        // Do the actual merge
        bool bMergeOk = CUtils::applyThreeWayMerge(sBaseContent, sFromMarker, sFromContent, sToMarker, sToContent, sNewContent);

        if (m_bDebug)
        {
            OUT_DEBUG(QString("AC: %1").arg(sFromContent));
            OUT_DEBUG(QString("BC: %1").arg(sBaseContent));
            OUT_DEBUG(QString("CC: %1").arg(sToContent));
            OUT_DEBUG(QString("NC: %1").arg(sNewContent));
            OUT_DEBUG("----------------------------------------");
        }

        if (not bMergeOk)
            bHasConflicts = true;

        CUtils::putTextFileContent(sFullSourceName, sNewContent);
        QString sId = CUtils::idFromByteArray(sNewContent.toUtf8());
        mProcessedFiles[QPair<QString, bool>(sFileName, bMergeOk)] = sId;
    }

    // Stage the merged files
    if (bAddToStage && IS_NOT_NULL(pStagingCommit))
    {
        for (QPair<QString, bool> pFile : mProcessedFiles.keys())
        {
            QString sKey = mProcessedFiles[pFile];

            // Stage only if merge ok
            if (pFile.second)
            {
                pStagingCommit->addFile(m_pDatabase, pFile.first, sKey, true);
            }
        }
    }

    // Warn about conflict files
    if (bHasConflicts && not m_bSilent)
    {
        OUT_ERROR(QString("/!\\ Merge cannot be finished automatically. /!\\"));
        OUT_ERROR(QString("Files that are in conflict state:"));

        for (QPair<QString, bool> pFile : mProcessedFiles.keys())
        {
            if (not pFile.second)
            {
                OUT_ERROR(pFile.first);
            }
        }
    }

    return bHasConflicts ? CError::eMergeHasConflicts : CError::eNone;
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
    received push or branch switch (see TODO-037). \br
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
