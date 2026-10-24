
#pragma once

// Application
#include "CTestFixture.h"

//-------------------------------------------------------------------------------------------------

//! Tests list-branches and rename-branch commands.
class CTestBranch : public CTestFixture
{
    Q_OBJECT

public:

    explicit CTestBranch(const QString& sArgument0);

private slots:

    //! Tests branch inventory with the current branch marked
    void testListBranches();

    //! Tests branch renaming with current pointer move and error guards
    void testRenameBranch();
};

//-------------------------------------------------------------------------------------------------
