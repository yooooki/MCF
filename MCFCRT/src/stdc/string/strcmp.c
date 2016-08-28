// 这个文件是 MCF 的一部分。
// 有关具体授权说明，请参阅 MCFLicense.txt。
// Copyleft 2013 - 2016, LH_Mouse. All wrongs reserved.

#include "../../env/_crtdef.h"
#include <emmintrin.h>

int strcmp(const char *s1, const char *s2){
	register const char *rp1 = s1;
	register const char *rp2 = s2;
	// 如果 rp 是对齐到字的，就不用考虑越界的问题。
	// 因为内存按页分配的，也自然对齐到页，并且也对齐到字。
	// 每个字内的字节的权限必然一致。
	while(((uintptr_t)rp1 & 15) != 0){
		const int32_t rc1 = (uint8_t)*rp1;
		const int32_t rc2 = (uint8_t)*rp2;
		const int32_t d = rc1 - rc2;
		if(d != 0){
			return (d >> 31) | 1;
		}
		if(rc1 == 0){
			return 0;
		}
		++rp1;
		++rp2;
	}
#define SSE2_CMP(load1_, load2_, care_about_page_boundaries_)	\
	{	\
		const __m128i xz = _mm_setzero_si128();	\
		for(;;){	\
			if((care_about_page_boundaries_) && (((uintptr_t)rp2 & 0xFFF) > 0xFF0)){	\
				for(unsigned i = 0; i < 16; ++i){	\
					const int32_t rc1 = (uint8_t)*rp1;	\
					const int32_t rc2 = (uint8_t)*rp2;	\
					const int32_t d = rc1 - rc2;	\
					if(d != 0){	\
						return (d >> 31) | 1;	\
					}	\
					if(rc1 == 0){	\
						return 0;	\
					}	\
					++rp1;	\
					++rp2;	\
				}	\
			}	\
			const __m128i xw1 = (load1_)((const __m128i *)rp1);	\
			const __m128i xw2 = (load2_)((const __m128i *)rp2);	\
			__m128i xt = _mm_cmpeq_epi8(xw1, xw2);	\
			unsigned mask = (uint16_t)~_mm_movemask_epi8(xt);	\
			if(mask != 0){	\
				const int32_t tzne = __builtin_ctz(mask);	\
				const __m128i shift = _mm_set1_epi8(-0x80);	\
				xt = _mm_cmpgt_epi8(_mm_add_epi8(xw1, shift),	\
				                    _mm_add_epi8(xw2, shift));	\
				mask = (unsigned)_mm_movemask_epi8(xt);	\
				if(mask == 0){	\
					return -1;	\
				}	\
				const int32_t tzgt = __builtin_ctz(mask);	\
				const int32_t d = tzne - tzgt;	\
				return (d >> 31) | 1;	\
			}	\
			xt = _mm_cmpeq_epi8(xw1, xz);	\
			mask = (unsigned)_mm_movemask_epi8(xt);	\
			if(mask != 0){	\
				return 0;	\
			}	\
			rp1 += 16;	\
			rp2 += 16;	\
		}	\
	}
	if(((uintptr_t)rp2 & 15) == 0){
		SSE2_CMP(_mm_load_si128, _mm_load_si128, false)
	} else {
		SSE2_CMP(_mm_load_si128, _mm_loadu_si128, true)
	}
}
