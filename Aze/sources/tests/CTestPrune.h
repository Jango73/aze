
#pragma once

// Application
#include "CTestFixture.h"

//-------------------------------------------------------------------------------------------------

class CTestPrune : public CTestFixture
{
    Q_OBJECT

public:

    explicit CTestPrune(const QString& sArgument0);

private slots:

    //! Tests prune mark-and-sweep over dangling commits and objects
    void testPrune();
};

//-------------------------------------------------------------------------------------------------
