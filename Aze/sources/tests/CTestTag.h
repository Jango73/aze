
#pragma once

// Application
#include "CTestFixture.h"

//-------------------------------------------------------------------------------------------------

class CTestTag : public CTestFixture
{
    Q_OBJECT

public:

    explicit CTestTag(const QString& sArgument0);

private slots:

    //! Tests tag creation from the current branch tip
    void testCreateTag();

    //! Tests branch and tag namespace guards
    void testTagGuards();

    //! Tests tag deletion
    void testDeleteTag();

    //! Tests tag push and pull through the branch protocol
    void testPushPullTag();
};

//-------------------------------------------------------------------------------------------------
