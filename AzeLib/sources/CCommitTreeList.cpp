
// Qt
#include <QDebug>
#include <QDir>

// qt-plus
#include "File/CDirectoryListing.h"

// Application
#include "CCommitTreeList.h"
#include "CEnums.h"
#include "CStrings.h"
#include "CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CCommitTreeList::CCommitTreeList(CDatabase* pDatabase, QObject* parent, bool bSilent, bool bDebug)
    : QObject(parent)
    , m_pDatabase(pDatabase)
    , m_bSilent(bSilent)
    , m_bDebug(bDebug)
    , m_bDirty(false)
{
    m_sFileName = m_pDatabase->composeCacheFileName(metaObject()->className());
    m_sFileName = QString("%1.%2").arg(m_sFileName).arg(CStrings::s_sCompressedXMLExtension);

    fromNode(m_sFileName);
}

//-------------------------------------------------------------------------------------------------

CCommitTreeList::CCommitTreeList(const CCommitTreeList& target)
    : QObject(target.parent())
{
    *this = target;
}

//-------------------------------------------------------------------------------------------------

CCommitTreeList::~CCommitTreeList()
{
    toNode().save(m_sFileName);
}

//-------------------------------------------------------------------------------------------------

CCommitTreeList& CCommitTreeList::operator = (const CCommitTreeList& target)
{
    m_pDatabase = target.m_pDatabase;
    m_bSilent = target.m_bSilent;
    m_bDebug = target.m_bDebug;
    m_mCommitParentList = target.m_mCommitParentList;
    return *this;
}

//-------------------------------------------------------------------------------------------------

bool CCommitTreeList::operator == (const CCommitTreeList& target)
{
    return m_mCommitParentList == target.m_mCommitParentList;
}

//-------------------------------------------------------------------------------------------------

// TODO : save and load in binary format
CXMLNode CCommitTreeList::toNode()
{
    CXMLNode xReturnValue(CStrings::s_sParamCommitTreeList);

    QStringList lCommitParentList;

    for (const QString& sKey : m_mCommitParentList.keys())
    {
        lCommitParentList << CUtils::packFileAndId(sKey, m_mCommitParentList[sKey].join(":"));
    }

    xReturnValue.attributes()[CStrings::s_sParamCommitParentList] = lCommitParentList.join("|");

    return xReturnValue;
}

//-------------------------------------------------------------------------------------------------

void CCommitTreeList::fromNode(const CXMLNode& xNode)
{
    m_mCommitParentList.clear();

    QStringList lCommitParentList = xNode.attributes()[CStrings::s_sParamCommitParentList].split("|");

    for (const QString& sItem : lCommitParentList)
    {
        QString sId;
        QString sParents;
        CUtils::unpackFileAndId(sItem, sId, sParents);

        m_mCommitParentList[sId] = sParents.split(":");
    }
}

//-------------------------------------------------------------------------------------------------

QList<CCommit*> CCommitTreeList::commitParentList(const CCommit* pCommit, QObject* parent)
{
    QList<CCommit*> lParents;

    for (QString sParentId : pCommit->parents())
    {
        QString sCommitFileName = m_pDatabase->composeCommitFileName(sParentId);
        lParents << CCommit::fromFile(sCommitFileName, parent, sParentId);
    }

    return lParents;
}

//-------------------------------------------------------------------------------------------------

const QStringList& CCommitTreeList::commitParentIds(const QString& sCommitId)
{
    if (m_mCommitParentList.contains(sCommitId))
    {
        return m_mCommitParentList[sCommitId];
    }

    CCommit* pCommit = CCommit::fromNode(CXMLNode::load(m_pDatabase->composeCommitFileName(sCommitId)), nullptr, sCommitId);
    QStringList lParents = pCommit->parents();
    delete pCommit;

    m_mCommitParentList[sCommitId] = lParents;

    return m_mCommitParentList[sCommitId];
}

//-------------------------------------------------------------------------------------------------

}
