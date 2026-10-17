
#pragma once

// Application
#include "CTestFixture.h"

//-------------------------------------------------------------------------------------------------

class CTestMerge : public CTestFixture
{
    Q_OBJECT

public:

    explicit CTestMerge(const QString& sArgument0);

private slots:

    //! Tests merge functionality
    void testMerge();

    //! Tests merge error paths and conflict resolution
    void testMergeErrorsAndResolution();
};

//-------------------------------------------------------------------------------------------------
