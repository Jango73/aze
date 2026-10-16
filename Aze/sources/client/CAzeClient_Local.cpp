
// Qt
#include <QDebug>
#include <QDir>
#include <QDirIterator>

// qt-plus
#include <CXMLNode.h>
#include <File/CFileUtilities.h>

// Aze
#include "CUtils.h"

// Application
#include "CAzeClient.h"

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::init()
{
    return m_pRepository->init();
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::createBranch()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);
    RETURN_WHEN_FALSE(m_tArguments.m_lFilesAndIds.count() > 0, Aze::CError::eNoBranchNameGiven);

    // Create the branch
    RETURN_WHEN_ERROR(m_pRepository->createBranch(m_tArguments.m_lFilesAndIds[0]), Aze::CError::eCouldNotCreateBranch);

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::deleteBranch()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    if (m_tArguments.m_lFilesAndIds.count() == 0)
    {
        OUT_ERROR(Aze::CError::message(Aze::CError::eNoBranchNameGiven));
        return Aze::CError::eNoBranchNameGiven;
    }

    QString sBranchName = m_tArguments.m_lFilesAndIds[0];

    bool bForce = m_tArguments.m_tParser.isSet(m_tArguments.m_oForce);

    // Delete the branch, engine refuses the current branch and, without force, unmerged branches with distinct codes
    return m_pRepository->deleteBranch(sBranchName, bForce);
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::switchToBranch()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);
    RETURN_WHEN_FALSE(m_tArguments.m_lFilesAndIds.count() > 0, Aze::CError::eNoBranchNameGiven);

    bool bAllowFileDelete = m_tArguments.m_tParser.isSet(m_tArguments.m_oAllowFileDelete);

    QString sBranchName = m_tArguments.m_lFilesAndIds[0];

    if (sBranchName == m_pRepository->currentBranchName())
    {
        (*m_pOutStream) << QString(CConstants::s_sTextYouAreAlreadyOnBranch).arg(sBranchName) << "\n";
        return Aze::CError::eNone;
    }

    RETURN_WHEN_ERROR(m_pRepository->switchToBranch(sBranchName, bAllowFileDelete), Aze::CError::eCouldNotSetCurrentBranch);

    RETURN_WHEN_ERROR(m_pRepository->writeCurrentBranch(), Aze::CError::eCouldNotWriteCurrentBranch);

    RETURN_WHEN_ERROR(m_pRepository->writeGeneralInformation(), Aze::CError::eCouldNotWriteGeneralInformation);

    (*m_pOutStream) << QString(CConstants::s_sTextYouAreNowOnBranch).arg(sBranchName) << "\n";

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::status()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    QMap<Aze::CEnums::EFileStatus, bool> bVisibility;

    bVisibility[Aze::CEnums::eLoose]    = m_tArguments.m_tParser.isSet(m_tArguments.m_oLoose);
    bVisibility[Aze::CEnums::eClean]    = m_tArguments.m_tParser.isSet(m_tArguments.m_oClean);
    bVisibility[Aze::CEnums::eModified] = m_tArguments.m_tParser.isSet(m_tArguments.m_oModified);
    bVisibility[Aze::CEnums::eAdded]    = m_tArguments.m_tParser.isSet(m_tArguments.m_oAdded);
    bVisibility[Aze::CEnums::eDeleted]  = m_tArguments.m_tParser.isSet(m_tArguments.m_oDeleted);
    bVisibility[Aze::CEnums::eMissing]  = m_tArguments.m_tParser.isSet(m_tArguments.m_oMissing);
    bVisibility[Aze::CEnums::eIgnored]  = m_tArguments.m_tParser.isSet(m_tArguments.m_oIgnored);
    bVisibility[Aze::CEnums::eAll]      = m_tArguments.m_tParser.isSet(m_tArguments.m_oAll);

    processWildCards();

    QString sOutput;
    QList<Aze::CFile> lFiles = m_pRepository->fileStatus(m_tArguments.m_lFilesAndIds);

    if (not m_tArguments.m_lFilesAndIds.isEmpty())
    {
        for (QString sFullName : m_tArguments.m_lFilesAndIds)
        {
            QString sRelativeName = m_pRepository->database()->relativeFileName(sFullName);

            QList<Aze::CFile>::iterator file = std::find_if(
                        lFiles.begin(),
                        lFiles.end(),
                        [&](const Aze::CFile& f) { return f.relativeName() == sRelativeName; }
            );

            if (file != lFiles.end())
            {
                sOutput += QString("%1 %2\n")
                        .arg(Aze::CEnums::FileStatusSymbol(file->status()))
                        .arg(sRelativeName);
            }
        }
    }
    else
    {
        bool bAtLeastOneSwitchTrue = false;

        for (bool bSwitch : bVisibility.values())
            if (bSwitch)
                bAtLeastOneSwitchTrue = true;

        if (not bAtLeastOneSwitchTrue)
        {
            bVisibility[Aze::CEnums::eAdded] = true;
            bVisibility[Aze::CEnums::eDeleted] = true;
            bVisibility[Aze::CEnums::eModified] = true;
            bVisibility[Aze::CEnums::eMissing] = true;
        }

        for (Aze::CFile& file : lFiles)
        {
            QString sRelativeName = file.relativeName();

            if (bVisibility[Aze::CEnums::eAll] || bVisibility[file.status()])
            {
                sOutput += QString("%1 %2\n")
                        .arg(Aze::CEnums::FileStatusSymbol(file.status()))
                        .arg(sRelativeName);
            }
        }
    }

    if (sOutput.isEmpty())
    {
        sOutput = QString(CConstants::s_sAllFilesAreClean)
                .arg(m_pRepository->currentBranchName())
                + QString("\n");
    }
    else
    {
        sOutput = QString(CConstants::s_sStatusOfFiles)
                .arg(m_pRepository->currentBranchName())
                + QString("\n")
                + sOutput;
    }

    (*m_pOutStream) << sOutput;

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::stage()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    if (m_tArguments.m_lFilesAndIds.count() == 0)
    {
        OUT_ERROR(Aze::CError::message(Aze::CError::eNoFileNameGiven));
        return Aze::CError::eNoFileNameGiven;
    }

    processWildCards();

    // Update the stage
    RETURN_WHEN_ERROR(m_pRepository->stage(m_tArguments.m_lFilesAndIds), Aze::CError::eCouldNotAddFiles);
    RETURN_WHEN_ERROR(m_pRepository->writeStage(), Aze::CError::eCouldNotWriteStage);

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::unstage()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    if (m_tArguments.m_lFilesAndIds.count() == 0)
    {
        OUT_ERROR(Aze::CError::message(Aze::CError::eNoFileNameGiven));
        return Aze::CError::eNoFileNameGiven;
    }

    processWildCards();

    // Update the stage
    RETURN_WHEN_ERROR(m_pRepository->unstage(m_tArguments.m_lFilesAndIds), Aze::CError::eCouldNotUnstageFiles);
    RETURN_WHEN_ERROR(m_pRepository->writeStage(), Aze::CError::eCouldNotWriteStage);

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::revert()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    bool bAllowFileDelete = m_tArguments.m_tParser.isSet(m_tArguments.m_oAllowFileDelete);

    RETURN_WHEN_FALSE(m_tArguments.m_lFilesAndIds.count() > 0, Aze::CError::eNoFileNameGiven);

    processWildCards();

    // Revert specified files
    RETURN_WHEN_ERROR(m_pRepository->revert(m_tArguments.m_lFilesAndIds, bAllowFileDelete), Aze::CError::eCouldNotRevertFiles);

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::move()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    RETURN_WHEN_FALSE(m_tArguments.m_lFilesAndIds.count() > 1, Aze::CError::eNoFileNameGiven);

    // Expand source wildcards, the last entry stays the literal target
    QString sTarget = m_tArguments.m_lFilesAndIds.takeLast();
    processWildCards();
    m_tArguments.m_lFilesAndIds << sTarget;

    // Move the specified files
    RETURN_WHEN_ERROR(m_pRepository->move(m_tArguments.m_lFilesAndIds), Aze::CError::eCouldNotMoveFiles);

    // Update the stage
    RETURN_WHEN_ERROR(m_pRepository->writeStage(), Aze::CError::eCouldNotWriteStage);

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::remove()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    RETURN_WHEN_FALSE(m_tArguments.m_lFilesAndIds.count() > 0, Aze::CError::eNoFileNameGiven);

    processWildCards();

    // Remove the specified files
    RETURN_WHEN_ERROR(m_pRepository->remove(m_tArguments.m_lFilesAndIds), Aze::CError::eCouldNotRemoveFiles);

    // Update the stage
    RETURN_WHEN_ERROR(m_pRepository->writeStage(), Aze::CError::eCouldNotWriteStage);

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::commit()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    QString sAuthor = m_tArguments.m_tParser.value(m_tArguments.m_oAuthor);
    QString sMessage = m_tArguments.m_tParser.value(m_tArguments.m_oMessage);

    // Commit
    RETURN_WHEN_ERROR(m_pRepository->commit(sAuthor, sMessage), Aze::CError::eCouldNotCommit);
    RETURN_WHEN_ERROR(m_pRepository->writeCurrentBranch(), Aze::CError::eCouldNotWriteCurrentBranch);

    // Clear the stage file
    RETURN_WHEN_ERROR(m_pRepository->clearStage(), Aze::CError::eCouldNotWriteStage);
    RETURN_WHEN_ERROR(m_pRepository->writeStage(), Aze::CError::eCouldNotWriteStage);

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::amend()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    QString sAuthor = m_tArguments.m_tParser.value(m_tArguments.m_oAuthor);
    QString sMessage = m_tArguments.m_tParser.value(m_tArguments.m_oMessage);

    // Rewrite the tip commit
    RETURN_WHEN_ERROR(m_pRepository->amend(sAuthor, sMessage), Aze::CError::eCouldNotAmend);
    RETURN_WHEN_ERROR(m_pRepository->writeCurrentBranch(), Aze::CError::eCouldNotWriteCurrentBranch);

    // Clear the stage file
    RETURN_WHEN_ERROR(m_pRepository->clearStage(), Aze::CError::eCouldNotWriteStage);
    RETURN_WHEN_ERROR(m_pRepository->writeStage(), Aze::CError::eCouldNotWriteStage);

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::cleanUp()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    // Revert all files
    m_tArguments.m_lFilesAndIds.clear();
    m_tArguments.m_lFilesAndIds << "*";
    processWildCards();

    // Clear the stage file and revert the working folder
    RETURN_WHEN_ERROR(m_pRepository->cleanUp(m_tArguments.m_lFilesAndIds, true), Aze::CError::eCouldNotRevertFiles);

    RETURN_WHEN_ERROR(m_pRepository->writeStage(), Aze::CError::eCouldNotWriteStage);

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::log()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    bool bGraph = m_tArguments.m_tParser.isSet(m_tArguments.m_oGraph);
    int iStart = m_tArguments.m_tParser.value(m_tArguments.m_oStart).toInt();
    int iCount = m_tArguments.m_tParser.value(m_tArguments.m_oCount).toInt();

    (*m_pOutStream) << m_pRepository->log(m_tArguments.m_lFilesAndIds, bGraph, iStart, iCount);
    m_pOutStream->flush();

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::diff()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    QString sFirst = m_tArguments.m_lFilesAndIds.isEmpty() ? "" : m_tArguments.m_lFilesAndIds.takeFirst();
    QString sSecond = m_tArguments.m_lFilesAndIds.isEmpty() ? "" : m_tArguments.m_lFilesAndIds.takeFirst();

    (*m_pOutStream) << Aze::CUtils::printableUnifiedDiff(m_pRepository->diff(sFirst, sSecond));
    m_pOutStream->flush();

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::merge()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);
    RETURN_WHEN_FALSE(m_tArguments.m_lFilesAndIds.count() > 0, Aze::CError::eNoBranchNameGiven);

    QString sBranchName = m_tArguments.m_lFilesAndIds.takeFirst();

    if (sBranchName == m_pRepository->currentBranchName())
    {
        (*m_pOutStream) << Aze::CError::message(Aze::CError::eCannotMergeSelf) << "\n";
        m_pOutStream->flush();
        return Aze::CError::eCannotMergeSelf;
    }

    bool bHasConflicts = false;
    bool bAllowFastForward = not m_tArguments.m_tParser.isSet(m_tArguments.m_oNoFastForward);
    Aze::CError::EErrorCode eMergeCode = m_pRepository->merge(sBranchName, bHasConflicts, bAllowFastForward);

    if (eMergeCode != Aze::CError::eNone && eMergeCode != Aze::CError::eMergeHasConflicts)
        return eMergeCode;

    RETURN_WHEN_ERROR(m_pRepository->writeCurrentBranch(), Aze::CError::eCouldNotWriteCurrentBranch);
    RETURN_WHEN_ERROR(m_pRepository->writeStage(), Aze::CError::eCouldNotWriteStage);

    return bHasConflicts ? Aze::CError::eMergeHasConflicts : Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::saveStash()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    QString sStashName;
    if (m_tArguments.m_lFilesAndIds.count() > 0)
        sStashName = m_tArguments.m_lFilesAndIds.takeFirst();

    RETURN_WHEN_ERROR(m_pRepository->saveStash(sStashName), Aze::CError::eCouldNotSaveStash);

    RETURN_WHEN_ERROR(m_pRepository->writeStage(), Aze::CError::eCouldNotWriteStage);

    RETURN_WHEN_ERROR(m_pRepository->writeGeneralInformation(), Aze::CError::eCouldNotWriteGeneralInformation);

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::popStash()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    QString sStashId;
    if (m_tArguments.m_lFilesAndIds.count() > 0)
        sStashId = m_tArguments.m_lFilesAndIds.takeFirst();

    RETURN_WHEN_ERROR(m_pRepository->popStash(sStashId), Aze::CError::eCouldNotRestoreStash);

    RETURN_WHEN_ERROR(m_pRepository->writeStage(), Aze::CError::eCouldNotWriteStage);

    RETURN_WHEN_ERROR(m_pRepository->writeGeneralInformation(), Aze::CError::eCouldNotWriteGeneralInformation);

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::patch()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    for (QString sFilename : m_tArguments.m_lFilesAndIds)
    {
        RETURN_WHEN_ERROR(m_pRepository->patch(sFilename), Aze::CError::eCouldNotPatch);
    }

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::setRemoteHost()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    RETURN_WHEN_FALSE(m_tArguments.m_lFilesAndIds.count() > 0, Aze::CError::eNoRemoteHostNameGiven);

    QString sRemoteHostName = m_tArguments.m_lFilesAndIds.takeFirst();

    Aze::CError::EErrorCode eCode = m_pRepository->setRemoteHost(sRemoteHostName);

    if (eCode != Aze::CError::eNone)
        return eCode;

    RETURN_WHEN_ERROR(m_pRepository->writeGeneralInformation(), Aze::CError::eCouldNotWriteGeneralInformation);

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------

Aze::CError::EErrorCode CAzeClient::dump()
{
    // Repository sanity check
    RETURN_WHEN_FALSE(isASainRepository(), Aze::CError::eNotARepository);

    (*m_pOutStream) << m_pRepository->dump(m_tArguments.m_lFilesAndIds);
    m_pOutStream->flush();

    return Aze::CError::eNone;
}

//-------------------------------------------------------------------------------------------------
