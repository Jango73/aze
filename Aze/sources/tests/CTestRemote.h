
#pragma once

// Application
#include "CTestFixture.h"

//-------------------------------------------------------------------------------------------------

class CTestRemote : public CTestFixture
{
    Q_OBJECT

public:

    explicit CTestRemote(const QString& sArgument0);

private slots:

    //! Tests push request building and applying to another repository
    void testRemotePush();

    //! Tests pull convergence after push then pull
    void testPull();

    //! Tests cloning of branches, branch tips and file contents
    void testClone();
};

//-------------------------------------------------------------------------------------------------
