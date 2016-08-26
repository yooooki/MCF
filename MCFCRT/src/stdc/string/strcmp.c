// 这个文件是 MCF 的一部分。
// 有关具体授权说明，请参阅 MCFLicense.txt。
// Copyleft 2013 - 2016, LH_Mouse. All wrongs reserved.

#include "../../env/_crtdef.h"

int strcmp(const char *s1, const char *s2){
	register const char *rp1 = s1, *rp2 = s2;
	for(;;){
		const int32_t c1 = (uint8_t)*rp1;
		const int32_t c2 = (uint8_t)*rp2;
		const int32_t d = c1 - c2;
		if(d != 0){
			return (d >> 31) | 1;
		}
		if(c1 == 0){
			return 0;
		}
		++rp1;
		++rp2;
	}
}
