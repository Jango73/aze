
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CDiffCommand.h"
#include "../CRepository.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CDiffCommand::CDiffCommand(CRepository* pRepository, const QString& sFirst, const QString& sSecond, QString* pResult)
    : CBaseCommand(pRepository)
    , m_sFirst(sFirst)
    , m_sSecond(sSecond)
    , m_pResult(pResult)
{
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CDiffCommand::execute()
{
    QString sObject1 = m_sFirst;
    QString sObject2 = m_sSecond;
    int iDelta1 = 0;
    int iDelta2 = 0;
    bool bWorkOnCommits = false;

    // Check presence of current branch
    if (m_pRepository->currentBranch().isNull())
    {
        m_pRepository->tellError(CError::eNoCurrentBranch);
        return CError::eNoCurrentBranch;
    }

    if (m_pRepository->tipCommit().isNull())
    {
        m_pRepository->tellError(CError::eNoTipCommit);
        return CError::eNoTipCommit;
    }

    if (sObject1.isEmpty() && sObject2.isEmpty())
    {
        sObject1 = m_pRepository->currentBranch()->tipCommitId();
    }

    // Get deltas if any
    sObject1 = m_pRepository->processDeltas(sObject1, iDelta1);
    sObject2 = m_pRepository->processDeltas(sObject2, iDelta2);

    // Process special keywords
    sObject1 = m_pRepository->processKeywords(sObject1);
    sObject2 = m_pRepository->processKeywords(sObject2);

    // Get file paths of both commits (does not mean they exist)
    QString sCommitFileName1 = m_pRepository->database()->composeCommitFileName(sObject1);
    QString sCommitFileName2 = m_pRepository->database()->composeCommitFileName(sObject2);

    // Work on commits if both object names are existing files
    if (QFile(sCommitFileName1).exists() && QFile(sCommitFileName2).exists())
    {
        bWorkOnCommits = true;
    }

    // Work on commits if object 1 is an existing file and object 2 is empty
    // We assume object 2 to be current stage
    if (QFile(sCommitFileName1).exists() && sObject2.isEmpty())
    {
        bWorkOnCommits = true;
    }

    // Work on commits if object 2 is an existing file and object 1 is empty
    // We assume object 1 to be current stage
    if (sObject1.isEmpty() && QFile(sCommitFileName2).exists())
    {
        bWorkOnCommits = true;
    }

    // The provided object(s) is a(are) commit(s)
    if (bWorkOnCommits)
    {
        CCommit* pCommit1 = nullptr;
        CCommit* pCommit2 = nullptr;

        if (not sObject1.isEmpty() && not sObject2.isEmpty())
        {
            pCommit1 = m_pRepository->commitFunctions()->loadCommit(sObject1, this);
            pCommit2 = m_pRepository->commitFunctions()->loadCommit(sObject2, this);
        }
        else if (not sObject1.isEmpty() && sObject2.isEmpty())
        {
            if (m_pRepository->stagingCommit().isNull())
                return CError::eNoStagingCommit;

            pCommit1 = m_pRepository->commitFunctions()->loadCommit(sObject1, this);
            pCommit2 = m_pRepository->commitFunctions()->folderAsCommit(this);
        }
        else if (sObject1.isEmpty() && not sObject2.isEmpty())
        {
            if (m_pRepository->stagingCommit().isNull())
                return CError::eNoStagingCommit;

            pCommit1 = m_pRepository->commitFunctions()->loadCommit(sObject2, this);
            pCommit2 = m_pRepository->commitFunctions()->folderAsCommit(this);
        }

        if (not IS_NULL(pCommit1) && not IS_NULL(pCommit2))
        {
            m_pRepository->commitFunctions()->diffCommits(*m_pResult, pCommit1, pCommit2, QStringList(), iDelta1, iDelta2);
        }
    }
    else
    {
        QString sFileName = sObject1;
        QString sFileName2 = m_pRepository->database()->composeLocalFileName(sFileName);

        if (not QFile(sFileName2).exists())
        {
            m_pRepository->tellError(CError::eNoSuchFile, sFileName);
            return CError::eNoSuchFile;
        }

        QString sId2 = m_pRepository->tipCommit()->files()[sObject1];

        if (sId2.isEmpty())
            return CError::eNoSuchFile;

        QString sText1 = m_pRepository->tipCommit()->fileContent(m_pRepository->database(), sFileName);
        QString sText2 = CUtils::getTextFileContent(sFileName2);

        m_pRepository->commitFunctions()->diffText(*m_pResult, sFileName, sText1, sText2);
    }

    return CError::eNone;
}

//-------------------------------------------------------------------------------------------------

}
