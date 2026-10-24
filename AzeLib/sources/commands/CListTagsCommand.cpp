
// Application
#include "CListTagsCommand.h"
#include "../objects/CBranch.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CListTagsCommand. \br\br
    \a pRepository is the repository holding the tags \br
    \a pNames receives the sorted tag names, branches excluded \br
*/
CListTagsCommand::CListTagsCommand(CRepository* pRepository, QStringList* pNames)
    : CListBranchesCommand(pRepository, pNames)
{
}

/************************************************************************/

/*!
    Returns true for a listed object kind. \br\br
    Tags refuse branches sharing the name namespace and conversely. \br
*/
bool CListTagsCommand::acceptsType(const CBranch* pBranch) const
{
    return IS_NOT_NULL(pBranch) && pBranch->isTag();
}

/************************************************************************/

} // namespace Aze
