// 这个文件是 MCF 的一部分。
// 有关具体授权说明，请参阅 MCFLicense.txt。
// Copyleft 2013 - 2015, LH_Mouse. All wrongs reserved.

#include "../StdMCF.hpp"
#include "ConditionVariable.hpp"
#include "../Utilities/Defer.hpp"
#include <winternl.h>
#include <ntstatus.h>

extern "C" __attribute__((__dllimport__, __stdcall__))
NTSTATUS NtWaitForKeyedEvent(HANDLE hKeyedEvent, void *pKey, BOOLEAN bAlertable, const LARGE_INTEGER *pliTimeout) noexcept;
extern "C" __attribute__((__dllimport__, __stdcall__))
NTSTATUS NtReleaseKeyedEvent(HANDLE hKeyedEvent, void *pKey, BOOLEAN bAlertable, const LARGE_INTEGER *pliTimeout) noexcept;

namespace MCF {

// 其他非静态成员函数。
bool ConditionVariable::Wait(Impl_UniqueLockTemplate::UniqueLockTemplateBase &vLock, std::uint64_t u64MilliSeconds) noexcept {
	if(u64MilliSeconds > static_cast<std::uint64_t>(INT64_MIN) / 10000){
		Wait(vLock);
		return true;
	}

	x_uWaitingThreads.Increment(kAtomicRelaxed);
	const auto uCount = vLock.X_UnlockAll();
	ASSERT_MSG(uCount != 0, L"你会用条件变量吗？");
	DEFER([&]{
		vLock.X_RelockAll(uCount);
		x_uWaitingThreads.Decrement(kAtomicRelaxed);
	});

	::LARGE_INTEGER liTimeout;
	liTimeout.QuadPart = -static_cast<std::int64_t>(u64MilliSeconds * 10000);
	const auto lStatus = ::NtWaitForKeyedEvent(nullptr, this, false, &liTimeout);
	if(!NT_SUCCESS(lStatus)){
		ASSERT_MSG(false, L"NtWaitForKeyedEvent() 失败。");
	}
	return lStatus != STATUS_TIMEOUT;
}
void ConditionVariable::Wait(Impl_UniqueLockTemplate::UniqueLockTemplateBase &vLock) noexcept {
	x_uWaitingThreads.Increment(kAtomicRelaxed);
	const auto uCount = vLock.X_UnlockAll();
	ASSERT_MSG(uCount != 0, L"你会用条件变量吗？");
	DEFER([&]{
		vLock.X_RelockAll(uCount);
		x_uWaitingThreads.Decrement(kAtomicRelaxed);
	});

	const auto lStatus = ::NtWaitForKeyedEvent(nullptr, this, false, nullptr);
	if(!NT_SUCCESS(lStatus)){
		ASSERT_MSG(false, L"NtWaitForKeyedEvent() 失败。");
	}
}

namespace {
	constexpr ::LARGE_INTEGER kZeroTimeout = { };
}

std::size_t ConditionVariable::Signal(std::size_t uMaxToWakeUp) noexcept {
	const auto uToWakeUp = std::min(x_uWaitingThreads.Load(kAtomicRelaxed), uMaxToWakeUp);
	std::size_t uWokenUp = 0;
	while(uWokenUp < uToWakeUp){
		const auto lStatus = ::NtReleaseKeyedEvent(nullptr, this, false, &kZeroTimeout);
		if(!NT_SUCCESS(lStatus)){
			ASSERT_MSG(false, L"NtReleaseKeyedEvent() 失败。");
		}
		if(lStatus == STATUS_TIMEOUT){
			break;
		}
		++uWokenUp;
	}
	return uWokenUp;
}
std::size_t ConditionVariable::Broadcast() noexcept {
	return Signal(static_cast<std::size_t>(-1));
}

}
