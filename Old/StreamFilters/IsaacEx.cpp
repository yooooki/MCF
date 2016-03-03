// 这个文件是 MCF 的一部分。
// 有关具体授权说明，请参阅 MCFLicense.txt。
// Copyleft 2013 - 2016, LH_Mouse. All wrongs reserved.
#if 0
#include "../StdMCF.hpp"
#include "IsaacEx.hpp"
#include "../Utilities/Endian.hpp"
#include "../Hash/Sha256.hpp"

namespace MCF {

namespace {
	Array<std::uint32_t, 8> GenerateKeyHash(const void *pKey, std::size_t uKeyLen) noexcept {
		Sha256 vShaHasher;
		vShaHasher.Update(pKey, uKeyLen);
		const auto vHash = vShaHasher.Finalize();

		Array<std::uint32_t, 8> vKeyHash;
		for(std::size_t i = 0; i < 8; ++i){
			vKeyHash[i] = LoadBe(reinterpret_cast<const std::uint32_t *>(vHash.GetData())[i]);
		}
		return vKeyHash;
	}
}

// ========== IsaacExEncoder ==========
IsaacExEncoder::IsaacExEncoder(const void *pKey, std::size_t uKeyLen) noexcept
	: x_au32KeyHash(GenerateKeyHash(pKey, uKeyLen))
{
}

void IsaacExEncoder::X_DoInit(){
	x_vIsaacGenerator.Init(x_au32KeyHash);
	x_byLastEncoded = 0;
	x_lLastHighWord = -1;
}
void IsaacExEncoder::X_DoUpdate(const void *pData, std::size_t uSize){
	auto pbyRead = static_cast<const unsigned char *>(pData);
	const auto pbyEnd = pbyRead + uSize;

	const auto EncodeByte = [&](unsigned uSeed){
		register unsigned char by = *pbyRead;
		++pbyRead;
		const unsigned char byRot = (x_byLastEncoded ^ (x_byLastEncoded >> 3) ^ (x_byLastEncoded >> 6)) & 7;

		by = static_cast<unsigned char>(by ^ uSeed);
		__asm__("rol %b0, cl \n" : "+q"(by) : "c"(byRot));
		x_byLastEncoded = static_cast<unsigned char>(by ^ (uSeed >> 8));

		X_Output(by);
	};

	if(uSize > 4){
		unsigned uSeed;
		if(x_lLastHighWord != -1){
			uSeed = (std::uint16_t)x_lLastHighWord;
			x_lLastHighWord = -1;
			EncodeByte(uSeed);
		}
		register auto i = (std::size_t)(pbyEnd - pbyRead) / 2;
		while(i != 0){
			uSeed = x_vIsaacGenerator.Get();
			EncodeByte(uSeed);
			EncodeByte(uSeed >> 16);
			--i;
		}
	}
	while(pbyRead != pbyEnd){
		unsigned uSeed;
		if(x_lLastHighWord == -1){
			const auto u32Word = x_vIsaacGenerator.Get();
			uSeed = u32Word;
			x_lLastHighWord = (long)(u32Word >> 16);
		} else {
			uSeed = (std::uint16_t)x_lLastHighWord;
			x_lLastHighWord = -1;
		}
		EncodeByte(uSeed);
	}
}
void IsaacExEncoder::X_DoFinalize(){
}

// ========== IsaacExDecoder ==========
IsaacExDecoder::IsaacExDecoder(const void *pKey, std::size_t uKeyLen) noexcept
	: x_au32KeyHash(GenerateKeyHash(pKey, uKeyLen))
{
}

void IsaacExDecoder::X_DoInit(){
	x_vIsaacGenerator.Init(x_au32KeyHash);
	x_byLastEncoded = 0;
	x_lLastHighWord = -1;
}
void IsaacExDecoder::X_DoUpdate(const void *pData, std::size_t uSize){
	auto pbyRead = static_cast<const unsigned char *>(pData);
	const auto pbyEnd = pbyRead + uSize;

	const auto DecodeByte = [&](unsigned uSeed){
		register unsigned char by = *pbyRead;
		++pbyRead;
		const unsigned char byRot = (x_byLastEncoded ^ (x_byLastEncoded >> 3) ^ (x_byLastEncoded >> 6)) & 7;

		x_byLastEncoded = static_cast<unsigned char>(by ^ (uSeed >> 8));
		__asm__("ror %b0, cl \n" : "+q"(by) : "c"(byRot));
		by = static_cast<unsigned char>(by ^ uSeed);

		X_Output(by);
	};

	if(uSize > 4){
		unsigned uSeed;
		if(x_lLastHighWord != -1){
			uSeed = (std::uint16_t)x_lLastHighWord;
			x_lLastHighWord = -1;
			DecodeByte(uSeed);
		}
		register auto i = (std::size_t)(pbyEnd - pbyRead) / 2;
		while(i != 0){
			uSeed = x_vIsaacGenerator.Get();
			DecodeByte(uSeed);
			DecodeByte(uSeed >> 16);
			--i;
		}
	}
	while(pbyRead != pbyEnd){
		unsigned uSeed;
		if(x_lLastHighWord == -1){
			const auto u32Word = x_vIsaacGenerator.Get();
			uSeed = u32Word;
			x_lLastHighWord = (long)(u32Word >> 16);
		} else {
			uSeed = (std::uint16_t)x_lLastHighWord;
			x_lLastHighWord = -1;
		}
		DecodeByte(uSeed);
	}
}
void IsaacExDecoder::X_DoFinalize(){
}

}
#endif