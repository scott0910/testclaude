/**
 * @file    hello.c
 * @brief   Print the multiplication table (1x1 to 9x9)
 * @author  Scott
 * @date    2026-03-30
 */

#include <stdio.h>

int main(void) {
    /* Outer loop: multiplier, ranging from 1 to 9 */
    for (int multiplier = 1; multiplier <= 9; multiplier++) {

        /* Inner loop: multiplicand, ranging from 1 to 9 */
        for (int multiplicand = 1; multiplicand <= 9; multiplicand++) {
            /* Print each expression; result is right-aligned in 2 characters */
            printf("%d x %d = %2d  ", multiplier, multiplicand, multiplier * multiplicand);
        }

        /* Move to the next line after each row */
        printf("\n");
    }

    return 0; /* Return 0 to indicate successful execution */
}
