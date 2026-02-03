// cf_test.c
#include <stdio.h>
#include <stdlib.h>
#include "branch_runtime.h"   // add this


static int helper(int x) {              // Direct call target
    if (x & 1)                          // Conditional branch
        return x * 3;                   // Return
    return x + 10;                      // Return
}

static int foo(int n) {                 // Direct call target
    int sum = 0;

    // Loop header (for)
    for (int i = 0; i < n; i++) {       // Conditional branch at loop condition
        if (i == 2)                     // Conditional branch
            continue;                   // Unconditional branch (jump to next iter)

        if (i == 5)                     // Conditional branch
            break;                      // Unconditional branch (jump out of loop)

        sum += helper(i);               // Direct call
    }

    // Loop header (while)
    int j = n;
    while (j > 0) {                     // Conditional branch at loop condition
        sum += (j % 2) ? 1 : 2;         // Conditional branch (ternary)
        j--;
        if (j == 3) goto early;         // Conditional + unconditional branch (goto)
    }

early:
    // Loop header (do-while)
    int k = 0;
    do {
        sum += k;
        k++;
        if (k == 2) return sum;         // Conditional branch + Return
    } while (k < 4);                    // Conditional branch at loop condition

    if (sum < 0) exit(2);               // Conditional branch + Exit (rarely taken)

    return sum;                         // Return
}

int main(int argc, char **argv) {

    init_branch_stats();        // add this (before measurement)



    int n = (argc > 1) ? atoi(argv[1]) : 8;  // Conditional branch (ternary)
    int r = foo(n);                           // Direct call
    printf("result=%d\n", r);                 // Direct call (printf)

    print_branch_stats();       // add this (after run)

    return 0;                                 // Return/exit


}
