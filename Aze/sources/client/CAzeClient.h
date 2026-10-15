
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
    int run();

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
    int init();

    //! Creates a branch
    int createBranch();

    //! Deletes a branch
    int deleteBranch();

    //! Switches to a branch
    int switchToBranch();

    //! Prints repo status
    int status();

    //! Stages files
    int stage();

    //! Unstages files
    int unstage();

    //! Reverts files
    int revert();

    //! Moves some files
    int move();

    //! Removes some files
    int remove();

    //! Commits the staged files
    int commit();

    //! Rewrites the tip commit with the staged files
    int amend();

    //! Reverts all files
    int cleanUp();

    //! Show the log
    int log();

    //! Shows a diff between two commits
    int diff();

    //!
    int merge();

    //!
    int saveStash();

    //!
    int popStash();

    //!
    int patch();

    //!
    int setRemoteHost();

    //!
    int dump();

    //-------------------------------------------------------------------------------------------------
    // Remote commands
    //-------------------------------------------------------------------------------------------------

    //!
    int pull();

    //!
    int push();

    //!
    void postNetworkRequest(CXMLNode xRequest);

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
    bool                    m_bOutputStreamIsMine;
    bool                    m_bNetworkAccessFinished;
};

//-------------------------------------------------------------------------------------------------
