
#pragma once

// Application
#include "CTestFixture.h"

//-------------------------------------------------------------------------------------------------

class CTestWorkflow : public CTestFixture
{
    Q_OBJECT

public:

    explicit CTestWorkflow(const QString& sArgument0);

private slots:

    //! Test stage and commit functionalities
    void testCommit();

    //! Test unstage functionality
    void testUnstage();

    //! Test amend functionality
    void testAmend();

    //! Test file deletion functionality
    void testDelete();

    //! Test file move with history preservation
    void testMove();

    //! Test revert of working folder files to the tip commit
    void testRevert();

    //! Test clean-up clearing the stage and reverting all files
    void testCleanUp();
};

//-------------------------------------------------------------------------------------------------
