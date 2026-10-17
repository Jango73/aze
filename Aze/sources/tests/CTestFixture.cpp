

// Qt
#include <QCoreApplication>
#include <QFile>
#include <QDir>

// AzeLib
#include "CRepository.h"
#include "CError.h"

// Application
#include "../client/CAzeClient.h"
#include "CTestFixture.h"

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Constructor, points the fixture at the isolated test folder.
* \param sArgument0 Binary path, forwarded to the client layer by exec.
*/
/***************************************************************************/
CTestFixture::CTestFixture(const QString& sArgument0)
    : m_sArgument0(sArgument0)
{
    // Tests manipulate and delete files: run them in an isolated folder
    // under <repository>/tmp, never in the current working folder
    m_sRootPath = testDirectoryPath();
    QDir::setCurrent(m_sRootPath);

    m_sDataPath = "./.aze";
    m_sAuthor = "CTestAze";
    m_sBranch1 = "branch1";
    m_sBranch2 = "branch2";

    m_sInfoPath = QString("%1/%2")
            .arg(m_sDataPath)
            .arg(Aze::CStrings::s_sGeneralInformationFileName);

    m_sTrunkPath = QString("%1/%2/%3.%4")
            .arg(m_sDataPath)
            .arg(Aze::CStrings::s_sPathAzeBranchPath)
            .arg(Aze::CStrings::s_sDefaultBranchName)
            .arg(Aze::CStrings::s_sCompressedXMLExtension);

    m_sBranch1Path = QString("%1/%2/%3.%4")
            .arg(m_sDataPath)
            .arg(Aze::CStrings::s_sPathAzeBranchPath)
            .arg(m_sBranch1)
            .arg(Aze::CStrings::s_sCompressedXMLExtension);

    m_sFilesFolderName = "Files";
    m_sFilesFolderPath = QString("%1/%2").arg(m_sRootPath).arg(m_sFilesFolderName);

    m_sFilesSubFolderName = "OtherFiles";
    m_sFilesSubFolderPath = QString("%1/%2/%3").arg(m_sRootPath).arg(m_sFilesFolderName).arg(m_sFilesSubFolderName);
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Deletes all folders and files in the test repository.
*/
/***************************************************************************/
void CTestFixture::clearRepository()
{
    // Remove repo
    QDir repo(m_sDataPath);
    if (repo.exists())
        repo.removeRecursively();

    // Remove files
    QDir files(m_sFilesFolderName);
    if (files.exists())
        files.removeRecursively();
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Returns the isolated test folder, created if needed.
* \return The <repository>/tmp/aze-tests folder path.
*/
/***************************************************************************/
QString CTestFixture::testDirectoryPath()
{
    // Locate the repository root from the binary folder, then use tmp/
    QDir tFolder(QCoreApplication::applicationDirPath());
    while (not tFolder.exists("Aze.pro") and tFolder.cdUp())
    {
    }

    if (not tFolder.exists("Aze.pro"))
        qFatal("CTestAze: repository root (Aze.pro) not found, tests refuse to run outside a repository checkout");

    QString sTestPath = QString("%1/tmp/aze-tests").arg(tFolder.absolutePath());
    QDir().mkpath(sTestPath);

    return sTestPath;
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Creates a file with the given content.
* \param sName File name.
* \param sContent File content.
* \return True on success.
*/
/***************************************************************************/
bool CTestFixture::createFile(const QString& sName, const QString& sContent)
{
    QFile file(sName);

    if (file.open(QIODevice::WriteOnly))
    {
        file.write(sContent.toUtf8().constData());
        file.close();

        return true;
    }

    return false;
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Deletes a file.
* \param sName File name.
* \return True on success.
*/
/***************************************************************************/
bool CTestFixture::deleteFile(const QString& sName)
{
    QFile file(sName);
    return file.remove();
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Reads a file.
* \param sName File name.
* \param sContent Output file content.
* \return True on success.
*/
/***************************************************************************/
bool CTestFixture::readFile(const QString& sName, QString& sContent)
{
    QFile file(sName);

    if (file.open(QIODevice::ReadOnly))
    {
        sContent = QString(file.readAll());
        file.close();

        return true;
    }

    return false;
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Executes a client command and returns its output or exit code.
* \param lInputArguments Command arguments without the binary path.
* \return Command output, or the exit code when output is empty.
*/
/***************************************************************************/
QString CTestFixture::exec(const QStringList& lInputArguments)
{
    QStringList lFinalArguments(lInputArguments);
    QString baOutput;
    QTextStream sOutput(&baOutput);

    lFinalArguments.prepend(m_sArgument0);
    int argc = lFinalArguments.count();
    char** argv = new char*[std::size_t(argc)];
    int index;

    // Add arguments
    for (index = 0; index < argc; index++)
    {
        argv[index] = new char[std::size_t(lFinalArguments[index].count() + 2)];
        strcpy(argv[index], lFinalArguments[index].toUtf8().constData());
    }

    // Instantiate and run aze
    Aze::CError::EErrorCode eCode = CAzeClient(argc, argv, &sOutput).run();

    for (index = 0; index < argc; index++)
        delete [] argv[index];

    delete [] argv;

    // If output is empty, return the error code
    if (baOutput.count() == 0)
    {
        return QString::number(Aze::CError::toExitCode(eCode));
    }

    return baOutput;
}

//-------------------------------------------------------------------------------------------------

/***************************************************************************/
/*!
* \brief Converts an error code to the string returned by exec.
* \param eCode Error code.
* \return Exit code as string.
*/
/***************************************************************************/
QString CTestFixture::expectedCode(Aze::CError::EErrorCode eCode)
{
    return QString::number(Aze::CError::toExitCode(eCode));
}

//-------------------------------------------------------------------------------------------------
