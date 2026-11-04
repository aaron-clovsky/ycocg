/*******************************************************************************
RGB <-> YCoCg conversion
Test SIMD vs Scalar conversion with random data
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
#include <time.h>
#include <string.h>
#include <stdio.h>

/* Required for get_time() */
#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(__unix__) || defined(__APPLE__) || defined(__posix__) || \
    (defined(_POSIX_VERSION) && _POSIX_VERSION >= 199309L)
    #ifndef _POSIX_C_SOURCE
        #define _POSIX_C_SOURCE 199309L
    #endif
    #include <unistd.h>
#endif

/* Required for _ReadWriteBarrier() */
#if defined(_MSC_VER)
    #include <intrin.h>
#endif

/*******************************************************************************
Compile time constants
*******************************************************************************/
#ifndef MAX_WIDTH
    #define MAX_WIDTH 1280
#endif
#ifndef MAX_HEIGHT
    #define MAX_HEIGHT 720
#endif
#ifndef DEFAULT_REPS
    #define DEFAULT_REPS 1000
#endif

/*******************************************************************************
Macros
*******************************************************************************/
/* Buffer alignment */
#if defined(_MSC_VER)
    #define ALIGNAS(n) __declspec(align(n))
#elif defined(__GNUC__) || defined(__clang__)
    #define ALIGNAS(n) __attribute__((aligned(n)))
#else
    #define ALIGNAS(n)
#endif

/* Try to prevent over-optimization */
#if defined(_MSC_VER)
    #define MEMORY_BARRIER() _ReadWriteBarrier()
#elif defined(__GNUC__) || defined(__clang__)
    #define MEMORY_BARRIER() __asm__ volatile("" : : : "memory")
#else
    #define MEMORY_BARRIER()
#endif

/*******************************************************************************
Globals
*******************************************************************************/
ALIGNAS(64)
uint32_t test_image[MAX_WIDTH * MAX_HEIGHT * sizeof(uint32_t)];
ALIGNAS(64)
uint32_t test_output[MAX_WIDTH * MAX_HEIGHT * sizeof(uint32_t)];
ALIGNAS(64)
uint32_t test_output_scalar[MAX_WIDTH * MAX_HEIGHT * sizeof(uint32_t)];

/*******************************************************************************
Utilities
*******************************************************************************/
static double get_time(void)
{
#ifdef _WIN32
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart / (double)freq.QuadPart;
#elif defined(CLOCK_MONOTONIC)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
#else
    return (double)clock() / CLOCKS_PER_SEC;
#endif
}

static void fill_rand(uint32_t * buffer, size_t stride, uint32_t height)
{
    size_t i, k;

    srand((unsigned)time(NULL));

    for (i = 0; i < height; i++) {
        for (k = 0; k < stride / sizeof(uint32_t); k++) {
            uint8_t * channels = (uint8_t *)&buffer[(i * stride) + k];
            channels[0]        = (uint8_t)rand();
            channels[1]        = (uint8_t)rand();
            channels[2]        = (uint8_t)rand();
            channels[3]        = (uint8_t)rand();
        }
    }
}

static void print_hex(const uint8_t * data, size_t size)
{
    size_t i;
    for (i = 0; i < size; i++) {
        printf("%02X ", data[i]);
        if (!((i + 1) & 15)) printf("\n");
    }

    printf("\n");
}

static void compare_and_debug(size_t cmp_size)
{
    if (memcmp(test_output, test_output_scalar, cmp_size) == 0) {
        puts(" - SIMD and Scalar outputs match");
    } else {
        size_t k, off, len;

        for (k = 0; k < cmp_size; k++) {
            if (test_output[k] != test_output_scalar[k]) break;
        }

        if (k == cmp_size) {
            puts(" - BUG: compare_and_debug(): No miscompare offset found");
            exit(1);
        }

        off = (k < 4) ? 0 : ((k & ~3) - 4) * sizeof(uint32_t);
        len = ((cmp_size - off) > 256) ? 256 : (cmp_size - off);

        printf(" - SIMD and Scalar outputs do not match (dword @ offset "
               "%u)\n"
               "SIMD (@ offset %u):\n",
               (unsigned)(k * sizeof(uint32_t)),
               (unsigned)off);
        print_hex((uint8_t *)&test_output[off / sizeof(uint32_t)], len);
        printf("Scalar (@ offset %u):\n", (unsigned)off);
        print_hex((uint8_t *)&test_output_scalar[off / sizeof(uint32_t)], len);

        exit(1);
    }
}

/*******************************************************************************
main()
*******************************************************************************/
int main(int argc, char ** argv)
{
    uint32_t width  = MAX_WIDTH;
    uint32_t height = MAX_HEIGHT;
    int      reps   = DEFAULT_REPS;
    double   timer_simd, timer_scalar;
    int      i;

    /*
        Args
    */
    if (argc == 4) {
        int in_width, in_height, in_reps;
        in_width = atoi(argv[1]);
        if (in_width <= 0 || in_width > MAX_WIDTH) {
            puts("Bad width");
            exit(2);
        }
        width     = (uint32_t)in_width;
        in_height = atoi(argv[2]);
        if (in_height <= 0 || in_height > MAX_HEIGHT) {
            puts("Bad height");
            exit(2);
        }
        height  = (uint32_t)in_height;
        in_reps = atoi(argv[3]);
        if (in_reps <= 0) {
            puts("Bad reps");
            exit(2);
        }
        reps = in_reps;
    } else if (argc != 1) {
        printf("Usage: %s [width height repetitions]\n", argv[0]);
        exit(2);
    }

    /*
        Info
    */
    printf("SIMD Support: %s\n", ycocg_simd_string(ycocg_simd_support()));

    /*
        Test 1
    */
    printf("Running Test 1: BGRX -> YUY2 (%ux%u)\n",
           (unsigned)width,
           (unsigned)height);

    /* Setup */
    memset(test_image, 0, sizeof(test_image));
    fill_rand(test_image, width * sizeof(uint32_t), height);
    memset(test_output, 0, sizeof(test_output));
    fill_rand(test_output, YCOCG_YUY2_MIN_STRIDE(width), height);
    memset(test_output_scalar, 0, sizeof(test_output_scalar));
    fill_rand(test_output_scalar, YCOCG_YUY2_MIN_STRIDE(width), height);

    /* Run */
    timer_simd = get_time();
    for (i = 0; i < reps; i++) {
        ycocg_yuy2_from_bgrx(width,
                             height,
                             width * sizeof(uint32_t),
                             YCOCG_YUY2_MIN_STRIDE(width),
                             test_image,
                             test_output);
        MEMORY_BARRIER();
    }
    timer_simd = get_time() - timer_simd;

    timer_scalar = get_time();
    for (i = 0; i < reps; i++) {
        ycocg_yuy2_from_bgrx_scalar(width,
                                    height,
                                    width * sizeof(uint32_t),
                                    YCOCG_YUY2_MIN_STRIDE(width),
                                    test_image,
                                    test_output_scalar);
        MEMORY_BARRIER();
    }
    timer_scalar = get_time() - timer_scalar;

    /* Check */
    compare_and_debug(YCOCG_YUY2_MIN_STRIDE(width) * height);

    /* Report */
    printf(" - ycocg_yuy2_from_bgrx(): %.3fms\n",
           (timer_simd * 1000.) / (double)reps);
    printf(" - ycocg_yuy2_from_bgrx_scalar(): %.3fms\n",
           (timer_scalar * 1000.) / (double)reps);
    printf(" - ratio: %.2fx\n", timer_scalar / timer_simd);

    /*
        Test 2
    */
    printf("Running Test 2: YUY2 -> BGRA (%ux%u)\n",
           (unsigned)width,
           (unsigned)height);

    /* Setup */
    memset(test_image, 0, sizeof(test_image));
    fill_rand(test_image, YCOCG_YUY2_MIN_STRIDE(width), height);
    memset(test_output, 0, sizeof(test_output));
    fill_rand(test_output, width * sizeof(uint32_t), height);
    memset(test_output_scalar, 0, sizeof(test_output_scalar));
    fill_rand(test_output_scalar, width * sizeof(uint32_t), height);

    /* Run */
    timer_simd = get_time();
    for (i = 0; i < reps; i++) {
        ycocg_yuy2_to_bgra(width,
                           height,
                           YCOCG_YUY2_MIN_STRIDE(width),
                           width * sizeof(uint32_t),
                           test_image,
                           test_output);
        MEMORY_BARRIER();
    }
    timer_simd = get_time() - timer_simd;

    timer_scalar = get_time();
    for (i = 0; i < reps; i++) {
        ycocg_yuy2_to_bgra_scalar(width,
                                  height,
                                  YCOCG_YUY2_MIN_STRIDE(width),
                                  width * sizeof(uint32_t),
                                  test_image,
                                  test_output_scalar);
        MEMORY_BARRIER();
    }
    timer_scalar = get_time() - timer_scalar;

    /* Check */
    compare_and_debug(width * height * sizeof(uint32_t));

    /* Report */
    printf(" - ycocg_yuy2_to_bgra(): %.3fms\n",
           (timer_simd * 1000.0) / (double)reps);
    printf(" - ycocg_yuy2_to_bgra_scalar(): %.3fms\n",
           (timer_scalar * 1000.0) / (double)reps);
    printf(" - ratio: %.2fx\n", timer_scalar / timer_simd);

    return 0;
}
