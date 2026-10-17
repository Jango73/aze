
// Qt
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>

// AzeLib
#include "CRepository.h"

// Application
#include "CTestMassive.h"
#include "CTestMacros.h"

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Builds one deterministic line of a massive file.
* \param iFileNumber Stable number identifying the file.
* \param iGeneration Content generation, each rewrite uses a new generation.
* \param iLineIndex Index of the line inside the file.
* \param iLineWidth Line width in characters, without the trailing new line.
* \return One line of text terminated by a new line.
*/
/***************************************************************************/
static QString massiveLine(
        const int iFileNumber,
        const int iGeneration,
        const int iLineIndex,
        const int iLineWidth)
{
    QString sLine = QString("MASSIVE-FILE-%1-GENERATION-%2-LINE-%3-")
            .arg(iFileNumber, 5, 10, QChar('0'))
            .arg(iGeneration, 2, 10, QChar('0'))
            .arg(iLineIndex, 6, 10, QChar('0'));

    while (sLine.count() < iLineWidth)
        sLine += "0123456789ABCDEF";

    return sLine.left(iLineWidth) + "\n";
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Returns the repository relative path of a base file.
* \param iFileIndex Index of the base file.
* \return Repository relative file path.
*/
/***************************************************************************/
static QString massiveBaseFilePath(const int iFileIndex)
{
    return QString("Files/Massive/Massive-%1.txt").arg(iFileIndex, 3, 10, QChar('0'));
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Returns the repository relative path of a side branch file.
* \param iFileIndex Index of the side branch file.
* \return Repository relative file path.
*/
/***************************************************************************/
static QString massiveSideFilePath(const int iFileIndex)
{
    return QString("Files/Massive/Massive-Side-%1.txt").arg(iFileIndex, 3, 10, QChar('0'));
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Returns the repository relative path of a fast branch file.
* \param iFileIndex Index of the fast branch file.
* \return Repository relative file path.
*/
/***************************************************************************/
static QString massiveFastFilePath(const int iFileIndex)
{
    return QString("Files/Massive/Massive-Fast-%1.txt").arg(iFileIndex, 3, 10, QChar('0'));
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Writes a massive file with deterministic content.
* \param sPath Repository relative file path.
* \param iFileNumber Stable number identifying the file.
* \param iGeneration Content generation to write.
* \param iLineCount Number of lines to write.
* \param iLineWidth Line width in characters, without the trailing new line.
* \return True on success.
*/
/***************************************************************************/
static bool writeMassiveFile(
        const QString& sPath,
        const int iFileNumber,
        const int iGeneration,
        const int iLineCount,
        const int iLineWidth)
{
    QFile oFile(sPath);

    if (not oFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    for (int iLineIndex = 0; iLineIndex < iLineCount; iLineIndex++)
        oFile.write(massiveLine(iFileNumber, iGeneration, iLineIndex, iLineWidth).toUtf8());

    oFile.close();

    return true;
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Computes the expected hash of a massive file without storing it.
* \param iFileNumber Stable number identifying the file.
* \param iGeneration Expected content generation.
* \param iLineCount Expected number of lines.
* \param iLineWidth Line width in characters, without the trailing new line.
* \return Hexadecimal SHA-256 hash of the expected content.
*/
/***************************************************************************/
static QByteArray massiveFileHash(
        const int iFileNumber,
        const int iGeneration,
        const int iLineCount,
        const int iLineWidth)
{
    QCryptographicHash oHash(QCryptographicHash::Sha256);

    for (int iLineIndex = 0; iLineIndex < iLineCount; iLineIndex++)
        oHash.addData(massiveLine(iFileNumber, iGeneration, iLineIndex, iLineWidth).toUtf8());

    return oHash.result().toHex();
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Constructor.
* \param sArgument0 Binary path, forwarded to the fixture.
*/
/***************************************************************************/
CTestMassive::CTestMassive(const QString& sArgument0)
    : CTestFixture(sArgument0)
{
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Tests a massive repository, only the final state is verified.
*
* Builds around 29 megabytes of deterministic text files, then drives commits,
* branches, merges with and without fast forward, and an amend without checking
* the intermediate steps.
* The final working folder and the tip commit are compared against the
* expected deterministic content.
*/
/***************************************************************************/
void CTestMassive::testMassive()
{
    const int iBaseFileCount = 60;
    const int iSideFileCount = 20;
    const int iFastFileCount = 10;
    const int iLineCount = 4000;
    const int iLineWidth = 80;
    const int iSideFileNumberOffset = 1000;
    const int iFastFileNumberOffset = 2000;
    const int iSideFileGeneration = 10;
    const int iFastFileGeneration = 30;
    const qint64 iMinimumTotalByteCount = 20 * 1024 * 1024;

    QString sResult;

    INIT();

    QVERIFY(QDir().mkpath("Files/Massive"));

    auto stagePaths = [&](const QStringList& lPaths) -> QString {
        QStringList lArguments;
        lArguments << CConstants::s_sSwitchStage;
        lArguments << QString("--%1").arg(SILENT_OR_DEBUG);
        lArguments << lPaths;
        return exec(lArguments);
    };

    // Base commit on trunk, 60 files of generation 0
    for (int iFileIndex = 0; iFileIndex < iBaseFileCount; iFileIndex++)
    {
        QVERIFY(writeMassiveFile(
            massiveBaseFilePath(iFileIndex),
            iFileIndex,
            0,
            iLineCount,
            iLineWidth));
    }

    {
        QStringList lPaths;
        for (int iFileIndex = 0; iFileIndex < iBaseFileCount; iFileIndex++)
            lPaths << massiveBaseFilePath(iFileIndex);
        QVERIFY(stagePaths(lPaths) == expectedCode(Aze::CError::eNone));
    }
    COMMIT("Massive base commit");

    // Side branch rewrites the first 20 files with generation 1
    CREATE_BRANCH("massive-side");
    SWITCH_TO_BRANCH("massive-side");

    for (int iFileIndex = 0; iFileIndex < 20; iFileIndex++)
    {
        QVERIFY(writeMassiveFile(
            massiveBaseFilePath(iFileIndex),
            iFileIndex,
            1,
            iLineCount,
            iLineWidth));
    }

    {
        QStringList lPaths;
        for (int iFileIndex = 0; iFileIndex < 20; iFileIndex++)
            lPaths << massiveBaseFilePath(iFileIndex);
        QVERIFY(stagePaths(lPaths) == expectedCode(Aze::CError::eNone));
    }
    COMMIT("Massive side commit");

    // Side branch adds 20 new files
    for (int iFileIndex = 0; iFileIndex < iSideFileCount; iFileIndex++)
    {
        QVERIFY(writeMassiveFile(
            massiveSideFilePath(iFileIndex),
            iSideFileNumberOffset + iFileIndex,
            iSideFileGeneration,
            iLineCount,
            iLineWidth));
    }

    {
        QStringList lPaths;
        for (int iFileIndex = 0; iFileIndex < iSideFileCount; iFileIndex++)
            lPaths << massiveSideFilePath(iFileIndex);
        QVERIFY(stagePaths(lPaths) == expectedCode(Aze::CError::eNone));
    }
    COMMIT("Massive side files commit");

    // Trunk rewrites files 40 to 58 with generation 2, file 59 is left for amend
    SWITCH_TO_BRANCH("trunk");

    for (int iFileIndex = 40; iFileIndex < 59; iFileIndex++)
    {
        QVERIFY(writeMassiveFile(
            massiveBaseFilePath(iFileIndex),
            iFileIndex,
            2,
            iLineCount,
            iLineWidth));
    }

    {
        QStringList lPaths;
        for (int iFileIndex = 40; iFileIndex < 59; iFileIndex++)
            lPaths << massiveBaseFilePath(iFileIndex);
        QVERIFY(stagePaths(lPaths) == expectedCode(Aze::CError::eNone));
    }
    COMMIT("Massive trunk commit");

    // Trunk rewrites file 59 with generation 3, then amends it to generation 4
    QVERIFY(writeMassiveFile(massiveBaseFilePath(59), 59, 3, iLineCount, iLineWidth));
    QVERIFY(stagePaths(QStringList() << massiveBaseFilePath(59)) == expectedCode(Aze::CError::eNone));
    COMMIT("Massive trunk file commit");

    QVERIFY(writeMassiveFile(massiveBaseFilePath(59), 59, 4, iLineCount, iLineWidth));
    QVERIFY(stagePaths(QStringList() << massiveBaseFilePath(59)) == expectedCode(Aze::CError::eNone));
    QVERIFY(AMEND("Massive trunk file amended", m_sAuthor) == expectedCode(Aze::CError::eNone));

    // Merge the side branch on trunk without fast forward so a merge commit is created,
    // file sets are disjoint so no conflict occurs
    MERGE_BRANCH_NO_FAST_FORWARD(sResult, "massive-side");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));
    COMMIT("Massive merge of side branch");

    // Extra branch rewrites files 20 to 39 with generation 5
    CREATE_BRANCH("massive-extra");
    SWITCH_TO_BRANCH("massive-extra");

    for (int iFileIndex = 20; iFileIndex < 40; iFileIndex++)
    {
        QVERIFY(writeMassiveFile(
            massiveBaseFilePath(iFileIndex),
            iFileIndex,
            5,
            iLineCount,
            iLineWidth));
    }

    {
        QStringList lPaths;
        for (int iFileIndex = 20; iFileIndex < 40; iFileIndex++)
            lPaths << massiveBaseFilePath(iFileIndex);
        QVERIFY(stagePaths(lPaths) == expectedCode(Aze::CError::eNone));
    }
    COMMIT("Massive extra commit");

    // Merge the extra branch on trunk without fast forward so a merge commit is created
    SWITCH_TO_BRANCH("trunk");
    MERGE_BRANCH_NO_FAST_FORWARD(sResult, "massive-extra");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));
    COMMIT("Massive merge of extra branch");

    // Fast branch adds 10 new files on top of the trunk tip
    CREATE_BRANCH("massive-fast");
    SWITCH_TO_BRANCH("massive-fast");

    for (int iFileIndex = 0; iFileIndex < iFastFileCount; iFileIndex++)
    {
        QVERIFY(writeMassiveFile(
            massiveFastFilePath(iFileIndex),
            iFastFileNumberOffset + iFileIndex,
            iFastFileGeneration,
            iLineCount,
            iLineWidth));
    }

    {
        QStringList lPaths;
        for (int iFileIndex = 0; iFileIndex < iFastFileCount; iFileIndex++)
            lPaths << massiveFastFilePath(iFileIndex);
        QVERIFY(stagePaths(lPaths) == expectedCode(Aze::CError::eNone));
    }
    COMMIT("Massive fast commit");

    // Merge the fast branch on trunk with fast forward allowed, the trunk tip
    // moves forward onto the fast commit without a merge commit
    SWITCH_TO_BRANCH("trunk");
    MERGE_BRANCH(sResult, "massive-fast");
    QVERIFY(sResult == expectedCode(Aze::CError::eNone));

    // Expected final state, each path maps to its file number and generation
    QMap<QString, QPair<int, int>> mExpectedFiles;

    for (int iFileIndex = 0; iFileIndex < 20; iFileIndex++)
        mExpectedFiles[massiveBaseFilePath(iFileIndex)] = qMakePair(iFileIndex, 1);

    for (int iFileIndex = 20; iFileIndex < 40; iFileIndex++)
        mExpectedFiles[massiveBaseFilePath(iFileIndex)] = qMakePair(iFileIndex, 5);

    for (int iFileIndex = 40; iFileIndex < 59; iFileIndex++)
        mExpectedFiles[massiveBaseFilePath(iFileIndex)] = qMakePair(iFileIndex, 2);

    mExpectedFiles[massiveBaseFilePath(59)] = qMakePair(59, 4);

    for (int iFileIndex = 0; iFileIndex < iSideFileCount; iFileIndex++)
    {
        mExpectedFiles[massiveSideFilePath(iFileIndex)] =
            qMakePair(iSideFileNumberOffset + iFileIndex, iSideFileGeneration);
    }

    for (int iFileIndex = 0; iFileIndex < iFastFileCount; iFileIndex++)
    {
        mExpectedFiles[massiveFastFilePath(iFileIndex)] =
            qMakePair(iFastFileNumberOffset + iFileIndex, iFastFileGeneration);
    }

    // No unexpected file is present in the massive folder
    {
        QStringList lFoundPaths;
        QDirIterator oIterator(
            "Files/Massive",
            QStringList() << "*.txt",
            QDir::Files,
            QDirIterator::Subdirectories);

        while (oIterator.hasNext())
            lFoundPaths << oIterator.next();

        QCOMPARE(lFoundPaths.count(), mExpectedFiles.count());
    }

    // Every file holds the expected deterministic content
    {
        qint64 iTotalByteCount = 0;

        for (auto iIterator = mExpectedFiles.constBegin();
            iIterator != mExpectedFiles.constEnd();
            iIterator++)
        {
            QFile oFile(iIterator.key());
            QVERIFY(oFile.exists());
            QVERIFY(oFile.open(QIODevice::ReadOnly));

            QByteArray bContent = oFile.readAll();
            oFile.close();

            iTotalByteCount += bContent.count();

            QByteArray bActualHash = QCryptographicHash::hash(bContent, QCryptographicHash::Sha256).toHex();
            QByteArray bExpectedHash = massiveFileHash(
                iIterator.value().first,
                iIterator.value().second,
                iLineCount,
                iLineWidth);

            QCOMPARE(bActualHash, bExpectedHash);
        }

        QVERIFY(iTotalByteCount >= iMinimumTotalByteCount);
    }

    // The tip commit is the fast commit reached by fast forward, it tracks
    // every expected file through a single parent
    {
        Aze::CRepository oRepository(m_sRootPath);
        QVERIFY(oRepository.ok());
        QVERIFY(not oRepository.tipCommit().isNull());
        QCOMPARE(oRepository.tipCommit()->message(), QString("Massive fast commit"));
        QVERIFY(not oRepository.tipCommit()->isMerge());
        QCOMPARE(oRepository.tipCommit()->parents().count(), 1);
        QCOMPARE(oRepository.tipCommit()->files().count(), mExpectedFiles.count());

        for (auto iIterator = mExpectedFiles.constBegin();
            iIterator != mExpectedFiles.constEnd();
            iIterator++)
        {
            QVERIFY(oRepository.tipCommit()->files().contains(iIterator.key()));

            QByteArray bStoredContent = oRepository.tipCommit()->fileContent(
                oRepository.database(),
                iIterator.key());

            QByteArray bStoredHash = QCryptographicHash::hash(bStoredContent, QCryptographicHash::Sha256).toHex();
            QByteArray bExpectedHash = massiveFileHash(
                iIterator.value().first,
                iIterator.value().second,
                iLineCount,
                iLineWidth);

            QCOMPARE(bStoredHash, bExpectedHash);
        }
    }
}

//-------------------------------------------------------------------------------------------------
