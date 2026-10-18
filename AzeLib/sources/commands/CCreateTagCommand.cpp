
// Application
#include "CCreateTagCommand.h"
#include "../objects/CTag.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CCreateTagCommand. \br\br
    \a pRepository is the repository holding the tags \br
    \a sName is the name of the tag to create \br
*/
CCreateTagCommand::CCreateTagCommand(CRepository* pRepository, const QString& sName)
    : CCreateBranchCommand(pRepository, sName)
{
}

/************************************************************************/

/*!
    Creates the tag object. \br\br
    The inherited execution stores it from the current branch tip. \br
*/
CBranch* CCreateTagCommand::createBranchObject()
{
    return new CTag(this);
}

/************************************************************************/

} // namespace Aze
