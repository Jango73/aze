
// Qt
#include <QtTest/QtTest>

// Application
#include "client/CAzeClient.h"
#include "server/CAzeServer.h"
#include "tests/CTestAze.h"

int main(int argc, char *argv[])
{
    if (argc > 1 && QString(argv[1]) == "run-tests")
    {
        CTestAze test(argv[0]);
        QTEST_SET_MAIN_SOURCE_PATH
        return test.runAllTests();
    }

    if (argc > 1 && QString(argv[1]) == "serve")
    {
        CAzeServer oServer(argc, argv);
        return oServer.run();
    }

    return Aze::CError::toExitCode(CAzeClient(argc, argv).run());
}
