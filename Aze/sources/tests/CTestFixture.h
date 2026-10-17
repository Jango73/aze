
#pragma once

// Qt
#include <QtTest/QtTest>

// qt-plus
#include "CTextGenerator.h"

// AzeLib
#include "CRepository.h"

//-------------------------------------------------------------------------------------------------

class CTestFixture : public QObject
{
    Q_OBJECT

public:

    explicit CTestFixture(const QString& sArgument0);

protected:

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

    //! Converts an error code to the string returned by exec on empty output
    static QString expectedCode(Aze::CError::EErrorCode eCode);

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

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
