
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CBaseCommand.h"
#include "../CRepository.h"
#include "../CStrings.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CBaseCommand::CBaseCommand(CRepository* pRepository)
    : m_bCalledInternally(false)
    , m_pRepository(pRepository)
{
}

//-------------------------------------------------------------------------------------------------

CError::EErrorCode CBaseCommand::execute()
{
    return CError::eNone;
}

/************************************************************************/

/*!
    Stores the object nodes carried by \a xPayload into the repository database. \br\br
    \a sDetail receives the object identifier when storing fails, or the
    invalid payload message when a node is malformed. \br
    \a eInvalidCode is returned for a malformed node so pull and push keep
    their own invalid payload code. \br
*/
CError::EErrorCode CBaseCommand::storePayloadObjects(const CXMLNode& xPayload, QString& sDetail, CError::EErrorCode eInvalidCode)
{
    CXMLNodeList lObjects = xPayload.getNodesByTagName(CStrings::s_sParamObject);

    for (const CXMLNode& xObject : lObjects)
    {
        QString sObjectId = xObject.attributes()[CStrings::s_sParamId];
        QString sContent = xObject.value();

        if (sObjectId.isEmpty() || not sContent.startsWith(CStrings::s_sBase64Marker))
        {
            sDetail = CError::message(eInvalidCode);
            return eInvalidCode;
        }

        sContent.remove(0, CStrings::s_sBase64Marker.length());
        QByteArray baData = QByteArray::fromBase64(sContent.toLatin1());

        if (not m_pRepository->database()->storeObject(sObjectId, baData))
        {
            sDetail = sObjectId;
            return CError::eObjectStoreFailed;
        }
    }

    return CError::eNone;
}

/************************************************************************/

/*!
    Stores the commit nodes carried by \a xPayload into the repository. \br\br
    Stored commit identifiers are collected into \a lCommitIds. \br
    \a sDetail receives the invalid payload message when a node is malformed. \br
    \a eInvalidCode is returned for a malformed node so pull and push keep
    their own invalid payload code. \br
*/
CError::EErrorCode CBaseCommand::storePayloadCommits(const CXMLNode& xPayload, QStringList& lCommitIds, QString& sDetail, CError::EErrorCode eInvalidCode)
{
    CXMLNodeList lCommits = xPayload.getNodesByTagName(CStrings::s_sParamCommit);

    for (const CXMLNode& xCommit : lCommits)
    {
        QString sCommitId = xCommit.attributes()[CStrings::s_sParamId];

        if (sCommitId.isEmpty())
        {
            sDetail = CError::message(eInvalidCode);
            return eInvalidCode;
        }

        CCommit* pCommit = CCommit::fromNode(xCommit, this, sCommitId);
        QString sStoredId = m_pRepository->commitFunctions()->storeCommit(pCommit, sCommitId);
        delete pCommit;

        if (sStoredId.isEmpty())
        {
            sDetail = CError::message(eInvalidCode);
            return eInvalidCode;
        }

        if (not lCommitIds.contains(sStoredId))
            lCommitIds << sStoredId;
    }

    return CError::eNone;
}

}
