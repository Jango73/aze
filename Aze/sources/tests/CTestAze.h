
#pragma once

// Qt
#include <QtTest/QtTest>

// qt-plus
#include "CTextGenerator.h"

// Application
#include "CRepository.h"

//-------------------------------------------------------------------------------------------------

class CTestAze : public QObject
{
    Q_OBJECT

public:

    CTestAze(const QString& sArgument0);

private:

    class CTransformedFile
    {
    public:
        CTransformedFile()
        {
        }

        CTransformedFile(QString sFileName, QString sFileContent1, QString sFileContent2)
            : m_sFilePath(sFileName)
            , m_sFileContent1(sFileContent1)
            , m_sFileContent2(sFileContent2)
        {
        }

        QString m_sFilePath;
        QString m_sFileContent1;
        QString m_sFileContent2;
    };

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //! Deletes all folders and files in the test repository
    void clearRepository();

    //! Returns the isolated test folder (<repository>/tmp/aze-tests), created if needed
    QString testDirectoryPath();

    //!
    QMap<QString, QString> generateFiles(int iCount);

    //!
    bool createFile(const QString& sName, const QString& sContent);

    //!
    bool deleteFile(const QString& sName);

    //!
    bool readFile(const QString& sName, QString& sContent);

    //!
    void createManyFiles();

    //! Executes a command
    QString exec(const QStringList& lInputArguments);

    //-------------------------------------------------------------------------------------------------
    // Slots
    //-------------------------------------------------------------------------------------------------

private slots:

    //! Test stage and commit functionalities
    void testCommit();

    //! Test unstage functionality
    void testUnstage();

    //! Test amend functionality
    void testAmend();

    //! Test file deletion functionality
    void testDelete();

    //! Tests patch functionality
    void testPatch();

    //! Tests merge functionality
    void testMerge();

    //! Tests merge error paths and conflict resolution
    void testMergeErrorsAndResolution();

    //! Tests push request building and applying to another repository
    void testRemotePush();

    //! Tests functionalities under heavy load
    void testHeavy();

    //!
    void testFinalize();

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

private:

    CTextGenerator m_tTextGenerator;
    QStringList lStage;
    QString m_sArgument0;
    QString m_sRootPath;
    QString m_sDataPath;
    QString m_sAuthor;
    QString m_sInfoPath;
    QString m_sTrunkPath;
    QString m_sBranch1Path;
    QString m_sFilesFolderName;
    QString m_sFilesFolderPath;
    QString m_sFilesSubFolderName;
    QString m_sFilesSubFolderPath;
    QString m_sBranch1;
    QString m_sBranch2;
    int m_iCommitIndex;
};

//-------------------------------------------------------------------------------------------------
