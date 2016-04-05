// 这个文件是 MCF 的一部分。
// 有关具体授权说明，请参阅 MCFLicense.txt。
// Copyleft 2013 - 2016, LH_Mouse. All wrongs reserved.

#include "mutex.h"
#include "_nt_timeout.h"
#include "../ext/assert.h"
#include "../ext/expect.h"
#include <limits.h>
#include <winternl.h>
#include <ntstatus.h>

extern __attribute__((__dllimport__, __stdcall__))
NTSTATUS NtWaitForKeyedEvent(HANDLE hKeyedEvent, void *pKey, BOOLEAN bAlertable, const LARGE_INTEGER *pliTimeout);
extern __attribute__((__dllimport__, __stdcall__))
NTSTATUS NtReleaseKeyedEvent(HANDLE hKeyedEvent, void *pKey, BOOLEAN bAlertable, const LARGE_INTEGER *pliTimeout);

#define FLAG_LOCKED             ((uintptr_t)0x0001)
#define FLAG_SPIN_TOKEN         ((uintptr_t)0x0002)
#define FLAGS_RESERVED          ((size_t)4)

#define GET_THREAD_COUNT(v_)    ((size_t)(uintptr_t)(v_) >> FLAGS_RESERVED)
#define MAKE_THREAD_COUNT(v_)   ((uintptr_t)(size_t)(v_) << FLAGS_RESERVED)

static inline bool ReallyWaitForMutex(_MCFCRT_Mutex *pMutex, size_t uMaxSpinCount, bool bMayTimeOut, uint64_t u64UntilFastMonoClock){
	{
		uintptr_t uOld, uNew;
		uOld = __atomic_load_n(pMutex, __ATOMIC_CONSUME);
		if(_MCFCRT_EXPECT(!(uOld & FLAG_LOCKED))){
			uNew = uOld | FLAG_LOCKED;
			if(_MCFCRT_EXPECT(__atomic_compare_exchange_n(pMutex, &uOld, uNew, false, __ATOMIC_ACQ_REL, __ATOMIC_CONSUME))){
				return true;
			}
		}
	}
	if(bMayTimeOut && _MCFCRT_EXPECT(u64UntilFastMonoClock == 0)){
		return false;
	}

	for(;;){
		if(uMaxSpinCount != 0){
			bool bTaken, bCanSpin;
			{
				uintptr_t uOld, uNew;
				uOld = __atomic_load_n(pMutex, __ATOMIC_CONSUME);
				do {
					bTaken = !(uOld & FLAG_LOCKED);
					if(!bTaken){
						bCanSpin = !(uOld & FLAG_SPIN_TOKEN);
						if(!bCanSpin){
							break;
						}
						uNew = uOld + FLAG_SPIN_TOKEN; // uOld | FLAG_SPIN_TOKEN;
					} else {
						uNew = uOld + FLAG_LOCKED; // uOld | FLAG_LOCKED;
					}
				} while(_MCFCRT_EXPECT_NOT(!__atomic_compare_exchange_n(pMutex, &uOld, uNew, false, __ATOMIC_ACQ_REL, __ATOMIC_CONSUME)));
			}
			if(_MCFCRT_EXPECT(bTaken)){
				return true;
			}
			if(bCanSpin){
				for(size_t i = 0; i < uMaxSpinCount; ++i){
					bool bTaken;
					{
						uintptr_t uOld, uNew;
						uOld = __atomic_load_n(pMutex, __ATOMIC_CONSUME);
						do {
							bTaken = !(uOld & FLAG_LOCKED);
							if(!bTaken){
								break;
							}
							uNew = uOld + FLAG_LOCKED; // uOld | FLAG_LOCKED;
						} while(_MCFCRT_EXPECT_NOT(!__atomic_compare_exchange_n(pMutex, &uOld, uNew, false, __ATOMIC_ACQ_REL, __ATOMIC_CONSUME)));
					}
					if(_MCFCRT_EXPECT_NOT(bTaken)){
						return true;
					}
					__builtin_ia32_pause();
				}
			}
		}

		bool bTaken;
		{
			uintptr_t uOld, uNew;
			uOld = __atomic_load_n(pMutex, __ATOMIC_CONSUME);
			do {
				bTaken = !(uOld & FLAG_LOCKED);
				if(!bTaken){
					uNew = uOld + MAKE_THREAD_COUNT(1);
				} else {
					uNew = uOld + FLAG_LOCKED; // uOld | FLAG_LOCKED;
				}
			} while(_MCFCRT_EXPECT_NOT(!__atomic_compare_exchange_n(pMutex, &uOld, uNew, false, __ATOMIC_ACQ_REL, __ATOMIC_CONSUME)));
		}
		if(_MCFCRT_EXPECT(bTaken)){
			return true;
		}
		if(bMayTimeOut){
			LARGE_INTEGER liTimeout;
			__MCF_CRT_InitializeNtTimeout(&liTimeout, u64UntilFastMonoClock);
			NTSTATUS lStatus = NtWaitForKeyedEvent(nullptr, (void *)pMutex, false, &liTimeout);
			_MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtWaitForKeyedEvent() 失败。");
			if(_MCFCRT_EXPECT(lStatus == STATUS_TIMEOUT)){
				bool bDecremented;
				{
					uintptr_t uOld, uNew;
					uOld = __atomic_load_n(pMutex, __ATOMIC_CONSUME);
					do {
						bDecremented = (GET_THREAD_COUNT(uOld) != 0);
						if(!bDecremented){
							break;
						}
						uNew = uOld - MAKE_THREAD_COUNT(1);
					} while(_MCFCRT_EXPECT_NOT(!__atomic_compare_exchange_n(pMutex, &uOld, uNew, false, __ATOMIC_ACQ_REL, __ATOMIC_CONSUME)));
				}
				if(bDecremented){
					return false;
				}
				lStatus = NtWaitForKeyedEvent(nullptr, (void *)pMutex, false, nullptr);
				_MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtWaitForKeyedEvent() 失败。");
				_MCFCRT_ASSERT(lStatus != STATUS_TIMEOUT);
				return false;
			}
		} else {
			NTSTATUS lStatus = NtWaitForKeyedEvent(nullptr, (void *)pMutex, false, nullptr);
			_MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtWaitForKeyedEvent() 失败。");
			_MCFCRT_ASSERT(lStatus != STATUS_TIMEOUT);
		}
	}
}

static inline void ReallySignalMutex(_MCFCRT_Mutex *pMutex){
	bool bSignalOne;
	{
		uintptr_t uOld, uNew;
		uOld = __atomic_load_n(pMutex, __ATOMIC_CONSUME);
		do {
			_MCFCRT_ASSERT_MSG(uOld & FLAG_LOCKED, L"互斥体没有被任何线程锁定。");

			bSignalOne = (GET_THREAD_COUNT(uOld) != 0);
			uNew = uOld & ~(FLAG_LOCKED | FLAG_SPIN_TOKEN);
			if(bSignalOne){
				uNew -= MAKE_THREAD_COUNT(1);
			}
		} while(_MCFCRT_EXPECT_NOT(!__atomic_compare_exchange_n(pMutex, &uOld, uNew, false, __ATOMIC_ACQ_REL, __ATOMIC_CONSUME)));
	}
	if(bSignalOne){
		NTSTATUS lStatus = NtReleaseKeyedEvent(nullptr, (void *)pMutex, false, nullptr);
		_MCFCRT_ASSERT_MSG(NT_SUCCESS(lStatus), L"NtReleaseKeyedEvent() 失败。");
		_MCFCRT_ASSERT(lStatus != STATUS_TIMEOUT);
	}
}

bool _MCFCRT_WaitForMutex(_MCFCRT_Mutex *pMutex, size_t uMaxSpinCount, uint64_t u64UntilFastMonoClock){
	const bool bLocked = ReallyWaitForMutex(pMutex, uMaxSpinCount, true, u64UntilFastMonoClock);
	return bLocked;
}
void _MCFCRT_WaitForMutexForever(_MCFCRT_Mutex *pMutex, size_t uMaxSpinCount){
	const bool bLocked = ReallyWaitForMutex(pMutex, uMaxSpinCount, false, UINT64_MAX);
	_MCFCRT_ASSERT(bLocked);
}
void _MCFCRT_SignalMutex(_MCFCRT_Mutex *pMutex){
	ReallySignalMutex(pMutex);
}
