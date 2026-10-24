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
- [x] TODO-011 - Implement `move` with history preservation because `CAzeClient::move` is a stub which writes only the staging area.
- [x] TODO-039 - Add the missing `merge` help text in `CConstants::initCommandMap` because every other branch command is documented there.
- [x] TODO-040 - Remove or repair the dead `CTwoWayMergeCommand.cpp` which redefines `CMergeCommand` with a stale two-argument constructor and is excluded from `AzeLib.pri`.
- [x] TODO-041 - Unify the self-merge guard in the engine because `CAzeClient::merge` refuses it by branch name while `CMergeCommand` refuses it by tip identifier with a different message.
- [x] TODO-042 - Return distinct error codes for merge failures (unknown branch, dirty working folder, nothing to merge) instead of reusing `s_iError_CouldNotMerge` on every path.
- [x] TODO-043 - Refuse `merge` when the stage is not empty because `CMergeCommand` overwrites merge parents without checking staged files.
- [x] TODO-044 - Handle fast forward in `CMergeCommand` when the current tip is an ancestor of the merged branch instead of creating a merge commit with two parents.
- [x] TODO-045 - Extend the conflict resolution test to verify both parents after resolving and committing because `testMergeErrorsAndResolution` checks file content only.

## Push client and server
- [x] TODO-012 - Store and apply the server response in `CAzeClient::onNetworkAccessFinished` which ignores success and errors.
- [x] TODO-013 - Reset `m_bNetworkAccessFinished` before each network request because the second push or pull operation blocks or returns without waiting.
- [x] TODO-014 - Replace the busy wait in `CAzeClient::postNetworkRequest` with asynchronous handling without `processEvents`.
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
- [x] TODO-030 - Avoid `qFatal` in `CAzeServer` on invalid HTTPS configuration and return a clean error code.

## Index and acceleration
- [x] TODO-031 - Add a working folder hash cache keyed by path, size and modification time because `folderAsCommit` recomputes each identifier with `idFromByteArray`.
- [x] TODO-033 - Route `log` and ancestor search through the parent cache instead of loading each commit file.
- [x] TODO-034 - Persist `CCommitTreeList` in binary format as indicated by the TODO marker instead of text XML.
- [x] TODO-035 - Save the cache only when `dirty` is true because the destructor writes the file on each destruction.
- [x] TODO-036 - Copy `m_mCommitParentList` and flags in the assignment operator and copy constructor of `CCommitTreeList`.
- [x] TODO-037 - Invalidate the commit cache and the working folder cache after commit, merge, received push and branch switch.
- [x] TODO-038 - Handle files missing left or right in `threeWayMerge` as indicated by the TODO marker in `CCommitFunctions`.

## Housekeeping
- [x] TODO-047 - Implement `prune` mark-and-sweep in the engine with dry-run and a modification-time grace period because `deleteBranch`, `amend` and refused `CPushCommand` payloads leave unreachable commit and object files with no deletion path (`CCleanUpCommand` only reverts the working folder) and requests run in worker threads concurrent with a prune.
- [x] TODO-048 - Trigger `prune` automatically with a conditional best-effort hook after `push` and `delete-branch` (threshold-gated, never failing the originating request) plus a periodic `QTimer` sweep over the repositories base path as complement, and expose a manual local `prune` for the server operator, because housekeeping must not depend on a manual client call, a timer alone lets repositories grow between passes, and no unprivileged network client may trigger deletion.
- [x] TODO-049 - Add a `prune` test which creates dangling commits and objects via force `delete-branch` or `amend` then verifies only unreachable files are removed with reachable tips and referenced objects preserved.

## Branch, tag and stash management
- [x] TODO-050 - Add a `list-branches` command which lists local branches with the current branch marked because `list-tags` exists and branches have no inventory command.
- [x] TODO-051 - Add a `rename-branch` command which renames the branch file and updates the current branch pointer because branches can only be created and deleted.

## History rewriting and inspection
- [ ] TODO-057 - Implement a `reset` command which moves the current branch tip to an arbitrary commit with working folder update because `revert` and `clean-up` restore files only from the tip commit.
- [ ] TODO-058 - Implement a `revert-commit` command which records an inverse commit for a given identifier because the existing `revert` restores working folder files without recording history.
- [ ] TODO-062 - Add `merge --abort` and `merge --continue` handling around conflict markers because a conflicted merge has no controlled exit or resume path.

## Remote collaboration and server
- [ ] TODO-064 - Add upstream tracking with push of all branches and tags plus guarded force push, and pull of all tracked branches, because `push` and `pull` operate on the current branch only and `push-tag` handles one tag at a time.
- [ ] TODO-066 - Add repository creation and deletion through the server protocol with path confinement checks because the server serves only repositories created out of band.

## Files, ignore rules and attributes
- [ ] TODO-067 - Implement an ignore file read from the working folder which feeds the `eIgnored` status because the ignored state exists in `CEnums` with a `--ignored` filter and no producer assigns it.
- [ ] TODO-068 - Preserve executable bit, symbolic link target and binary type through stage, commit, merge and checkout because object files store content only and platform attributes are lost.
- [ ] TODO-069 - Add large file handling with out of band object storage or chunked transfer because every object file travels whole through push, pull and clone payloads.
- [ ] TODO-070 - Define submodule or nested repository linkage because repositories cannot compose and nested `.aze` folders have no specified behavior.

## Diff, patch and export
- [ ] TODO-071 - Extend `diff` with staged comparison and summary output (`--cached`, `--stat`) because `diff` compares tip against working folder or two commits only.
- [ ] TODO-072 - Extend `log` with filters by author, message text, date range and file path plus one line output because `log` supports only branch selection, graph display and pagination.
- [ ] TODO-073 - Add commit series export and import (`format-patch`, `archive`, `bundle`) because `patch` applies a working folder diff and history cannot be exchanged as files.

## Configuration and user experience
- [ ] TODO-074 - Add persistent configuration for user identity, editor, aliases and default branch because author passes through `--author` on each `commit` and `amend` without stored defaults.
- [ ] TODO-075 - Add client and server hooks around commit, merge, push and pull because automation has no extension point.
- [ ] TODO-076 - Add copy detection to `move` and similarity based rename tracking across commits because only explicit moves preserve the object identifier.

## Documentation, test coverage and platform support
- [ ] TODO-077 - Document `clone`, `prune`, `serve` and `run-tests` in `README.md` because implemented commands remain absent from the client command reference.
- [ ] TODO-078 - Add dedicated tests for status filter flags, log pagination, named stashes, `set-remote-host`, network `push` and `pull` through `CAzeClient`, automatic housekeeper prune and global help and version flags because `doc/Aze.md` marks each area as indirect coverage only.
- [ ] TODO-079 - Provide build and service scripts beyond `scripts/linux` with continuous integration coverage because Linux shell scripts are the only supported build, test and serve path.
