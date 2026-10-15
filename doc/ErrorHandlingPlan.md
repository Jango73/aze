# Error Handling Unification Plan

## Goal
Unify error handling with a single format in all layers, a single enumeration of all possible error codes, and a single function mapping an error code to a `QString`.

## Scope And Constraints
- `qt-plus` remains untouched. `OUT_ERROR`, `OUT_INFO`, `IS_NULL`, `IS_NOT_NULL`, `SAFE_USE`, `SAFE_DELETE` remain in use.
- `AzeLib` holds the reusable error module. `Aze` depends on `AzeLib`, never the reverse.
- Dependencies remain unidirectional. No cycle between client, server, and engine.
- No global variable is introduced.
- Public `AzeLib` headers remain downstream-safe.
- Qt containers and `QString` remain in use for text and collections.
- Numeric exit codes `0-30` defined in `CConstants` remain stable for scripts and continuous integration.

## Inventory Of Existing Mechanisms
- `CConstants::s_iError_*`: 31 integer codes returned as process exit codes by `CAzeClient`.
- `CMergeCommand::EMergeError`: separate merge enum, mapped manually in `CAzeClient::merge`.
- `CStrings::s_sText*`: free error texts shown through `CRepository::tellError(QString)`.
- `CRemoteRepository::applyPushRequest` and helpers: `bool` plus free `QString& sErrorMessage`.
- `CAzeServerProcessor::servePush` / `servePull`: `CXMLNode` with `result=success|error` plus free `message` attribute.
- `ERROR_WHEN_FALSE` macros in `CAzeClient`: repeated guard plus return pattern.
- `qFatal`: restricted to unrecoverable startup failure (`CAzeServer` HTTPS configuration, `CTestAze` repository root check). Out of scope except TODO-030.
- Known defects to fix during migration: `CAzeClient::run` returns `0` on unknown command, `CAzeServerProcessor::getContent` builds an error node without assigning it on invalid repository, `servePull` is a stub, `serveRequest` returns an empty node on unknown action.

## Design

### New Module `Aze::CError`
Location: `AzeLib/sources/CError.h`, `AzeLib/sources/CError.cpp`.
Class: `Aze::CError : public QObject` with `Q_OBJECT` and `Q_ENUM`, same pattern as `CEnums` and `CStrings`.

Responsibilities:
- Define `enum EErrorCode` with all error codes.
- Map each code to a message template.
- Build server result nodes.
- Convert a code to a process exit code.

Public interface:
- `enum EErrorCode`
- `static QString message(EErrorCode eCode)`
- `static QString message(EErrorCode eCode, const QString& sDetail)`
- `static CXMLNode toResultNode(EErrorCode eCode, const QString& sDetail = QString())`
- `static EErrorCode fromResultNode(const CXMLNode& xNode)`
- `static int toExitCode(EErrorCode eCode)`
- `static bool isOk(EErrorCode eCode)`

Message with detail applies `QString::arg(sDetail)` to templates containing `%1`. Examples: file name for `NoSuchFile`, object identifier for `ObjectStoreFailed`, branch name for `UnknownBranch` where useful.

### Enumeration Content
Preserved values `0-30` from `CConstants`:
`None`, `CouldNotAddFiles`, `CouldNotAmend`, `CouldNotCommit`, `CouldNotCreateBranch`, `CouldNotDeleteBranch`, `CouldNotMerge`, `CouldNotMoveFiles`, `CouldNotPatch`, `CouldNotPopStash`, `CouldNotReadStage`, `CouldNotRemoveFiles`, `CouldNotRestoreStash`, `CouldNotRevertFiles`, `CouldNotSaveStash`, `CouldNotSetCurrentBranch`, `CouldNotUnstageFiles`, `CouldNotWriteCurrentBranch`, `CouldNotWriteGeneralInformation`, `CouldNotWriteStage`, `NoBranchNameGiven`, `NoFileNameGiven`, `NoRemoteHostNameGiven`, `NotARepository`, `MergeHasConflicts`, `UnknownCommand`, `UnknownSwitch`, `UnknownBranch`, `DirtyWorkingFolder`, `NothingToMerge`, `StageNotEmpty`.

Added values for errors expressed today as free text:
`Generic`, `NoCurrentBranch`, `NoStagingCommit`, `NoTipCommit`, `NoSuchFile`, `BranchAlreadyExists`, `BranchDoesNotExist`, `CannotDeleteCurrentBranch`, `CannotMergeSelf`, `NoCommonAncestor`, `NoCommitToMerge`, `CommitEmpty`, `CouldNotRemoveFile`, `CouldNotApplyPatch`, `ObjectStoreFailed`, `PushRequestInvalid`, `PushNotFastForward`, `StashSaveFailed`, `StashPopFailed`, `NothingToStash`, `NetworkError`, `InvalidResponse`, `UnknownAction`, `EmptyRequest`.

`MergeHasConflicts` denotes success with conflicts, not failure. It remains a distinct non-zero code returned on the success path.

### Single Format In All Layers
- Engine: functions return `EErrorCode`. Success equals `eNone`. No `bool` plus separate text channel.
- Client: single rendering point calls `CError::message(eCode, sDetail)`, logs through `OUT_ERROR`, returns `CError::toExitCode(eCode)`.
- Server: single rendering point calls `CError::toResultNode(eCode, sDetail)`, parsed by clients through `CError::fromResultNode`.
- Silent mode: `CRepository::tellError(EErrorCode, detail)` suppresses output when `m_bSilent` is set. The code still propagates to the caller.

## File By File Changes

### 1. Create `CError`
- Add `AzeLib/sources/CError.h` with declaration order: macros, type definitions, inline functions, external functions, other members.
- Add `AzeLib/sources/CError.cpp` with doxygen header on each function and 75-character separators.
- Register the new files in `AzeLib` build files.
- Move error message templates from `CStrings::s_sText*` into the `CError` table. Keep parameter keys (`s_sParamResult`, `s_sParamError`, `s_sParamMessage`, `s_sParamCode`) in `CStrings`.

### 2. Migrate `CRepository` And Commands
- `CRepository::tellError(QString)` becomes `tellError(EErrorCode, const QString& sDetail = QString())`.
- `CRepository` command methods (`init`, `createBranch`, `deleteBranch`, `switchToBranch`, `stage`, `unstage`, `remove`, `commit`, `amend`, `revert`, `merge`, `saveStash`, `popStash`, `patch`, `readGeneralInformation`, `writeGeneralInformation`, `writeCurrentBranch`, `writeStage`, `clearStage`) return `EErrorCode`.
- `CBaseCommand::execute()` returns `EErrorCode`.
- All `*Command::execute()` implementations return the precise code and call `tellError` with that code plus optional detail.
- Remove `CMergeCommand::EMergeError` and `m_iMergeError`. `CMergeCommand::execute` returns `eUnknownBranch`, `eDirtyWorkingFolder`, `eNothingToMerge`, `eStageNotEmpty`, or `eGeneric`. Keep `bool& bHasConflicts` as independent out parameter.
- Remove dead `CTwoWayMergeCommand` duplication or align it with the same contract (see TODO-040).

### 3. Migrate `CRemoteRepository`
- `applyPushRequest`, `storeRequestObjects`, `storeRequestCommits`, `updateBranchFromPush` return `EErrorCode`.
- Remove all `QString& sErrorMessage` out parameters.
- Return `ePushRequestInvalid`, `eObjectStoreFailed`, `ePushNotFastForward` with detail set to object identifier, branch name, or tip identifier where applicable.
- Add pull counterparts on the same contract when pull payload handling is implemented (TODO-018, TODO-020).

### 4. Migrate `CAzeServerProcessor`
- `serveRequest`, `servePull`, `servePush` build responses through `CError::toResultNode`.
- Fix `getContent`: assign the error node when `repository.ok()` is false instead of dropping it (TODO-015).
- Return `eUnknownAction` for unknown actions instead of an empty node (TODO-027).
- Serve non-JSON or empty requests with `eEmptyRequest` or `eInvalidResponse` instead of a silent empty response (TODO-028).
- Implement `servePull` on the same contract (TODO-019).

### 5. Migrate `CAzeClient`
- Replace `ERROR_WHEN_FALSE(cond, intCode)` guards with an `EErrorCode`-based guard defined in `Aze` (not qt-plus). The guard logs `CError::message` once and returns the exit code.
- Each local command propagates the engine code directly. Remove manual `switch(iMergeError)` mapping in `merge`.
- `isASainRepository` returns `eNotARepository` through the same path and logs `CError::message(eNotARepository)`.
- Fix unknown command path to return `eUnknownCommand` instead of `0`.
- Store and apply server responses in `onNetworkAccessFinished` through `CError::fromResultNode` (TODO-012).
- Keep `CConstants::s_iError_*` as deprecated aliases during migration, then remove them. Keep command and switch tables in `CConstants` unchanged.

### 6. Migrate Tests
- Extend `CTestAze` to assert `EErrorCode` values on failure paths: unknown branch merge, dirty folder merge, empty stage merge, invalid push request, non fast-forward push.
- Keep test isolation in `tmp/aze-tests` through `CTestAze::testDirectoryPath`. No `.aze` folder is committed.

## Validation
- Build with `scripts/linux/build.sh --debug` then `--release`, sequentially.
- Run `scripts/linux/ci-local.sh --debug` then `--release`.
- Verify CLI exit codes on representative failures: outside repository, unknown branch, dirty merge, invalid push.
- Verify server push responses contain `result`, `code`, and `message` fields.

## Risks
- Wide call-site churn from `bool` to `EErrorCode`. Mitigation: migrate one group at a time in the order above, keep deprecated aliases until the client migration is complete.
- Message detail loss if callers omit `sDetail`. Mitigation: require detail for codes with `%1` placeholder in code review.
- Conflict state confusion. Mitigation: `bHasConflicts` remains separate from the error code; `eMergeHasConflicts` is used only on the client success-with-conflicts path.
