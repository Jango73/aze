
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

    // Keep track of shown commits
    QHash<QString, int> hAddedCommits;

    // Get the starting commit
    m_vBranches << QMap<int, CCommit*>();
    m_vBranches.last()[0] = m_pRepository->tipCommit()->clone(this);

    // We stop when no more branches to show
    while (true)
    {
        if (m_iStart == 0)
        {
            // Iterate through each branch
            for (int index = 0; index < m_vBranches.last().count(); index++)
            {
                CCommit* pCommit = m_vBranches.last()[index];

                if (m_bGraph)
                {
                    outputBranches(index, index == 0);

                    (*m_pResult) += QString("%1 - %2 - %3 - %4%5")
                            .arg(pCommit->shortId())
                            .arg(PRINTABLE_ISO_DATE(m_vBranches.last()[index]->date()))
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
            }

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

        QMap<int, CCommit*> mNewBranches;
        int iNewBranchIndex = 0;

        for (int index = 0; index < m_vBranches.last().count(); index++)
        {
            QList<CCommit*> lParents = m_pRepository->commitFunctions()->commitTreeList()->commitParentList(
                        m_vBranches.last()[index],
                        this
                        );

            for (CCommit* pCommit : lParents)
            {
                if (not hAddedCommits.contains(pCommit->id()))
                {
                    hAddedCommits[pCommit->id()] = 0;
                    mNewBranches[iNewBranchIndex++] = pCommit;
                }
                else
                {
                    delete pCommit;
                }
            }
        }

        m_vBranches << mNewBranches;

        if (m_vBranches.last().count() == 0)
            break;
    }

    for (QMap<int, CCommit*>& mLevel : m_vBranches)
    {
        qDeleteAll(mLevel.values());
        mLevel.clear();
    }

    m_vBranches.clear();

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

void CLogCommand::outputBranches(int iCurrentBranch, bool bFirstLine)
{
    for (int index = 0; index < m_vBranches.last().count(); index++)
    {
        if (m_vBranches.count() > 1)
        {
            if (index == iCurrentBranch)
                (*m_pResult) += QString(" * ");
            else if (
                    bFirstLine
                    && m_vBranches[m_vBranches.count() - 2].count() < m_vBranches[m_vBranches.count() - 1].count()
                    && index == m_vBranches.last().count() - 1
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
