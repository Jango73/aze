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

Run the app with command "run-tests"

## Concepts

[Here is information about architecture](AzeLib/concept/Aze.md)

## Implemented functionality

### Client side
#### Local
* Initialize a repository
* Show status of files
* Create a branch
* Switch branches
* Add files to stage
* Remove files from stage
* Commit staged files
* Show commit log
* Show commit graph => WIP
  * Needs to show commits in correct order
* Diff between two commits
* Merge a branch onto the current one => WIP
  * Need to handle merge conflicts
* Save stash
* Pop stash

#### Remote
* Pull => WIP
* Push => WIP

### Server side
* WIP
