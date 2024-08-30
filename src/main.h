#pragma once

#define EXPORT extern "C" __declspec(dllexport)

#include <JojoAPI.h>
#include <nlohmann/json.hpp>
#define USE_BINARY_TYPES
#include <binary.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <unordered_map>
#include <sstream>
#include <format>

namespace fs = std::filesystem;
using JSON = nlohmann::ordered_json;

using u128 = __uint128_t;
using i128 = __int128_t;

EXPORT ModMeta __stdcall GetModInfo();
EXPORT void __stdcall ModInit();

ModMeta __stdcall GetModInfo() {
    static ModMeta meta = {
        "PlayerColorParam JSON Loader", // Name
        "PlayerColorParam_JSON_Loader", // GUID
        "1.0.0",                        // Version
        "Kojo Bailey"                   // Author
    };

    return meta;
}

// #define DEBUG_BUILD

#define JFATAL(message, ...) JAPI_LogFatal(std::format(message, ##__VA_ARGS__).c_str(), ##__VA_ARGS__)
#define JERROR(message, ...) JAPI_LogError(std::format(message, ##__VA_ARGS__).c_str(), ##__VA_ARGS__)
#define JWARN(message, ...) JAPI_LogWarn(std::format(message, ##__VA_ARGS__).c_str(), ##__VA_ARGS__)
#define JINFO(message, ...) JAPI_LogInfo(std::format(message, ##__VA_ARGS__).c_str(), ##__VA_ARGS__)

#ifdef DEBUG_BUILD
    #define JDEBUG(message, ...) JAPI_LogDebug(std::format(message, ##__VA_ARGS__).c_str(), ##__VA_ARGS__)
    #define JTRACE(message, ...) JAPI_LogTrace(std::format(message, ##__VA_ARGS__).c_str(), ##__VA_ARGS__)
#else
    #define JDEBUG(message, ...)
    #define JTRACE(message, ...)
#endif

// Function definitions:
template<typename RETURN, typename... PARAMS> auto define_function(long long address) {
    return (RETURN(__fastcall*)(PARAMS...))(JAPI_GetASBRModuleBase() + address);
}

auto Load_nuccBinary = define_function<u64*, const char*, const char*>(0x671C30);
auto NUCC_Encrypt = define_function<u64, const char*>(0x6C92A0);
auto RGBA_Int_to_Float = define_function<float*, float*, int>(0x6DC840);
auto sub_47EB58 = define_function<float*, u64*, u128*, u128*>(0x47EB58);