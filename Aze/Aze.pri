
# Sources

HEADERS += \
    sources/server/CAzeHousekeeper.h \
    sources/server/CAzeServer.h \
    sources/server/CAzeServerArguments.h \
    sources/server/CAzeServerProcessor.h \
    sources/client/CAzeClient.h \
    sources/client/CAzeClientArguments.h \
    sources/common/CConstants.h \
    sources/tests/CTestAze.h \
    sources/tests/CTestBranch.h \
    sources/tests/CTestCache.h \
    sources/tests/CTestDiffPatch.h \
    sources/tests/CTestFixture.h \
    sources/tests/CTestMacros.h \
    sources/tests/CTestMassive.h \
    sources/tests/CTestMerge.h \
    sources/tests/CTestPrune.h \
    sources/tests/CTestRemote.h \
    sources/tests/CTestServe.h \
    sources/tests/CTestTag.h \
    sources/tests/CTestWorkflow.h \

SOURCES += \
    sources/server/CAzeHousekeeper.cpp \
    sources/server/CAzeServer.cpp \
    sources/server/CAzeServerArguments.cpp \
    sources/server/CAzeServerProcessor.cpp \
    sources/client/CAzeClient.cpp \
    sources/client/CAzeClient_Local.cpp \
    sources/client/CAzeClient_Remote.cpp \
    sources/client/CAzeClientArguments.cpp \
    sources/common/CConstants.cpp \
    sources/tests/CTestAze.cpp \
    sources/tests/CTestBranch.cpp \
    sources/tests/CTestCache.cpp \
    sources/tests/CTestDiffPatch.cpp \
    sources/tests/CTestFixture.cpp \
    sources/tests/CTestMassive.cpp \
    sources/tests/CTestMerge.cpp \
    sources/tests/CTestPrune.cpp \
    sources/tests/CTestRemote.cpp \
    sources/tests/CTestServe.cpp \
    sources/tests/CTestTag.cpp \
    sources/tests/CTestWorkflow.cpp \
    sources/main.cpp \

# Deployment

LIB_VERSION = 65

unix {
    QT_LIB_NAMES = \
        libicudata.so.$$LIB_VERSION \
        libicui18n.so.$$LIB_VERSION \
        libicuuc.so.$$LIB_VERSION \
        libQt5Core.so.5 \
        libQt5Gui.so.5 \
        libQt5Network.so.5 \
        libQt5XcbQpa.so.5 \
        libQt5Xml.so.5

    QT_PLUGIN_NAMES = \
        platforminputcontexts/libcomposeplatforminputcontextplugin.so \
        platforms/libqxcb.so \
        xcbglintegrations/libqxcb-egl-integration.so \
        xcbglintegrations/libqxcb-glx-integration.so

    DEPLOY_NAMES = \
        deploy/linux/"$$TARGET".sh \
        deploy/linux/qt.conf

    LIB_COPY_NAMES = \
        ../AzeLib/bin/lib"$$TARGET".so.1

    EXEC_NAMES = \
        "$$TARGET".sh
} else {
    CONFIG(debug, debug|release) {
        QT_BIN_NAMES = \
            libgcc_s_seh-1.dll \
            libwinpthread-1.dll \
            # libstdc++-6.dll \
            Qt5Cored.dll \
            Qt5Guid.dll \
            Qt5Networkd.dll \
            Qt5Testd.dll \
            Qt5Xmld.dll
    } else {
        QT_BIN_NAMES = \
            libgcc_s_seh-1.dll \
            libwinpthread-1.dll \
            # libstdc++-6.dll \
            Qt5Core.dll \
            Qt5Gui.dll \
            Qt5Network.dll \
            Qt5Test.dll \
            Qt5Xml.dll
    }

    QT_PLUGIN_NAMES = \
        platforms/qwindows.dll

    DEPLOY_NAMES = \
        deploy/windows/qt.conf

    LIB_COPY_NAMES = \
        ../AzeLib/bin/$$TARGET.dll

    EXEC_NAMES =
}

QT_QML_NAMES =
