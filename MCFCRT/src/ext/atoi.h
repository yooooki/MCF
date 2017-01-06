// 这个文件是 MCF 的一部分。
// 有关具体授权说明，请参阅 MCFLicense.txt。
// Copyleft 2013 - 2017, LH_Mouse. All wrongs reserved.

#ifndef __MCFCRT_EXT_ATOI_H_
#define __MCFCRT_EXT_ATOI_H_

#include "../env/_crtdef.h"

_MCFCRT_EXTERN_C_BEGIN

extern char *_MCFCRT_atoi_d(_MCFCRT_STD  intptr_t *__pnValue, const char *__pchBuffer) _MCFCRT_NOEXCEPT;
extern char *_MCFCRT_atoi_u(_MCFCRT_STD uintptr_t *__puValue, const char *__pchBuffer) _MCFCRT_NOEXCEPT;
extern char *_MCFCRT_atoi_x(_MCFCRT_STD uintptr_t *__puValue, const char *__pchBuffer) _MCFCRT_NOEXCEPT;
extern char *_MCFCRT_atoi_X(_MCFCRT_STD uintptr_t *__puValue, const char *__pchBuffer) _MCFCRT_NOEXCEPT;
extern char *_MCFCRT_atoi0d(_MCFCRT_STD  intptr_t *__pnValue, const char *__pchBuffer, unsigned __uMaxDigits) _MCFCRT_NOEXCEPT;
extern char *_MCFCRT_atoi0u(_MCFCRT_STD uintptr_t *__puValue, const char *__pchBuffer, unsigned __uMaxDigits) _MCFCRT_NOEXCEPT;
extern char *_MCFCRT_atoi0x(_MCFCRT_STD uintptr_t *__puValue, const char *__pchBuffer, unsigned __uMaxDigits) _MCFCRT_NOEXCEPT;
extern char *_MCFCRT_atoi0X(_MCFCRT_STD uintptr_t *__puValue, const char *__pchBuffer, unsigned __uMaxDigits) _MCFCRT_NOEXCEPT;

_MCFCRT_EXTERN_C_END

#endif