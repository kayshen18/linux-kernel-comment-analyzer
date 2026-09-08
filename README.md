# Linux Kernel Comment Analyzer
[![CMake CI](https://github.com/kayshen18/linux-kernel-comment-analyzer/actions/workflows/cmake.yml/badge.svg)](https://github.com/kayshen18/linux-kernel-comment-analyzer/actions/workflows/cmake.yml)

A multithreaded tool for extracting comments and measuring comment density in large source-code repositories.

Evaluated on Linux 6.12.38: **65,518 files**, **35.4M lines**, and **2.31M extracted comment blocks**.

## Features

- Supports C/C++, Assembly, Python, Shell, and Makefile
- Uses language-aware state machines to handle strings and escaped characters
- Extracts line, block, and inline comments
- Reports metrics by language and top-level directory
- Exports structured JSONL records
- Supports configurable multithreading and summary-only analysis
- Includes nine automated CTest tests

## Pipeline

```text
SourceScanner
     ¡ý
Language-specific parsers
     ¡ý
ParseResult
     ©À©¤©¤ Statistics
     ©¸©¤©¤ JSONL output
```

## Build and test

```powershell
cmake --build .\out\build\x64-Release

ctest `
    --test-dir .\out\build\x64-Release `
    -C Release `
    --output-on-failure
```

## Usage

Extract comments to JSONL:

```powershell
.\out\build\x64-Release\linux-comment-analyzer.exe `
    .\linux-6.12.38 `
    .\results\linux-comments.jsonlESH `
    --threads 16
```

Run statistics only:

```powershell
.\out\build\x64-Release\linux-comment-analyzer.exe `
    .\linux-6.12.38 `
    --summary-only `
    --threads 16
```

## Results

Tested on the complete Linux 6.12.38 source tree:

| Metric | Result |
|---|---:|
| Parsed files | 65,518 |
| Failed files | 0 |
| Physical lines | 35,416,722 |
| Comment blocks | 2,313,867 |
| Comment density | 16.00% |
| Valid JSONL records | 2,313,867 |
| Manual validation | 250/250 correct |

### Parallel performance

| Threads | Time | Speedup |
|---:|---:|---:|
| 1 | 45.863 s | 1.00¡Á |
| 2 | 25.109 s | 1.83¡Á |
| 4 | 15.998 s | 2.87¡Á |
| 8 | 14.423 s | 3.18¡Á |
| 16 | 14.092 s | 3.25¡Á |

The 16-thread JSONL run completed in **19.792 seconds** and produced a **474.78 MiB** output file.

> Manual validation results refer to a stratified sample of 250 extracted comments.