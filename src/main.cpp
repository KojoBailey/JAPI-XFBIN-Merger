#include "main.h"

#include "nlohmann/json.hpp"

// This function is called when the mod is loaded.
// It should return a ModMeta struct with the mod's information.
ModMeta __stdcall GetModInfo() {
    static ModMeta meta = {
        "PlayerColorParam Reader", // Name
        "PlayerColorParam", // GUID
        "1.0.0", // Version
        "Kojo Bailey" // Author
    };

    return meta;
}

union U128 {
    __uint128_t value;
    struct {
        std::int64_t low;   // Lower 64 bits
        std::int64_t high;  // Upper 64 bits
    };
    struct {
        std::int32_t part0;  // Lower 32 bits
        std::int32_t part1;  // Lower-middle 32 bits
        std::int32_t part2;  // Upper-middle 32 bits
        std::int32_t part3;  // Upper 32 bits
        
    };
};

template<typename T>
T get_offset_value(std::uint64_t* buffer, int size) {
    return *reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(buffer) + size);
}
template<typename T>
T* get_offset_ptr(std::uint64_t* buffer, int size) {
    return reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(buffer) + size);
}

typedef std::uint64_t*(__fastcall* Parse_PlayerColorParam_t)(std::uint64_t*);
Parse_PlayerColorParam_t Parse_PlayerColorParam_original;

std::uint64_t* __fastcall Parse_PlayerColorParam(std::uint64_t* a1) {
    std::uint64_t* buffer;
    std::uint32_t entry_count;
    std::uint64_t* first_pointer;
    std::uint64_t* first_entry_start;
    std::uint64_t* entry_start;
    struct {
        const char* character_id;
        std::uint32_t encrypted_character_id;
        std::uint32_t rgb;
    } entry;
    U128 v12;
    __int128_t rgb_float;
    __uint128_t* v11;

    buffer = Load_nuccBinary("data/param/battle/PlayerColorParam.bin.xfbin", "PlayerColorParam");
    if (buffer) {
        entry_count = get_offset_value<std::uint32_t>(buffer, 4);
        first_pointer = get_offset_ptr<std::uint64_t>(buffer, 8);
        if (first_pointer) {
            buffer = get_offset_ptr<std::uint64_t>(first_pointer, *first_pointer);
            first_entry_start = buffer;
            if (buffer) {
                for (int i = 0; i < entry_count; i++) {
                    entry_start = get_offset_ptr<std::uint64_t>(first_entry_start, 24 * i);
                    if (*entry_start) {
                        entry.character_id = get_offset_ptr<const char>(entry_start, *entry_start); // First value in entry_start is a pointer.
                    } else {
                        entry.character_id = 0;
                    }
                    entry.encrypted_character_id = NUCC_Encrypt(entry.character_id);
                    entry.rgb = (get_offset_value<std::uint32_t>(entry_start, 20) | ((get_offset_value<std::uint32_t>(entry_start, 16) | (get_offset_value<std::uint32_t>(entry_start, 12) << 8)) << 8)) << 8;
                    v12.part0 = entry.encrypted_character_id;
                    v12.part1 = get_offset_value<std::uint32_t>(entry_start, 8); // Costume index
                    buffer = (std::uint64_t*)RGBA_Int_to_Float((float*)&rgb_float, entry.rgb | 0xFFu);
                    v11 = (__uint128_t*)a1[5];
                    if ((__uint128_t*)a1[6] == v11) {
                        buffer = (std::uint64_t*)sub_47EB58(a1 + 1, v11, &v12.value);
                    } else {
                        *v11 = v12.value;
                        v11[1] = rgb_float;
                        a1[5] += 32;
                    }
                }
            }
        }
    }
    return buffer;
}

Hook Parse_PlayerColorParam_hook = {
    (void*)0x47F114, // Address of the function we want to hook
    (void*)Parse_PlayerColorParam, // Address of our hook function
    (void**)&Parse_PlayerColorParam_original, // Address of the variable that will store the original function address
    "Parse_PlayerColorParam" // Name of the function we want to hook
};

// This function is called when the mod is loaded.
void __stdcall ModInit() {
    JAPI_LogInfo("Initialised!");

    if(!JAPI_HookASBRFunction(&Parse_PlayerColorParam_hook))
        JAPI_LogError("Failed to hook Parse_PlayerColorParam!");
    if(!JAPI_HookASBRFunction(&Load_nuccBinary_hook))
        JAPI_LogError("Failed to hook Load_nuccBinary!");
    if(!JAPI_HookASBRFunction(&NUCC_Encrypt_hook))
        JAPI_LogError("Failed to hook NUCC_Encrypt!");
    if(!JAPI_HookASBRFunction(&RGBA_Int_to_Float_hook))
        JAPI_LogError("Failed to hook RGBA_Int_to_Float!");
    if(!JAPI_HookASBRFunction(&sub_47EB58_hook))
        JAPI_LogError("Failed to hook sub_47EB58!");

    JAPI_LogInfo("Applied patches.");
}