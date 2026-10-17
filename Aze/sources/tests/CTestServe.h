
#pragma once

// Application
#include "CTestFixture.h"

//-------------------------------------------------------------------------------------------------

class CTestServe : public CTestFixture
{
    Q_OBJECT

public:

    explicit CTestServe(const QString& sArgument0);

private slots:

    //! Tests push request applying through the server layer
    void testServePush();

    //! Tests delete-branch request applying through the server layer
    void testServeDeleteBranch();

    //! Tests repository path confinement under the server base folder
    void testServePathIsolation();

    //! Tests HTTP and HTTPS connectivity against a live server processor
    void testServeConnectivity();
};

//-------------------------------------------------------------------------------------------------
