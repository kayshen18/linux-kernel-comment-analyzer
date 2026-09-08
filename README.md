# Linux Kernel Comment Analyzer

A C++17 static-analysis tool for extracting and analyzing source-code
comments from large, heterogeneous codebases.

The analyzer was evaluated on the Linux 6.12.38 source tree. It supports
C/C++-style source files, assembly, Python, Shell, and Makefiles, and
produces both aggregate metrics and structured JSONL output.

## Features

- Recursively scans large source-code directories using `std::filesystem`.
- Supports C/C++, assembly, Python, Shell, and Makefiles.
- Uses language-aware state machines instead of simple regular expressions.
- Distinguishes comments from string literals and character literals.
- Recognizes standalone, block, and inline comments.
- Handles language-specific constructs, including:
  - C/C++ escaped strings and block comments.
  - Python quoted and triple-quoted strings.
  - Shell shebangs, quoted strings, and word-internal `#` characters.
  - Makefile recipes and escaped `\#` characters.
  - Architecture-dependent assembly comment symbols.
  - Assembly preprocessor directives such as `#define` and `#include`.
- Computes metrics by language and top-level source directory.
- Exports comment records as streaming JSONL.
- Supports a summary-only mode without writing the extracted comment corpus.

## Architecture

```text
Source directory
       |
       v
+------------------+
| SourceScanner    |
| File discovery   |
+------------------+
       |
       v
+------------------------------+
| Language-specific parsers    |
|                              |
| CFamilyParser                |
| AssemblyParser               |
| PythonParser                 |
| ShellParser                  |
| MakefileParser               |
+------------------------------+
       |
       v
+------------------+
| ParseResult      |
| Unified records  |
+------------------+
       |
       +----------------------+
       |                      |
       v                      v
+------------------+   +------------------+
| Statistics       |   | JsonlWriter      |
| Language/dirs    |   | Structured data  |
+------------------+   +------------------+