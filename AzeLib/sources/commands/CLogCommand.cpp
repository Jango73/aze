
// std
#include <string>
#include <sstream>

// Qt
#include <QDebug>
#include <QDir>
#include <QHash>

// Application
#include "CLogCommand.h"
#include "../CRepository.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CLogCommand::CLogCommand(
        CRepository* pRepository
        , const QStringList& lFileNames
        , QString* pResult
        , bool bGraph
        , int iStart
        , int iCount
        )
    : CBaseCommand(pRepository)
    , m_lFileNames(lFileNames)
    , m_pResult(pResult)
    , m_bGraph(bGraph)
    , m_iStart(iStart)
    , m_iCount(iCount)
{
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CLogCommand::execute()
{
    bool bGetAllLog = m_iCount == 0;

    // Check presence of current branch
    if (m_pRepository->currentBranch().isNull())
    {
        m_pRepository->tellError(CError::eNoCurrentBranch);
        return CError::eNoCurrentBranch;
    }

    // Check presence of tip commit
    if (m_pRepository->tipCommit().isNull())
    {
        m_pRepository->tellError(CError::eNoTipCommit);
        return CError::eNoTipCommit;
    }

    QString sTipId = m_pRepository->currentBranch()->tipCommitId();

    if (sTipId.isEmpty())
    {
        m_pRepository->tellError(CError::eNoTipCommit);
        return CError::eNoTipCommit;
    }

    // Keep track of shown commits by identifier
    // The walk below resolves parents through the parent cache, commit files
    // are loaded only when a level is displayed
    QHash<QString, int> hAddedCommits;
    hAddedCommits[sTipId] = 0;

    // Get the starting commit identifier
    m_vBranchCommitIds << QMap<int, QString>();
    m_vBranchCommitIds.last()[0] = sTipId;

    // We stop when no more branches to show
    while (true)
    {
        if (m_iStart == 0)
        {
            outputCommitLevel(m_vBranchCommitIds.last());

            if (not bGetAllLog)
            {
                m_iCount--;
                if (m_iCount == 0)
                    break;
            }
        }
        else
        {
            m_iStart--;
        }

        QMap<int, QString> mNewBranches = nextParentLevel(m_vBranchCommitIds.last(), hAddedCommits);

        m_vBranchCommitIds << mNewBranches;

        if (m_vBranchCommitIds.last().count() == 0)
            break;
    }

    m_vBranchCommitIds.clear();

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

/*!
    Collects the parent identifiers of a log level through the parent cache. \br\br
    Each parent identifier is looked up once in CCommitTreeList, so already
    visited ancestries perform no file access. Identifiers present in
    \a hAddedCommits are skipped and new ones are recorded there. \br
    \a mCommitLevel maps branch indexes to commit identifiers of the level \br
    \a hAddedCommits tracks the identifiers already placed in the walk \br
*/
QMap<int, QString> CLogCommand::nextParentLevel(const QMap<int, QString>& mCommitLevel, QHash<QString, int>& hAddedCommits)
{
    QMap<int, QString> mNewBranches;
    int iNewBranchIndex = 0;

    for (int iIndex = 0; iIndex < mCommitLevel.count(); iIndex++)
    {
        QStringList lParentIds = m_pRepository->commitFunctions()->commitTreeList()->commitParentIds(
                    mCommitLevel[iIndex]
                    );

        for (const QString& sParentId : lParentIds)
        {
            if (sParentId.isEmpty())
                continue;

            if (not hAddedCommits.contains(sParentId))
            {
                hAddedCommits[sParentId] = 0;
                mNewBranches[iNewBranchIndex++] = sParentId;
            }
        }
    }

    return mNewBranches;
}

//-------------------------------------------------------------------------------------------------

/*!
    Outputs the commits of a log level. \br\br
    Commit files are loaded here only, after the identifier walk has selected
    the level to display, so skipped levels never touch commit files. \br
    \a mCommitLevel maps branch indexes to commit identifiers of the level \br
*/
void CLogCommand::outputCommitLevel(const QMap<int, QString>& mCommitLevel)
{
    for (int iIndex = 0; iIndex < mCommitLevel.count(); iIndex++)
    {
        CCommit* pCommit = m_pRepository->commitFunctions()->loadCommit(mCommitLevel[iIndex], this);

        if (IS_NULL(pCommit))
            continue;

        if (m_bGraph)
        {
            outputBranches(iIndex, iIndex == 0);

            (*m_pResult) += QString("%1 - %2 - %3 - %4%5")
                    .arg(pCommit->shortId())
                    .arg(PRINTABLE_ISO_DATE(pCommit->date()))
                    .arg(pCommit->author())
                    .arg(pCommit->message())
                    .arg(CStrings::s_sNewLine);
        }
        else
        {
            (*m_pResult) += QString("%1%2%3%4by: %5%6%7%8%9%10")
                    .arg(pCommit->id())
                    .arg(CStrings::s_sNewLine)
                    .arg(pCommit->date())
                    .arg(CStrings::s_sNewLine)
                    .arg(pCommit->author())
                    .arg(CStrings::s_sNewLine)
                    .arg(CStrings::s_sNewLine)
                    .arg(pCommit->message())
                    .arg(CStrings::s_sNewLine)
                    .arg(CStrings::s_sNewLine);
        }

        delete pCommit;
    }
}

//-------------------------------------------------------------------------------------------------

void CLogCommand::outputBranches(int iCurrentBranch, bool bFirstLine)
{
    for (int index = 0; index < m_vBranchCommitIds.last().count(); index++)
    {
        if (m_vBranchCommitIds.count() > 1)
        {
            if (index == iCurrentBranch)
                (*m_pResult) += QString(" * ");
            else if (
                    bFirstLine
                    && m_vBranchCommitIds[m_vBranchCommitIds.count() - 2].count() < m_vBranchCommitIds[m_vBranchCommitIds.count() - 1].count()
                    && index == m_vBranchCommitIds.last().count() - 1
                    )
            {
                (*m_pResult) += QString("\\  ");
            }
            else
            {
                (*m_pResult) += QString(" | ");
            }
        }
        else
        {
            (*m_pResult) += QString(" | ");
        }
    }
}

//-------------------------------------------------------------------------------------------------

}
