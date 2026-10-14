# AGENTS.md

This file provides guidance to agents when working with code in this repository.

## Project Overview
`aze` is a versioning tool (git-like, custom `.aze` repositories), built with
qmake for Qt6 (see `README.md`). Layout:
- `Aze/` (application): `sources/client` (`CAzeClient`), `sources/server`
  (`CAzeServer`), `sources/common`, `sources/tests` (`CTestAze`), `deploy/`.
- `AzeLib/` (library): reusable versioning engine (`CRepository`, `CUtils`,
  `CCommitFunctions`, ...), plus `sources/google` (diff-match-patch) and the
  `dtl` git submodule (`AzeLib/sources/dtl`, branch `dev/conflict`).
- `qt-plus/` (git submodule, branch `qt6`): shared Qt utility library. Check
  here first before adding helpers. Build files consumed here: `functions.pri`,
  `qt-plus-nolib-web.pri` (subset without multimedia/image/geo modules).
- `Aze.pro` is the `subdirs` project (`AzeLib`, then `Aze`). There is no CMake
  build; the reference Linux build is qmake6 + make via `scripts/linux/`.

## Main rule
**If the guidelines below are not followed, all modifications will be rejected.**

## Communication Guidelines
- NEVER use emojis in responses.
- DON'T act like a human being with emotions, just be a machine.
- DON'T says "Great!", "Perfect!", "You're right" all the time.
- If a demand DOES NOT make sense (for instance, breaks an architecture instead of refactoring it), SAY IT and ask for confirmation BEFORE DOING ANYTHING.
- NEVER create a commit unless the user explicitly asks for it in the current conversation.

## Architecture and Reuse Rules
- Bidirectional coupling is **STRICTLY FORBIDDEN**, both when writing code from scratch and when delivering a fix. Keep dependencies unidirectional and break cycles instead of introducing or preserving them.
- `Aze` (client/server/tests) depends on `AzeLib`, never the reverse.
- Any behavior likely to appear in multiple places MUST be implemented as a reusable module in `AzeLib` or `qt-plus`, not duplicated in the application.
- **Container usage**: use Qt containers (`QVector`, `QMap`, `QStringList`) where they exist. Do not introduce custom linked-list infrastructure.
- **Public headers**: keep `AzeLib` headers downstream-safe (no internals leaking into the include surface).
- **No globals**: Before adding a global variable, **ALWAYS ASK** if permitted.

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
- **Types**: Use Qt types (`QString`, `QMap`, ...) where they exist instead of introducing parallel raw-C representations.
- **Dependencies**: This is a Qt user-space project. Qt and the STL are allowed. Do NOT add new third-party libraries/modules (vendored or system) without asking.
- **TEXT literals**: Write log/UI messages directly as `QString`.
- **Declaration order**: Group declarations by type. 1: macros / 2: type definitions / 3: inline functions / 4: external functions / 5: other
- **Function order**: DO NOT OVERUSE forward declarations. Define functions before they are used.
- **I18n**: Write comments, console output and technical doc in english.
- **Naming**: PascalCase for classes/variables/members (`C`-prefixed classes, `m_`/`g_`/`p_` prefixes as in the existing code), SCREAMING_SNAKE_CASE for structs/defines.
- **Naming clarity**: In addition to using full words, every name must express its intent clearly and without ambiguity.
- **Comments**: For single-line comments, use `//`, not `/*`.
- **Style**: 4-space indentation. Match surrounding code instead of reformatting whole files.
- **Numbers**: Hexadecimal for constant numbers, except for sizes, vectors, points and time.
- **Number suffixes**: Do not add numeric suffixes like `u` to constants; they are not wanted here.
- **Documentation wording**: Use timeless technical wording. Do not use temporal terms like "now", "currently", "at this time" in documentation/comments.
- **Languages**: C++ for the engine and the test suite, avoid Python.
- **Unused parameters**: Use `Q_UNUSED()` to suppress the "unused parameter" warning.
- **Functions**: Add a doxygen header to functions and separate all functions with a 75 character line such as : /************************************************************************/
- **File size**: Keep source files under 1000 lines; split by responsibility before crossing this limit.
- **Vocabulary**:
  - NEVER use abbreviations; ALWAYS use full words (acronyms are OK).
  - No figurative language: respect the repository vocabulary, use every word
    in its primary (literal) sense. Name things for what they are, not for
    what they resemble.
  - No "directory" : use "folder".

### Commit message
The agent always signs its commits: append a last line `Agent: <agent-name>`
(for example `Agent: opencode/muse-spark`) so authorship stays identifiable
in the log, whatever the commit prefix above. Never leave a blank line in
the commit message: lines follow each other directly.

## Common Build Commands
- Dependencies: `./scripts/linux/deps.sh` (submodules + apt/dnf/pacman packages;
  needs `sudo`, which prompts for a password here)
- Build (shadow): `./scripts/linux/build.sh [--debug|--release]`
  (`qmake6 Aze.pro CONFIG+=<debug|release>` in `build-<debug|release>`, then `make`).
  Builds `AzeLib` (`libaze[d].so`), then `Aze` (`aze[d]`).
- Clean: `./scripts/linux/clean.sh [--debug|--release|--all]`
- Run: `./scripts/linux/run.sh [--debug|--release] [-- <args>]`
  (logs to `tmp/run.log`), e.g. `... -- run-tests` for the unit tests, which
  run isolated in `tmp/aze-tests/` whatever the current folder is.

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
