#pragma once

#define DEBUG_BUILD true
#define USE_BINARY_TYPES

#define EXPORT extern "C" __declspec(dllexport)

#include <JojoAPI.h>
#include <nucc/xfbin.hpp>
#include <nucc/chunks/binary/asbr.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <unordered_map>
#include <sstream>
#include <format>

namespace fs = std::filesystem;
namespace ASBR = nucc::ASBR;
using JSON = nlohmann::ordered_json;

EXPORT JAPIModMeta __stdcall GetModMeta();
EXPORT void __stdcall ModInit();

JAPIModMeta __stdcall GetModMeta() {
    static JAPIModMeta meta = {
        "Merging", // Name
        "Merging", // GUID
        "Kojo Bailey", // Author
        "1.0.0", // Version
        "Allows for easy merging of otherwise incompatible mods!" // Description
    };
    return meta;
}

#define JFATAL(message, ...) JAPI_LogFatal(std::format(message, ##__VA_ARGS__).c_str())
#define JERROR(message, ...) JAPI_LogError(std::format(message, ##__VA_ARGS__).c_str())
#define JWARN(message, ...) JAPI_LogWarn(std::format(message, ##__VA_ARGS__).c_str())
#define JINFO(message, ...) JAPI_LogInfo(std::format(message, ##__VA_ARGS__).c_str())

#if DEBUG_BUILD
    #define JDEBUG(message, ...) JAPI_LogDebug(std::format(message, ##__VA_ARGS__).c_str())
    #define JTRACE(message, ...) JAPI_LogTrace(std::format(message, ##__VA_ARGS__).c_str())
#else
    #define JDEBUG(message, ...)
    #define JTRACE(message, ...)
#endif

// Function definitions:
void log_bytes(char* addr, int size, std::string name) {
    std::ofstream debug("japi\\dll-plugins\\" + name + ".bin", std::ios::binary);
    for (int i = 0; i < size; i++) {
        debug << addr[i];
    }
    debug.close();
}
template<typename RETURN, typename... PARAMS> auto define_function(long long address) {
    return (RETURN(__fastcall*)(PARAMS...))(JAPI_GetModuleBase() + address);
}

auto NUCC_Hash = define_function<int,
    const char*>(0x6C92A0);
auto Fetch_String_from_Hash = define_function<const char*,
    u64, u32>(0x77B110);
auto RGBA_Int_to_Float = define_function<float*,
    float*, int>(0x6DC840);
auto Allocate_PlayerColorParam_Data = define_function<u64*,
    ASBR::cache::vector*, ASBR::cache::PlayerColorParam*, ASBR::cache::PlayerColorParam*>(0x47EB58);
auto Load_XFBIN_Data = define_function<u64*,
    u64, const char*>(0x6E5230);
auto Get_Chunk_Address = define_function<u64*,
    u64*, const char*, u64*>(0x6E3290);