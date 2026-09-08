#include "statistics.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

    int failureCount = 0;

    void expectEqual(
        std::size_t actual,
        std::size_t expected,
        const std::string& testName)
    {
        if (actual == expected) {
            std::cout << "[PASS] " << testName << '\n';
            return;
        }

        std::cerr
            << "[FAIL] "
            << testName
            << ": expected "
            << expected
            << ", actual "
            << actual
            << '\n';

        ++failureCount;
    }

    void expectNear(
        double actual,
        double expected,
        double tolerance,
        const std::string& testName)
    {
        if (std::abs(actual - expected) <= tolerance) {
            std::cout << "[PASS] " << testName << '\n';
            return;
        }

        std::cerr
            << "[FAIL] "
            << testName
            << ": expected "
            << expected
            << ", actual "
            << actual
            << '\n';

        ++failureCount;
    }

} // namespace

int main()
{
    ParseResult firstResult;
    firstResult.metrics.physicalLines = 18;
    firstResult.metrics.codeLines = 9;
    firstResult.metrics.commentLines = 5;
    firstResult.metrics.mixedLines = 1;
    firstResult.metrics.blankLines = 5;
    firstResult.comments.resize(3);

    ParseResult secondResult;
    secondResult.metrics.physicalLines = 9;
    secondResult.metrics.codeLines = 5;
    secondResult.metrics.commentLines = 5;
    secondResult.metrics.mixedLines = 1;
    secondResult.metrics.blankLines = 0;
    secondResult.comments.resize(5);

    AggregateMetrics aggregate;

    aggregate.add(firstResult);
    aggregate.add(secondResult);

    expectEqual(
        aggregate.parsedFiles,
        2,
        "parsed file count"
    );

    expectEqual(
        aggregate.physicalLines,
        27,
        "aggregate physical lines"
    );

    expectEqual(
        aggregate.codeLines,
        14,
        "aggregate code lines"
    );

    expectEqual(
        aggregate.commentLines,
        10,
        "aggregate comment lines"
    );

    expectEqual(
        aggregate.mixedLines,
        2,
        "aggregate mixed lines"
    );

    expectEqual(
        aggregate.blankLines,
        5,
        "aggregate blank lines"
    );

    expectEqual(
        aggregate.commentBlocks,
        8,
        "aggregate comment blocks"
    );

    expectNear(
        aggregate.commentDensity(),
        45.4545,
        0.001,
        "comment density"
    );

    AggregateMetrics emptyAggregate;

    expectNear(
        emptyAggregate.commentDensity(),
        0.0,
        0.001,
        "empty input comment density"
    );

    AggregateMetrics blankOnlyAggregate;
    blankOnlyAggregate.physicalLines = 5;
    blankOnlyAggregate.blankLines = 5;

    expectNear(
        blankOnlyAggregate.commentDensity(),
        0.0,
        0.001,
        "blank-only input comment density"
    );

    if (failureCount == 0) {
        std::cout << "\nAll statistics tests passed.\n";
        return EXIT_SUCCESS;
    }

    std::cerr
        << "\n"
        << failureCount
        << " statistics test(s) failed.\n";

    return EXIT_FAILURE;
}