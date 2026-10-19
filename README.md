# Aze

[![Build](https://github.com/Jango73/aze/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/Jango73/aze/actions/workflows/build.yml)

A versioning system

## Dependencies

* Qt 6 (modules `core`, `xml`, `network`, `testlib`), `qmake6`, C++ compiler, `git`
  * Debian/Ubuntu: `build-essential git qt6-base-dev qt6-base-dev-tools`
  * Fedora: `gcc-c++ git qt6-qtbase-devel`
  * Arch: `base-devel git qt6-base`
  * Reference list per package manager: `scripts/linux/deps.sh`
* Submodules (fetched by `scripts/linux/deps.sh` via `git submodule update --init --recursive`)
  * qt-plus (`master`)
    * https://github.com/Jango73/qt-plus
  * dtl fork (`dev/conflict-v121`)
    * https://github.com/Jango73/dtl

## Building

* Install dependencies and submodules
```
    ./scripts/linux/deps.sh
```
* Build (shadow build with `qmake6` + `make`)
```
    ./scripts/linux/build.sh [--debug|--release]
```

## Running tests

Run the local CI (same steps as the GitHub workflow):
```
    ./scripts/linux/ci-local.sh
```

## Running the server

Launch the server with HTTPS enabled by default (self-signed dev
certificate is generated in `tmp/certs` when missing):
```
    ./scripts/linux/serve.sh
```
Pass `--http-only` for plain HTTP without TLS.

## Concepts

[Here is information about architecture](AzeLib/concept/Aze.md)

## Client commands

Usage:
```
    aze <command> [options] [arguments]
    aze help
```

Global options:
* `--silent` : silent mode (no messages).
* `--debug` : debug mode (tons of messages).
* `--version` : show version number.

### `init`
Makes the current folder an Aze repository.
```
    aze init
```

### `status`
Shows status of files.
```
    aze status [files] [--all] [--loose] [--clean] [--modified] [--added] [--deleted] [--missing] [--ignored]
```
* `--all` : include all files.
* `--loose` : include loose files.
* `--clean` : include clean files.
* `--modified` : include modified files.
* `--added` : include added files.
* `--deleted` : include deleted files.
* `--missing` : include missing files.
* `--ignored` : include ignored files.

### `stage`
Adds some files to the stage.
```
    aze stage <files>
```

### `unstage`
Removes some files from the stage.
```
    aze unstage <files>
```

### `revert`
Reverts some files using tip commit.
```
    aze revert <files> [-d]
```
* `-d`, `--allow-delete` : allow Aze to delete files.

### `move`
Moves some files.
```
    aze move
```

### `remove`
Removes some files from tracking.
```
    aze remove <files>
```

### `commit`
Creates a commit from the stage.
```
    aze commit [-a <author>] [-m <message>]
```
* `-a`, `--author <author>` : set commit author.
* `-m`, `--message <message>` : set commit message.

### `amend`
Rewrites the tip commit with the staged files.
```
    aze amend [-a <author>] [-m <message>]
```
* `-a`, `--author <author>` : set commit author.
* `-m`, `--message <message>` : set commit message.

### `clean-up`
Clears the stage and reverts all files. Warning: loose files are deleted.
```
    aze clean-up
```

### `log`
Shows a log of a branch.
```
    aze log [branch] [-g] [-s <start>] [-c <count>]
```
* `-g`, `--graph` : show log as a graph.
* `-s`, `--start <start>` : begin listing after `<start>` items.
* `-c`, `--count <count>` : list `<count>` items.

### `diff`
Shows a diff between two commits or files.
```
    aze diff [<id-1>] [<id-2>]
```
* without argument : shows a diff between the tip commit and the working folder.
* with one commit id : shows a diff between that commit and the working folder.
* with two commit ids : shows a diff between those two commits.
* with a file name : shows a diff between the tip commit version and the working folder version of that file.

### `create-branch`
Creates a branch.
```
    aze create-branch <branch-name>
```

### `delete-branch`
Deletes a branch.
```
    aze delete-branch <branch-name> [-f]
```
* `-f`, `--force` : delete the branch even when it is not merged into the current branch.

### `create-tag`
Creates a tag from the current branch tip.
```
    aze create-tag <tag-name>
```

### `delete-tag`
Deletes a tag.
```
    aze delete-tag <tag-name> [-f]
```
* `-f`, `--force` : delete the tag even when it is not merged into the current branch.

### `list-tags`
Lists the tags.
```
    aze list-tags
```

### `set-branch`
Switches to a branch.
```
    aze set-branch <branch-name> [-d]
```
* `-d`, `--allow-delete` : allow Aze to delete files.

### `merge`
Merges a branch onto the current one.
```
    aze merge <branch-name>
```

### `save-stash`
Saves changed files and cleans up the working folder.
```
    aze save-stash [stash-name]
```

### `pop-stash`
Restores the contents of a stash.
```
    aze pop-stash [stash-id]
```

### `patch`
Applies a patch (diff) to the repository.
```
    aze patch <patch-files>
```

### `set-remote-host`
Sets the remote host name for this repository (e.g. http://myhost.com:8080).
```
    aze set-remote-host <host-name>
```

### `show`
Dumps the content of a database object.
```
    aze show <ids>
```

### `pull`
Gets remote changes from a server.
```
    aze pull
```

### `push`
Sends local changes to a server.
```
    aze push
```

### `push-tag`
Sends a tag to a server.
```
    aze push-tag <tag-name>
```

### `pull-tag`
Gets a tag from a server.
```
    aze pull-tag <tag-name>
```

### `help`
Shows the help text.
```
    aze help
```
