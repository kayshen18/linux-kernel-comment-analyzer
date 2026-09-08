cmake_minimum_required(VERSION 3.16)

project(LinuxCommentAnalyzer
    VERSION 1.0
    DESCRIPTION "Large-scale Linux kernel comment analyzer"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 核心分析模块：整个文件只能有一个 analyzer-core
add_library(analyzer-core
    src/scanner.cpp
    src/parser.cpp
    src/python_parser.cpp
    src/shell_parser.cpp
    src/makefile_parser.cpp
    src/assembly_parser.cpp
    src/statistics.cpp
    src/writer.cpp
)

target_include_directories(analyzer-core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

# 主程序
add_executable(linux-comment-analyzer
    src/main.cpp
)

target_link_libraries(linux-comment-analyzer
    PRIVATE
        analyzer-core
)

# 启用测试
enable_testing()

# 解析器测试
add_executable(parser-tests
    tests/parser_test.cpp
)

target_link_libraries(parser-tests
    PRIVATE
        analyzer-core
)

target_compile_definitions(parser-tests
    PRIVATE
        TEST_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures"
)

add_test(
    NAME CFamilyParserTests
    COMMAND parser-tests
)

# 统计模块测试
add_executable(statistics-tests
    tests/statistics_test.cpp
)

target_link_libraries(statistics-tests
    PRIVATE
        analyzer-core
)

add_test(
    NAME StatisticsTests
    COMMAND statistics-tests
)

# 扫描器测试
add_executable(scanner-tests
    tests/scanner_test.cpp
)

target_link_libraries(scanner-tests
    PRIVATE
        analyzer-core
)

target_compile_definitions(scanner-tests
    PRIVATE
        TEST_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures"
)

add_test(
    NAME SourceScannerTests
    COMMAND scanner-tests
)

# JSONL 输出模块测试
add_executable(writer-tests
    tests/writer_test.cpp
)

target_link_libraries(writer-tests
    PRIVATE
        analyzer-core
)

add_test(
    NAME JsonlWriterTests
    COMMAND writer-tests
)

# Python 注释解析器测试
add_executable(python-parser-tests
    tests/python_parser_test.cpp
)

target_link_libraries(python-parser-tests
    PRIVATE
        analyzer-core
)

target_compile_definitions(python-parser-tests
    PRIVATE
        TEST_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures"
)

add_test(
    NAME PythonParserTests
    COMMAND python-parser-tests
)

# Shell 注释解析器测试
add_executable(shell-parser-tests
    tests/shell_parser_test.cpp
)

target_link_libraries(shell-parser-tests
    PRIVATE
        analyzer-core
)

target_compile_definitions(shell-parser-tests
    PRIVATE
        TEST_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures"
)

add_test(
    NAME ShellParserTests
    COMMAND shell-parser-tests
)

add_executable(makefile-parser-tests
    tests/makefile_parser_test.cpp
)

target_link_libraries(makefile-parser-tests
    PRIVATE
        analyzer-core
)

target_compile_definitions(makefile-parser-tests
    PRIVATE
        TEST_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures"
)

add_test(
    NAME MakefileParserTests
    COMMAND makefile-parser-tests
)

add_executable(assembly-parser-tests
    tests/assembly_parser_test.cpp
)

target_link_libraries(assembly-parser-tests
    PRIVATE
        analyzer-core
)

target_compile_definitions(assembly-parser-tests
    PRIVATE
        TEST_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures"
)

add_test(
    NAME AssemblyParserTests
    COMMAND assembly-parser-tests
)