
// Qt
#include <QtTest/QtTest>
#include <QDebug>

// Application
#include "client/CAzeClient.h"
#include "server/CAzeServer.h"
#include "tests/CTestAze.h"

#if defined(__has_include)
#  if __has_include("BuildInfo.h")
#    include "BuildInfo.h"
#  endif
#endif

#ifndef BUILD_TIMESTAMP
#define BUILD_TIMESTAMP "unknown"
#endif

int main(int argc, char *argv[])
{
    qInfo() << QString("Aze %1 built %2").arg(AZE_VERSION_STRING).arg(BUILD_TIMESTAMP);
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
