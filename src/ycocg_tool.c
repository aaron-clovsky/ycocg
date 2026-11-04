/*******************************************************************************
RGB <-> YCoCg
Quantitative analyzer
Copyright (c) 2026 Aaron Clovsky

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

/*******************************************************************************
Headers
*******************************************************************************/
#include <ycocg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*******************************************************************************
Macros
*******************************************************************************/
#define FALSE 0
#define TRUE  1

/*******************************************************************************
Utilities
*******************************************************************************/
static void help(void)
{
    puts("Usage: %s <MODE> [OPTIONS]\n"
         " modes: rgb | ycocg | analyze | all | allrgb | allycocg\n"
         "  rgb Options: <r> <g> <b>\n"
         "  ycocg Options: <y> <co> <cg>");
    exit(2);
}

/*******************************************************************************
main()
*******************************************************************************/
int main(int argc, char ** argv)
{
    int analyze = FALSE;
    int verify  = FALSE;
    int all     = FALSE;
    int rgb     = FALSE;

    switch (argc) {
        default: help(); exit(2);
        case 2:
            verify = TRUE;

            if (strcmp(argv[1], "analyze") == 0) {
                analyze = TRUE;
            } else if (strcmp(argv[1], "all") == 0) {
                all = TRUE;
            } else if (strcmp(argv[1], "allrgb") == 0) {
                all = FALSE;
                rgb = TRUE;
            } else if (strcmp(argv[1], "allycocg") == 0) {
                all = FALSE;
                rgb = FALSE;
            } else {
                help();
                exit(2);
            }

            break;
        case 5:
            verify = FALSE;

            if (strcmp(argv[1], "rgb") == 0) {
                rgb = TRUE;
                break;
            } else if (strcmp(argv[1], "ycocg") == 0) {
                rgb = FALSE;
                break;
            } else {
                help();
                exit(2);
            }

            break;
    }

    if (analyze) {
        unsigned same     = 0; /* Number of conversions with no difference */
        unsigned diff     = 0; /* Number of conversions with any difference */
        unsigned diff_one = 0; /* Number of conversions where */
                               /* exactly one field differs by exactly one */
        unsigned diff_two = 0; /* Number of conversions where
                                  two fields differ by exactly one */
        unsigned diff_any_one = 0; /* Number of conversions where any number
                                      of fields differ by as much as one */
        unsigned diff_any_two = 0; /* Number of conversions where any number
                                      of fields differ by as much as two */

        unsigned r, g, b;
        for (r = 0; r <= 255; r++) {
            for (g = 0; g <= 255; g++) {
                for (b = 0; b <= 255; b++) {
                    uint8_t y;
                    int8_t  co, cg;        /* RGB -> YCoCg Result */
                    uint8_t rr, gg, bb;    /* RGB -> YCoCg -> RGB Result */
                    int     d_r, d_g, d_b; /* Difference between Original
                                              and double converted values */

                    ycocg_from_rgb((uint8_t)r,
                                   (uint8_t)g,
                                   (uint8_t)b,
                                   &y,
                                   &co,
                                   &cg);

                    ycocg_to_rgb(y, co, cg, &rr, &gg, &bb);

                    /* Calculate difference */
                    d_r = rr - r;
                    d_r = (d_r < 0) ? -d_r : d_r;
                    d_g = gg - g;
                    d_g = (d_g < 0) ? -d_g : d_g;
                    d_b = bb - b;
                    d_b = (d_b < 0) ? -d_b : d_b;

                    /* Identical or not */
                    if (r == rr && g == gg && b == bb) {
                        same++;
                    } else {
                        diff++;
                    }

                    /* Off by exactly one in exactly one channel */
                    if ((d_r == 1 && d_g == 0 && d_b == 0) ||
                        (d_r == 0 && d_g == 1 && d_b == 0) ||
                        (d_r == 0 && d_g == 0 && d_b == 1)) {
                        diff_one++;
                    }

                    /* Off by exactly one in exactly two channels */
                    if ((d_r == 0 && d_g == 1 && d_b == 1) ||
                        (d_r == 1 && d_g == 0 && d_b == 1) ||
                        (d_r == 1 && d_g == 1 && d_b == 0)) {
                        diff_two++;
                    }

                    /* Off by no more than one in any number of channels */
                    if (d_r <= 1 && d_g <= 1 && d_b <= 1) {
                        diff_any_one++;
                    }

                    /* Off by no more than two in any number of channels */
                    if (d_r <= 2 && d_g <= 2 && d_b <= 2) {
                        diff_any_two++;
                    } else {
                        printf("Off by more than two in any channel:\n"
                               " -> r: %d, g: %d, b: %d\n",
                               (int)r,
                               (int)g,
                               (int)b);
                        printf(" --> Y: %d, Co: %d, Cg: %d\n",
                               (int)y,
                               (int)co,
                               (int)cg);
                        printf(" ---> rr: %d, gg: %d, bb: %d\n\n",
                               (int)rr,
                               (int)gg,
                               (int)bb);
                    }
                }
            }
        }

        printf("Total: 2^24: %d\n", 1 << 24);
        printf("Identical: %u\n", same);
        printf("Different: %u\n", diff);
        printf("Off by one in exactly one channel: %u\n", diff_one);
        printf("Off by one in exactly two channels: %u\n", diff_two);
        printf("Off by no more than one in each channel: %u\n", diff_any_one);
        printf("Off by no more than two in each channel: %u\n", diff_any_two);
        printf("Off more than two in any channel: %u\n",
               (1U << 24) - diff_any_two);
    } else if (verify) {
        if (all) {
            unsigned r, g, b;
            for (r = 0; r <= 255; r++) {
                for (g = 0; g <= 255; g++) {
                    for (b = 0; b <= 255; b++) {
                        uint8_t y;
                        int8_t  co, cg;
                        uint8_t rr, gg, bb;

                        ycocg_from_rgb((uint8_t)r,
                                       (uint8_t)g,
                                       (uint8_t)b,
                                       &y,
                                       &co,
                                       &cg);

                        ycocg_to_rgb(y, co, cg, &rr, &gg, &bb);

                        printf("%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
                               r,
                               g,
                               b,
                               (unsigned)y,
                               (unsigned)co,
                               (unsigned)cg,
                               rr,
                               gg,
                               bb);
                    }
                }
            }
        } else if (rgb) {
            unsigned r, g, b;
            for (r = 0; r <= 255; r++) {
                for (g = 0; g <= 255; g++) {
                    for (b = 0; b <= 255; b++) {
                        uint8_t y;
                        int8_t  co, cg;

                        ycocg_from_rgb((uint8_t)r,
                                       (uint8_t)g,
                                       (uint8_t)b,
                                       &y,
                                       &co,
                                       &cg);

                        printf("%u,%u,%u,%u,%u,%u\n",
                               r,
                               g,
                               b,
                               (unsigned)y,
                               (unsigned)co,
                               (unsigned)cg);
                    }
                }
            }
        } else {
            unsigned y, co, cg;
            for (y = 0; y <= 255; y++) {
                for (co = 0; co <= 255; co++) {
                    for (cg = 0; cg <= 255; cg++) {
                        uint8_t r, g, b;

                        ycocg_to_rgb((uint8_t)y,
                                     (uint8_t)co,
                                     (uint8_t)cg,
                                     &r,
                                     &g,
                                     &b);

                        printf("%u,%u,%u,%u,%u,%u\n",
                               y,
                               co,
                               cg,
                               (unsigned)r,
                               (unsigned)g,
                               (unsigned)b);
                    }
                }
            }
        }
    } else if (rgb) {
        int     r, g, b;
        uint8_t y;
        int8_t  co, cg;

        r = atoi(argv[2]);
        g = atoi(argv[3]);
        b = atoi(argv[4]);

        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
            puts("Input out of range [0-255]");
            exit(2);
        }

        ycocg_from_rgb((uint8_t)r, (uint8_t)g, (uint8_t)b, &y, &co, &cg);

        printf("RGB: %d,%d,%d -> YCoCg: %u,%u,%u\n",
               r,
               g,
               b,
               (unsigned)y,
               (unsigned)co,
               (unsigned)cg);
    } else {
        int     y, co, cg;
        uint8_t r, g, b;

        y  = atoi(argv[2]);
        co = atoi(argv[3]);
        cg = atoi(argv[4]);

        if (y < 0 || y > 255 || co < 0 || co > 255 || cg < 0 || cg > 255) {
            puts("Input out of range [0-255]");
            exit(2);
        }

        ycocg_to_rgb((uint8_t)y, (int8_t)co, (int8_t)cg, &r, &g, &b);

        printf("YCoCg: %d,%d,%d -> RGB: %u,%u,%u\n",
               y,
               co,
               cg,
               (unsigned)r,
               (unsigned)g,
               (unsigned)b);
    }

    return 0;
}
