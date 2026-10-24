
// Qt
#include <QDir>

// Application
#include "CListBranchesCommand.h"
#include "../CRepository.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CListBranchesCommand. \br\br
    \a pRepository is the repository holding the branches \br
    \a pNames receives the sorted branch names, tags excluded \br
*/
CListBranchesCommand::CListBranchesCommand(CRepository* pRepository, QStringList* pNames)
    : CBaseCommand(pRepository)
    , m_pNames(pNames)
{
}

/************************************************************************/

/*!
    Lists the local branches into \a m_pNames. \br\br
    Tags share the branch folder but are never listed here. Names are
    sorted case-insensitively. Returns eGeneric when \a m_pNames is null. \br
*/
CError::EErrorCode CListBranchesCommand::execute()
{
    if (IS_NULL(m_pNames))
    {
        m_pRepository->tellError(CError::eGeneric);
        return CError::eGeneric;
    }

    m_pNames->clear();

    QStringList lFilters;
    lFilters << QString("*.%1").arg(CStrings::s_sCompressedXMLExtension);

    QDir tBranchFolder(m_pRepository->database()->branchPath());
    QFileInfoList lFiles = tBranchFolder.entryInfoList(lFilters, QDir::Files | QDir::NoSymLinks, QDir::Name | QDir::IgnoreCase);

    for (const QFileInfo& tFile : lFiles)
    {
        QString sName = CUtils::fileNameWithoutExtension(tFile.fileName(), CStrings::s_sCompressedXMLExtension);

        CBranch* pBranch = m_pRepository->database()->getBranch(sName, nullptr);

        if (acceptsType(pBranch))
            (*m_pNames) << sName;

        delete pBranch;
    }

    m_pNames->sort(Qt::CaseInsensitive);

    return CError::eNone;
}

/************************************************************************/

/*!
    Returns true for a listed object kind. \br\br
    Branches refuse tags sharing the name namespace and conversely. \br
*/
bool CListBranchesCommand::acceptsType(const CBranch* pBranch) const
{
    return IS_NOT_NULL(pBranch) && not pBranch->isTag();
}

/************************************************************************/

} // namespace Aze
