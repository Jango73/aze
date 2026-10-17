
#pragma once

// Application
#include "CTestFixture.h"

//-------------------------------------------------------------------------------------------------

class CTestMassive : public CTestFixture
{
    Q_OBJECT

public:

    explicit CTestMassive(const QString& sArgument0);

private slots:

    //! Tests a massive repository, only the final state is verified
    void testMassive();
};

//-------------------------------------------------------------------------------------------------
