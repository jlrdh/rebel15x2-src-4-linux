

#ifndef SIMD_H
#define SIMD_H

#include <stdint.h>

#define USE_SSE false
#define USE_AVX2 true


#if USE_SSE
#include <emmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>
#endif
#if USE_AVX2
#include <immintrin.h>
#endif

#if USE_AVX512
typedef __m512i vec_t;
typedef __m256i psqt_vec_t;
#define vec_load(a) _mm512_load_si512(a)
#define vec_store(a,b) _mm512_store_si512(a,b)
#define vec_add_16(a,b) _mm512_add_epi16(a,b)
#define vec_sub_16(a,b) _mm512_sub_epi16(a,b)
#define vec_load_psqt(a) _mm256_load_si256(a)
#define vec_store_psqt(a,b) _mm256_store_si256(a,b)
#define vec_add_psqt_32(a,b) _mm256_add_epi32(a,b)
#define vec_sub_psqt_32(a,b) _mm256_sub_epi32(a,b)
#define vec_zero_psqt() _mm256_setzero_si256()
#define NumRegistersSIMD 32

#elif USE_AVX2
typedef __m256i vec_t;
typedef __m256i psqt_vec_t;
#define vec_load(a) _mm256_load_si256(a)
#define vec_store(a,b) _mm256_store_si256(a,b)
#define vec_add_16(a,b) _mm256_add_epi16(a,b)
#define vec_sub_16(a,b) _mm256_sub_epi16(a,b)
#define vec_load_psqt(a) _mm256_load_si256(a)
#define vec_store_psqt(a,b) _mm256_store_si256(a,b)
#define vec_add_psqt_32(a,b) _mm256_add_epi32(a,b)
#define vec_sub_psqt_32(a,b) _mm256_sub_epi32(a,b)
#define vec_zero_psqt() _mm256_setzero_si256()
#define NumRegistersSIMD 16


#elif USE_SSE
typedef __m128i vec_t;
typedef __m128i psqt_vec_t;
#define vec_load(a) (*(a))
#define vec_store(a,b) *(a)=(b)
#define vec_add_16(a,b) _mm_add_epi16(a,b)
#define vec_sub_16(a,b) _mm_sub_epi16(a,b)
#define vec_load_psqt(a) (*(a))
#define vec_store_psqt(a,b) *(a)=(b)
#define vec_add_psqt_32(a,b) _mm_add_epi32(a,b)
#define vec_sub_psqt_32(a,b) _mm_sub_epi32(a,b)
#define vec_zero_psqt() _mm_setzero_si128()
#define NumRegistersSIMD 16
#endif


void simd_linear_forward(uint8_t* input, int32_t* output, int ninputs,
    int noutputs, int32_t* biases, int8_t* weights);


void simd_scale_and_clamp(int32_t* input, uint8_t* output, int shift,
    int nvalues);

void simd_clamp(int16_t* input, uint8_t* output, int nvalues);

void simd_copy_i16(int16_t* from, int16_t* to, int nvalues);
void simd_add_i16(int16_t* from, int16_t* to, int nvalues);
void simd_sub_i16(int16_t* from, int16_t* to, int nvalues);

//void simd_zero_i32(int32* psqtacc, int n_values);
//void simd_copy_i32(int32_t* from, int32_t* to, int nvalues);
//void simd_sub_i32(int32* from, int32* to, int n_values);
//void simd_add_i32(int32* from, int32* to, int n_values);
#endif

