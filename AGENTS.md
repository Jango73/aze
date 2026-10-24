# AGENTS.md

This file provides guidance to agents when working with code in this repository.

## Project Overview
`aze` is a versioning tool (git-like, custom `.aze` repositories), built with
qmake for Qt6 (see `README.md`). Layout:
- `Aze/` (application): `sources/client` (`CAzeClient`), `sources/server`
  (`CAzeServer`), `sources/common`, `sources/tests` (`CTestAze`).
- `AzeLib/` (library): reusable versioning engine (`CRepository`, `CUtils`,
  `CCommitFunctions`, ...), plus `sources/google` (diff-match-patch) and the
  `dtl` git submodule (`AzeLib/sources/dtl`, branch `dev/conflict`).
- `qt-plus/` (git submodule, branch `master`): shared Qt utility library. Check
  here first before adding helpers. Build file consumed here:
  `qt-plus-nolib-web.pri` (subset without multimedia/image/geo modules).
- `Aze.pro` is the `subdirs` project (`AzeLib`, then `Aze`). There is no CMake
  build; the reference Linux build is qmake6 + make via `scripts/linux/`.

## Main rule
If the guidelines below are not followed, all modifications will be rejected.

## Communication Guidelines
- NEVER use emojis in responses.
- DON'T act like a human being with emotions, just be a machine.
- DON'T says "Great!", "Perfect!", "You're right" all the time.
- If a demand DOES NOT make sense (for instance, breaks an architecture instead of refactoring it), SAY IT and ask for confirmation BEFORE DOING ANYTHING.
- NEVER create a commit unless the user explicitly asks for it in the current conversation.

## Architecture and Reuse Rules
- Bidirectional coupling is STRICTLY FORBIDDEN, both when writing code from scratch and when delivering a fix. Keep dependencies unidirectional and break cycles instead of introducing or preserving them.
- `Aze` (client/server/tests) depends on `AzeLib`, never the reverse.
- Any behavior likely to appear in multiple places MUST be implemented as a reusable module in `AzeLib` or `qt-plus`, not duplicated in the application.
- Container usage: use Qt containers (`QVector`, `QMap`, `QStringList`) where they exist. Do not introduce custom linked-list infrastructure.
- Public headers: keep `AzeLib` headers downstream-safe (no internals leaking into the include surface).
- No globals: Before adding a global variable, ALWAYS ASK if permitted.
- Command pattern: every user-facing operation MUST go through a command class in `AzeLib` (`C*Command`, invoked via `CRepository` or `CRemoteRepository`). Bypassing the command layer from `Aze` (direct `database()` access, duplicated domain logic in the client) is STRICTLY FORBIDDEN, both when writing code from scratch and when delivering a fix. `CAzeClient` only parses arguments, orchestrates calls and formats output.

## Test Isolation Rules
- `CTestAze` manipulates and DELETES files by design. It MUST run in an
  isolated folder and NEVER in the project source tree.
- The isolated folder is `<repository>/tmp/aze-tests` (see
  `CTestAze::testDirectoryPath()`, which locates the repository root from the
  binary folder and calls `QDir::setCurrent()` on it). Keep this mechanism:
  do not reintroduce `QDir::currentPath()` as the test root.
- `tmp/` at the repo root is gitignored scratch (see Tool Execution Policy).
  Test residue (`.aze/`, `Files/`) belongs there only.
- `.aze` folders are generated test repositories: NEVER commit them.

## Coding Conventions
- Types: Use Qt types (`QString`, `QMap`, ...) where they exist instead of introducing parallel raw-C representations.
- Dependencies: This is a Qt user-space project. Qt and the STL are allowed. Do NOT add new third-party libraries/modules (vendored or system) without asking.
- TEXT literals: Write log/UI messages directly as `QString`.
- Declaration order: Group declarations by type. 1: macros / 2: type definitions / 3: inline functions / 4: external functions / 5: other
- Function order: DO NOT OVERUSE forward declarations. Define functions before they are used.
- I18n: Write comments, console output and technical doc in english.
- Naming: PascalCase for classes/variables/members (`C`-prefixed classes, `m_`/`g_`/`p_` prefixes as in the existing code), SCREAMING_SNAKE_CASE for structs/defines.
- Naming clarity: In addition to using full words, every name must express its intent clearly and without ambiguity.
- Comments: For single-line comments, use `//`, not `/*`.
- Style: 4-space indentation. Match surrounding code instead of reformatting whole files.
- Numbers: Hexadecimal for constant numbers, except for sizes, vectors, points and time.
- Number suffixes: Do not add numeric suffixes like `u` to constants; they are not wanted here.
- Documentation wording: Use timeless technical wording. Do not use temporal terms like "now", "currently", "at this time" in documentation/comments.
- Languages: C++ for the engine and the test suite, avoid Python.
- Unused parameters: Use `Q_UNUSED()` to suppress the "unused parameter" warning.
- Functions: Add a doxygen header to functions and separate all functions with a 75 character line such as : //
- File size: Keep source files under 1000 lines; split by responsibility before crossing this limit.
- Vocabulary:
  - NEVER use abbreviations; ALWAYS use full words (acronyms are OK).
  - No figurative language: respect the repository vocabulary, use every word
    in its primary (literal) sense. Name things for what they are, not for
    what they resemble.
  - No "directory" : use "folder".

### Commit message
Describe the changes in a short sentence.

Test: append a second line with the test results: `Tests: <passed>/<total>`
Omit the line only when no test ran.

Sign: append a last line `Agent: <agent-name>`
(for example `Agent: opencode/muse-spark`) so authorship stays identifiable
in the log, whatever the commit prefix above.

Never leave a blank line in the commit message: lines follow each other directly.

## Code review
- When the user asks for a code review, review the target commit or commit range for OOP compliance, factorization, absence of duplicated code, and compliance with the Architecture and Reuse Rules plus the Coding Conventions of this document.
- Write the findings in english in a dedicated file under `quick3d/doc/` (for example `quick3d/doc/Review-<hash>.md`).
- Do not wrap issue lines at 80 characters: one issue uses one full line, however long the line is.
- Group issues by criticality under `Critical`, `Major`, and `Minor` headings.
- Give every issue a sequential number, a `file:line` location, and a checkbox (for example `- [ ] #12 \`quick3d/source/mesh/CWorldTerrain.cpp:1098\` — description`).

## Common Build Commands
- Dependencies: `./scripts/linux/deps.sh` (submodules + apt/dnf/pacman packages;
  needs `sudo`, which prompts for a password here)
- Build (shadow): `./scripts/linux/build.sh [--debug|--release]`
  (`qmake6 Aze.pro CONFIG+=<debug|release>` in `build-<debug|release>`, then `make`).
  Builds `AzeLib` (`libaze[d].so`), then `Aze` (`aze[d]`).
- Local CI: `./scripts/linux/ci-local.sh [--debug|--release]`
  (local mirror of the GitHub CI: builds, then runs the unit tests via
  `aze run-tests`, logs to `tmp/ci-local.log`). This is the only supported
  way to run the tests. They run isolated in `tmp/aze-tests/` whatever the
  current folder is.
- Serve: `./scripts/linux/serve.sh [--debug|--release] [--http-only]`
  (launches `aze serve` with HTTP+HTTPS by default, plain HTTP only with
  `--http-only`, logs to `tmp/serve.log`).
- Clean: `./scripts/linux/clean.sh [--debug|--release|--all]`

## Tool Execution Policy
- `tmp/` at the repo root is the folder for temporary/scratch files. It is
  gitignored; scripts must create it (`mkdir -p`) if missing.
- Never write scratch/generated artifacts into the repo without asking;
  keep the tree clean (`build-debug/`, `build-release/`, `tmp/` are gitignored).
- `sudo` prompts for a password on this machine: never assume elevation works;
  ask the user to run installs themselves or explicitly approve.
- Run builds sequentially, not in parallel.

## Documentation
- Build setup: `README.md`, `AzeLib/concept/Aze.md`
