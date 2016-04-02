// 这个文件是 MCF 的一部分。
// 有关具体授权说明，请参阅 MCFLicense.txt。
// Copyleft 2013 - 2016, LH_Mouse. All wrongs reserved.

#ifndef __MCFCRT_ENV_THREAD_H_
#define __MCFCRT_ENV_THREAD_H_

#include "_crtdef.h"

__MCFCRT_EXTERN_C_BEGIN

extern bool __MCFCRT_ThreadEnvInit(void) _MCFCRT_NOEXCEPT;
extern void __MCFCRT_ThreadEnvUninit(void) _MCFCRT_NOEXCEPT;

extern void __MCFCRT_TlsThreadCleanup() _MCFCRT_NOEXCEPT;

typedef unsigned long (*_MCFCRT_TlsConstructor)(void *__pStorage); // 成功应当返回 0，否则返回值当成错误码。
typedef void (*_MCFCRT_TlsDestructor)(void *__pStorage);

// 失败返回 nullptr。
extern void *_MCFCRT_TlsAllocKey(_MCFCRT_STD size_t __uSize, _MCFCRT_TlsConstructor __pfnConstructor, _MCFCRT_TlsDestructor __pfnDestructor) _MCFCRT_NOEXCEPT;
extern bool _MCFCRT_TlsFreeKey(void *__pTlsKey) _MCFCRT_NOEXCEPT;

extern _MCFCRT_STD size_t _MCFCRT_TlsGetSize(void *__pTlsKey) _MCFCRT_NOEXCEPT;
extern _MCFCRT_TlsConstructor _MCFCRT_TlsGetConstructor(void *__pTlsKey) _MCFCRT_NOEXCEPT;
extern _MCFCRT_TlsDestructor _MCFCRT_TlsGetDestructor(void *__pTlsKey) _MCFCRT_NOEXCEPT;

// 下面两个函数都不触发回调。__ppStorage 返回一个指向当前 TLS 对象的指针。
// 如果未设定过任何值，_MCFCRT_TlsGet() 返回一个空指针，_MCFCRT_TlsRequire() 创建一个对象并返回指向它的指针。
extern bool _MCFCRT_TlsGet(void *__pTlsKey, void **restrict __ppStorage) _MCFCRT_NOEXCEPT;
extern bool _MCFCRT_TlsRequire(void *__pTlsKey, void **restrict __ppStorage) _MCFCRT_NOEXCEPT;

typedef void (*_MCFCRT_AtThreadExitCallback)(_MCFCRT_STD intptr_t __nContext);

extern int _MCFCRT_AtThreadExit(_MCFCRT_AtThreadExitCallback __pfnProc, _MCFCRT_STD intptr_t __nContext) _MCFCRT_NOEXCEPT;

typedef unsigned long (*__attribute__((__stdcall__)) _MCFCRT_NativeThreadProc)(void *__pParam);
typedef unsigned (*_MCFCRT_ThreadProc)(_MCFCRT_STD intptr_t __nParam);

extern void *_MCFCRT_CreateNativeThread(_MCFCRT_NativeThreadProc __pfnThreadProc, void *__pParam, bool __bSuspended, _MCFCRT_STD uintptr_t *restrict __puThreadId) _MCFCRT_NOEXCEPT;
extern void *_MCFCRT_CreateThread(_MCFCRT_ThreadProc __pfnThreadProc, _MCFCRT_STD intptr_t __nParam, bool __bSuspended, _MCFCRT_STD uintptr_t *restrict __puThreadId) _MCFCRT_NOEXCEPT;
extern void _MCFCRT_CloseThread(void *__hThread) _MCFCRT_NOEXCEPT;

extern _MCFCRT_STD uintptr_t _MCFCRT_GetCurrentThreadId(void) _MCFCRT_NOEXCEPT;

extern void _MCFCRT_Sleep(_MCFCRT_STD uint64_t __u64UntilFastMonoClock) _MCFCRT_NOEXCEPT;
// 被 APC 打断返回 true，超时返回 false。
extern bool _MCFCRT_AlertableSleep(_MCFCRT_STD uint64_t __u64UntilFastMonoClock) _MCFCRT_NOEXCEPT;
extern void _MCFCRT_AlertableSleepForever(void) _MCFCRT_NOEXCEPT;
extern void _MCFCRT_YieldThread(void) _MCFCRT_NOEXCEPT;

extern long _MCFCRT_SuspendThread(void *__hThread) _MCFCRT_NOEXCEPT;
extern long _MCFCRT_ResumeThread(void *__hThread) _MCFCRT_NOEXCEPT;

// 线程结束返回 true，超时返回 false。
extern bool _MCFCRT_WaitForThread(void *__hThread, _MCFCRT_STD uint64_t __u64UntilFastMonoClock) _MCFCRT_NOEXCEPT;
extern void _MCFCRT_WaitForThreadForever(void *__hThread) _MCFCRT_NOEXCEPT;

__MCFCRT_EXTERN_C_END

#endif
