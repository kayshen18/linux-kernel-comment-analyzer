// File-level comment

#include <stdio.h>

int main(void)
{
    const char* url = "https://example.com";
    const char* fake = "/* this is not a comment */";

    int value = 42; // Inline comment

    /*
     * Multi-line block comment
     */
    printf("%d\n", value);

    return 0;
}