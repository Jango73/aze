# Plan: Adaptive Sharded Commit-Parent Index

## 1. Goal

Replace the monolithic `CCommitTreeList` persistence (full load on every command startup, full rewrite on every dirty exit) with adaptive sharding (extendible hashing), so `stage` and `commit` cost stays flat as history depth grows. Shards only, no legacy migration: aze runs nowhere in prod.

## 2. Evidence (measured with `scripts/linux/perf-history.sh`)

- 1000 commits (2k-20k files): `stage` avg 14 ms, `commit` avg 17 ms, `log` full 135 ms, `.aze` 8.4 MB.
- 5000 commits: `stage` avg 19 ms, `commit` avg 27 ms (last commits ~27/44 ms vs ~12/14 ms at start), `log` full 556 ms, `.aze` 42 MB.
- Index file `.aze/cache/Aze::CCommitTreeList.bin` is 1.3 MB at 5000 commits (~260 bytes per entry).
- Root cause chain:
  - `CCommitTreeList` ctor loads the whole file (`fromNode`, O(n)) on every CLI process start (`AzeLib/sources/commits/CCommitTreeList.cpp:20`).
  - Dtor rewrites the whole file when dirty (`CCommitTreeList.cpp:71`).
  - `commit` dirties it via `storeCommit` -> `registerCommitParents` (`AzeLib/sources/commits/CCommitFunctions.cpp:122`).
  - `stage` dirties it too: `addSingleFile` touches `tipCommit()` (`AzeLib/sources/commands/CStageCommand.cpp:60`), loaded through `loadCommit`, which registers parents (`CCommitFunctions.cpp:146`).
- Conclusion: every `stage`/`commit` process rewrites the full index. Fixed sharding would only divide the slope; adaptive sharding keeps the per-command write bounded.

## 3. Design

### 3.1 New specialized class: `CShardFileStore`

Lives in `AzeLib/sources/commits/` next to `CCommitTreeList`. Owns shard mechanics only, no commit semantics. Generic over `QString -> QStringList` entries so it stays reusable inside `AzeLib`.

Responsibilities:

- Route a key to a shard by hex prefix of the key (longest-prefix match). Commit ids are 64-char hex hashes, uniformly distributed, so shards stay balanced.
- Build the routing table once at construction from a single listing of the cache folder (O(shard count), not O(commit count)).
- Lazy-load a shard file on first access, keep it in memory.
- Track dirty shards; persist dirty shards only (destructor or explicit `save()`).
- Split a shard into 16 child shards (next hex char) when it exceeds the entry threshold; redistribute entries; materialize all 16 children (even empty ones, they are ~100 bytes) so routing needs no manifest file; delete the parent shard file.
- File naming reuses the existing scheme: base `CDatabase::composeCacheFileName("Aze::CShardFileStore-<prefix>")` plus `.bin` (`CXMLNode::sExtension_BIN`) by default or `.<CompressedXMLExtension>` when `bTextCache` is set, mirroring `CCommitTreeList.cpp:27-40`. Per-shard payload reuses the existing pack format (`CUtils::packFileAndId`, entries joined with `|`, parents joined with `:`) so `toNode`/`fromNode` logic moves over nearly unchanged, applied per shard.

Suggested public API (naming follows repo conventions: `C` prefix, full words, Qt containers):

- `CShardFileStore(CDatabase* pDatabase, QObject* parent, bool bSilent, bool bDebug, bool bTextCache, int iSplitThreshold = 2000)`
- `bool containsValue(const QString& sKey) const`
- `QStringList value(const QString& sKey)` (miss = empty list, no I/O beyond the shard file)
- `void setValue(const QString& sKey, const QStringList& lValue)` (no-op when unchanged, marks shard dirty otherwise)
- `void removeValue(const QString& sKey)`
- `void save()` (writes dirty shards, deletes orphaned parent files after splits)
- Shard key normalization: `toLower()` on the routing prefix, defensive against non-hex keys (route to root shard).

Threshold guidance: 2000 entries ~= 500 KB per shard at the measured ~260 bytes/entry. Rewrite cost per `commit` then stays in the low milliseconds regardless of total history size.

### 3.2 `CCommitTreeList` becomes a thin facade

- Keeps its exact public API (`containsCommit`, `registerCommitParents`, `removeCommit`, `commitParentList` x2, `commitParentIds`) so `CCommitFunctions`, `CLogCommand`, `CDeleteBranchCommand`, `CPruneCommand` are untouched.
- Internally delegates the map to `CShardFileStore`; drops `m_mCommitParentList`, `m_sFileName`, `m_bDirty`, `toNode`, `fromNode` (verify no other caller uses `toNode`/`fromNode`, copy ctor, assignment, `operator==` first; they appear unused outside the class — remove if confirmed, per the no-dead-code rule; keep them only if a test references them).
- Constructor no longer touches the filesystem except the cheap folder listing done by the store. Destructor saves via the store.
- Fallback behavior on shard miss stays in `CCommitFunctions` (`commitParentIds` loads the commit file once), so a fresh or partially cached index is always correct, just colder.

### 3.3 Concurrency and process model

- Same assumptions as today: one short-lived process per CLI command. No locking added (the current single file has none either). Concurrent writers can race on the same shard; document the limitation, do not solve it here.

## 4. Files and build

- New: `AzeLib/sources/commits/CShardFileStore.h`, `AzeLib/sources/commits/CShardFileStore.cpp` (keep each under 1000 lines; expect ~400).
- Modified: `AzeLib/sources/commits/CCommitTreeList.h`, `AzeLib/sources/commits/CCommitTreeList.cpp`.
- Register the new files in `AzeLib/AzeLib.pri` (`HEADERS` + `SOURCES`, alphabetical placement like the existing `CCommit*` entries).
- Follow repo rules: 4-space indent, `//` comments, doxygen header per function with the 75-char separator line, `Q_UNUSED()` for unused parameters, Qt types, no new third-party dependency, no globals, timeless wording in comments.

## 5. Tests

- Existing suite must stay green: `./scripts/linux/ci-local.sh` (builds release, runs `aze run-tests` isolated in `tmp/aze-tests`).
- New unit test following the `CTest*` fixture pattern (see `Aze/sources/tests/CTestCache.*` for the closest example): shard routing incl. longest-prefix after splits, split threshold behavior, dirty-only persistence (assert untouched shard files keep their modification time), remove path used by prune, roundtrip through a fresh store instance. Register it in `CTestAze::runAllTests()` and the `Aze` project test sources (check how `CTestCache` is wired in `Aze/Aze.pro` or its `.pri`).
- Perf acceptance with `scripts/linux/perf-history.sh`:
  - `--commits 1000` then `--commits 5000` (defaults 2k-20k).
  - Expect `stage`/`commit` averages flat across depth (no growth from first to last progress lines) and 5k averages back near the 1k numbers (stage ~14 ms, commit ~17 ms).
  - Expect `log` full linear (~135 ms at 1k, ~550-700 ms at 5k).
  - Work folder is gitignored scratch `tmp/aze-perf`; never commit it.

## 6. Implementation steps

1. Add `CShardFileStore.{h,cpp}` with routing, lazy load, dirty tracking, split, save; wire into `AzeLib.pri`.
2. Rewrite `CCommitTreeList` as facade over the store; remove dead members/helpers if confirmed unused.
3. Build: `./scripts/linux/build.sh`.
4. Add the unit test, register it, run `./scripts/linux/ci-local.sh`.
5. Run perf script at 1k and 5k commits; compare against the section 2 baseline numbers.
6. Commit with message lines back-to-back (no blank line) ending in `Agent: <agent-name>`. Never commit `tmp/` or `.aze/` residue.

## 7. Non-goals

- No migration from the legacy monolithic cache file; it is ignored and the cache re-warms via commit-file fallback.
- No change to `CCommitFunctions`, commands, or the `log` path.
- No SQLite or new dependency; no manifest file; no locking.
