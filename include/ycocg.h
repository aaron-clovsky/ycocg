/*******************************************************************************
RGB <-> YCoCg Conversion
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

Compile time options; define these symbols to change this header's behavior
- YCOCG_C89
  Does not include stdint.h, in which case the following types must be defined
  before including this header: int8_t, uint8_t, int16_t, uint32_t, uint64_t
- YCOCG_NO_SIMD
  Disables all SIMD code paths
- YCOCG_NO_SIMD_YUY2_FROM_BGRX
  Disables SIMD code path for ycocg_yuy2_from_bgrx()
- YCOCG_NO_SIMD_YUY2_TO_BGRA
  Disables SIMD code path for ycocg_yuy2_to_bgra()
*******************************************************************************/
#ifndef YCOCG_H
#define YCOCG_H

/*******************************************************************************
Headers
*******************************************************************************/
#include <stddef.h>

#ifndef YCOCG_C89
    #include <stdint.h>
#endif

/*******************************************************************************
Basic macros
*******************************************************************************/
/* inline support */
#ifdef __cplusplus
    #define YCOCG_INLINE static inline
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define YCOCG_INLINE static inline
#elif defined(_MSC_VER)
    #define YCOCG_INLINE static __inline
#elif defined(__GNUC__)
    #define YCOCG_INLINE static __inline__
#else
    #define YCOCG_INLINE static
#endif

/* restrict support */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define YCOCG_RESTRICT restrict
#elif defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER)
    #define YCOCG_RESTRICT __restrict
#else
    #define YCOCG_RESTRICT
#endif

/* thread_local support */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    #define YCOCG_THREAD_LOCAL thread_local
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define YCOCG_THREAD_LOCAL _Thread_local
#elif defined(__cplusplus) && __cplusplus >= 201103L
    #define YCOCG_THREAD_LOCAL thread_local
#elif defined(__GNUC__) || defined(__clang__)
    #define YCOCG_THREAD_LOCAL __thread
#elif defined(_MSC_VER)
    #define YCOCG_THREAD_LOCAL __declspec(thread)
#else
    #define YCOCG_THREAD_LOCAL
#endif

/*******************************************************************************
SIMD support
*******************************************************************************/
/* Define AVX2 implementation on x86/x64 */
#ifndef YCOCG_NO_SIMD
    #if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || \
        defined(_M_IX86)
        #include <immintrin.h>

        #if defined(__GNUC__) || defined(__clang__)
            #define YCOCG_AVX2 __attribute__((target("avx2")))
        #else
            #define YCOCG_AVX2
        #endif

        #define YCOCG_HAS_SIMD
    #endif
#endif

/*******************************************************************************
User basic conversion function definitions
*******************************************************************************/
/*
    Convert RGB to YCoCg
*/
YCOCG_INLINE
void ycocg_from_rgb(uint8_t   r,
                    uint8_t   g,
                    uint8_t   b,
                    uint8_t * y,
                    int8_t *  co,
                    int8_t *  cg)
{
    int16_t ir = (int16_t)r;
    int16_t ib = (int16_t)b;
    int16_t ig = (int16_t)g;

    *y  = (uint8_t)((ir + (ig << 1) + ib + 1) >> 2);
    *co = (int8_t)((ir - ib) >> 1);
    *cg = (int8_t)(((ig << 1) - ir - ib + 1) >> 2);
}

/*
    Convert YCoCg to RGB
*/
YCOCG_INLINE
void ycocg_to_rgb(uint8_t   y,
                  int8_t    co,
                  int8_t    cg,
                  uint8_t * r,
                  uint8_t * g,
                  uint8_t * b)
{
    int16_t iy  = (int16_t)y;
    int16_t ico = (int16_t)co;
    int16_t icg = (int16_t)cg;

    int16_t ir = iy - icg + ico;
    int16_t ig = iy + icg;
    int16_t ib = iy - icg - ico;

    *r = (uint8_t)((ir < 0) ? 0 : (ir > 255) ? 255 : ir);
    *g = (uint8_t)((ig < 0) ? 0 : (ig > 255) ? 255 : ig);
    *b = (uint8_t)((ib < 0) ? 0 : (ib > 255) ? 255 : ib);
}

/*
    Convert two BGRX pixels to one YUY2 (4:2:2) YCoCg macropixel

    Note: Output is stored as Y0,Co,Y1,Cg
*/
YCOCG_INLINE
void ycocg_yuy2_from_bgrx_pixels(const uint32_t * in, uint32_t * out)
{
    uint8_t         y[2];
    int8_t          co[2], cg[2];
    uint8_t *       output = (uint8_t *)out;
    const uint8_t * input  = (const uint8_t *)in;

    ycocg_from_rgb(input[2], input[1], input[0], &y[0], &co[0], &cg[0]);
    ycocg_from_rgb(input[6], input[5], input[4], &y[1], &co[1], &cg[1]);

    output[0] = y[0];
    output[1] = (int8_t)((co[0] + co[1]) >> 1);
    output[2] = y[1];
    output[3] = (int8_t)((cg[0] + cg[1]) >> 1);
}

/*
    Convert one YUY2 (4:2:2) YCoCg macropixel to two BGRA pixels
*/
YCOCG_INLINE
void ycocg_yuy2_to_bgra_pixels(const uint32_t * in, uint32_t * out)
{
    uint8_t         b[2], g[2], r[2];
    const uint8_t * input  = (const uint8_t *)in;
    uint8_t *       output = (uint8_t *)out;

    ycocg_to_rgb(input[0], input[1], input[3], &r[0], &g[0], &b[0]);
    ycocg_to_rgb(input[2], input[1], input[3], &r[1], &g[1], &b[1]);

    output[0] = b[0];
    output[1] = g[0];
    output[2] = r[0];
    output[3] = 255;
    output[4] = b[1];
    output[5] = g[1];
    output[6] = r[1];
    output[7] = 255;
}

/*******************************************************************************
User utility macros
*******************************************************************************/
/* Calculate minimum YUY2 image stride in bytes given width */
#define YCOCG_YUY2_MIN_STRIDE(width) ((((size_t)(width) + 1) >> 1) << 2)

/*******************************************************************************
User types
*******************************************************************************/
enum ycocg_simd
{
    YCOCG_SIMD_INIT, /* Internal use only */
    YCOCG_SIMD_NONE,
    YCOCG_SIMD_AVX2
};

/*******************************************************************************
User utility functions
*******************************************************************************/
/*
    Determine which SIMD code path is active (if any)
*/
YCOCG_INLINE
enum ycocg_simd ycocg_simd_support(void);

/*
    Determine the number of BGRA pixels processed in parallel by simd
*/
YCOCG_INLINE
unsigned int ycocg_simd_bgra_pixel_count(enum ycocg_simd simd);

/*
    Stringify enum ycocg_simd
*/
YCOCG_INLINE
const char * ycocg_simd_string(enum ycocg_simd simd);

/*******************************************************************************
User image conversion functions
*******************************************************************************/
/*
    Convert row of BGRX pixels (in) to YUY2 macropixels (out)

    Note: out must have a minimum size of YCOCG_YUY2_MIN_STRIDE(width)
*/
YCOCG_INLINE
void ycocg_yuy2_from_bgrx_row(const uint32_t * YCOCG_RESTRICT in,
                              uint32_t * YCOCG_RESTRICT       out,
                              uint32_t                        width);

/*
    Convert row of BGRX pixels (in) to YUY2 macropixels (out)
    without using explicit SIMD code

    Note: out must have a minimum size of YCOCG_YUY2_MIN_STRIDE(width)
*/
YCOCG_INLINE
void ycocg_yuy2_from_bgrx_row_scalar(const uint32_t * YCOCG_RESTRICT in,
                                     uint32_t * YCOCG_RESTRICT       out,
                                     uint32_t                        width);

/*
    Convert row of YUY2 macropixels (in) to BGRA pixels (out)
*/
YCOCG_INLINE
void ycocg_yuy2_to_bgra_row(const uint32_t * YCOCG_RESTRICT in,
                            uint32_t * YCOCG_RESTRICT       out,
                            uint32_t                        width);

/*
    Convert row of YUY2 macropixels (in) to BGRA pixels (out)
    without using explicit SIMD code
*/
YCOCG_INLINE
void ycocg_yuy2_to_bgra_row_scalar(const uint32_t * YCOCG_RESTRICT in,
                                   uint32_t * YCOCG_RESTRICT       out,
                                   uint32_t                        width);

/*
    Convert BGRX image (in) to YUY2 (out)

    Note: out must have a minimum size of YCOCG_YUY2_MIN_STRIDE(width) * height
*/
YCOCG_INLINE
void ycocg_yuy2_from_bgrx(uint32_t                        width,
                          uint32_t                        height,
                          size_t                          in_stride,
                          size_t                          out_stride,
                          const uint32_t * YCOCG_RESTRICT in,
                          uint32_t * YCOCG_RESTRICT       out);

/*
    Convert BGRX image (in) to YUY2 (out) without using explicit SIMD code

    Note: out must have a minimum size of YCOCG_YUY2_MIN_STRIDE(width) * height
*/
YCOCG_INLINE
void ycocg_yuy2_from_bgrx_scalar(uint32_t                        width,
                                 uint32_t                        height,
                                 size_t                          in_stride,
                                 size_t                          out_stride,
                                 const uint32_t * YCOCG_RESTRICT in,
                                 uint32_t * YCOCG_RESTRICT       out);

/*
    Convert YUY2 image (in) to BGRA (out)
*/
YCOCG_INLINE
void ycocg_yuy2_to_bgra(uint32_t                        width,
                        uint32_t                        height,
                        size_t                          in_stride,
                        size_t                          out_stride,
                        const uint32_t * YCOCG_RESTRICT in,
                        uint32_t * YCOCG_RESTRICT       out);

/*
    Convert YUY2 image (in) to BGRA (out) without using explicit SIMD code
*/
YCOCG_INLINE
void ycocg_yuy2_to_bgra_scalar(uint32_t                        width,
                               uint32_t                        height,
                               size_t                          in_stride,
                               size_t                          out_stride,
                               const uint32_t * YCOCG_RESTRICT in,
                               uint32_t * YCOCG_RESTRICT       out);

/*******************************************************************************
Internal SIMD utilities
*******************************************************************************/
#ifdef YCOCG_AVX2
YCOCG_INLINE
enum ycocg_simd ycocg_simd_avx2(void)
{
    #if defined(__GNUC__) || defined(__clang__)
    return __builtin_cpu_supports("avx2") ? YCOCG_SIMD_AVX2 : YCOCG_SIMD_NONE;
    #elif defined(_MSC_VER)
    int cpu_info[4];
    __cpuidex(cpu_info, 7, 0);
    return ((cpu_info[1] & (1 << 5)) != 0) ? YCOCG_SIMD_AVX2 : YCOCG_SIMD_NONE;
    #else
    return YCOCG_SIMD_NONE;
    #endif
}
#endif

/*******************************************************************************
Internal SIMD conversion functions
*******************************************************************************/
#ifdef YCOCG_AVX2
/* Convert 16 BGRX pixels (64 Bytes) to 8 YUY2 YCoCg macropixels (32 Bytes)

   RGB to YCoCg Scalar formula:
       Y  = (R + (G << 1) + B + 1) >> 2;
       Co = (R - B) >> 1;
       Cg = ((G << 1) - R - B + 1) >> 2;

   RGB to YCoCg SIMD formula:
       Y  = (B * 1  + G * 2 + R * 1  + A * 0 + 1) >> 2
       Co = (B * -1 + G * 0 + R * 1  + A * 0) >> 1
       Cg = (B * -1 + G * 2 + R * -1 + A * 0 + 1) >> 2 */
YCOCG_AVX2 YCOCG_INLINE void ycocg_yuy2_from_bgrx_avx2(const uint32_t * in,
                                                       uint32_t *       out)
{
    /* Constants */
    const __m256i ymm_one16     = _mm256_set1_epi16(1);
    const __m256i ymm_weight_y  = _mm256_set1_epi32(0x00010201);
    const __m256i ymm_weight_co = _mm256_set1_epi32(0x000100FF);
    const __m256i ymm_weight_cg = _mm256_set1_epi32(0x00FF02FF);

    /* Raw pixels */
    __m256i ymm_bgra1, ymm_bgra2;
    /* Intermedates */
    __m256i ymm_y1, ymm_co1, ymm_cg1;
    __m256i ymm_y2, ymm_co2, ymm_cg2;
    /* Output */
    __m256i ymm_yuy2;

    /* Read pixels */
    ymm_bgra1 = _mm256_loadu_si256((const __m256i *)in);
    ymm_bgra2 = _mm256_loadu_si256((const __m256i *)(&in[8]));

    /* Apply YCoCg weights and sum neighboring channels into signed 16-bits:
        Low  16-bits: (B * weight_b) + (G * weight_g)
        High 16-bits: (R * weight_b) + (A * weight_g) */
    ymm_y1  = _mm256_maddubs_epi16(ymm_bgra1, ymm_weight_y);
    ymm_y2  = _mm256_maddubs_epi16(ymm_bgra2, ymm_weight_y);
    ymm_co1 = _mm256_maddubs_epi16(ymm_bgra1, ymm_weight_co);
    ymm_co2 = _mm256_maddubs_epi16(ymm_bgra2, ymm_weight_co);
    ymm_cg1 = _mm256_maddubs_epi16(ymm_bgra1, ymm_weight_cg);
    ymm_cg2 = _mm256_maddubs_epi16(ymm_bgra2, ymm_weight_cg);

    /* Sum low and high signed 16-bit results into signed 32-bit results */
    ymm_y1  = _mm256_madd_epi16(ymm_y1, ymm_one16);
    ymm_y2  = _mm256_madd_epi16(ymm_y2, ymm_one16);
    ymm_co1 = _mm256_madd_epi16(ymm_co1, ymm_one16);
    ymm_co2 = _mm256_madd_epi16(ymm_co2, ymm_one16);
    ymm_cg1 = _mm256_madd_epi16(ymm_cg1, ymm_one16);
    ymm_cg2 = _mm256_madd_epi16(ymm_cg2, ymm_one16);

    /* Downconvert resulting signed 32-bit sums into signed 16-bit sums */
    ymm_y1  = _mm256_packs_epi32(ymm_y1, ymm_y2);
    ymm_co1 = _mm256_packs_epi32(ymm_co1, ymm_co2);
    ymm_cg1 = _mm256_packs_epi32(ymm_cg1, ymm_cg2);

    /* Finish conversion by adding one to Y and Cg and then doing shifts */
    ymm_y1  = _mm256_add_epi16(ymm_y1, ymm_one16);
    ymm_cg1 = _mm256_add_epi16(ymm_cg1, ymm_one16);
    ymm_y1  = _mm256_srli_epi16(ymm_y1, 2);
    ymm_co1 = _mm256_srai_epi16(ymm_co1, 1);
    ymm_cg1 = _mm256_srai_epi16(ymm_cg1, 2);

    /* Average neighboring chroma samples */
    ymm_co1 = _mm256_madd_epi16(ymm_co1, ymm_one16);
    ymm_co1 = _mm256_srai_epi32(ymm_co1, 1);
    ymm_cg1 = _mm256_madd_epi16(ymm_cg1, ymm_one16);
    ymm_cg1 = _mm256_srai_epi32(ymm_cg1, 1);

    /* Assemble macropixels */
    ymm_co1  = _mm256_slli_epi32(ymm_co1, 8);
    ymm_cg1  = _mm256_slli_epi32(ymm_cg1, 24);
    ymm_yuy2 = _mm256_blend_epi16(ymm_co1, ymm_cg1, 0xAA);
    ymm_yuy2 = _mm256_or_si256(ymm_yuy2, ymm_y1);

    /* Permute output */
    ymm_yuy2 = _mm256_permute4x64_epi64(ymm_yuy2, _MM_SHUFFLE(3, 1, 2, 0));

    /* Write macropixels */
    _mm256_storeu_si256((__m256i *)out, ymm_yuy2);
}

/* Convert 8 YUY2 YCoCg macropixels (32 Bytes) to 16 BGRA pixels (64 Bytes) */
YCOCG_AVX2 YCOCG_INLINE void ycocg_yuy2_to_bgra_avx2(const uint32_t * in,
                                                     uint32_t *       out)
{
    /* Constants */
    const __m256i ymm_alpha = _mm256_set1_epi16(255);
    /* clang-format off */
    const __m256i ymm_shuf_y = _mm256_set_epi8(
        -1, 30, -1, 28, -1, 26, -1, 24,
        -1, 22, -1, 20, -1, 18, -1, 16,
        -1, 14, -1, 12, -1, 10, -1, 8,
        -1, 6,  -1, 4,  -1, 2,  -1, 0);
    const __m256i ymm_shuf_co = _mm256_set_epi8(
        29, -1, 29, -1, 25, -1, 25, -1,
        21, -1, 21, -1, 17, -1, 17, -1,
        13, -1, 13, -1,  9, -1,  9, -1,
        5,  -1, 5,  -1, 1,  -1,  1, -1);
    const __m256i ymm_shuf_cg = _mm256_set_epi8(
        31, -1, 31, -1, 27, -1, 27, -1,
        23, -1, 23, -1, 19, -1, 19, -1,
        15, -1, 15, -1, 11, -1, 11, -1,
        7,  -1, 7,  -1, 3,  -1,  3, -1);
    /* clang-format on */

    /* Raw macropixels */
    __m256i ymm_yuy2;
    /* Intermedates */
    __m256i ymm_tmp;
    __m256i ymm_y, ymm_co, ymm_cg;
    __m256i ymm_r, ymm_g, ymm_b;
    __m256i ymm_bg, ymm_ra;
    __m256i ymm_br, ymm_ga;
    __m256i ymm_lo, ymm_hi;
    /* Output */
    __m256i ymm_bgra1, ymm_bgra2;

    /* Read macropixels */
    ymm_yuy2 = _mm256_loadu_si256((const __m256i *)in);

    /* Extract components and convert values to 16-bit signed */
    ymm_y  = _mm256_shuffle_epi8(ymm_yuy2, ymm_shuf_y);
    ymm_co = _mm256_shuffle_epi8(ymm_yuy2, ymm_shuf_co);
    ymm_cg = _mm256_shuffle_epi8(ymm_yuy2, ymm_shuf_cg);
    ymm_co = _mm256_srai_epi16(ymm_co, 8);
    ymm_cg = _mm256_srai_epi16(ymm_cg, 8);

    /* Convert Y, Co and Cg to R, G and B */
    ymm_tmp = _mm256_subs_epi16(ymm_y, ymm_cg);
    ymm_r   = _mm256_adds_epi16(ymm_tmp, ymm_co);
    ymm_g   = _mm256_adds_epi16(ymm_y, ymm_cg);
    ymm_b   = _mm256_subs_epi16(ymm_tmp, ymm_co);

    /* Combine and convert component pairs to 8-bits */
    ymm_bg = _mm256_packus_epi16(ymm_b, ymm_g);
    ymm_ra = _mm256_packus_epi16(ymm_r, ymm_alpha);

    /* Combine and permute output */
    ymm_br    = _mm256_unpacklo_epi8(ymm_bg, ymm_ra);
    ymm_ga    = _mm256_unpackhi_epi8(ymm_bg, ymm_ra);
    ymm_lo    = _mm256_unpacklo_epi8(ymm_br, ymm_ga);
    ymm_hi    = _mm256_unpackhi_epi8(ymm_br, ymm_ga);
    ymm_bgra1 = _mm256_permute2x128_si256(ymm_lo, ymm_hi, 0x20);
    ymm_bgra2 = _mm256_permute2x128_si256(ymm_lo, ymm_hi, 0x31);

    /* Write pixels */
    _mm256_storeu_si256((__m256i *)out, ymm_bgra1);
    _mm256_storeu_si256((__m256i *)(out + 8), ymm_bgra2);
}
#endif

/*******************************************************************************
Internal image conversion helper functions
*******************************************************************************/
YCOCG_INLINE
void ycocg_yuy2_from_bgrx_row_body(const uint32_t * YCOCG_RESTRICT in,
                                   uint32_t * YCOCG_RESTRICT       out,
                                   uint32_t                        width,
                                   enum ycocg_simd                 simd)
{
    size_t count = (size_t)(width & ~1);
    size_t i     = 0;

    (void)simd; /* suppress unused parameter warnings */
#ifndef YCOCG_NO_SIMD_YUY2_FROM_BGRX
    #ifdef YCOCG_AVX2
    if (simd == YCOCG_SIMD_AVX2) {
        const size_t chunk_count = (size_t)(width & ~15);

        for (; i < chunk_count; i += 16) {
            ycocg_yuy2_from_bgrx_avx2(&in[i], &out[i >> 1]);
        }
    }
    #endif
#endif

    for (; i < count; i += 2) {
        ycocg_yuy2_from_bgrx_pixels(&in[i], &out[i >> 1]);
    }

    if (width & 1) {
        uint8_t         y;
        int8_t          co, cg;
        const uint8_t * input  = (const uint8_t *)&in[i];
        uint8_t *       output = (uint8_t *)&out[i >> 1];

        ycocg_from_rgb(input[2], input[1], input[0], &y, &co, &cg);
        output[0] = y;
        output[1] = co;
        output[2] = y;
        output[3] = cg;
    }
}

YCOCG_INLINE
void ycocg_yuy2_from_bgrx_body(uint32_t                        width,
                               uint32_t                        height,
                               size_t                          in_stride,
                               size_t                          out_stride,
                               const uint32_t * YCOCG_RESTRICT in,
                               uint32_t * YCOCG_RESTRICT       out,
                               enum ycocg_simd                 simd)
{
    const size_t in_stride_dwords  = in_stride >> 2;
    const size_t out_stride_dwords = out_stride >> 2;
    size_t       y                 = 0;
    size_t       i                 = 0;
    size_t       k                 = 0;
    for (; y < height; y++, i += in_stride_dwords, k += out_stride_dwords) {
        ycocg_yuy2_from_bgrx_row_body(&in[i], &out[k], width, simd);
    }
}

YCOCG_INLINE
void ycocg_yuy2_to_bgra_row_body(const uint32_t * YCOCG_RESTRICT in,
                                 uint32_t * YCOCG_RESTRICT       out,
                                 uint32_t                        width,
                                 enum ycocg_simd                 simd)
{
    size_t count = (size_t)(width >> 1);
    size_t i     = 0;

    (void)simd; /* suppress unused parameter warnings */
#ifndef YCOCG_NO_SIMD_YUY2_TO_BGRA
    #ifdef YCOCG_AVX2
    if (simd == YCOCG_SIMD_AVX2) {
        const size_t chunk_count = (size_t)((width >> 1) & ~7);

        for (; i < chunk_count; i += 8) {
            ycocg_yuy2_to_bgra_avx2(&in[i], &out[i << 1]);
        }
    }
    #endif
#endif

    for (; i < count; i++) {
        ycocg_yuy2_to_bgra_pixels(&in[i], &out[i << 1]);
    }

    if (width & 1) {
        uint8_t         b, g, r;
        const uint8_t * input  = (const uint8_t *)&in[i];
        uint8_t *       output = (uint8_t *)&out[i << 1];

        ycocg_to_rgb(input[0], input[1], input[3], &r, &g, &b);

        output[0] = b;
        output[1] = g;
        output[2] = r;
        output[3] = 255;
    }
}

YCOCG_INLINE
void ycocg_yuy2_to_bgra_body(uint32_t                        width,
                             uint32_t                        height,
                             size_t                          in_stride,
                             size_t                          out_stride,
                             const uint32_t * YCOCG_RESTRICT in,
                             uint32_t * YCOCG_RESTRICT       out,
                             enum ycocg_simd                 simd)
{
    const size_t in_stride_dwords  = in_stride >> 2;
    const size_t out_stride_dwords = out_stride >> 2;
    size_t       y                 = 0;
    size_t       i                 = 0;
    size_t       k                 = 0;
    for (; y < height; y++, i += in_stride_dwords, k += out_stride_dwords) {
        ycocg_yuy2_to_bgra_row_body(&in[i], &out[k], width, simd);
    }
}

/*******************************************************************************
User image conversion function definitions
*******************************************************************************/
YCOCG_INLINE
void ycocg_yuy2_from_bgrx_row(const uint32_t * YCOCG_RESTRICT in,
                              uint32_t * YCOCG_RESTRICT       out,
                              uint32_t                        width)
{
    ycocg_yuy2_from_bgrx_row_body(in, out, width, ycocg_simd_support());
}

YCOCG_INLINE
void ycocg_yuy2_from_bgrx_row_scalar(const uint32_t * YCOCG_RESTRICT in,
                                     uint32_t * YCOCG_RESTRICT       out,
                                     uint32_t                        width)
{
    ycocg_yuy2_from_bgrx_row_body(in, out, width, YCOCG_SIMD_NONE);
}

YCOCG_INLINE
void ycocg_yuy2_from_bgrx(uint32_t                        width,
                          uint32_t                        height,
                          size_t                          in_stride,
                          size_t                          out_stride,
                          const uint32_t * YCOCG_RESTRICT in,
                          uint32_t * YCOCG_RESTRICT       out)
{
    ycocg_yuy2_from_bgrx_body(width,
                              height,
                              in_stride,
                              out_stride,
                              in,
                              out,
                              ycocg_simd_support());
}

YCOCG_INLINE
void ycocg_yuy2_from_bgrx_scalar(uint32_t                        width,
                                 uint32_t                        height,
                                 size_t                          in_stride,
                                 size_t                          out_stride,
                                 const uint32_t * YCOCG_RESTRICT in,
                                 uint32_t * YCOCG_RESTRICT       out)
{
    ycocg_yuy2_from_bgrx_body(width,
                              height,
                              in_stride,
                              out_stride,
                              in,
                              out,
                              YCOCG_SIMD_NONE);
}

YCOCG_INLINE
void ycocg_yuy2_to_bgra_row(const uint32_t * YCOCG_RESTRICT in,
                            uint32_t * YCOCG_RESTRICT       out,
                            uint32_t                        width)
{
    ycocg_yuy2_to_bgra_row_body(in, out, width, ycocg_simd_support());
}

YCOCG_INLINE
void ycocg_yuy2_to_bgra_row_scalar(const uint32_t * YCOCG_RESTRICT in,
                                   uint32_t * YCOCG_RESTRICT       out,
                                   uint32_t                        width)
{
    ycocg_yuy2_to_bgra_row_body(in, out, width, YCOCG_SIMD_NONE);
}

YCOCG_INLINE
void ycocg_yuy2_to_bgra(uint32_t                        width,
                        uint32_t                        height,
                        size_t                          in_stride,
                        size_t                          out_stride,
                        const uint32_t * YCOCG_RESTRICT in,
                        uint32_t * YCOCG_RESTRICT       out)
{
    ycocg_yuy2_to_bgra_body(width,
                            height,
                            in_stride,
                            out_stride,
                            in,
                            out,
                            ycocg_simd_support());
}

YCOCG_INLINE
void ycocg_yuy2_to_bgra_scalar(uint32_t                        width,
                               uint32_t                        height,
                               size_t                          in_stride,
                               size_t                          out_stride,
                               const uint32_t * YCOCG_RESTRICT in,
                               uint32_t * YCOCG_RESTRICT       out)
{
    ycocg_yuy2_to_bgra_body(width,
                            height,
                            in_stride,
                            out_stride,
                            in,
                            out,
                            YCOCG_SIMD_NONE);
}

/*******************************************************************************
User utility function definitions
*******************************************************************************/
YCOCG_INLINE
enum ycocg_simd ycocg_simd_support(void)
{
#ifdef YCOCG_HAS_SIMD
    static enum ycocg_simd simd = YCOCG_SIMD_INIT;

    if (simd == YCOCG_SIMD_INIT) {
    #ifdef YCOCG_AVX2
        simd = ycocg_simd_avx2();
    #else
        simd = YCOCG_SIMD_NONE;
    #endif
    }

    return simd;
#else
    return YCOCG_SIMD_NONE;
#endif
}

YCOCG_INLINE
unsigned int ycocg_simd_bgra_pixel_count(enum ycocg_simd simd)
{
    switch (simd) {
        case YCOCG_SIMD_AVX2: return 16;
        default:              return 1;
    }
}

YCOCG_INLINE
const char * ycocg_simd_string(enum ycocg_simd simd)
{
    switch (simd) {
        case YCOCG_SIMD_NONE: return "None";
        case YCOCG_SIMD_AVX2: return "AVX2";
        default:              return "Error";
    }
}

#endif
