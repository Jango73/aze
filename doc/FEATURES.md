# Aze features and automated test coverage

This document lists the features of `aze` and the automated test that covers
each feature. Test suites run through `aze run-tests` (`CTestAze::runAllTests`)
in an isolated folder (`tmp/aze-tests`). A feature marked `No dedicated test`
has no isolated test case and is covered only indirectly, if at all.

Test file root: `Aze/sources/tests/`.

## Local repository workflow

| Feature (`aze` command) | Description | Covering test |
| --- | --- | --- |
| `init` | Creates an `.aze` repository in the current folder | Covered implicitly by the `INIT()` macro used in every suite (`CTestMacros.h`); no isolated test case |
| `status` | Shows working folder status against tip commit and stage | `CTestWorkflow::testCommit`, `testUnstage`, `testDelete`, `testCleanUp`; `CTestMerge::testMerge`; `CTestDiffPatch::testPatch` (`CTestWorkflow.cpp`, `CTestMerge.cpp`, `CTestDiffPatch.cpp`). Filter flags (`--all`, `--loose`, `--clean`, `--modified`, `--added`, `--deleted`, `--missing`, `--ignored`) have no dedicated test |
| `stage` | Adds files to the stage | `CTestWorkflow::testCommit`, `testDelete` (`CTestWorkflow.cpp`) |
| `unstage` | Removes files from the stage without touching working folder files | `CTestWorkflow::testUnstage` (`CTestWorkflow.cpp`) |
| `revert` | Restores working folder files from the tip commit; `--allow-delete` deletes untracked files | `CTestWorkflow::testRevert` (`CTestWorkflow.cpp`) |
| `move` | Moves a file and preserves its object identifier through the stage into the next commit | `CTestWorkflow::testMove` (`CTestWorkflow.cpp`) |
| `remove` | Removes a file from tracking through the stage | `CTestWorkflow::testDelete` via `REMOVE()` (`CTestWorkflow.cpp`) |
| `commit` | Creates a commit from the stage with author and message | `CTestWorkflow::testCommit` (`CTestWorkflow.cpp`) |
| `amend` | Rewrites the tip commit with staged content, preserving parents; supports message-only rewrite | `CTestWorkflow::testAmend` (`CTestWorkflow.cpp`); rewrite path also exercised in `CTestRemote::testRemotePush`, `CTestServe::testServePush`, `CTestPrune::testPrune`, `CTestMassive::testMassive` |
| `clean-up` | Clears the stage, restores tracked files, deletes loose files | `CTestWorkflow::testCleanUp` (`CTestWorkflow.cpp`) |

## Branches, history, diff, stash

| Feature (`aze` command) | Description | Covering test |
| --- | --- | --- |
| `create-branch`, `set-branch` | Creates a branch; switches the working folder to a branch tip | `CTestMerge::testMerge`, `testMergeErrorsAndResolution`; branch switching also exercised in `CTestMassive::testMassive`, `CTestPrune::testPrune`, `CTestRemote::testClone` |
| `delete-branch` | Deletes a branch file; refuses the current branch, unknown branches and unmerged branches with distinct codes; `--force` overrides the unmerged guard | `CTestMerge::testMergeErrorsAndResolution` (local engine, all guards plus `--force`); `CTestPrune::testPrune` (force deletion setup) |
| `create-tag`, `delete-tag`, `list-tags` | Creates a tag from the current branch tip; deletes a tag file with distinct codes; lists the tags; branch commands refuse tag names and tag commands refuse branch names; tags are never checked out | `CTestTag::testCreateTag`, `testTagGuards`, `testDeleteTag` |
| `merge` | Three-way merge of a branch onto the current branch; fast forward when the tip is an ancestor; `--no-fast-forward` forces a merge commit; conflicts use merge marker blocks | `CTestMerge::testMerge` (multi-branch scenario with conflict at the end); `CTestMerge::testMergeErrorsAndResolution` (self merge, unknown branch, non-empty stage, dirty folder, clean merge, conflict, resolution with two parents, nothing to merge); `CTestMassive::testMassive` (`--no-fast-forward` merges plus one fast forward merge) |
| `log` | Lists branch history; `--graph` draws forks and merges | `CTestWorkflow::testAmend`; `CTestMerge::testMerge`, `testMergeErrorsAndResolution` (plain and `--graph`); `CTestPrune::testPrune`; `CTestCache::testCommitTreeListBinaryCache`, `testCommitTreeListTextCache`. Pagination (`--start`, `--count`) has no dedicated test |
| `diff` | Unified diff: no argument (tip against working folder), one commit identifier, two commit identifiers (`tip~1 tip`), one file name | `CTestDiffPatch::testDiff` (no argument, `tip`, `tip~1`, single file; empty output on clean folder); `CTestWorkflow::testCommit`, `testDelete`; `CTestMerge::testMerge` (two-commit form through `DIFF_LAST`) |
| `patch` | Applies a patch file to the working folder; fails on incompatible content | `CTestDiffPatch::testPatch` (`CTestDiffPatch.cpp`) |
| `save-stash`, `pop-stash` | Saves dirty files away and restores the working folder; restores stashed content back | `CTestMerge::testMerge` (unnamed stash save and pop). Named stashes have no dedicated test |
| `prune` | Mark-and-sweep of unreachable commits and objects; `--dry-run` lists without deleting; `--retention` sets the grace period in seconds | `CTestPrune::testPrune` (force-deleted branch, amended commit, dry run, real run, default retention grace period) (`CTestPrune.cpp`) |

## Remote collaboration (engine)

| Feature | Description | Covering test |
| --- | --- | --- |
| `push` (engine) | Announces branch and tip, answers the server tip, then builds a data request with missing commits and unshared objects only; applies it fast forward only; refuses rewritten tips and unknown tips; full requests stay accepted | `CTestRemote::testRemotePush` (full requests); `CTestRemote::testPushDelta` (announce, single-commit data, converged no-op, early rewritten-tip refusal); push path also in `CTestRemote::testPull`, `CTestCache::testCacheInvalidation` |
| `pull` (engine) | Pull request carries branch and local tip; response carries missing commits and unshared objects only, attributes only when converged, full history on unknown or divergent tip; response applies fast forward or three-way merge; divergent histories produce conflict markers | `CTestRemote::testPull` (convergence, second fast forward, up-to-date pull, divergent conflict, single-line conflict, unknown branch); `CTestRemote::testPullDelta` (full first pull, single-commit data, converged no-op, divergent full fallback with conflict) |
| `clone` (engine) | Creates `.aze` in the target folder and copies branches, commits and objects from the server response | `CTestRemote::testClone` (branch tips, file content, missing target folder argument, duplicate target refusal) |
| `push-tag`, `pull-tag` (engine) | Tags travel through the branch protocol with the object type as attribute; the receiver materializes the matching class; moving a tag tip is refused | `CTestTag::testPushPullTag` (push, converged re-push, pull into a fresh repository, move refusal) |
| `set-remote-host` | Stores the remote host name in repository information | No dedicated test |
| Network `push`, `pull`, `clone` through `CAzeClient` | Sends requests over `HTTP` and stores the server response | No dedicated test; engine and server layers are tested separately (see above and below) |
| `show` (`dump`) | Dumps the content of a database object by identifier: commit identifier dumps the stored commit document, file object identifier dumps the stored file content, several identifiers are concatenated, unknown identifier yields empty output | `CTestDiffPatch::testShow` (`CTestDiffPatch.cpp`) |
| `help`, `--version`, `--silent`, `--debug` | Help text and global output flags | No dedicated test |

## Server

| Feature | Description | Covering test |
| --- | --- | --- |
| `serve push` | Applies a push request through `CAzeServerProcessor::servePush`, including `JSON` round trip; answers announces through `servePushAnnounce` | `CTestServe::testServePush` (success, duplicate tip, non fast forward refusal, invalid tip refusal); `CTestServe::testServePushAnnounce` (empty tip, invalid announce, full then delta data, converged announce) |
| `serve delete-branch` | Applies a delete-branch request with branch name and force flag through the server layer | `CTestServe::testServeDeleteBranch` (success, unknown branch, missing name, current branch, unmerged branch, forced deletion) |
| Repository path confinement | Resolves the request path under the server base folder; refuses parent markers, empty segments, embedded separators, drive specifications | `CTestServe::testServePathIsolation` |
| `HTTP` and `HTTPS` connectivity | Listeners accept `GET` and `POST` and return the empty-request `JSON` result; covers `TCP`, `HTTP` framing, `TLS` and `JSON` layers | `CTestServe::testServeConnectivity` |
| `serve pull`, `serve clone` request dispatch | Routes pull and clone actions to the corresponding handlers; pull answers missing commits only through `CAzeServerProcessor::servePull` | `CTestServe::testServePullDelta` (full first pull, single-commit data, converged no-op, unknown branch); pull and clone logic is covered at engine level in `CTestRemote::testPull` and `CTestRemote::testClone` |
| Housekeeper automatic and periodic prune | Best-effort prune hook after `push` and `delete-branch` plus timer sweep over the repositories base path | No dedicated test; manual local `prune` is covered by `CTestPrune::testPrune` |

## Performance and scale

| Feature | Description | Covering test |
| --- | --- | --- |
| Working folder hash cache | File identifiers keyed by path, size and modification time; hit, miss, invalidation, clear, missing file handling | `CTestCache::testFileHashCache` |
| Cache invalidation | Tip and hash caches follow commit, amend, merge, received push and branch switch on the same repository instance | `CTestCache::testCacheInvalidation` |
| Commit parent cache, binary format | Persisted parent list through the destructor, reused by `log` and ancestor search | `CTestCache::testCommitTreeListBinaryCache` |
| Commit parent cache, text format | Same persistence with `--text-cache` (compressed `XML` instead of binary) | `CTestCache::testCommitTreeListTextCache` |
| Massive repository | Around 29 megabytes of deterministic files across commits, branches, merges with and without fast forward, amend, plus intermittent delta pushes of the current branch with final remote tip and content verification | `CTestMassive::testMassive` |
