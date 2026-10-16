
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CCloneCommand.h"
#include "../CRepository.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Constructs a CCloneCommand. \br\br
    \a pTargetRepository is the repository to create, built on the target folder \br
    \a xResponse is the clone payload built by CRemoteRepository::getCloneResponse \br
    \a sDetail receives message detail (branch name, object identifier) on failure \br
*/
CCloneCommand::CCloneCommand(CRepository* pTargetRepository, const CXMLNode& xResponse, QString& sDetail)
    : CBaseCommand(pTargetRepository)
    , m_xResponse(xResponse)
    , m_sDetail(sDetail)
{
}

/************************************************************************/

/*!
    Creates the target repository from the clone response. \br\br
    Creates the target folder with its `.aze` structure, copies branches,
    commits and objects from the response, then extracts the current
    branch tip into the working folder. \br
    Returns eNone on success, the precise failure reason otherwise. \br
*/
CError::EErrorCode CCloneCommand::execute()
{
    CError::EErrorCode eCode = validateResponse();

    if (eCode != CError::eNone)
        return eCode;

    eCode = createTargetFolder();

    if (eCode != CError::eNone)
        return eCode;

    eCode = initDatabase();

    if (eCode != CError::eNone)
        return eCode;

    eCode = storeObjects();

    if (eCode != CError::eNone)
        return eCode;

    eCode = storeCommits();

    if (eCode != CError::eNone)
        return eCode;

    eCode = storeBranches();

    if (eCode != CError::eNone)
        return eCode;

    eCode = writeGeneralInformation();

    if (eCode != CError::eNone)
        return eCode;

    eCode = writeEmptyStage();

    if (eCode != CError::eNone)
        return eCode;

    return checkoutWorkingFolder();
}

/************************************************************************/

CError::EErrorCode CCloneCommand::validateResponse()
{
    if (m_xResponse.attributes()[CStrings::s_sParamRequest] != CStrings::s_sParamClone)
    {
        m_sDetail = CError::message(CError::eCloneRequestInvalid);
        return CError::eCloneRequestInvalid;
    }

    m_lBranchNames.clear();
    m_sCurrentBranchName = m_xResponse.attributes()[CStrings::s_sParamCurrent];

    CXMLNodeList lBranches = m_xResponse.getNodesByTagName(CStrings::s_sParamBranch);

    for (const CXMLNode& xBranch : lBranches)
    {
        QString sBranchName = xBranch.attributes()[CStrings::s_sParamName];

        if (sBranchName.isEmpty())
        {
            m_sDetail = CError::message(CError::eCloneRequestInvalid);
            return CError::eCloneRequestInvalid;
        }

        if (not m_lBranchNames.contains(sBranchName))
            m_lBranchNames << sBranchName;
    }

    if (m_lBranchNames.isEmpty())
    {
        m_sDetail = CError::message(CError::eCloneRequestInvalid);
        return CError::eCloneRequestInvalid;
    }

    return CError::eNone;
}

/************************************************************************/

CError::EErrorCode CCloneCommand::createTargetFolder()
{
    QString sRootPath = m_pRepository->database()->rootPath();

    if (QDir(m_pRepository->database()->dataPath()).exists())
    {
        m_sDetail = CError::message(CError::eCouldNotClone, sRootPath);
        return CError::eCouldNotClone;
    }

    if (not QDir().mkpath(sRootPath))
    {
        m_sDetail = CError::message(CError::eCouldNotClone, sRootPath);
        return CError::eCouldNotClone;
    }

    return CError::eNone;
}

/************************************************************************/

CError::EErrorCode CCloneCommand::initDatabase()
{
    if (not m_pRepository->database()->init())
    {
        m_sDetail = CError::message(CError::eCouldNotClone, m_pRepository->database()->dataPath());
        return CError::eCouldNotClone;
    }

    return CError::eNone;
}

/************************************************************************/

CError::EErrorCode CCloneCommand::storeObjects()
{
    CXMLNodeList lObjects = m_xResponse.getNodesByTagName(CStrings::s_sParamObject);

    for (const CXMLNode& xObject : lObjects)
    {
        QString sObjectId = xObject.attributes()[CStrings::s_sParamId];
        QString sContent = xObject.value();

        if (sObjectId.isEmpty() || not sContent.startsWith(CStrings::s_sBase64Marker))
        {
            m_sDetail = CError::message(CError::eCloneRequestInvalid);
            return CError::eCloneRequestInvalid;
        }

        sContent.remove(0, CStrings::s_sBase64Marker.length());
        QByteArray baData = QByteArray::fromBase64(sContent.toLatin1());

        if (not m_pRepository->database()->storeObject(sObjectId, baData))
        {
            m_sDetail = sObjectId;
            return CError::eObjectStoreFailed;
        }
    }

    return CError::eNone;
}

/************************************************************************/

CError::EErrorCode CCloneCommand::storeCommits()
{
    m_lCommitIds.clear();

    CXMLNodeList lCommits = m_xResponse.getNodesByTagName(CStrings::s_sParamCommit);

    for (const CXMLNode& xCommit : lCommits)
    {
        QString sCommitId = xCommit.attributes()[CStrings::s_sParamId];

        if (sCommitId.isEmpty())
        {
            m_sDetail = CError::message(CError::eCloneRequestInvalid);
            return CError::eCloneRequestInvalid;
        }

        CCommit* pCommit = CCommit::fromNode(xCommit, this, sCommitId);
        QString sStoredId = m_pRepository->commitFunctions()->storeCommit(pCommit, sCommitId);
        delete pCommit;

        if (sStoredId.isEmpty())
        {
            m_sDetail = CError::message(CError::eCloneRequestInvalid);
            return CError::eCloneRequestInvalid;
        }

        if (not m_lCommitIds.contains(sStoredId))
            m_lCommitIds << sStoredId;
    }

    return CError::eNone;
}

/************************************************************************/

CError::EErrorCode CCloneCommand::storeBranches()
{
    CXMLNodeList lBranches = m_xResponse.getNodesByTagName(CStrings::s_sParamBranch);

    for (const CXMLNode& xBranch : lBranches)
    {
        QString sBranchName = xBranch.attributes()[CStrings::s_sParamName];

        CBranch* pBranch = CBranch::fromNode(xBranch, this);

        if (IS_NULL(pBranch))
        {
            m_sDetail = CError::message(CError::eCloneRequestInvalid);
            return CError::eCloneRequestInvalid;
        }

        if (not pBranch->tipCommitId().isEmpty() and not m_lCommitIds.contains(pBranch->tipCommitId()))
        {
            m_sDetail = pBranch->tipCommitId();
            delete pBranch;
            return CError::eCloneRequestInvalid;
        }

        pBranch->toNode().save(m_pRepository->database()->composeBranchFileName(sBranchName));
        delete pBranch;
    }

    if (m_sCurrentBranchName.isEmpty() || not m_lBranchNames.contains(m_sCurrentBranchName))
        m_sCurrentBranchName = m_lBranchNames.first();

    return CError::eNone;
}

/************************************************************************/

CError::EErrorCode CCloneCommand::writeGeneralInformation()
{
    m_pRepository->setCurrentBranchName(m_sCurrentBranchName);

    return m_pRepository->writeGeneralInformation();
}

/************************************************************************/

CError::EErrorCode CCloneCommand::writeEmptyStage()
{
    CCommit oStage(this);

    if (not oStage.toFile(m_pRepository->stagingCommitFileName()))
    {
        m_sDetail = CError::message(CError::eCouldNotClone, m_pRepository->stagingCommitFileName());
        return CError::eCouldNotClone;
    }

    return CError::eNone;
}

/************************************************************************/

CError::EErrorCode CCloneCommand::checkoutWorkingFolder()
{
    CBranch* pBranch = m_pRepository->database()->getBranch(m_sCurrentBranchName, this);

    if (IS_NULL(pBranch))
    {
        m_sDetail = m_sCurrentBranchName;
        return CError::eCouldNotClone;
    }

    QString sTipId = pBranch->tipCommitId();
    delete pBranch;

    if (sTipId.isEmpty())
        return CError::eNone;

    CCommit* pTipCommit = m_pRepository->commitFunctions()->loadCommit(sTipId, this);

    if (IS_NULL(pTipCommit))
    {
        m_sDetail = sTipId;
        return CError::eCouldNotClone;
    }

    QDictionary mFiles = pTipCommit->files();
    delete pTipCommit;

    for (const QString& sRelativeFileName : mFiles.keys())
    {
        QByteArray baData = m_pRepository->database()->getObject(mFiles[sRelativeFileName]);

        if (baData.isEmpty())
        {
            m_sDetail = sRelativeFileName;
            return CError::eCouldNotClone;
        }

        if (not CUtils::putBinaryFileContent(m_pRepository->database()->composeLocalFileName(sRelativeFileName), baData))
        {
            m_sDetail = sRelativeFileName;
            return CError::eCouldNotClone;
        }
    }

    return CError::eNone;
}

/************************************************************************/

} // namespace Aze
