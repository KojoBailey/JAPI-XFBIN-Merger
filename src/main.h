#pragma once

#define EXPORT extern "C" __declspec(dllexport)

#include "types.h"
#include <JojoAPI.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using JSON = nlohmann::ordered_json;

EXPORT ModMeta __stdcall GetModInfo();
EXPORT void __stdcall ModInit();

// Unmodified functions:

typedef std::uint64_t*(__fastcall* Load_nuccBinary_t)(const char*, const char*);
Load_nuccBinary_t Load_nuccBinary_original;

std::uint64_t* __fastcall Load_nuccBinary(const char* a1, const char* a2) {
    return Load_nuccBinary_original(a1, a2);
}

typedef std::uint64_t(__fastcall* NUCC_Encrypt_t)(const char*);
NUCC_Encrypt_t NUCC_Encrypt_original;

std::uint64_t __fastcall NUCC_Encrypt(const char* a1) {
    return NUCC_Encrypt_original(a1);
}

typedef float*(__fastcall* RGBA_Int_to_Float_t)(float*, unsigned int);
RGBA_Int_to_Float_t RGBA_Int_to_Float_original;

float* __fastcall RGBA_Int_to_Float(float* float_ref, unsigned int rgba) {
    return RGBA_Int_to_Float_original(float_ref, rgba);
}

typedef float*(__fastcall* sub_47EB58_t)(std::uint64_t*, __uint128_t*, __uint128_t*);
sub_47EB58_t sub_47EB58_original;

float* __fastcall sub_47EB58(std::uint64_t* a1, __uint128_t* a2, __uint128_t* a3) {
    return sub_47EB58_original(a1, a2, a3);
}

Hook Load_nuccBinary_hook = {
    (void*)0x671C30, // Address of the function we want to hook
    (void*)Load_nuccBinary, // Address of our hook function
    (void**)&Load_nuccBinary_original, // Address of the variable that will store the original function address
    "Load_nuccBinary" // Name of the function we want to hook
};

Hook NUCC_Encrypt_hook = {
    (void*)0x6C92A0, // Address of the function we want to hook
    (void*)NUCC_Encrypt, // Address of our hook function
    (void**)&NUCC_Encrypt_original, // Address of the variable that will store the original function address
    "NUCC_Encrypt" // Name of the function we want to hook
};

Hook RGBA_Int_to_Float_hook = {
    (void*)0x6DC840, // Address of the function we want to hook
    (void*)RGBA_Int_to_Float, // Address of our hook function
    (void**)&RGBA_Int_to_Float_original, // Address of the variable that will store the original function address
    "RGBA_Int_to_Float" // Name of the function we want to hook
};

Hook sub_47EB58_hook = {
    (void*)0x47EB58, // Address of the function we want to hook
    (void*)sub_47EB58, // Address of our hook function
    (void**)&sub_47EB58_original, // Address of the variable that will store the original function address
    "sub_47EB58" // Name of the function we want to hook
};