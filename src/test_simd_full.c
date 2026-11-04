/*******************************************************************************
RGB <-> YCoCg Conversion
Exhaustively test SIMD vs Scalar conversion
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*******************************************************************************
Types
*******************************************************************************/
enum test
{
    TEST_UNSET,
    TEST_YUY2_FROM_BGRX,
    TEST_YUY2_TO_BGRA
};

/*******************************************************************************
Utilities
*******************************************************************************/
static void print_hex(const uint8_t * data, size_t size)
{
    size_t i;
    for (i = 0; i < size; i++) {
        printf("%02X ", data[i]);
        if (!((i + 1) & 15)) printf("\n");
    }

    printf("\n");
}

static void compare_and_debug(uint32_t * scalar,
                              uint32_t * simd,
                              size_t     cmp_size)
{
    if (memcmp(simd, scalar, cmp_size) != 0) {
        size_t k, off, len;

        for (k = 0; k < cmp_size; k++) {
            if (simd[k] != scalar[k]) break;
        }

        if (k == cmp_size) {
            puts("BUG: compare_and_debug(): No miscompare offset found");
            exit(1);
        }

        off = (k < 4) ? 0 : ((k & ~3) - 4) * sizeof(uint32_t);
        len = ((cmp_size - off) > 256) ? 256 : (cmp_size - off);

        printf("SIMD and Scalar outputs do not match (dword @ offset %u)\n"
               "SIMD (@ offset %u):\n",
               (unsigned)(k * sizeof(uint32_t)),
               (unsigned)off);
        print_hex((const uint8_t *)&simd[off / sizeof(uint32_t)], len);
        printf("Scalar (@ offset %u):\n", (unsigned)off);
        print_hex((const uint8_t *)&scalar[off / sizeof(uint32_t)], len);

        exit(1);
    }
}

/*******************************************************************************
yuy2_from_bgrx
*******************************************************************************/
static void calc_chunks_yuy2_from_bgrx(uint64_t * combinations, uint64_t count)
{
    uint32_t input[8192];
    uint32_t output[2][YCOCG_YUY2_MIN_STRIDE(sizeof(input) / sizeof(uint32_t))];
    const uint32_t chunk_size = sizeof(input) / (sizeof(uint32_t) * 2);
    const uint32_t width      = sizeof(input) / sizeof(uint32_t);
    uint32_t       i;

    while (count--) {
        for (i = 0; i < chunk_size; i++) {
            uint8_t * bytes = (uint8_t *)&input[i << 1];
            uint64_t  value = *combinations + i;

            bytes[0] = (uint8_t)(value);
            bytes[1] = (uint8_t)(value >> 8);
            bytes[2] = (uint8_t)(value >> 16);
            bytes[3] = 0;
            bytes[4] = (uint8_t)(value >> 24);
            bytes[5] = (uint8_t)(value >> 32);
            bytes[6] = (uint8_t)(value >> 40);
            bytes[7] = 0;
        }

        ycocg_yuy2_from_bgrx_row_scalar(input, output[0], width);
        ycocg_yuy2_from_bgrx_row(input, output[1], width);

        compare_and_debug(output[0],
                          output[1],
                          sizeof(output[0]) / sizeof(uint32_t));

        *combinations += chunk_size;
    }
}

/*******************************************************************************
yuy2_to_bgra
*******************************************************************************/
static void calc_chunks_yuy2_to_bgra(uint32_t * combinations, uint64_t count)
{
    uint32_t       input[4096];
    uint32_t       output[2][(sizeof(input) / sizeof(uint32_t)) * 2];
    const uint32_t chunk_size = sizeof(input) / sizeof(uint32_t);
    const uint32_t width      = sizeof(input) / sizeof(uint32_t);
    uint32_t       i;

    while (count--) {
        for (i = 0; i < chunk_size; i++) {
            uint8_t * bytes = (uint8_t *)&input[i];
            uint32_t  value = *combinations + i;

            bytes[0] = (uint8_t)(value);
            bytes[1] = (uint8_t)(value >> 8);
            bytes[2] = (uint8_t)(value >> 16);
            bytes[3] = (uint8_t)(value >> 24);
        }

        ycocg_yuy2_to_bgra_row_scalar(input, output[0], width);
        ycocg_yuy2_to_bgra_row(input, output[1], width);

        compare_and_debug(output[0],
                          output[1],
                          sizeof(output[0]) / sizeof(uint32_t));

        *combinations += chunk_size;
    }
}

/*******************************************************************************
main()
*******************************************************************************/
int main(int argc, char ** argv)
{
    uint32_t  start_block = 0;
    uint32_t  block_count = 10000;
    enum test run_test    = TEST_UNSET;

    /* Check for SIMD support */
    if (ycocg_simd_support() == YCOCG_SIMD_NONE) {
        puts("Error: No supported SIMD implementation available");
        exit(1);
    }

    /* Process and validate arguments */
    if (argc > 1 && argc <= 4) {
        if (strcmp(argv[1], "yuy2_from_bgrx") == 0) {
            run_test = TEST_YUY2_FROM_BGRX;
        } else if (strcmp(argv[1], "yuy2_to_bgra") == 0) {
            run_test = TEST_YUY2_TO_BGRA;
        }
    }

    if (run_test != TEST_UNSET && argc > 2 && strcmp(argv[1], "--help") != 0) {
        int start;

        start = atoi(argv[2]);

        if (start < 0 || start >= 10000) {
            printf("Error: Start block must be in range [0, 9999]\n");
            exit(2);
        }

        start_block = (uint32_t)start;

        if (argc == 4) {
            int count = atoi(argv[3]);

            if (count < 1) {
                printf("Error: Block count must be in range [1, 10000]\n");
                exit(2);
            } else if (start_block + count > 10000) {
                printf("Error: The sum of the starting block and block "
                       "count "
                       "cannot exceed 10000\n");
                exit(2);
            }

            block_count = (uint32_t)count;
        }
    } else if (argc == 2) {
        puts("Validating entire range, pass args to process a subset");
    } else {
        printf("Usage: %s <TEST> [starting block] [block count]\n"
               " tests: yuy2_from_bgrx | yuy2_to_bgra\n",
               argv[0]);
        exit(2);
    }

    /* Run test */
    switch (run_test) {
        case TEST_YUY2_FROM_BGRX: {
            /* 2x 24-bit BGR channels from 2x BGRX pixels form a 48-bit input,
               this is compressed via 4:2:2 subsampling to a 32-bit macropixel,
               48-bits are divided into 10,000 "blocks" of 4096 value "chunks",
               2^48 / 4096 = 2^36 chunks
               2^36 / 10000 = 6871947 -> Size of a block in chunks
               2^36 % 10000 = 6736

               Block 0 also includes the remainder (6736 + 6871947 chunks)*/
            const uint64_t block_size      = 6871947;
            const uint64_t block_remainder = 6736;
            uint64_t       combinations    = UINT64_C(0xFFFF000000000000);
            uint64_t       i;

            if (start_block == 0) {
                printf("Processing remainder...");
                fflush(stdout);

                calc_chunks_yuy2_from_bgrx(&combinations, block_remainder);

                printf("finished\n");
            } else {
                combinations += block_remainder * 4096;
                combinations += (start_block * block_size) * 4096;
            }

            printf("Starting block: %u\nBlocks to process: %u\nProcessing "
                   "chunks...\n",
                   (unsigned)start_block,
                   (unsigned)block_count);
            fflush(stdout);

            for (i = 0; i < block_count && combinations != 0; i++) {
                calc_chunks_yuy2_from_bgrx(&combinations, block_size);

                printf("\rProgress: %.2f%%", 100 * ((i + 1.0) / block_count));
                fflush(stdout);
            }

            break;
        }
        case TEST_YUY2_TO_BGRA: {
            /* Input is one 32-bit macropixel,
               this is uncompressed to 2x 32-bit BGRA pixels,
               32-bits are divided into 10,000 "blocks" of 4096 value "chunks",
               2^32 / 4096 = 2^20 chunks
               2^20 / 10000 = 104 -> Size of a block in chunks
               2^20 % 10000 = 8576

               Block 0 also includes the remainder (8576 + 104 chunks)*/
            const uint32_t block_size      = 104;
            const uint32_t block_remainder = 8576;
            uint32_t       combinations    = 0;
            uint64_t       i;

            if (start_block == 0) {
                printf("Processing remainder...");
                fflush(stdout);

                calc_chunks_yuy2_to_bgra(&combinations, block_remainder);

                printf("finished\n");
            } else {
                combinations += block_remainder * 4096;
                combinations += (start_block * block_size) * 4096;
            }

            printf("Starting block: %u\nBlocks to process: %u\nProcessing "
                   "chunks...\n",
                   (unsigned)start_block,
                   (unsigned)block_count);
            fflush(stdout);

            for (i = 0; i < block_count && combinations != 0; i++) {
                calc_chunks_yuy2_to_bgra(&combinations, block_size);

                printf("\rProgress: %.2f%%", 100 * ((i + 1.0) / block_count));
                fflush(stdout);
            }

            break;
        }
        default: break; /* Unreachable */
    }

    puts("\nComplete!");

    return 0;
}
