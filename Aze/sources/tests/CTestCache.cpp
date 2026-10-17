
// Qt
#include <QDir>
#include <QFile>

// AzeLib
#include "CRepository.h"
#include "CRemoteRepository.h"
#include "CFileHashCache.h"
#include "CUtils.h"
#include "CStrings.h"
#include <CXMLNode.h>

// Application
#include "../common/CConstants.h"
#include "CTestCache.h"
#include "CTestMacros.h"

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Constructor.
* \param sArgument0 Binary path, forwarded to the fixture.
*/
/***************************************************************************/
CTestCache::CTestCache(const QString& sArgument0)
    : CTestFixture(sArgument0)
{
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests the working folder hash cache (hits, misses and invalidation).
*/
/***************************************************************************/
void CTestCache::testFileHashCache()
{
    QString sFilePath = "Files/HashCacheTest.txt";
    QString sAbsoluteFileName = QDir(m_sRootPath).absoluteFilePath(sFilePath);

    QVERIFY(QDir().mkpath("Files"));
    QFile::remove(sAbsoluteFileName);

    Aze::CFileHashCache oCache(this);

    QVERIFY(createFile(sFilePath, "alpha"));
    QString sIdentifier1 = oCache.identifierForFile(sAbsoluteFileName);
    QVERIFY(not sIdentifier1.isEmpty());
    QCOMPARE(sIdentifier1, Aze::CUtils::idFromFileContent(sAbsoluteFileName));
    QCOMPARE(oCache.missCount(), qint64(1));
    QCOMPARE(oCache.hitCount(), qint64(0));
    QVERIFY(oCache.containsFile(sAbsoluteFileName));

    // An unchanged file is served from the cache
    QString sIdentifier2 = oCache.identifierForFile(sAbsoluteFileName);
    QCOMPARE(sIdentifier2, sIdentifier1);
    QCOMPARE(oCache.hitCount(), qint64(1));
    QCOMPARE(oCache.missCount(), qint64(1));

    // A modified file misses the cache and yields a new identifier
    QVERIFY(createFile(sFilePath, "alpha-beta"));
    QVERIFY(not oCache.containsFile(sAbsoluteFileName));
    QString sIdentifier3 = oCache.identifierForFile(sAbsoluteFileName);
    QVERIFY(not sIdentifier3.isEmpty());
    QVERIFY(sIdentifier3 != sIdentifier1);
    QCOMPARE(oCache.missCount(), qint64(2));

    // Explicit invalidation forces a new read with the same result
    oCache.invalidateFile(sAbsoluteFileName);
    QVERIFY(not oCache.containsFile(sAbsoluteFileName));
    QString sIdentifier4 = oCache.identifierForFile(sAbsoluteFileName);
    QCOMPARE(sIdentifier4, sIdentifier3);
    QCOMPARE(oCache.missCount(), qint64(3));

    // Clearing drops entries but keeps counters
    oCache.clear();
    QCOMPARE(oCache.count(), qint64(0));
    QCOMPARE(oCache.missCount(), qint64(3));
    QVERIFY(not oCache.containsFile(sAbsoluteFileName));
    QCOMPARE(oCache.identifierForFile(sAbsoluteFileName), sIdentifier3);
    QCOMPARE(oCache.missCount(), qint64(4));

    // A missing file returns empty without being cached
    QVERIFY(deleteFile(sFilePath));
    QVERIFY(oCache.identifierForFile(sAbsoluteFileName).isEmpty());
    QVERIFY(not oCache.containsFile(sAbsoluteFileName));

    // An empty file name returns empty
    QVERIFY(oCache.identifierForFile("").isEmpty());
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests cache invalidation after commit, merge, received push and branch switch.
*/
/***************************************************************************/
void CTestCache::testCacheInvalidation()
{
    QString sFilePath = "Files/CacheInvalidation.txt";
    QString sTrunk = Aze::CStrings::s_sDefaultBranchName;

    INIT();

    Aze::CRepository oRepository(m_sRootPath);
    QVERIFY(oRepository.ok());

    auto commitFile = [&](const QString& sContent, const QString& sMessage) -> bool {
        if (not createFile(sFilePath, sContent))
            return false;
        if (oRepository.stage(QStringList() << sFilePath) != Aze::CError::eNone)
            return false;
        if (oRepository.writeStage() != Aze::CError::eNone)
            return false;
        if (oRepository.commit(m_sAuthor, sMessage) != Aze::CError::eNone)
            return false;
        if (oRepository.writeCurrentBranch() != Aze::CError::eNone)
            return false;
        if (oRepository.clearStage() != Aze::CError::eNone)
            return false;
        return oRepository.writeStage() == Aze::CError::eNone;
    };

    // Commit twice on the same instance: the cached tip follows without a fresh instance
    QVERIFY(commitFile("C1", "C1"));
    QString sTip1 = oRepository.tipCommit()->id();
    QVERIFY(not sTip1.isEmpty());
    QVERIFY(commitFile("C2", "C2"));
    QCOMPARE(oRepository.tipCommit()->parents(), QStringList() << sTip1);
    QString sTip2 = oRepository.tipCommit()->id();
    QVERIFY(sTip2 != sTip1);

    // Amend on the same instance: the cached tip follows the rewritten commit
    QVERIFY(oRepository.amend(m_sAuthor, "C2 amended") == Aze::CError::eNone);
    QVERIFY(oRepository.writeCurrentBranch() == Aze::CError::eNone);
    QCOMPARE(oRepository.tipCommit()->parents(), QStringList() << sTip1);
    QCOMPARE(oRepository.tipCommit()->message(), QString("C2 amended"));
    sTip2 = oRepository.tipCommit()->id();

    // A file status call populates the working folder hash cache
    oRepository.fileStatus(QStringList());
    QVERIFY(oRepository.commitFunctions()->fileHashCache()->count() > 0);

    // Branch at the second commit, advance trunk, switch and merge back
    QVERIFY(oRepository.createBranch(m_sBranch1) == Aze::CError::eNone);
    QVERIFY(commitFile("C3", "C3"));
    QString sTip3 = oRepository.tipCommit()->id();

    QVERIFY(oRepository.switchToBranch(m_sBranch1) == Aze::CError::eNone);
    QVERIFY(oRepository.writeCurrentBranch() == Aze::CError::eNone);
    QVERIFY(oRepository.writeGeneralInformation() == Aze::CError::eNone);
    QCOMPARE(oRepository.tipCommit()->id(), sTip2);
    QCOMPARE(oRepository.commitFunctions()->fileHashCache()->count(), qint64(0));
    QString sContent;
    QVERIFY(readFile(sFilePath, sContent));
    QCOMPARE(sContent, QString("C2"));

    bool bHasConflicts = false;
    QVERIFY(oRepository.merge(sTrunk, bHasConflicts) == Aze::CError::eNone);
    QVERIFY(not bHasConflicts);
    QVERIFY(oRepository.writeCurrentBranch() == Aze::CError::eNone);
    QVERIFY(oRepository.writeStage() == Aze::CError::eNone);
    QCOMPARE(oRepository.tipCommit()->id(), sTip3);
    QCOMPARE(oRepository.commitFunctions()->fileHashCache()->count(), qint64(0));
    QVERIFY(readFile(sFilePath, sContent));
    QCOMPARE(sContent, QString("C3"));

    // A received push to the current branch refreshes the tip on the same server instance
    QVERIFY(oRepository.switchToBranch(sTrunk) == Aze::CError::eNone);
    QVERIFY(oRepository.writeCurrentBranch() == Aze::CError::eNone);
    QVERIFY(oRepository.writeGeneralInformation() == Aze::CError::eNone);

    QDir oParent(m_sRootPath);
    QVERIFY(oParent.cdUp());
    QString sRemotePath = oParent.absoluteFilePath("aze-tests-remote");

    if (QDir(sRemotePath).exists())
        QVERIFY(QDir(sRemotePath).removeRecursively());

    QVERIFY(QDir().mkpath(sRemotePath));

    Aze::CRepository oServer(sRemotePath);
    QVERIFY(oServer.init() == Aze::CError::eNone);

    Aze::CRemoteRepository oClientRemote(&oRepository);
    Aze::CRemoteRepository oServerRemote(&oServer);
    QString sDetail;
    QVERIFY(oServerRemote.applyPushRequest(oClientRemote.getPushRequest(), sDetail) == Aze::CError::eNone);
    QCOMPARE(oServer.tipCommit()->id(), sTip3);

    QVERIFY(commitFile("C4", "C4"));
    QString sTip4 = oRepository.tipCommit()->id();
    QVERIFY(oServerRemote.applyPushRequest(oClientRemote.getPushRequest(), sDetail) == Aze::CError::eNone);
    QCOMPARE(oServer.tipCommit()->id(), sTip4);

    QVERIFY(QDir(sRemotePath).removeRecursively());
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests that the commit parent cache persists in binary format.
*/
/***************************************************************************/
void CTestCache::testCommitTreeListBinaryCache()
{
    QString sFilePath = "Files/CacheTest.txt";
    QString sLogResult;

    INIT();
    QVERIFY(createFile(sFilePath, "C1\n--"));
    STAGE(sFilePath);
    COMMIT("C1");
    QVERIFY(createFile(sFilePath, "C2\n--"));
    STAGE(sFilePath);
    COMMIT("C2");
    LOG(sLogResult);
    QVERIFY(sLogResult.contains("C2"));

    // The parent cache is persisted in binary format through the destructor
    QDir tCacheDir(QString("%1/%2").arg(m_sDataPath).arg(Aze::CStrings::s_sPathAzeCachePath));
    QStringList lBinaries = tCacheDir.entryList(
        QStringList() << QString("*%1").arg(CXMLNode::sExtension_BIN), QDir::Files);
    QCOMPARE(lBinaries.count(), 1);

    QString sError;
    CXMLNode xCache = CXMLNode::load(tCacheDir.absoluteFilePath(lBinaries.first()), &sError);
    QVERIFY2(sError.isEmpty(), qPrintable(sError));
    QVERIFY(not xCache.isEmpty());
    QVERIFY(xCache.attributes().contains(Aze::CStrings::s_sParamCommitParentList));
    QVERIFY(not xCache.attributes()[Aze::CStrings::s_sParamCommitParentList].isEmpty());
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests that the commit parent cache persists in text format.
*/
/***************************************************************************/
void CTestCache::testCommitTreeListTextCache()
{
    QString sFilePath = "Files/CacheTest.txt";
    QString sLogResult;
    QString sTextCache = QString("--%1").arg(CConstants::s_sSwitchTextCache);

    clearRepository();
    QDir rootDir(m_sRootPath);
    QVERIFY(rootDir.mkdir(m_sFilesFolderName));
    QDir filesDir(m_sFilesFolderPath);
    QVERIFY(filesDir.mkdir(m_sFilesSubFolderName));
    exec({ CConstants::s_sSwitchInitRepository, sTextCache });

    QVERIFY(createFile(sFilePath, "C1\n--"));
    exec({ CConstants::s_sSwitchStage, QString("--%1").arg(SILENT_OR_DEBUG), sTextCache, sFilePath });
    exec({
        CConstants::s_sSwitchCommit,
        QString("--%1").arg(SILENT_OR_DEBUG),
        sTextCache,
        QString("--%1").arg(CConstants::s_sSwitchMessage),
        "C1",
        QString("--%1").arg(CConstants::s_sSwitchAuthor),
        m_sAuthor
    });
    QVERIFY(createFile(sFilePath, "C2\n--"));
    exec({ CConstants::s_sSwitchStage, QString("--%1").arg(SILENT_OR_DEBUG), sTextCache, sFilePath });
    exec({
        CConstants::s_sSwitchCommit,
        QString("--%1").arg(SILENT_OR_DEBUG),
        sTextCache,
        QString("--%1").arg(CConstants::s_sSwitchMessage),
        "C2",
        QString("--%1").arg(CConstants::s_sSwitchAuthor),
        m_sAuthor
    });
    sLogResult = exec({ CConstants::s_sSwitchLog, sTextCache });
    QVERIFY(sLogResult.contains("C2"));

    // The parent cache is persisted in text format through the destructor
    QDir tCacheDir(QString("%1/%2").arg(m_sDataPath).arg(Aze::CStrings::s_sPathAzeCachePath));
    QStringList lTextCaches = tCacheDir.entryList(
        QStringList() << QString("*.%1").arg(Aze::CStrings::s_sCompressedXMLExtension), QDir::Files);
    QCOMPARE(lTextCaches.count(), 1);
    QCOMPARE(tCacheDir.entryList(
        QStringList() << QString("*%1").arg(CXMLNode::sExtension_BIN), QDir::Files).count(), 0);

    QString sError;
    CXMLNode xCache = CXMLNode::load(tCacheDir.absoluteFilePath(lTextCaches.first()), &sError);
    QVERIFY2(sError.isEmpty(), qPrintable(sError));
    QVERIFY(not xCache.isEmpty());
    QVERIFY(xCache.attributes().contains(Aze::CStrings::s_sParamCommitParentList));
    QVERIFY(not xCache.attributes()[Aze::CStrings::s_sParamCommitParentList].isEmpty());
}

//-------------------------------------------------------------------------------------------------
