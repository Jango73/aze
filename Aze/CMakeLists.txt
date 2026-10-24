# Aze: single binary (client commands, "aze serve" server, bundled "run-tests" suites).

set(AZE_SOURCES
    sources/server/CAzeHousekeeper.cpp
    sources/server/CAzeServer.cpp
    sources/server/CAzeServerArguments.cpp
    sources/server/CAzeServerProcessor.cpp
    sources/client/CAzeClient.cpp
    sources/client/CAzeClient_Local.cpp
    sources/client/CAzeClient_Remote.cpp
    sources/client/CAzeClientArguments.cpp
    sources/common/CConstants.cpp
    sources/tests/CTestAze.cpp
    sources/tests/CTestBranch.cpp
    sources/tests/CTestCache.cpp
    sources/tests/CTestDiffPatch.cpp
    sources/tests/CTestFixture.cpp
    sources/tests/CTestMassive.cpp
    sources/tests/CTestMerge.cpp
    sources/tests/CTestPrune.cpp
    sources/tests/CTestRemote.cpp
    sources/tests/CTestServe.cpp
    sources/tests/CTestTag.cpp
    sources/tests/CTestWorkflow.cpp
    sources/main.cpp
)

set(AZE_HEADERS
    sources/server/CAzeHousekeeper.h
    sources/server/CAzeServer.h
    sources/server/CAzeServerArguments.h
    sources/server/CAzeServerProcessor.h
    sources/client/CAzeClient.h
    sources/client/CAzeClientArguments.h
    sources/common/CConstants.h
    sources/tests/CTestAze.h
    sources/tests/CTestBranch.h
    sources/tests/CTestCache.h
    sources/tests/CTestDiffPatch.h
    sources/tests/CTestFixture.h
    sources/tests/CTestMacros.h
    sources/tests/CTestMassive.h
    sources/tests/CTestMerge.h
    sources/tests/CTestPrune.h
    sources/tests/CTestRemote.h
    sources/tests/CTestServe.h
    sources/tests/CTestTag.h
    sources/tests/CTestWorkflow.h
)

add_executable(aze_app
    ${AZE_SOURCES}
    ${AZE_HEADERS}
)

set_target_properties(aze_app PROPERTIES
    OUTPUT_NAME "aze"
    DEBUG_POSTFIX "-dbg"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Aze/bin"
)

target_compile_definitions(aze_app PRIVATE
    "AZE_VERSION_STRING=\"${PROJECT_VERSION}\""
)

target_link_libraries(aze_app PRIVATE
    aze_lib
    Qt6::Core
    Qt6::Xml
    Qt6::Network
    Qt6::Test
)

install(TARGETS aze_app RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
