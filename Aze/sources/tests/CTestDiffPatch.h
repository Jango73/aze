
#pragma once

// Application
#include "CTestFixture.h"

//-------------------------------------------------------------------------------------------------

class CTestDiffPatch : public CTestFixture
{
    Q_OBJECT

public:

    explicit CTestDiffPatch(const QString& sArgument0);

private slots:

    //! Tests patch functionality
    void testPatch();

    //! Tests diff variants (no argument, one id, file name)
    void testDiff();
};

//-------------------------------------------------------------------------------------------------
