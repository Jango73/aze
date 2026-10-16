
#pragma once

// Qt
#include <QObject>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

// AzeLib
#include "CRepository.h"

// Application
#include "../common/CConstants.h"
#include "CAzeClientArguments.h"

//-------------------------------------------------------------------------------------------------

class CAzeClient : public QCoreApplication
{
    Q_OBJECT

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //! Constructor
    CAzeClient(int argc, char *argv[], QTextStream* pOutStream = nullptr);

    //! Destructor
    virtual ~CAzeClient();

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //!
    Aze::CError::EErrorCode run();

    //!
    bool isASainRepository();

    //!
    void processWildCards();

    //!
    void setupRemoteHost(const QString& sHostName);

    //-------------------------------------------------------------------------------------------------
    // Local commands
    //-------------------------------------------------------------------------------------------------

    //! Initializes a repository
    Aze::CError::EErrorCode init();

    //! Creates a branch
    Aze::CError::EErrorCode createBranch();

    //! Deletes a branch
    Aze::CError::EErrorCode deleteBranch();

    //! Switches to a branch
    Aze::CError::EErrorCode switchToBranch();

    //! Prints repo status
    Aze::CError::EErrorCode status();

    //! Stages files
    Aze::CError::EErrorCode stage();

    //! Unstages files
    Aze::CError::EErrorCode unstage();

    //! Reverts files
    Aze::CError::EErrorCode revert();

    //! Moves some files
    Aze::CError::EErrorCode move();

    //! Removes some files
    Aze::CError::EErrorCode remove();

    //! Commits the staged files
    Aze::CError::EErrorCode commit();

    //! Rewrites the tip commit with the staged files
    Aze::CError::EErrorCode amend();

    //! Reverts all files
    Aze::CError::EErrorCode cleanUp();

    //! Show the log
    Aze::CError::EErrorCode log();

    //! Shows a diff between two commits
    Aze::CError::EErrorCode diff();

    //!
    Aze::CError::EErrorCode merge();

    //!
    Aze::CError::EErrorCode saveStash();

    //!
    Aze::CError::EErrorCode popStash();

    //!
    Aze::CError::EErrorCode patch();

    //!
    Aze::CError::EErrorCode setRemoteHost();

    //!
    Aze::CError::EErrorCode dump();

    //-------------------------------------------------------------------------------------------------
    // Remote commands
    //-------------------------------------------------------------------------------------------------

    //!
    Aze::CError::EErrorCode pull();

    //!
    Aze::CError::EErrorCode push();

    //! Clones a repository from the server into a new folder
    Aze::CError::EErrorCode clone();

    //!
    Aze::CError::EErrorCode postNetworkRequest(CXMLNode xRequest);

protected slots:

    void onNetworkAccessFinished(QNetworkReply* pReply);

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    CAzeClientArguments     m_tArguments;
    QTextStream*            m_pOutStream;
    Aze::CRepository*       m_pRepository;
    QNetworkAccessManager*  m_pNetworkAccessManager;
    QNetworkRequest         m_tNetworkRequest;
    Aze::CError::EErrorCode m_eLastError;
    CXMLNode                m_xLastResponse;
    bool                    m_bOutputStreamIsMine;
    bool                    m_bNetworkAccessFinished;
};

//-------------------------------------------------------------------------------------------------
