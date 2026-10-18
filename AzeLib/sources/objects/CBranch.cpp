
// Application
#include "CBranch.h"
#include "CTag.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CBranch::CBranch(QObject* parent)
    : CObject(parent)
    , m_eType(CEnums::eBranch)
    , m_sDate(QDateTime::currentDateTimeUtc().toString(Qt::ISODate))
{
}

//-------------------------------------------------------------------------------------------------

CBranch::~CBranch()
{
}

//-------------------------------------------------------------------------------------------------

CXMLNode CBranch::toNode() const
{
    CXMLNode xNode(CStrings::s_sParamBranch);

    CXMLNode xInfo(CStrings::s_sParamInfo);
    xInfo.attributes()[CStrings::s_sParamAuthor] = m_sAuthor;
    xInfo.attributes()[CStrings::s_sParamDate] = m_sDate;
    xInfo.attributes()[CStrings::s_sParamType] = enumToQString(CEnums, EBranchType, m_eType);
    xInfo.attributes()[CStrings::s_sParamRoot] = m_sRootCommitId;
    xInfo.attributes()[CStrings::s_sParamTip] = m_sTipCommitId;
    xNode << xInfo;

    return xNode;
}

//-------------------------------------------------------------------------------------------------

/*!
    Creates a branch or a tag from \a eType. \br\br
    Centralizes instantiation so stored type values always materialize
    as the matching class. \br
*/
CBranch* CBranch::create(CEnums::EBranchType eType, QObject* parent)
{
    if (eType == CEnums::eTag)
        return new CTag(parent);

    return new CBranch(parent);
}

/************************************************************************/

/*!
    Returns the branch type for \a sType. \br\br
    Unknown or empty names map to eBranch so payloads from older
    versions keep their historical meaning. \br
*/
CEnums::EBranchType CBranch::typeFromString(const QString& sType)
{
    if (sType == enumToQString(CEnums, EBranchType, CEnums::eTag))
        return CEnums::eTag;

    return CEnums::eBranch;
}

/************************************************************************/

CBranch* CBranch::fromNode(const CXMLNode& xNode, QObject* parent)
{
    QDictionary mFiles;
    QString sId;
    QString sFilePath;

    CXMLNode xInfo = xNode.getNodeByTagName(CStrings::s_sParamInfo);

    QString sType = xInfo.attributes()[CStrings::s_sParamType];

    CBranch* pBranch = create(typeFromString(sType), parent);

    if (not xNode.isEmpty())
    {
        pBranch->setAuthor(xInfo.attributes()[CStrings::s_sParamAuthor]);
        pBranch->setDate(xInfo.attributes()[CStrings::s_sParamDate]);
        pBranch->setRootCommitId(xInfo.attributes()[CStrings::s_sParamRoot]);
        pBranch->setTipCommitId(xInfo.attributes()[CStrings::s_sParamTip]);
    }

    return pBranch;
}

//-------------------------------------------------------------------------------------------------

CBranch* CBranch::fromFile(const QString& sFileName, QObject* parent)
{
    return fromNode(CXMLNode::load(sFileName), parent);
}

//-------------------------------------------------------------------------------------------------

} // namespace Aze
