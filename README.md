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
```

## Build and test

```powershell
cmake --build .\out\build\x64-Release
ctest --test-dir .\out\build\x64-Release -C Release --output-on-failure
```

The project currently contains nine CTest checks, including parser tests for
all supported language families and a parallel end-to-end test.

## Usage

Write extracted comments to JSONL with one record per comment block:

```powershell
.\out\build\x64-Release\linux-comment-analyzer.exe `
    .\linux-6.12.38 `
    .\results\linux-comments.jsonl `
    --threads 16
```

Run statistics only, without creating the large JSONL corpus:

```powershell
.\out\build\x64-Release\linux-comment-analyzer.exe `
    .\linux-6.12.38 `
    --summary-only `
    --threads 16
```

`--threads N` sets the number of parser workers and defaults to `1`. Each
worker owns its parser state; only aggregation and JSONL writing are
synchronized. JSONL record order is intentionally unspecified when more than
one worker is used.

## Linux 6.12.38 evaluation

The complete source tree produced the same result at every tested thread
count: 65,518 parsed files, no failures, 35,416,722 physical lines, and
2,313,867 extracted comment blocks.

| Threads | Summary-only time | Speedup |
|--------:|------------------:|--------:|
| 1 | 45.863 s | 1.00x |
| 2 | 25.109 s | 1.83x |
| 4 | 15.998 s | 2.87x |
| 8 | 14.423 s | 3.18x |
| 16 | 14.092 s | 3.25x |

A 16-thread JSONL run completed in 19.792 seconds and emitted 2,313,867 valid
JSON records (474.78 MiB). Manual validation of 250 stratified samples across
C/C++, assembly, Python, Shell, and Makefile comments found 250 correct
extractions.
