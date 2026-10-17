
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

    //! Tests functionalities under heavy load
    void testHeavy();
};

//-------------------------------------------------------------------------------------------------
