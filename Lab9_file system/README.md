# Lab 9: File system

## Overview

This lab adds large files and symbolic links to the xv6 file system.

For details, hints, and grading criteria, please refer to the official MIT lab page:
https://pdos.csail.mit.edu/6.828/2022/labs/fs.html

## Exercises

### Large files (moderate)
Increase the maximum size of an xv6 file. Currently files are limited to 268 blocks
(12 direct blocks plus one singly-indirect block). Modify `bmap()` in `fs.c` to
support a doubly-indirect block: use 11 direct blocks, the 12th as a singly-indirect
block, and the 13th as the new doubly-indirect block. This allows files of up to
65803 blocks. You are done when `bigfile` writes 65803 blocks and `usertests -q`
passes.

### Symbolic links (moderate)
Implement the `symlink(char *target, char *path)` system call, which creates a new
symbolic link at `path` referring to the file named by `target`. Handle symbolic
links in the `open` system call: follow links recursively (with a depth limit to
detect cycles), honor the `O_NOFOLLOW` flag, and ensure other system calls like
`link` and `unlink` operate on the link itself. Pass `symlinktest` and
`usertests -q`.