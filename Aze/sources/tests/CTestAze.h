
#pragma once

// Application
#include "CTestFixture.h"

//-------------------------------------------------------------------------------------------------

//! Test orchestrator: runs every test suite sequentially, main stays simple.
class CTestAze : public CTestFixture
{
    Q_OBJECT

public:

    explicit CTestAze(const QString& sArgument0);

    //! Runs every test suite sequentially, returns the combined status.
    int runAllTests();
};

//-------------------------------------------------------------------------------------------------
