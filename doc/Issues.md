# Open Issues

Reference for open issues from https://github.com/Jango73/aze/issues.

## [ ] - Issue 1 - Ability to rename a file

- Identifier: 1
- Label: enhancement
- Description: none provided.
- Status: open. `CAzeClient::move()` is a stub and performs no move.

The repository supports file tracking. File renaming requires dedicated handling to preserve history across name changes.

## [x] - Issue 2 - Ability to delete a branch

- Identifier: 2
- Label: enhancement
- Description: none provided.
- Status: done. `delete-branch` removes branch metadata and refuses the current branch and unknown branches. Covered by `CTestAze::testMergeErrorsAndResolution`.

The repository supports branch creation. Branch deletion requires dedicated handling to remove branch metadata without affecting commit and object storage shared with other branches.

## [ ] - Issue 3 - Create server

- Identifier: 3
- Label: enhancement
- Description:
  - Implement classes:
    - CAzeServer
    - CAzeServerProcessor
    - CRemoteRepository
- Status: partial. Push path works at protocol level over HTTP+JSON: request builder with commit ids and base64 objects, object and commit storage with integrity check, branch creation and fast-forward update, success and error responses. Covered by `CTestAze::testRemotePush`. Pull handler stays a stub and client response handling stays a no-op.

The repository supports local operations. Remote operations require server-side components for repository hosting and request processing.

## [ ] - Issue 4 - Create a cache for working directory data

- Identifier: 4
- Label: invalid
- Description:
  - SHAs for files in the working directory require caching and refresh on demand.
  - SHA generation is slow, so all functions calling CUtils::idFromByteArray() are time consuming.
- Status: open. No working folder hash cache exists. Only the commit tree cache exists.

Working folder state computation requires file hashing. A cache keyed by path, size and modification time reduces redundant hash computation.

## [x] - Issue 5 - Handle merge conflicts

- Identifier: 5
- Label: enhancement
- Description: none provided.
- Status: done. Three-way merge detects conflicts, marks files and reports `MergeHasConflicts`. Resolution through edit, stage and commit works. Covered by `CTestAze::testMerge` and `CTestAze::testMergeErrorsAndResolution`.

The repository supports three-way merge. Conflicting changes require detection, marking and resolution support.

## [ ] - Issue 7 - Create a cache for commit chains

- Identifier: 7
- Label: enhancement
- Description:
  - An internal structure (tree) contains the whole commit tree (identifiers only). This accelerates ancestor and commit chain searches.
- Status: partial. `CCommitTreeList::commitParentIds()` memoizes identifier-only parent maps, persists them under `.aze/cache`, and serves ancestor search. The tree half stays unwired and `log` still loads parent files directly.

Commit traversal requires repeated access to commit files. An identifier-only memory structure reduces file access during ancestor search.
