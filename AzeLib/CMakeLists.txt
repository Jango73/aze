# AzeLib: reusable versioning engine, built as a shared library.
# The qt-plus subset (qt-plus-nolib-web) is compiled into this library,
# mirroring the qmake build (include of qt-plus-nolib-web.pri).

set(AZELIB_SOURCES
    sources/commits/CCommitDiffFunctions.cpp
    sources/commits/CCommitFunctions.cpp
    sources/commits/CCommitMergeFunctions.cpp
    sources/commits/CCommitTreeList.cpp
    sources/CEnums.cpp
    sources/CError.cpp
    sources/CFileHashCache.cpp
    sources/commands/CBaseCommand.cpp
    sources/commands/CAmendCommand.cpp
    sources/commands/CCleanUpCommand.cpp
    sources/commands/CCloneCommand.cpp
    sources/commands/CCommitCommand.cpp
    sources/commands/CCreateBranchCommand.cpp
    sources/commands/CCreateTagCommand.cpp
    sources/commands/CDeleteBranchCommand.cpp
    sources/commands/CDeleteTagCommand.cpp
    sources/commands/CDiffCommand.cpp
    sources/commands/CDumpCommand.cpp
    sources/commands/CInitRepositoryCommand.cpp
    sources/commands/CLogCommand.cpp
    sources/commands/CListBranchesCommand.cpp
    sources/commands/CListTagsCommand.cpp
    sources/commands/CMergeCommand.cpp
    sources/commands/CMoveCommand.cpp
    sources/commands/CPatchCommand.cpp
    sources/commands/CPopStashCommand.cpp
    sources/commands/CPruneCommand.cpp
    sources/commands/CPullCommand.cpp
    sources/commands/CPushCommand.cpp
    sources/commands/CRenameBranchCommand.cpp
    sources/commands/CRemoveCommand.cpp
    sources/commands/CRevertCommand.cpp
    sources/commands/CSaveStashCommand.cpp
    sources/commands/CSetRemoteHostCommand.cpp
    sources/commands/CStageCommand.cpp
    sources/commands/CStatusCommand.cpp
    sources/commands/CSwitchToBranchCommand.cpp
    sources/commands/CUnstageCommand.cpp
    sources/CRemoteHostInfo.cpp
    sources/CRemoteRepository.cpp
    sources/CRepository.cpp
    sources/CStrings.cpp
    sources/CUtils.cpp
    sources/objects/CBranch.cpp
    sources/objects/CTag.cpp
    sources/objects/CCommit.cpp
    sources/objects/CDatabase.cpp
    sources/objects/CFile.cpp
    sources/objects/CObject.cpp
    sources/objects/CStash.cpp
)

set(AZELIB_HEADERS
    sources/commits/CCommitDiffFunctions.h
    sources/commits/CCommitFunctions.h
    sources/commits/CCommitMergeFunctions.h
    sources/commits/CCommitTreeList.h
    sources/CEnums.h
    sources/CError.h
    sources/CFileHashCache.h
    sources/commands/CBaseCommand.h
    sources/commands/CAmendCommand.h
    sources/commands/CCleanUpCommand.h
    sources/commands/CCloneCommand.h
    sources/commands/CCommitCommand.h
    sources/commands/CCreateBranchCommand.h
    sources/commands/CCreateTagCommand.h
    sources/commands/CDeleteBranchCommand.h
    sources/commands/CDeleteTagCommand.h
    sources/commands/CDiffCommand.h
    sources/commands/CDumpCommand.h
    sources/commands/CInitRepositoryCommand.h
    sources/commands/CLogCommand.h
    sources/commands/CListBranchesCommand.h
    sources/commands/CListTagsCommand.h
    sources/commands/CMergeCommand.h
    sources/commands/CMoveCommand.h
    sources/commands/CPatchCommand.h
    sources/commands/CPopStashCommand.h
    sources/commands/CPruneCommand.h
    sources/commands/CPullCommand.h
    sources/commands/CPushCommand.h
    sources/commands/CRenameBranchCommand.h
    sources/commands/CRemoveCommand.h
    sources/commands/CRevertCommand.h
    sources/commands/CSaveStashCommand.h
    sources/commands/CSetRemoteHostCommand.h
    sources/commands/CStageCommand.h
    sources/commands/CStatusCommand.h
    sources/commands/CSwitchToBranchCommand.h
    sources/commands/CUnstageCommand.h
    sources/CRemoteHostInfo.h
    sources/CRemoteRepository.h
    sources/CRepository.h
    sources/CStrings.h
    sources/CUtils.h
    sources/objects/CBranch.h
    sources/objects/CTag.h
    sources/objects/CCommit.h
    sources/objects/CDatabase.h
    sources/objects/CFile.h
    sources/objects/CObject.h
    sources/objects/CStash.h
    sources/dtl/dtl/Conflict.hpp
    sources/dtl/dtl/Diff.hpp
    sources/dtl/dtl/Diff3.hpp
    sources/dtl/dtl/dtl.hpp
    sources/dtl/dtl/functors.hpp
    sources/dtl/dtl/Lcs.hpp
    sources/dtl/dtl/Sequence.hpp
    sources/dtl/dtl/Ses.hpp
    sources/dtl/dtl/variables.hpp
)

set(QTPLUS_SOURCES
    ../qt-plus/source/cpp/CSingletonPool.cpp
    ../qt-plus/source/cpp/CDumpable.cpp
    ../qt-plus/source/cpp/CXMLNodable.cpp
    ../qt-plus/source/cpp/CXMLNode.cpp
    ../qt-plus/source/cpp/CLogger.cpp
    ../qt-plus/source/cpp/CMacroable.cpp
    ../qt-plus/source/cpp/CTracableMutex.cpp
    ../qt-plus/source/cpp/CTimeSampler.cpp
    ../qt-plus/source/cpp/CTextGenerator.cpp
    ../qt-plus/source/cpp/CSecureContext.cpp
    ../qt-plus/source/cpp/File/CFileUtilities.cpp
    ../qt-plus/source/cpp/File/CRollingFiles.cpp
    ../qt-plus/source/cpp/File/CShardFileStore.cpp
    ../qt-plus/source/cpp/File/CDirectoryListing.cpp
    ../qt-plus/source/cpp/Web/CWebComposer.cpp
    ../qt-plus/source/cpp/Web/CWebContext.cpp
    ../qt-plus/source/cpp/Web/CHTTPServer.cpp
    ../qt-plus/source/cpp/Web/CHTTPRequestProcessor.cpp
    ../qt-plus/source/cpp/Web/CWebSession.cpp
    ../qt-plus/source/cpp/Web/CDynamicHTTPServer.cpp
    ../qt-plus/source/cpp/Web/WebControls/CWebButton.cpp
    ../qt-plus/source/cpp/Web/WebControls/CWebControl.cpp
    ../qt-plus/source/cpp/Web/WebControls/CWebDiv.cpp
    ../qt-plus/source/cpp/Web/WebControls/CWebFactory.cpp
    ../qt-plus/source/cpp/Web/WebControls/CWebLabel.cpp
    ../qt-plus/source/cpp/Web/WebControls/CWebPage.cpp
    ../qt-plus/source/cpp/Web/WebControls/CWebTextBox.cpp
    ../qt-plus/source/cpp/Web/WebControls/CWebTextEdit.cpp
    ../qt-plus/source/cpp/Web/WebControls/CWebFileInput.cpp
    ../qt-plus/source/cpp/Web/WebControls/CWebModelControl.cpp
    ../qt-plus/source/cpp/Web/WebControls/CWebListView.cpp
    ../qt-plus/source/cpp/ISerializable.cpp
    ../qt-plus/source/cpp/rsa/source/BigInt.cpp
    ../qt-plus/source/cpp/rsa/source/Key.cpp
    ../qt-plus/source/cpp/rsa/source/KeyPair.cpp
    ../qt-plus/source/cpp/rsa/source/PrimeGenerator.cpp
    ../qt-plus/source/cpp/rsa/source/RSA.cpp
    ../qt-plus/source/cpp/roke/ROKE.cpp
)

set(QTPLUS_HEADERS
    ../qt-plus/source/cpp/Macros.h
    ../qt-plus/source/cpp/CFactory.h
    ../qt-plus/source/cpp/CSingleton.h
    ../qt-plus/source/cpp/CSingletonPool.h
    ../qt-plus/source/cpp/CDumpable.h
    ../qt-plus/source/cpp/CXMLNodable.h
    ../qt-plus/source/cpp/CXMLNode.h
    ../qt-plus/source/cpp/QTree.h
    ../qt-plus/source/cpp/CLogger.h
    ../qt-plus/source/cpp/CMacroable.h
    ../qt-plus/source/cpp/CTracableMutex.h
    ../qt-plus/source/cpp/CTimeSampler.h
    ../qt-plus/source/cpp/CTextGenerator.h
    ../qt-plus/source/cpp/CSecureContext.h
    ../qt-plus/source/cpp/File/CFileUtilities.h
    ../qt-plus/source/cpp/File/CRollingFiles.h
    ../qt-plus/source/cpp/File/CShardFileStore.h
    ../qt-plus/source/cpp/File/CDirectoryListing.h
    ../qt-plus/source/cpp/Web/CWebComposer.h
    ../qt-plus/source/cpp/Web/CWebContext.h
    ../qt-plus/source/cpp/Web/CHTTPServer.h
    ../qt-plus/source/cpp/Web/CHTTPRequestProcessor.h
    ../qt-plus/source/cpp/Web/CWebSession.h
    ../qt-plus/source/cpp/Web/CDynamicHTTPServer.h
    ../qt-plus/source/cpp/Web/WebControls/CWebButton.h
    ../qt-plus/source/cpp/Web/WebControls/CWebControl.h
    ../qt-plus/source/cpp/Web/WebControls/CWebDiv.h
    ../qt-plus/source/cpp/Web/WebControls/CWebFactory.h
    ../qt-plus/source/cpp/Web/WebControls/CWebLabel.h
    ../qt-plus/source/cpp/Web/WebControls/CWebPage.h
    ../qt-plus/source/cpp/Web/WebControls/CWebTextBox.h
    ../qt-plus/source/cpp/Web/WebControls/CWebTextEdit.h
    ../qt-plus/source/cpp/Web/WebControls/CWebFileInput.h
    ../qt-plus/source/cpp/Web/WebControls/CWebModelControl.h
    ../qt-plus/source/cpp/Web/WebControls/CWebListView.h
    ../qt-plus/source/cpp/ISerializable.h
    ../qt-plus/source/cpp/IJSONModelProvider.h
    ../qt-plus/source/cpp/CInterpolator.h
    ../qt-plus/source/cpp/rsa/source/BigInt.h
    ../qt-plus/source/cpp/rsa/source/Key.h
    ../qt-plus/source/cpp/rsa/source/KeyPair.h
    ../qt-plus/source/cpp/rsa/source/PrimeGenerator.h
    ../qt-plus/source/cpp/rsa/source/RSA.h
    ../qt-plus/source/cpp/roke/ROKE.h
)

add_library(aze_lib SHARED
    ${AZELIB_SOURCES}
    ${AZELIB_HEADERS}
    ${QTPLUS_SOURCES}
    ${QTPLUS_HEADERS}
)

set_target_properties(aze_lib PROPERTIES
    OUTPUT_NAME "aze"
    DEBUG_POSTFIX "-dbg"
    VERSION 1.0.0
    SOVERSION 1
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/AzeLib/bin"
)

target_compile_definitions(aze_lib PRIVATE
    AZE_LIBRARY
    "AZELIB_VERSION_STRING=\"${PROJECT_VERSION}\""
    QTPLUS_LIBRARY
)

target_include_directories(aze_lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/sources"
    "${CMAKE_CURRENT_SOURCE_DIR}/../qt-plus/source/cpp"
)

target_link_libraries(aze_lib PUBLIC
    Qt6::Core
    Qt6::Xml
    Qt6::Network
    Qt6::Test
)

install(TARGETS aze_lib LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
