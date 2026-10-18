
// Application
#include "CDeleteTagCommand.h"
#include "../objects/CBranch.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CDeleteTagCommand. \br\br
    \a pRepository is the repository holding the tags \br
    \a sName is the name of the tag to delete \br
    \a bForce deletes a tag not merged into the current branch \br
*/
CDeleteTagCommand::CDeleteTagCommand(CRepository* pRepository, const QString& sName, bool bForce)
    : CDeleteBranchCommand(pRepository, sName, bForce)
{
}

/************************************************************************/

/*!
    Returns the missing tag code. \br\br
*/
CError::EErrorCode CDeleteTagCommand::missingCode() const
{
    return CError::eTagDoesNotExist;
}

/************************************************************************/

/*!
    Returns true for a tag object. \br\br
    Tags refuse branches sharing the name namespace and conversely. \br
*/
bool CDeleteTagCommand::acceptsType(const CBranch* pBranch) const
{
    return IS_NOT_NULL(pBranch) && pBranch->isTag();
}

/************************************************************************/

} // namespace Aze
