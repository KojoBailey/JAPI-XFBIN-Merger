#ifndef JAPI_MERGING
#define JAPI_MERGING

#define EXPORT extern "C" __declspec(dllexport)

#include <JoJoAPI.h>

#include <format>

EXPORT JAPIModMeta __stdcall GetModMeta();
EXPORT void __stdcall ModInit();

JAPIModMeta GetModMeta()
{
	return {
		.name        = "XFBIN Merging",
		.author      = "Kojo Bailey",
		.guid        = "merging",
		.version     = "1.0.0",
		.description = "Allows for easy merging of otherwise incompatible mods!",
	};
}

#define JFATAL(fmt, ...) JAPI_LogMessage(JAPI_LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__)
#define JERROR(fmt, ...) JAPI_LogMessage(JAPI_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define JWARN(fmt, ...) JAPI_LogMessage(JAPI_LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define JINFO(fmt, ...) JAPI_LogMessage(JAPI_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)

#define DEBUG_BUILD true

#if DEBUG_BUILD
	#define JDEBUG(fmt, ...) JAPI_LogMessage(JAPI_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
	#define JTRACE(fmt, ...) JAPI_LogMessage(JAPI_LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#else
	#define JDEBUG(fmt, ...)
	#define JTRACE(fmt, ...)
#endif

// template<typename RETURN, typename... PARAMS> auto define_function(long long address) {
// 	return (RETURN(__fastcall*)(PARAMS...))(JAPI_GetModuleBase() + address);
// }
//
// auto NUCC_Hash = define_function<int,
// 	const char*>(0x6C92A0);
// auto Fetch_String_from_Hash = define_function<const char*,
// 	u64, u32>(0x77B110);
// auto RGBA_Int_to_Float = define_function<float*,
// 	float*, int>(0x6DC840);
// auto Allocate_PlayerColorParam_Data = define_function<u64*,
// 	ASBR::cache::vector*, ASBR::cache::PlayerColorParam*, ASBR::cache::PlayerColorParam*>(0x47EB58);
// auto Load_XFBIN_Data = define_function<u64*,
// 	u64, const char*>(0x6E5230);
// auto Get_Chunk_Address = define_function<u64*,
// 	u64*, const char*, u64*>(0x6E3290);

#endif
