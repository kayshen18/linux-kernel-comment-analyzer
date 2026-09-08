#define VALUE 1
# Standalone assembly comment
mov $1, %eax # Inline assembly comment
.string "# not a comment"
nop
/* Block assembly comment
 * continued
 */