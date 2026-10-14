## XML files
The xml files should be compressed in production environment.
The extension of a compressed XML file is xmlc, which contains UTF8 text data compressed using Qt's built-in compression functions (qCompress and qUncompress).

## Identifiers
All identifiers are expressed using a SHA256 hex string.

## Repository structure

```
.aze
|- info.xml
|- branches
|  |- trunk.xml (.xmlc)
|  |- some-branch.xml (.xmlc)
|- stash
|- commits
|  |- 0123456789abcdef.xml (.xmlc)
|  |- fedbca9876543210.xml (.xmlc)
|  |- ...
|- objects
   |- abcdef0123456789
   |- 9876543210fedbca
   |- ...
```

## General information file structure (.aze/info.xml)

```
<info>
  <branches>
    current : String
  <stashList>
    <stash>
      id : Identifier
```

## Branch file structure
These files reside in .aze/branches.

```
<branch>
  <info>
    type   : String
    author : String
    date   : ISO 8601 Date String
    root   : Identifier (link to .aze/commits/<id>.xml (.xmlc))
    tip    : Identifier (link to .aze/commits/<id>.xml (.xmlc))
```

## Commit file structure
These files reside in .aze/commits.

```
<commit>
  <info>
    author  : String
    date    : ISO 8601 Date String
    parents : Comma separated Identifiers
  <message>
    String
  <user>
    <some key 1>
      some value 1
    <some key 2>
      some value 2
    ...
<files>
  some-folder-1/some-file-1.txt@Identifier\n
  some-folder-1/some-file-2.txt@Identifier\n
  some-folder-2/some-file-3.cpp@Identifier\n
  some-folder-2/some-file-4.h@Identifier
  ...
```

## Object files structure
These files reside in .aze/objects.

Each object file holds data for a single file in the repository.
A single object file may be referenced by many commits.

## Branching and merging

### Three-way merge

Apply (B-A) and (C-A) to [ A ] in order to create [ D ].

               [ A ]
                 |
                / \
      (B-A)    /   \    (C-A)
              /     \
             /       \
          [ B ]     [ C ]
            \         /
             \       /
              \     /
               \   /
                \ /
                 |
               [ D ]

In the following diagrams:
* Labels beginning with a C refer to a standard commit
* Labels beginning with a M refer to a merge commit

This sequence should be auto-tested.

### Step 1
Initial situation.

```
    Branch A root   Branch A tip
    v               v
  [CA1] - [CA2] - [CA3]
            |
            \---- [CB1] - [CB2] - [CB3]
            ^                       |^
            Branch B root           |Branch B tip
                                    |
                                    \---- [CC1] - [CC2]
                                    ^               ^
                                    Branch C root   Branch C tip
```

### Step 2
Merge B on A.
Common ancestor of CA3 and CB3 is CA2.

```
    Branch A root                           Branch A tip
    v                                       v
  [CA1] - [CA2] - [CA3] ----------------- [MA4]
            |                               |
            \---- [CB1] - [CB2] - [CB3] ----/
            ^                       |^
            Branch B root           |Branch B tip
                                    |
                                    \---- [CC1] - [CC2]
                                    ^               ^
                                    Branch C root   Branch C tip
```

### Step 3
Merge C on A.
Common ancestor of MA4 and CC2 is CB3.

```
    Branch A root                           Branch A tip
    v                                       v
  [CA1] - [CA2] - [CA3] ----------------- [MA4] --------- [MA5]
            |                               |               |
            \---- [CB1] - [CB2] - [CB3] ----/               |
            ^                       |^                      |
            Branch B root           |Branch B tip           |
                                    |                       |
                                    \---- [CC1] - [CC2] ----/
                                    ^               ^
                                    Branch C root   Branch C tip
```

### Step 4
Work on B and C.

```
    Branch A root                           Branch A tip
    v                                       v
  [CA1] - [CA2] - [CA3] ----------------- [MA4] --------- [MA5]
            |                               |               |
            \---- [CB1] - [CB2] - [CB3] ----/-------------- | --- [CB4] - [CB5]
            ^                       |                       |               ^
            Branch B root           |                       |               Branch B tip
                                    |                       |
                                    \---- [CC1] - [CC2] ----/ --- [CC3]
                                    ^                               ^
                                    Branch C root                   Branch C tip
```

### Step 5
Merge B on A.
Common ancestors of MA5 and CB5 is CB3.

```
    Branch A root                                                                   Branch A tip
    v                                                                               v
  [CA1] - [CA2] - [CA3] ----------------- [MA4] --------- [MA5] ----------------- [MA6]
            |                               |               |                       |
            \---- [CB1] - [CB2] - [CB3] ----/-------------- | --- [CB4] - [CB5] ----/
            ^                       |                       |               ^
            Branch B root           |                       |               Branch B tip
                                    |                       |
                                    \---- [CC1] - [CC2] ----/ --- [CC3]
                                    ^                               ^
                                    Branch B root                   Branch B tip
```

### Step 6
Merge A on C.
Common ancestors of CC3 and MA6 are multiple: CC2, CB3, CA2...

```
    Branch A root                                                                   Branch A tip
    v                                                                               v
  [CA1] - [CA2] - [CA3] ----------------- [MA4] --------- [MA5] ----------------- [MA6] ----\
            |                               |               |                       |       |
            \---- [CB1] - [CB2] - [CB3] ----/-------------- | --- [CB4] - [CB5] ----/       |
            ^                       |                       |               ^               |
            Branch B root           |                       |               Branch B tip    |
                                    |                       |                               |
                                    \---- [CC1] - [CC2] ----/ --- [CC3] ----------------- [MC4]
                                    ^                                                       ^
                                    Branch C root                                           Branch C tip
```

### Step 7
Work on B and C.

```
    Branch A root                                                                   Branch A tip
    v                                                                               v
  [CA1] - [CA2] - [CA3] ----------------- [MA4] --------- [MA5] ----------------- [MA6] ----\
            |                               |               |                       |       |
            \---- [CB1] - [CB2] - [CB3] ----/-------------- | --- [CB4] - [CB5] ----/------ | --- [CB6]
            ^                       |                       |               ^               |
            Branch B root           |                       |               Branch B tip    |
                                    |                       |                               |
                                    \---- [CC1] - [CC2] ----/ --- [CC3] ----------------- [MC4] - [CC5]
                                    ^                                                       ^
                                    Branch C root                                           Branch C tip
```

### Step 8
Merge B on A.
Common ancestor of CB6 and MA6 is CB5.

```
    Branch A root                                                                                           Branch A tip
    v                                                                                                       v
  [CA1] - [CA2] - [CA3] ----------------- [MA4] --------- [MA5] ----------------- [MA6] ----\-------------[MA7]
            |                               |               |                       |       |               |
            \---- [CB1] - [CB2] - [CB3] ----/-------------- | --- [CB4] - [CB5] ----/------ | --- [CB6] ----/
            ^                       |                       |                               |       ^
            Branch B root           |                       |                               |       Branch B tip
                                    |                       |                               |
                                    \---- [CC1] - [CC2] ----/ --- [CC3] ----------------- [MC4] - [CC5]
                                    ^                                                               ^
                                    Branch C root                                                   Branch C tip
```

### Step 9
Merge C on A.
Common ancestors of CC5 and MA7 are MA6, CB5...

```
    Branch A root                                                                                                   Branch A tip
    v                                                                                                               v
  [CA1] - [CA2] - [CA3] ----------------- [MA4] --------- [MA5] ----------------- [MA6] ----\-------------[MA7] - [MA8]
            |                               |               |                       |       |               |       |
            \---- [CB1] - [CB2] - [CB3] ----/-------------- | --- [CB4] - [CB5] ----/------ | --- [CB6] ----/       |
            ^                       |                       |                               |       ^               |
            Branch B root           |                       |                               |       Branch B tip    |
                                    |                       |                               |                       |
                                    \---- [CC1] - [CC2] ----/ --- [CC3] ----------------- [MC4] - [CC5] ------------/
                                    ^                                                               ^
                                    Branch C root                                                   Branch C tip
```

### Step 10
Merge A on B.

```
    Branch A root                                                                                                   Branch A tip
    v                                                                                                               v
  [CA1] - [CA2] - [CA3] ----------------- [MA4] --------- [MA5] ----------------- [MA6] ----\-------------[MA7] - [MA8] ---\
            |                               |               |                       |       |               |       |      |
            \---- [CB1] - [CB2] - [CB3] ----/-------------- | --- [CB4] - [CB5] ----/------ | --- [CB6] ----/------ | ---[MB7]
            ^                       |                       |                               |                       |      ^
            Branch B root           |                       |                               |                       |      Branch B tip
                                    |                       |                               |                       |
                                    \---- [CC1] - [CC2] ----/ --- [CC3] ----------------- [MC4] - [CC5] ------------/
                                    ^                                                               ^
                                    Branch C root                                                   Branch C tip
```

### Step 11
Work on B and C at same spot.

```
    Branch A root                                                                                                   Branch A tip
    v                                                                                                               v
  [CA1] - [CA2] - [CA3] ----------------- [MA4] --------- [MA5] ----------------- [MA6] ----\-------------[MA7] - [MA8] ---\
            |                               |               |                       |       |               |       |      |
            \---- [CB1] - [CB2] - [CB3] ----/-------------- | --- [CB4] - [CB5] ----/------ | --- [CB6] ----/------ | ---[MB7] - [CB8]
            ^                       |                       |                               |                       |              ^
            Branch B root           |                       |                               |                       |              Branch B tip
                                    |                       |                               |                       |
                                    \---- [CC1] - [CC2] ----/ --- [CC3] ----------------- [MC4] - [CC5] ------------/-- [CC6]
                                    ^                                                                                     ^
                                    Branch C root                                                                         Branch C tip
```

### Step 12
Merge C on B. Conflict.

```
    Branch A root                                                                                                   Branch A tip
    v                                                                                                               v
  [CA1] - [CA2] - [CA3] ----------------- [MA4] --------- [MA5] ----------------- [MA6] ----\-------------[MA7] - [MA8] ---\                        Branch B tip
            |                               |               |                       |       |               |       |      |                        v
            \---- [CB1] - [CB2] - [CB3] ----/-------------- | --- [CB4] - [CB5] ----/------ | --- [CB6] ----/------ | ---[MB7] - [CB8] ---------- [MB9]
            ^                       |                       |                               |                       |                               |
            Branch B root           |                       |                               |                       |                               |
                                    |                       |                               |                       |                               |
                                    \---- [CC1] - [CC2] ----/ --- [CC3] ----------------- [MC4] - [CC5] ------------/--- [CC6] ---------------------/
                                    ^                                                                                     ^
                                    Branch C root                                                                         Branch C tip
```

## Workflow samples

```
Working directory                Local database                   Remote database

```
