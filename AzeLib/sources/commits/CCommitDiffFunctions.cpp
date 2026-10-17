
// Qt
#include <QDebug>
#include <QFile>
#include <QHash>

// Application
#include "CCommitDiffFunctions.h"
#include "../CEnums.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CCommitDiffFunctions::CCommitDiffFunctions(CDatabase* pDatabase, CFileHashCache* pFileHashCache, QObject* parent, bool bSilent, bool bDebug)
    : QObject(parent)
    , m_pDatabase(pDatabase)
    , m_pFileHashCache(pFileHashCache)
    , m_bSilent(bSilent)
    , m_bDebug(bDebug)
{
}

//-------------------------------------------------------------------------------------------------

CCommitDiffFunctions::~CCommitDiffFunctions()
{
}

//-------------------------------------------------------------------------------------------------

/*!
    Appends the unified diff between two resolved commits to sOutput. \br\br
    Both commits must already be resolved by the caller, ancestor deltas are
    handled by CCommitFunctions before delegating here. \br
    \a sOutput receives the diff text \br
    \a pCommit1 is the first commit \br
    \a pCommit2 is the second commit \br
    \a lIgnoreFiles are the file names excluded from the diff \br
*/
void CCommitDiffFunctions::diffCommits(QString& sOutput, CCommit* pCommit1, CCommit* pCommit2, const QStringList& lIgnoreFiles)
{
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
}

//-------------------------------------------------------------------------------------------------

/*!
    Appends the unified diff of commits in lCommitList missing from lIgnoreList to sOutput. \br\br
    \a sOutput receives the diff text \br
    \a lCommitList are the commits to diff \br
    \a lIgnoreList are the commits excluded from the diff \br
*/
void CCommitDiffFunctions::diffCommitLists(QString& sOutput, const QList<CCommit*>& lCommitList, const QList<CCommit*>& lIgnoreList)
{
    if (m_bDebug)
    {
        OUT_DEBUG("--------------------");
        OUT_DEBUG("To list:");
        for (CCommit* pCommit : lCommitList)
            OUT_DEBUG(QString("%1 %2").arg(pCommit->shortId()).arg(pCommit->message()));
        OUT_DEBUG("--------------------");
        OUT_DEBUG("From list:");
        for (CCommit* pCommit : lIgnoreList)
            OUT_DEBUG(QString("%1 %2").arg(pCommit->shortId()).arg(pCommit->message()));
        OUT_DEBUG("--------------------");
    }

    for (int index = 1; index < lIgnoreList.count(); index++)
    {
        CCommit* pCommit1 = lIgnoreList[index - 1];
        CCommit* pCommit2 = lIgnoreList[index];

        bool bFound = false;

        for (CCommit* pCommit : lCommitList)
            if (CUtils::sameIds(pCommit->id(), pCommit2->id()))
            {
                bFound = true;
                break;
            }

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

/*!
    Appends the unified diff between two files on the file system to sOutput. \br\br
    \a sOutput receives the diff text \br
    \a sFile1 is the first file name \br
    \a sFile2 is the second file name \br
*/
void CCommitDiffFunctions::diffFiles(QString& sOutput, const QString& sFile1, const QString& sFile2)
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

/*!
    Returns the difference between two text files. \br\br
    \a sOutput receives the diff text \br
    \a sFileName is the file name shown in the diff header \br
    \a sText1 is the first text content \br
    \a sText2 is the second text content \br
*/
void CCommitDiffFunctions::diffText(QString& sOutput, const QString& sFileName, const QString& sText1, const QString& sText2)
{
    QString sDiffText = CUtils::unifiedDiff(sText1, sText2);

    if (not sDiffText.isEmpty())
    {
        sOutput += CUtils::fileDiffHeader(sFileName, sFileName);
        sOutput += sDiffText;
    }
}

//-------------------------------------------------------------------------------------------------

/*!
    Applies a diff to the working directory. \br\br
    Patched files transit through the merge folder, then move to the working
    folder. \br
    \a sFullDiff is the full diff text to apply \br
    \a bAddToStage stages the patched files when true \br
    \a pStagingCommit receives the staged files, may be null when not staging \br
*/
CError::EErrorCode CCommitDiffFunctions::applyDiff(const QString& sFullDiff, bool bAddToStage, CCommit* pStagingCommit)
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

    m_pFileHashCache->clear();

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

} // namespace Aze
