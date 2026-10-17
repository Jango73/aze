
#pragma once

// Application
#include "CTestFixture.h"

//-------------------------------------------------------------------------------------------------

class CTestCache : public CTestFixture
{
    Q_OBJECT

public:

    explicit CTestCache(const QString& sArgument0);

private slots:

    //! Tests the working folder hash cache (hits, misses and invalidation)
    void testFileHashCache();

    //! Tests cache invalidation after commit, merge, received push and branch switch on the same instance
    void testCacheInvalidation();

    //! Tests that the commit parent cache persists in binary format
    void testCommitTreeListBinaryCache();

    //! Tests that the commit parent cache persists in text format
    void testCommitTreeListTextCache();
};

//-------------------------------------------------------------------------------------------------
