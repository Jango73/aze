
// Application
#include "CError.h"
#include "CStrings.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

/*!
    Returns the message template for \a eCode without detail substitution.
*/
QString CError::message(EErrorCode eCode)
{
    switch (eCode)
    {
    case eNone: return tr("No error.");
    case eCouldNotAddFiles: return tr("Could not add files.");
    case eCouldNotAmend: return tr("Could not amend.");
    case eCouldNotCommit: return tr("Could not commit.");
    case eCouldNotCreateBranch: return tr("Could not create branch.");
    case eCouldNotDeleteBranch: return tr("Could not delete branch.");
    case eCouldNotMerge: return tr("Merge failed.");
    case eCouldNotMoveFiles: return tr("Could not move files.");
    case eCouldNotPatch: return tr("Could not patch.");
    case eCouldNotPopStash: return tr("Stash pop failed.");
    case eCouldNotReadStage: return tr("Could not read stage.");
    case eCouldNotRemoveFiles: return tr("Could not remove files.");
    case eCouldNotRestoreStash: return tr("Could not restore stash.");
    case eCouldNotRevertFiles: return tr("Could not revert files.");
    case eCouldNotSaveStash: return tr("Stash save failed.");
    case eCouldNotSetCurrentBranch: return tr("Could not set current branch.");
    case eCouldNotUnstageFiles: return tr("Could not unstage files.");
    case eCouldNotWriteCurrentBranch: return tr("Could not write current branch.");
    case eCouldNotWriteGeneralInformation: return tr("Could not write general information.");
    case eCouldNotWriteStage: return tr("Could not write stage.");
    case eNoBranchNameGiven: return tr("No branch name specified.");
    case eNoFileNameGiven: return tr("No file name specified.");
    case eNoRemoteHostNameGiven: return tr("No remote host name specified.");
    case eNotARepository: return tr("This is not an Aze repository.");
    case eMergeHasConflicts: return tr("Merge has conflicts.");
    case eUnknownCommand: return tr("Unknown command.");
    case eUnknownSwitch: return tr("Unknown switch.");
    case eUnknownBranch: return tr("No such branch.");
    case eDirtyWorkingFolder: return tr("Please clean up the working folder before attempting a merge.");
    case eNothingToMerge: return tr("There are no commits to merge.");
    case eStageNotEmpty: return tr("Please commit or clear the stage before attempting a merge.");
    case eGeneric: return tr("Generic error.");
    case eNoCurrentBranch: return tr("No current branch.");
    case eNoStagingCommit: return tr("Stage information missing.");
    case eNoTipCommit: return tr("No tip commit.");
    case eNoSuchFile: return tr("No such file: %1.");
    case eBranchAlreadyExists: return tr("Branch already exists.");
    case eBranchDoesNotExist: return tr("Branch does not exist.");
    case eCannotDeleteCurrentBranch: return tr("Cannot delete the current branch. Please switch to another branch and try again.");
    case eCannotMergeSelf: return tr("You are trying to merge a commit onto itself, cannot do that.");
    case eNoCommonAncestor: return tr("Cannot find common ancestors.");
    case eNoCommitToMerge: return tr("There are no commits to merge.");
    case eCommitEmpty: return tr("No files to commit.");
    case eCouldNotRemoveFile: return tr("Could not remove file %1.");
    case eCouldNotApplyPatch: return tr("Could not patch %1.");
    case eObjectStoreFailed: return tr("Could not store object %1.");
    case ePushRequestInvalid: return tr("Invalid push request.");
    case ePushNotFastForward: return tr("Push rejected: current tip is not an ancestor of the pushed tip.");
    case eStashSaveFailed: return tr("Stash save failed.");
    case eStashPopFailed: return tr("Stash pop failed.");
    case eNothingToStash: return tr("There is nothing to stash.");
    case eNetworkError: return tr("Network error: %1.");
    case eInvalidResponse: return tr("Invalid server response.");
    case eUnknownAction: return tr("Unknown action: %1.");
    case eEmptyRequest: return tr("Empty request.");
    case eCouldNotClone: return tr("Could not clone repository.");
    case eCloneRequestInvalid: return tr("Invalid clone response.");
    case eCannotDeleteUnmergedBranch: return tr("Cannot delete unmerged branch.");
    case ePullRequestInvalid: return tr("Invalid pull request.");
    case eDeleteBranchRequestInvalid: return tr("Invalid delete branch request.");
    case eInvalidHttpsConfiguration: return tr("Invalid HTTPS configuration: %1.");
    case eNotAuthorized: return tr("Not authorized.");
    case eCouldNotPrune: return tr("Could not prune.");
    case eCouldNotCreateTag: return tr("Could not create tag.");
    case eCouldNotDeleteTag: return tr("Could not delete tag.");
    case eNoTagNameGiven: return tr("No tag name specified.");
    case eTagAlreadyExists: return tr("Tag already exists.");
    case eTagDoesNotExist: return tr("Tag does not exist.");
    case eTagMoveRefused: return tr("Cannot move tag.");
    }

    return tr("Generic error.");
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns the message for \a eCode with \a sDetail applied to the %1 placeholder.
*/
QString CError::message(EErrorCode eCode, const QString& sDetail)
{
    QString sTemplate = message(eCode);

    if (sDetail.isEmpty())
        return sTemplate;

    if (sTemplate.contains(QStringLiteral("%1")))
        return sTemplate.arg(sDetail);

    if (sTemplate.endsWith(QStringLiteral(".")))
        return QString(QStringLiteral("%1 Detail: %2.")).arg(sTemplate.left(sTemplate.length() - 1), sDetail);

    return QString(QStringLiteral("%1 Detail: %2")).arg(sTemplate, sDetail);
}

//-------------------------------------------------------------------------------------------------

/*!
    Returns the message for \a eCode with \a sDetailOne and \a sDetailTwo applied.
*/
QString CError::message(EErrorCode eCode, const QString& sDetailOne, const QString& sDetailTwo)
{
    QString sTemplate = message(eCode);

    if (sDetailOne.isEmpty() && sDetailTwo.isEmpty())
        return sTemplate;

    if (sTemplate.contains(QStringLiteral("%1")) && sTemplate.contains(QStringLiteral("%2")))
        return sTemplate.arg(sDetailOne, sDetailTwo);

    return message(eCode, sDetailOne);
}

//-------------------------------------------------------------------------------------------------

/*!
    Builds a result node for \a eCode with optional \a sDetail.
    Success codes produce result=success, other codes produce result=error with code and message.
*/
CXMLNode CError::toResultNode(EErrorCode eCode, const QString& sDetail)
{
    CXMLNode xResult;

    if (isOk(eCode) && eCode != eMergeHasConflicts)
    {
        xResult.attributes()[CStrings::s_sParamResult] = CStrings::s_sParamSuccess;
        return xResult;
    }

    xResult.attributes()[CStrings::s_sParamResult] = CStrings::s_sParamError;
    xResult.attributes()[CStrings::s_sParamCode] = QString::number(toExitCode(eCode));
    xResult.attributes()[CStrings::s_sParamMessage] = message(eCode, sDetail);

    return xResult;
}

//-------------------------------------------------------------------------------------------------

/*!
    Parses \a xNode built by toResultNode and returns the contained code.
    An empty node maps to eEmptyRequest, a malformed node maps to eInvalidResponse.
*/
CError::EErrorCode CError::fromResultNode(const CXMLNode& xNode)
{
    if (xNode.isEmpty())
        return eEmptyRequest;

    QString sResult = xNode.attributes().value(CStrings::s_sParamResult);

    if (sResult == CStrings::s_sParamSuccess)
        return eNone;

    if (sResult != CStrings::s_sParamError)
        return eInvalidResponse;

    bool bOk = false;
    int iCode = xNode.attributes().value(CStrings::s_sParamCode).toInt(&bOk);

    if (bOk == false)
        return eGeneric;

    if (iCode >= static_cast<int>(eNone) && iCode <= static_cast<int>(eTagMoveRefused))
        return static_cast<EErrorCode>(iCode);

    return eGeneric;
}

//-------------------------------------------------------------------------------------------------

} // namespace Aze
