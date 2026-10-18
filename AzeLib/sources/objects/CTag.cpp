
// Application
#include "CTag.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CTag::CTag(QObject* parent)
    : CBranch(parent)
{
    m_eType = CEnums::eTag;
}

//-------------------------------------------------------------------------------------------------

CTag::~CTag()
{
}

//-------------------------------------------------------------------------------------------------

/*!
    Sets the tip commit identifier to \a sTipCommitId. \br\br
    The tip is written once at creation, later moves are refused
    as tags never move. Deserialization bypasses this guard through
    the base class setter. \br
*/
void CTag::setTipCommitId(const QString& sTipCommitId)
{
    if (m_sTipCommitId.isEmpty())
        CBranch::setTipCommitId(sTipCommitId);
}

//-------------------------------------------------------------------------------------------------

} // namespace Aze
