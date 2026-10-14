
// Qt
#include <QDebug>
#include <QDir>

// Application
#include "CDatabase.h"
#include "../CUtils.h"

namespace Aze {

//-------------------------------------------------------------------------------------------------

CDatabase::CDatabase(const QString& sRootPath, QObject* parent)
    : QObject(parent)
    , m_sStartPath(sRootPath)
{
    if (m_sStartPath.endsWith(CStrings::s_sPathSep))
        m_sStartPath.remove(m_sStartPath.count() - 1, 1);

    m_sRootPath = getActualRoot(m_sStartPath);

    m_sDataPath = QString("%1/%2").arg(m_sRootPath).arg(CStrings::s_sPathAzeDataRootPath);
    m_sStashPath = QString("%1/%2").arg(m_sDataPath).arg(CStrings::s_sPathAzeStashPath);
    m_sMergePath = QString("%1/%2").arg(m_sDataPath).arg(CStrings::s_sPathAzeMergePath);
    m_sBranchPath = QString("%1/%2").arg(m_sDataPath).arg(CStrings::s_sPathAzeBranchPath);
    m_sCommitPath = QString("%1/%2").arg(m_sDataPath).arg(CStrings::s_sPathAzeCommitPath);
    m_sObjectPath = QString("%1/%2").arg(m_sDataPath).arg(CStrings::s_sPathAzeObjectPath);
    m_sCachePath = QString("%1/%2").arg(m_sDataPath).arg(CStrings::s_sPathAzeCachePath);
}

//-------------------------------------------------------------------------------------------------

CBranch* CDatabase::getBranch(const QString& sName, QObject* parent)
{
    return CBranch::fromFile(composeBranchFileName(sName), parent);
}

//-------------------------------------------------------------------------------------------------

CCommit* CDatabase::getCommit(const QString& sId, QObject* parent)
{
    return CCommit::fromFile(composeCommitFileName(sId), parent, sId);
}

//-------------------------------------------------------------------------------------------------

CStash* CDatabase::getStash(const QString& sId, QObject* parent)
{
    return CStash::fromFile(composeStashFileName(sId), parent, sId);
}

//-------------------------------------------------------------------------------------------------

QByteArray CDatabase::getObject(const QString& sId)
{
    QByteArray baData;

    QFile tOutputFile(composeObjectFileName(sId));

    // Does the object file exist?
    if (tOutputFile.exists())
    {
        if (tOutputFile.open(QIODevice::ReadOnly))
        {
            // Uncompress object file data
            baData = qUncompress(tOutputFile.readAll());
            tOutputFile.close();
        }
    }

    return baData;
}

//-------------------------------------------------------------------------------------------------

QVector<CBranch*> CDatabase::getAllBranches(QObject* parent)
{
    QVector<CBranch*> lReturnValue;

    QStringList lFilter; lFilter << QString("*.%1").arg(Aze::CStrings::s_sCompressedXMLExtension);

    QDir dDirectory(m_sBranchPath);
    QFileInfoList lFiles = dDirectory.entryInfoList(lFilter, QDir::Files | QDir::NoSymLinks);

    for (QFileInfo iFile : lFiles)
    {
        lReturnValue << getBranch(iFile.baseName(), parent);
    }

    return lReturnValue;
}

//-------------------------------------------------------------------------------------------------

QString CDatabase::storeCommit(const CCommit* pCommit, const QString& sId)
{
    if (IS_NULL(pCommit))
        return "";

    QString sIdToUse = sId;

    if (sIdToUse.isEmpty())
        sIdToUse = pCommit->generateId();

    pCommit->toFile(composeCommitFileName(sIdToUse));

    return sIdToUse;
}

//-------------------------------------------------------------------------------------------------

QString CDatabase::storeStash(const CStash* pStash, const QString& sId)
{
    if (IS_NULL(pStash))
        return "";

    QString sIdToUse = sId;

    if (sIdToUse.isEmpty())
        sIdToUse = pStash->generateId();

    pStash->toFile(composeStashFileName(sIdToUse));

    return sIdToUse;
}

//-------------------------------------------------------------------------------------------------

bool CDatabase::init()
{
    QDir rootDir(m_sRootPath);

    if (not rootDir.exists())
        return false;

    rootDir.mkpath(m_sDataPath);
    rootDir.mkpath(m_sStashPath);
    rootDir.mkpath(m_sMergePath);
    rootDir.mkpath(m_sBranchPath);
    rootDir.mkpath(m_sCommitPath);
    rootDir.mkpath(m_sObjectPath);
    rootDir.mkpath(m_sStashPath);
    rootDir.mkpath(m_sCachePath);

    return true;
}

//-------------------------------------------------------------------------------------------------

bool CDatabase::removeStash(const QString& sId)
{
    QFile file(composeStashFileName(sId));
    return file.remove();
}

//-------------------------------------------------------------------------------------------------

QString CDatabase::relativeFileName(const QString& sFileName)
{
    return CUtils::relativeFileName(m_sRootPath, sFileName);
}

//-------------------------------------------------------------------------------------------------

QString CDatabase::absoluteFileName(const QString& sFileName)
{
    return CUtils::absoluteFileName(m_sRootPath, sFileName);
}

//-------------------------------------------------------------------------------------------------

QString CDatabase::startRelativeFileName(const QString& sFileName)
{
    return CUtils::relativeFileName(m_sStartPath, sFileName);
}

//-------------------------------------------------------------------------------------------------

QString CDatabase::generalInformationFileName()
{
    return QString("%1/%2").arg(m_sDataPath).arg(CStrings::s_sGeneralInformationFileName);
}

//-------------------------------------------------------------------------------------------------

QString CDatabase::composeLocalFileName(const QString& sFileName)
{
    return QString("%1/%2").arg(m_sRootPath).arg(sFileName);
}

//-------------------------------------------------------------------------------------------------

QString CDatabase::composeMergeFileName(const QString& sFileName)
{
    return QString("%1/%2").arg(m_sMergePath).arg(sFileName);
}

//-------------------------------------------------------------------------------------------------

QString CDatabase::composeBranchFileName(const QString& sBranchName)
{
    return QString("%1/%2.%3")
            .arg(m_sBranchPath)
            .arg(sBranchName)
            .arg(CStrings::s_sCompressedXMLExtension);
}

//-------------------------------------------------------------------------------------------------

QString CDatabase::composeCommitFileName(const QString& sCommitId)
{
    return QString("%1/%2.%3")
            .arg(m_sCommitPath)
            .arg(sCommitId)
            .arg(CStrings::s_sCompressedXMLExtension);
}

//-------------------------------------------------------------------------------------------------

QString CDatabase::composeObjectFileName(const QString& sId)
{
    return QString("%1/%2").arg(m_sObjectPath).arg(sId);
}

//-------------------------------------------------------------------------------------------------

QString CDatabase::composeStashFileName(const QString& sId)
{
    return QString("%1/%2.%3")
            .arg(m_sStashPath)
            .arg(sId)
            .arg(CStrings::s_sCompressedXMLExtension);
}

//-------------------------------------------------------------------------------------------------

QString CDatabase::composeCacheFileName(const QString& sFileName)
{
    return QString("%1/%2").arg(m_sCachePath).arg(sFileName);
}

//-------------------------------------------------------------------------------------------------

QString CDatabase::storeFileAsObject(const QString& sFileName)
{
    QString sId;
    QFile tInputFile(sFileName);

    if (tInputFile.exists())
    {
        if (tInputFile.open(QIODevice::ReadOnly))
        {
            QByteArray baData = tInputFile.readAll();
            tInputFile.close();

            // Get the Id of this file
            sId = CUtils::idFromByteArray(baData);

            // Check if it already exists
            QString sStoredObjectFileName = QString("%1/%2").arg(m_sObjectPath).arg(sId);
            QFile tOutputFile(sStoredObjectFileName);

            // If it does not exist, compress it and store it in objects folder
            if (not tOutputFile.exists())
            {
                baData = qCompress(baData);

                if (tOutputFile.open(QIODevice::WriteOnly))
                {
                    if (tOutputFile.write(baData) != baData.count())
                    {
                        sId = "";
                        tOutputFile.close();
                        tOutputFile.remove();
                    }
                    else
                    {
                        tOutputFile.close();
                    }
                }
            }
        }
    }

    return sId;
}

//-------------------------------------------------------------------------------------------------

/*!
    Stores \a baData as the object \a sId. \br\br
    Verifies content integrity, then compresses and stores the data
    when the object does not exist yet. \br
*/
bool CDatabase::storeObject(const QString& sId, const QByteArray& baData)
{
    if (sId.isEmpty())
        return false;

    // Check content integrity
    if (CUtils::idFromByteArray(baData) != sId)
        return false;

    QString sStoredObjectFileName = composeObjectFileName(sId);
    QFile tOutputFile(sStoredObjectFileName);

    // If it does not exist, compress it and store it in objects folder
    if (not tOutputFile.exists())
    {
        if (not tOutputFile.open(QIODevice::WriteOnly))
            return false;

        QByteArray baCompressed = qCompress(baData);

        if (tOutputFile.write(baCompressed) != baCompressed.count())
        {
            tOutputFile.close();
            tOutputFile.remove();
            return false;
        }

        tOutputFile.close();
    }

    return true;
}

//-------------------------------------------------------------------------------------------------

bool CDatabase::fileWithIdExists(const QString& sId)
{
    QString sStoredObjectFileName = QString("%1/%2").arg(m_sObjectPath).arg(sId);
    return QFile(sStoredObjectFileName).exists();
}

//-------------------------------------------------------------------------------------------------

QString CDatabase::printableFileContentById(const QString& sId)
{
    QString sText;
    QString sFileName;

    // Try in commits
    sFileName = composeCommitFileName(sId);

    if (QFile(sFileName).exists())
        return CUtils::getTextFileContent(sFileName);

    // Try in stash
    sFileName = composeStashFileName(sId);

    if (QFile(sFileName).exists())
        return CUtils::getTextFileContent(sFileName);

    // Try in objects
    sFileName = composeObjectFileName(sId);

    if (QFile(sFileName).exists())
        return QString(getObject(sId));

    return sText;
}

//-------------------------------------------------------------------------------------------------

QString CDatabase::getActualRoot(const QString& sPath)
{
    QString sSearchPath = sPath;

    while (true)
    {
        QString sDataPath = QString("%1/%2").arg(sSearchPath).arg(CStrings::s_sPathAzeDataRootPath);

        QDir dataPath(sDataPath);
        if (dataPath.exists())
            return sSearchPath;

        if (sSearchPath.contains(CStrings::s_sPathSep))
        {
            int index = sSearchPath.lastIndexOf(CStrings::s_sPathSep);
            if (index == -1)
                break;

            sSearchPath = sSearchPath.left(index);
        }
        else
        {
            break;
        }
    }

    return sPath;
}

//-------------------------------------------------------------------------------------------------

} // namespace Aze
