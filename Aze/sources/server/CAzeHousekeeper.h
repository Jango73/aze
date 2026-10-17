
#pragma once

// Qt
#include <QObject>
#include <QString>
#include <QTimer>

//-------------------------------------------------------------------------------------------------

class CAzeHousekeeper : public QObject
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

    static constexpr int s_iPruneThresholdFiles = 500;
    static constexpr qint64 s_iPruneRetentionSeconds = 86400;
    static constexpr int s_iPruneIntervalMs = 3600000;

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //! Constructor, starts the periodic sweep over sRepositoriesBasePath
    explicit CAzeHousekeeper(const QString& sRepositoriesBasePath, QObject* parent = nullptr);

    //! Destructor
    virtual ~CAzeHousekeeper() override;

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //! Runs a threshold-gated best-effort prune for a served push or delete-branch request
    //! Callable from worker threads, never alters the originating response
    void handleServedRepository(const QString& sRepositoryPath, const QString& sAction);

public slots:

    //! Prunes every repository found under the base folder, ignoring errors
    void sweepRepositories();

protected:

    //-------------------------------------------------------------------------------------------------
    // Protected control methods
    //-------------------------------------------------------------------------------------------------

    //! Prunes a single folder repository, threshold-gated when bGated is true
    void pruneSingleRepository(const QString& sRepositoryPath, bool bGated);

    //! Counts stored commit and object files, returns the threshold check input
    static int countStoredFiles(const QString& sRepositoryPath);

    //! Tells if a folder path holds a repository through its `.aze` subfolder
    static bool isRepositoryFolder(const QString& sFolderPath);

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    QString m_sRepositoriesBasePath;
    QTimer* m_pSweepTimer;
};

//-------------------------------------------------------------------------------------------------
