
// Qt
#include <QDebug>
#include <QFile>
#include <QSet>

// Application
#include "CCommitMergeFunctions.h"
#include "../CEnums.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CCommitMergeFunctions::CCommitMergeFunctions(CDatabase* pDatabase, CFileHashCache* pFileHashCache, QObject* parent, bool bSilent, bool bDebug)
    : QObject(parent)
    , m_pDatabase(pDatabase)
    , m_pFileHashCache(pFileHashCache)
    , m_bSilent(bSilent)
    , m_bDebug(bDebug)
{
}

//-------------------------------------------------------------------------------------------------

CCommitMergeFunctions::~CCommitMergeFunctions()
{
}

//-------------------------------------------------------------------------------------------------

/*!
    Do a 3-way merge and add (if requested) the modified files to the stage. \br\br
    Files changed on a single side are taken as is, deletions included. Files
    changed on both sides go through a text three-way merge with conflict
    markers, deletions against modifications always conflict. \br
    \a pBaseCommit is the common ancestor commit \br
    \a pFromTipCommit is the tip commit of the merged branch \br
    \a pToTipCommit is the tip commit of the target branch \br
    \a bAddToStage stages the merged files when true \br
    \a pStagingCommit receives the staged files, may be null when not staging \br
*/
CError::EErrorCode CCommitMergeFunctions::threeWayMerge(CCommit* pBaseCommit, CCommit* pFromTipCommit, CCommit* pToTipCommit, bool bAddToStage, CCommit* pStagingCommit)
{
    bool bHasConflicts = false;

    if (IS_NULL(pBaseCommit) || IS_NULL(pFromTipCommit) || IS_NULL(pToTipCommit))
        return CError::eCouldNotMerge;

    QDictionary mBaseFiles = pBaseCommit->files();
    QDictionary mFromFiles = pFromTipCommit->files();
    QDictionary mToFiles = pToTipCommit->files();

    QSet<QString> sAllFileNames;
    for (const QString& sName : mBaseFiles.keys())
        sAllFileNames.insert(sName);
    for (const QString& sName : mFromFiles.keys())
        sAllFileNames.insert(sName);
    for (const QString& sName : mToFiles.keys())
        sAllFileNames.insert(sName);

    // Keep track of merged files
    QMap<QPair<QString, bool>, QString> mProcessedFiles;    // Maps relative file names to IDs

    for (const QString& sFileName : sAllFileNames)
    {
        bool bInBase = mBaseFiles.contains(sFileName);
        bool bInFrom = mFromFiles.contains(sFileName);
        bool bInTo = mToFiles.contains(sFileName);

        QString sIdBase = mBaseFiles.value(sFileName);
        QString sIdFrom = mFromFiles.value(sFileName);
        QString sIdTo = mToFiles.value(sFileName);

        bool bChangedFrom = (bInFrom != bInBase) || (bInFrom && bInBase && sIdFrom != sIdBase);
        bool bChangedTo = (bInTo != bInBase) || (bInTo && bInBase && sIdTo != sIdBase);

        if (not bChangedFrom && not bChangedTo)
            continue;

        QString sFullLocalName = m_pDatabase->composeLocalFileName(sFileName);

        // Changed only on the from side: take the from side, deletions included
        if (bChangedFrom && not bChangedTo)
        {
            if (bInFrom)
            {
                QString sFromContent = QString(pFromTipCommit->fileContent(m_pDatabase, sFileName));
                CUtils::putTextFileContent(sFullLocalName, sFromContent);
                mProcessedFiles[QPair<QString, bool>(sFileName, true)] = sIdFrom;
            }
            else
            {
                QFile::remove(sFullLocalName);
                mProcessedFiles[QPair<QString, bool>(sFileName, true)] = "";
            }
            continue;
        }

        // Changed only on the to side: take the to side, deletions included
        if (not bChangedFrom && bChangedTo)
        {
            if (bInTo)
            {
                QString sToContent = QString(pToTipCommit->fileContent(m_pDatabase, sFileName));
                CUtils::putTextFileContent(sFullLocalName, sToContent);
                mProcessedFiles[QPair<QString, bool>(sFileName, true)] = sIdTo;
            }
            else
            {
                QFile::remove(sFullLocalName);
                mProcessedFiles[QPair<QString, bool>(sFileName, true)] = "";
            }
            continue;
        }

        // Changed on both sides
        // Deleted on both sides: remove the file without conflict
        if (not bInFrom && not bInTo)
        {
            QFile::remove(sFullLocalName);
            mProcessedFiles[QPair<QString, bool>(sFileName, true)] = "";
            continue;
        }

        // Changed identically on both sides: take either side without conflict
        if (bInFrom && bInTo && sIdFrom == sIdTo)
        {
            QString sSameContent = QString(pFromTipCommit->fileContent(m_pDatabase, sFileName));
            CUtils::putTextFileContent(sFullLocalName, sSameContent);
            mProcessedFiles[QPair<QString, bool>(sFileName, true)] = sIdFrom;
            continue;
        }

        // Deleted on one side and changed on the other: conflict with markers
        if (not bInFrom || not bInTo)
        {
            QString sBaseContent = bInBase ? QString(pBaseCommit->fileContent(m_pDatabase, sFileName)) : "";
            QString sFromContent = bInFrom ? QString(pFromTipCommit->fileContent(m_pDatabase, sFileName)) : "";
            QString sToContent = bInTo ? QString(pToTipCommit->fileContent(m_pDatabase, sFileName)) : "";
            QString sNewContent;

            QString sFromMarker = QString("%1 %2").arg(pFromTipCommit->shortId()).arg(pFromTipCommit->message().left(16));
            QString sToMarker = QString("%1 %2").arg(pToTipCommit->shortId()).arg(pToTipCommit->message().left(16));

            CUtils::applyThreeWayMerge(sBaseContent, sFromMarker, sFromContent, sToMarker, sToContent, sNewContent);

            // Deletion against modification never auto merges
            bHasConflicts = true;
            CUtils::putTextFileContent(sFullLocalName, sNewContent);
            mProcessedFiles[QPair<QString, bool>(sFileName, false)] = "";
            continue;
        }

        // Modified on both sides: text three-way merge
        if (m_bDebug)
        {
            OUT_DEBUG(QString("3-way merging: %1").arg(sFileName));
        }

        QString sBaseContent = bInBase ? QString(pBaseCommit->fileContent(m_pDatabase, sFileName)) : "";
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

        CUtils::putTextFileContent(sFullLocalName, sNewContent);
        QString sId = CUtils::idFromByteArray(sNewContent.toUtf8());
        mProcessedFiles[QPair<QString, bool>(sFileName, bMergeOk)] = sId;
    }

    // Bail out if nothing changed
    // An empty merge is reported as conflicts to preserve the historical
    // bHasConflicts semantics of the caller
    if (mProcessedFiles.isEmpty())
    {
        return CError::eMergeHasConflicts;
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

    m_pFileHashCache->clear();

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

} // namespace Aze
