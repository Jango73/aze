# TODO

## Stage and unstage
- [x] TODO-001 - Add an `unstage` test in `CTestAze` because `stage` is covered and `unstage` has no coverage.
- [x] TODO-002 - Unify the error code returned by `CAzeClient::unstage` which reuses `s_iError_CouldNotAddFiles` instead of a dedicated code.

## Commit and amend
- [x] TODO-003 - Implement `amend` in the engine with rewrite of the tip commit without creation of a new parent.
- [x] TODO-004 - Expose `amend` in `CAzeClient`, `CConstants` and the argument parser with author and message options.
- [x] TODO-005 - Add an `amend` test which verifies parent identifier preservation and message update.
- [x] TODO-006 - Fix `CAzeClient::commit` which returns `s_iError_CouldNotRemoveFiles` on commit failure.

## Branch creation, deletion and merge
- [x] TODO-007 - Propagate `create-branch` through `push` because the push request carries branch, root and tip and creates the missing branch on the server.
- [x] TODO-008 - Move branch deletion guards into the engine because only the client refuses the current branch.
- [x] TODO-009 - Refuse deletion of an unmerged branch with a distinct error code (unknown branches are refused with `eBranchDoesNotExist`).
- [x] TODO-046 - Add a `force` option to `delete-branch` which deletes a branch not merged into the current branch because TODO-009 refuses it without an override.
- [x] TODO-010 - Add a successful deletion test which verifies disappearance of the branch file.
- [ ] TODO-011 - Implement `move` with history preservation because `CAzeClient::move` is a stub which writes only the staging area.
- [x] TODO-039 - Add the missing `merge` help text in `CConstants::initCommandMap` because every other branch command is documented there.
- [x] TODO-040 - Remove or repair the dead `CTwoWayMergeCommand.cpp` which redefines `CMergeCommand` with a stale two-argument constructor and is excluded from `AzeLib.pri`.
- [ ] TODO-041 - Unify the self-merge guard in the engine because `CAzeClient::merge` refuses it by branch name while `CMergeCommand` refuses it by tip identifier with a different message.
- [x] TODO-042 - Return distinct error codes for merge failures (unknown branch, dirty working folder, nothing to merge) instead of reusing `s_iError_CouldNotMerge` on every path.
- [x] TODO-043 - Refuse `merge` when the stage is not empty because `CMergeCommand` overwrites merge parents without checking staged files.
- [x] TODO-044 - Handle fast forward in `CMergeCommand` when the current tip is an ancestor of the merged branch instead of creating a merge commit with two parents.
- [x] TODO-045 - Extend the conflict resolution test to verify both parents after resolving and committing because `testMergeErrorsAndResolution` checks file content only.

## Push client and server
- [x] TODO-012 - Store and apply the server response in `CAzeClient::onNetworkAccessFinished` which ignores success and errors.
- [x] TODO-013 - Reset `m_bNetworkAccessFinished` before each network request because the second push or pull operation blocks or returns without waiting.
- [ ] TODO-014 - Replace the busy wait in `CAzeClient::postNetworkRequest` with asynchronous handling without `processEvents`.
- [x] TODO-015 - Return the error response in `CAzeServerProcessor::getContent` because the built error node is never assigned to the response.
- [x] TODO-016 - Remove the unused `CRepository::getPushRequest` declaration without a definition in `CRepository.cpp`.
- [x] TODO-017 - Add an HTTP test for `servePush` because only the engine is covered by `testRemotePush` without server or command line interface.

## Pull client and server
- [x] TODO-018 - Implement `applyPullResponse` and `getPullResponse` in `CRemoteRepository` because only `getPullRequest` exists without commit and object payload.
- [x] TODO-019 - Implement `CAzeServerProcessor::servePull` which returns an empty success without reading the requested repository.
- [x] TODO-020 - Store received commits and objects in `CAzeClient::pull` and update the local branch with fast forward or merge.
- [x] TODO-021 - Include the requested branch and the tip identifier in the pull request instead of dumping all local branches.
- [x] TODO-022 - Add an end to end pull test which verifies content convergence after push then pull.

## Clone
- [x] TODO-023 - Implement `clone` in the engine with creation of the `.aze` folder and copy of branches, commits and objects from the server response.
- [x] TODO-024 - Expose `clone` in `CAzeClient`, `CAzeServerProcessor` and `CConstants` with handling of an empty or missing target folder.
- [x] TODO-025 - Add a `clone` test which verifies branches, branch tip and file content after cloning.

## General server
- [x] TODO-026 - Route `delete-branch` requests in `CAzeServerProcessor` which handles only `pull`, `push` and `clone`.
- [x] TODO-027 - Return a structured error for an unknown action instead of an empty node in `serveRequest`.
- [x] TODO-028 - Serve non JSON or empty requests with an error code instead of a silent empty response.
- [ ] TODO-029 - Add authentication and per repository isolation in `CAzeServerProcessor::getContent` which composes the path from the address without validation.
- [x] TODO-030 - Avoid `qFatal` in `CAzeServer` on invalid HTTPS configuration and return a clean error code.

## Index and acceleration
- [x] TODO-031 - Add a working folder hash cache keyed by path, size and modification time because `folderAsCommit` recomputes each identifier with `idFromByteArray`.
- [x] TODO-033 - Route `log` and ancestor search through the parent cache instead of loading each commit file.
- [x] TODO-034 - Persist `CCommitTreeList` in binary format as indicated by the TODO marker instead of text XML.
- [x] TODO-035 - Save the cache only when `dirty` is true because the destructor writes the file on each destruction.
- [x] TODO-036 - Copy `m_mCommitParentList` and flags in the assignment operator and copy constructor of `CCommitTreeList`.
- [x] TODO-037 - Invalidate the commit cache and the working folder cache after commit, merge, received push and branch switch.
- [x] TODO-038 - Handle files missing left or right in `threeWayMerge` as indicated by the TODO marker in `CCommitFunctions`.
