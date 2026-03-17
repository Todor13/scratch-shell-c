# Shell from Scratch (CodeCrafters)

A minimal Unix-like shell built from scratch in C as part of the CodeCrafters challenge.
The goal of this project is **learning by implementation** — understanding how shells work internally rather than optimizing for performance or completeness.

---

## Overview

This shell supports core functionality such as:

* Command execution (external programs)
* Built-in commands
* Input parsing and tokenization
* Command history
* Basic autocomplete

The implementation intentionally avoids third-party libraries (e.g. `readline`) to stay true to the “from scratch” philosophy of the challenge.

---

## Design Philosophy

* **Educational first** – clarity and simplicity over performance
* **Minimal dependencies** – no external libraries
* **Modular structure** – split into logical components
* **Incremental development** – built step-by-step following the challenge

---

## Project Structure

The codebase is organized into modules, each with a `.c` implementation file and a corresponding `.h` header (except `main.c`):

```
.
├── main.c
├── builtin.c / builtin.h
├── exec.c / exec.h
├── history.c / history.h
├── input.c / input.h
├── tokenizer.c / tokenizer.h
```

### `main.c`

* Entry point of the shell
* Runs the main REPL loop (Read → Evaluate → Print → Loop)
* Coordinates input, parsing, and execution

### `input`

* Handles raw user input
* Reads from stdin without external helpers (no readline)
* Responsible for basic line handling

### `tokenizer`

* Splits input into arguments suitable for command execution incl. pipes
* Handles parsing logic required for execution

### `exec`

* Handles executables search (not optimized)

### `builtin`

* Implements built-in shell commands (e.g. `cd`, `exit`)
* Keeps them separate from external commands

### `history`

* Stores previously entered commands
* Enables navigation and reuse of past input

---

## Features

* REPL loop
* Built-in commands
* External command execution
* Token-based parsing
* Command history
* Basic autocomplete (via directory traversal)

---

## Known Limitations / Work in Progress

This project prioritizes correctness and understanding over optimization. Some areas are intentionally simplified:

* **No caching for autocomplete**

  * Directory traversal happens on every autocomplete request
  * Can be improved with caching or indexing

* **Limited parsing capabilities**

  * No advanced shell features (only partial redirection, quoting edge cases, etc.)

* **Basic history handling**

  * Could be extended

* **Minimal error handling in edge cases**

* **No performance optimizations**

  * Straightforward implementations used for clarity

---

## Future Improvements

* Add caching for autocomplete
* Improve tokenizer
* Enhance history (persistent storage, reverse search)
* Better error handling and reporting
* Performance tuning where appropriate

---

## Build & Formatting

* Compiled using `clang`
* Includes a **custom `.clang-format` configuration** for consistent code style

---

## Notes

* No third-party libraries are used (including `readline`)
* Everything is implemented manually to better understand system-level behavior

---

## Purpose

This project is not meant to compete with production shells like `bash` or `zsh`.
Instead, it serves as a hands-on exploration of:

* Process management
* Input parsing
* Shell architecture
* System programming in C

---

## Status

✔ Core functionality complete
⚠ Polishing and improvements in progress

---

