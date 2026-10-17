
#pragma once

// Qt
#include <QObject>
#include <QCoreApplication>
#include <QString>

// qt-plus
#include "Macros.h"

// AzeLib
#include "CRepository.h"

// Application
#include "../common/CConstants.h"
#include "CAzeHousekeeper.h"
#include "CAzeServerArguments.h"
#include "CAzeServerProcessor.h"

//-------------------------------------------------------------------------------------------------

class CAzeServer : public QCoreApplication
{
    Q_OBJECT

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

    Q_FAST_PROPERTY(QString, s, repositoriesBasePath, RepositoriesBasePath)

public:

    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //! Constructor
    CAzeServer(int argc, char *argv[]);

    //! Destructor
    virtual ~CAzeServer();

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //! Returns the HTTPS initialization error, eNone when the server can run
    Aze::CError::EErrorCode initializationError() const;

    //! Returns the detail of the HTTPS initialization error
    QString initializationErrorDetail() const;

    //! Runs the server event loop, returns a clean exit code on invalid HTTPS configuration
    int run();

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

protected:

    CAzeServerArguments     m_tArguments;
    CAzeServerProcessor*    m_pProcessor;
    CAzeHousekeeper*        m_pHousekeeper;
    Aze::CError::EErrorCode m_eInitializationError;
    QString                 m_sInitializationDetail;
};

//-------------------------------------------------------------------------------------------------
