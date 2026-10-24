
#pragma once

// Application
#include "CTestFixture.h"

//-------------------------------------------------------------------------------------------------

class CTestServe : public CTestFixture
{
    Q_OBJECT

public:

    explicit CTestServe(const QString& sArgument0);

private slots:

    //! Tests push request applying through the server layer
    void testServePush();

    //! Tests push announce and delta data through the server layer
    void testServePushAnnounce();

    //! Tests delta pull carrying missing commits only through the server layer
    void testServePullDelta();

    //! Tests delete-branch request applying through the server layer
    void testServeDeleteBranch();

    //! Tests repository path confinement under the server base folder
    void testServePathIsolation();

    //! Tests HTTP and HTTPS connectivity against a live server processor
    void testServeConnectivity();

    //! Tests loopback binding and invalid address refusal
    void testServeBindAddress();

    //! Tests POST body and push payload limit enforcement
    void testServePayloadLimits();

    //! Tests repository path resolution across symlink swaps
    void testServeSymlinkSwap();

    //! Tests portal client certificate enforcement on the HTTPS endpoint
    void testServeMutualTls();

    //! Tests --ssl-verify-peer and --ssl-ca option validation with fail-closed errors
    void testServeVerifyPeerOptions();
};

//-------------------------------------------------------------------------------------------------
