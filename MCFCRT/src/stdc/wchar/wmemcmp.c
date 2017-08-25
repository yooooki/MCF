// 这个文件是 MCF 的一部分。
// 有关具体授权说明，请参阅 MCFLicense.txt。
// Copyleft 2013 - 2017, LH_Mouse. All wrongs reserved.

#include "../../env/_crtdef.h"
#include "../../env/expect.h"
#include "../string/_sse2.h"
#include <tmmintrin.h>

#undef wmemcmp

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

static inline uintptr_t wswap_ptr(uintptr_t w){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return _Generic(w,
		uint32_t: __builtin_bswap32(((w << 8) & 0xFF00FF00        ) | ((w & 0xFF00FF00        ) >> 8)),
		uint64_t: __builtin_bswap64(((w << 8) & 0xFF00FF00FF00FF00) | ((w & 0xFF00FF00FF00FF00) >> 8)));
#else
	return w;
#endif
}

static int wmemcmp_small(const wchar_t *s1, const wchar_t *s2, size_t n){
	const wchar_t *rp1 = s1;
	const wchar_t *rp2 = s2;
	size_t rem;
	rem = n / (sizeof(uintptr_t) / sizeof(wchar_t));
	if(_MCFCRT_EXPECT_NOT(rem != 0)){
		switch(rem % 8){
			uintptr_t w, c;
		diff_wc:
			w = wswap_ptr(w);
			c = wswap_ptr(c);
			return (w < c) ? -1 : 1;
			do {
#define STEP	\
				__builtin_memcpy(&w, rp1, sizeof(w));	\
				__builtin_memcpy(&c, rp2, sizeof(c));	\
				rp1 += sizeof(w) / sizeof(wchar_t);	\
				rp2 += sizeof(c) / sizeof(wchar_t);	\
				--rem;	\
				if(_MCFCRT_EXPECT_NOT(w != c)){	\
					goto diff_wc;	\
				}
//=============================================================================
		default: STEP
		case 7:  STEP
		case 6:  STEP
		case 5:  STEP
		case 4:  STEP
		case 3:  STEP
		case 2:  STEP
		case 1:  STEP
//=============================================================================
#undef STEP
			} while(_MCFCRT_EXPECT(rem != 0));
		}
	}
	rem = n % (sizeof(uintptr_t) / sizeof(wchar_t));
	while(_MCFCRT_EXPECT(rem != 0)){
		if(*rp1 != *rp2){
			return (*rp1 < *rp2) ? -1 : 1;
		}
		++rp1;
		++rp2;
		--rem;
	}
	return 0;
}

#pragma GCC diagnostic pop

__attribute__((__noinline__))
static int wmemcmp_large(const wchar_t *s1, const wchar_t *s2, size_t n){
	// 如果 arp1 和 arp2 是对齐到字的，就不用考虑越界的问题。
	// 因为内存按页分配的，也自然对齐到页，并且也对齐到字。
	// 每个字内的字节的权限必然一致。
	register const wchar_t *arp1 __asm__("si") = (const wchar_t *)((uintptr_t)s1 & (uintptr_t)-64);
	register const wchar_t *arp2 __asm__("di") = (const wchar_t *)((uintptr_t)s2 & (uintptr_t)-64);
	const unsigned align = (unsigned)(32 - ((const wchar_t *)s1 - arp1) + ((const wchar_t *)s2 - arp2));

	__m128i s2v[12];
	bool s2z;
	__m128i xw[4], xc[4];
	uint32_t mask;
	ptrdiff_t dist;
//=============================================================================
#define BEGIN(offset_, align_)	\
	arp1 = __MCFCRT_xmmload_4(xw, arp1, _mm_load_si128);	\
	if(_MCFCRT_EXPECT(!s2z)){	\
		arp2 = __MCFCRT_xmmload_4(s2v + ((offset_) + 8) % 12, arp2, _mm_load_si128);	\
		s2z = arp2 >= (const wchar_t *)s2 + n;	\
	}	\
	for(unsigned i = 0; i < 4; ++i){	\
		xc[i] = _mm_alignr_epi8(s2v[((offset_) + (align_) / 8 + i + 1) % 12], s2v[((offset_) + (align_) / 8 + i) % 12], (align_) % 8 * 2);	\
	}	\
	mask = ~__MCFCRT_xmmcmp_44w(xw, xc);
#define END	\
	dist = arp1 - ((const wchar_t *)s1 + n);	\
	if(_MCFCRT_EXPECT_NOT(dist >= 0)){	\
		goto end_trunc;	\
	}	\
	dist = 0;	\
	if(_MCFCRT_EXPECT_NOT(mask != 0)){	\
		goto end;	\
	}
//=============================================================================
	if(_MCFCRT_EXPECT_NOT(n == 0)){
		goto end_equal;
	}
	__MCFCRT_xmmsetz_4(s2v);
	arp2 = __MCFCRT_xmmload_4(s2v + 4, arp2, _mm_load_si128);
	s2z = arp2 >= (const wchar_t *)s2 + n;
	switch(align){
#define CASE(k_)	\
	case (k_):	\
		BEGIN(0, k_)	\
		dist = (const wchar_t *)s1 - (arp1 - 32);	\
		mask &= (uint32_t)-1 << dist;	\
		for(;;){	\
			END	\
			BEGIN(4, k_)	\
			END	\
			BEGIN(8, k_)	\
			END	\
			BEGIN(0, k_)	\
		}
	// 两个位于区间 [0,31] 的数相减，结果位于区间 ［-31,31]；加上 32，结果位于区间 [1,63]。
	           CASE(001)  CASE(002)  CASE(003)  CASE(004)  CASE(005)  CASE(006)  CASE(007)
	CASE(010)  CASE(011)  CASE(012)  CASE(013)  CASE(014)  CASE(015)  CASE(016)  CASE(017)
	CASE(020)  CASE(021)  CASE(022)  CASE(023)  CASE(024)  CASE(025)  CASE(026)  CASE(027)
	CASE(030)  CASE(031)  CASE(032)  CASE(033)  CASE(034)  CASE(035)  CASE(036)  CASE(037)
	CASE(040)  CASE(041)  CASE(042)  CASE(043)  CASE(044)  CASE(045)  CASE(046)  CASE(047)
	CASE(050)  CASE(051)  CASE(052)  CASE(053)  CASE(054)  CASE(055)  CASE(056)  CASE(057)
	CASE(060)  CASE(061)  CASE(062)  CASE(063)  CASE(064)  CASE(065)  CASE(066)  CASE(067)
	CASE(070)  CASE(071)  CASE(072)  CASE(073)  CASE(074)  CASE(075)  CASE(076)  CASE(077)
	default:
		__builtin_trap();
	}
end_trunc:
	mask |= ~((uint32_t)-1 >> dist);
end:
	__asm__ volatile ("" : "+c"(dist));
	if((mask << dist) != 0){
		arp1 = arp1 - 32 + (unsigned)__builtin_ctzl(mask);
		arp2 = arp1 - (const wchar_t *)s1 + (const wchar_t *)s2;
		return (*arp1 < *arp2) ? -1 : 1;
	}
end_equal:
	return 0;
}

int wmemcmp(const wchar_t *s1, const wchar_t *s2, size_t n){
	if(_MCFCRT_EXPECT(n <= 256)){
		return wmemcmp_small(s1, s2, n);
	} else {
		return wmemcmp_large(s1, s2, n);
	}
}
