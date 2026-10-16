
// Application
#include "CFile.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CFile::CFile(QObject* parent)
    : QObject(parent)
    , m_eStatus(CEnums::eLoose)
{
}

//-------------------------------------------------------------------------------------------------

CFile::~CFile()
{
}

//-------------------------------------------------------------------------------------------------

CFile::CFile(const CFile& target)
    : QObject(target.parent())
{
    *this = target;
}

//-------------------------------------------------------------------------------------------------

CFile& CFile::operator = (const CFile& target)
{
    if (this == &target)
        return *this;

    m_sRelativeName = target.m_sRelativeName;
    m_eStatus = target.m_eStatus;

    return *this;
}

//-------------------------------------------------------------------------------------------------

} // namespace Aze
