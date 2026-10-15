# Aze

[![Build](https://github.com/Jango73/aze/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/Jango73/aze/actions/workflows/build.yml)

A versioning system (just for pleasure of coding)

## Dependencies

* Qt 5.13
* QtCreator 4.9.1
* qt-plus - Copyright myself
  * https://github.com/Jango73/qt-plus
* Diff template library - Copyright (c) 2015 Tatsuhiko Kubo <cubicdaiya@gmail.com>
  * https://github.com/cubicdaiya/dtl

## Building

* Make sure to update submodules
```
    git submodule init
    git submodule update
```
* Open /Aze.pro in QtCreator
* Build and run

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

### `aze init`
Makes the current folder an Aze repository.
```
    aze init
```

### `aze status`
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

### `aze stage`
Adds some files to the stage.
```
    aze stage <files>
```

### `aze unstage`
Removes some files from the stage.
```
    aze unstage <files>
```

### `aze revert`
Reverts some files using tip commit.
```
    aze revert <files> [-d]
```
* `-d`, `--allow-delete` : allow Aze to delete files.

### `aze move`
Moves some files.
```
    aze move
```

### `aze remove`
Removes some files from tracking.
```
    aze remove <files>
```

### `aze commit`
Creates a commit from the stage.
```
    aze commit [-a <author>] [-m <message>]
```
* `-a`, `--author <author>` : set commit author.
* `-m`, `--message <message>` : set commit message.

### `aze amend`
Rewrites the tip commit with the staged files.
```
    aze amend [-a <author>] [-m <message>]
```
* `-a`, `--author <author>` : set commit author.
* `-m`, `--message <message>` : set commit message.

### `aze clean-up`
Clears the stage and reverts all files. Warning: loose files are deleted.
```
    aze clean-up
```

### `aze log`
Shows a log of a branch.
```
    aze log [branch] [-g] [-s <start>] [-c <count>]
```
* `-g`, `--graph` : show log as a graph.
* `-s`, `--start <start>` : begin listing after `<start>` items.
* `-c`, `--count <count>` : list `<count>` items.

### `aze diff`
Shows a diff between two commits or files.
```
    aze diff [<id-1>] [<id-2>]
```
* without argument : shows a diff between the tip commit and the working folder.
* with one commit id : shows a diff between that commit and the working folder.
* with two commit ids : shows a diff between those two commits.
* with a file name : shows a diff between the tip commit version and the working folder version of that file.

### `aze create-branch`
Creates a branch.
```
    aze create-branch <branch-name>
```

### `aze delete-branch`
Deletes a branch.
```
    aze delete-branch <branch-name>
```

### `aze set-branch`
Switches to a branch.
```
    aze set-branch <branch-name> [-d]
```
* `-d`, `--allow-delete` : allow Aze to delete files.

### `aze merge`
Merges a branch onto the current one.
```
    aze merge <branch-name>
```

### `aze save-stash`
Saves changed files and cleans up the working folder.
```
    aze save-stash [stash-name]
```

### `aze pop-stash`
Restores the contents of a stash.
```
    aze pop-stash [stash-id]
```

### `aze patch`
Applies a patch (diff) to the repository.
```
    aze patch <patch-files>
```

### `aze set-remote-host`
Sets the remote host name for this repository (e.g. http://myhost.com:8080).
```
    aze set-remote-host <host-name>
```

### `aze show`
Dumps the content of a database object.
```
    aze show <ids>
```

### `aze pull`
Gets remote changes from a server.
```
    aze pull
```

### `aze push`
Sends local changes to a server.
```
    aze push
```

### `aze help`
Shows the help text.
```
    aze help
```
