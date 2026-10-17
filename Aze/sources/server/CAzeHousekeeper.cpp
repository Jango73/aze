
// Qt
#include <QDir>
#include <QDirIterator>

// Aze
#include "CRepository.h"
#include "CStrings.h"

// Application
#include "CAzeHousekeeper.h"

/*!
    \class CAzeHousekeeper
    \inmodule Aze
    \section1 General
*/

//-------------------------------------------------------------------------------------------------

CAzeHousekeeper::CAzeHousekeeper(const QString& sRepositoriesBasePath, QObject* parent)
    : QObject(parent)
    , m_sRepositoriesBasePath(sRepositoriesBasePath)
    , m_pSweepTimer(new QTimer(this))
{
    connect(m_pSweepTimer, &QTimer::timeout, this, &CAzeHousekeeper::sweepRepositories);
    m_pSweepTimer->start(s_iPruneIntervalMs);
}

//-------------------------------------------------------------------------------------------------

/************************************************************************/

/*!
    Destroys a CAzeHousekeeper.
*/
CAzeHousekeeper::~CAzeHousekeeper()
{
}

//-------------------------------------------------------------------------------------------------

/************************************************************************/

/*!
    Runs a threshold-gated best-effort prune for a served request. \br\br
    Only push and delete-branch actions are collected, other actions return
    without work. The stored file count gates the sweep so small repositories
    skip the ancestor walk on every request. Prune errors are ignored so the
    originating response is never altered. No unprivileged network client
    triggers deletion directly, the prune runs locally on the server. This
    method opens its own silent repository from the folder path, it holds no
    shared mutable state besides the read-only base folder path. \br
    \a sRepositoryPath is the served repository folder path \br
    \a sAction is the served request action \br
*/
void CAzeHousekeeper::handleServedRepository(const QString& sRepositoryPath, const QString& sAction)
{
    if (sAction != Aze::CStrings::s_sParamPush && sAction != Aze::CStrings::s_sParamDeleteBranch)
        return;

    pruneSingleRepository(sRepositoryPath, true);
}

//-------------------------------------------------------------------------------------------------

/************************************************************************/

/*!
    Prunes every repository found under the base folder. \br\br
    The base folder itself and each nested folder holding a `.aze` subfolder
    is pruned with the grace period. Folders inside a `.aze` storage folder
    are skipped. Errors are ignored so one damaged repository never stops the
    sweep. This periodic complement covers repositories staying below the
    request hook threshold between passes. Runs on the server event loop. \br
*/
void CAzeHousekeeper::sweepRepositories()
{
    if (m_sRepositoriesBasePath.isEmpty())
        return;

    QDir tBaseFolder(m_sRepositoriesBasePath);

    if (not tBaseFolder.exists())
        return;

    QString sAbsoluteBase = QDir::cleanPath(tBaseFolder.absolutePath());

    pruneSingleRepository(sAbsoluteBase, false);

    QDirIterator tIterator(sAbsoluteBase, QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDirIterator::Subdirectories);

    while (tIterator.hasNext())
    {
        QString sFolderPath = QDir::cleanPath(tIterator.next());

        if (sFolderPath.contains(QStringLiteral("/.aze/")))
            continue;

        if (sFolderPath.endsWith(QStringLiteral("/.aze")))
            continue;

        if (not isRepositoryFolder(sFolderPath))
            continue;

        pruneSingleRepository(sFolderPath, false);
    }
}

//-------------------------------------------------------------------------------------------------

/************************************************************************/

/*!
    Prunes a single folder repository. \br\br
    When \a bGated is true the stored file count gates the sweep, otherwise
    the prune runs unconditionally. A silent repository keeps best-effort
    failures out of the server output, errors are ignored. \br
    \a sRepositoryPath is the repository folder path \br
    \a bGated tells if the file count threshold applies \br
*/
void CAzeHousekeeper::pruneSingleRepository(const QString& sRepositoryPath, bool bGated)
{
    if (sRepositoryPath.isEmpty())
        return;

    if (not isRepositoryFolder(sRepositoryPath))
        return;

    if (bGated && countStoredFiles(sRepositoryPath) < s_iPruneThresholdFiles)
        return;

    Aze::CRepository tRepository(sRepositoryPath, nullptr, true);

    if (tRepository.ok())
        tRepository.prune(false, s_iPruneRetentionSeconds);
}

//-------------------------------------------------------------------------------------------------

/************************************************************************/

/*!
    Counts stored commit and object files of a repository folder. \br\br
    Returns zero when the folder holds no repository. The count gates the
    automatic prune so the mark-and-sweep walk runs only above threshold. \br
    \a sRepositoryPath is the repository folder path \br
*/
int CAzeHousekeeper::countStoredFiles(const QString& sRepositoryPath)
{
    Aze::CRepository tRepository(sRepositoryPath, nullptr, true);

    if (not tRepository.ok())
        return 0;

    if (IS_NULL(tRepository.database()))
        return 0;

    QDir tCommitFolder(tRepository.database()->commitPath());
    QDir tObjectFolder(tRepository.database()->objectPath());

    int iCount = tCommitFolder.entryList(QDir::Files | QDir::NoSymLinks).count();
    iCount += tObjectFolder.entryList(QDir::Files | QDir::NoSymLinks).count();

    return iCount;
}

//-------------------------------------------------------------------------------------------------

/************************************************************************/

/*!
    Tells if \a sFolderPath holds a repository. \br\br
    A folder holds a repository when its `.aze` subfolder exists. \br
    \a sFolderPath is the folder path to inspect \br
*/
bool CAzeHousekeeper::isRepositoryFolder(const QString& sFolderPath)
{
    if (sFolderPath.isEmpty())
        return false;

    QDir tDataFolder(sFolderPath + QLatin1Char('/') + Aze::CStrings::s_sPathAzeDataRootPath);

    return tDataFolder.exists();
}

//-------------------------------------------------------------------------------------------------
