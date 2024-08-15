#include "main.h"

// This function is called when the mod is loaded.
// It should return a ModMeta struct with the mod's information.
ModMeta __stdcall GetModInfo() {
    static ModMeta meta = {
        "PlayerColorParam Reader", // Name
        "PlayerColorParam", // GUID
        "ALPHA", // Version
        "Kojo Bailey" // Author
    };

    return meta;
}

fs::path json_directory{"japi\\mods\\PlayerColorParam"};

JSON json_data;

union U128 {
    u128 value;
    struct {
        i64 low;   // Lower 64 bits
        i64 high;  // Upper 64 bits
    };
    struct {
        i32 part0;  // Lower 32 bits
        i32 part1;  // Lower-middle 32 bits
        i32 part2;  // Upper-middle 32 bits
        i32 part3;  // Upper 32 bits
    };
};

template<typename T>
T get_offset_value(u64* start, int bytes_forward) {
    return *reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(start) + bytes_forward);
}
template<typename T>
T* get_offset_ptr(u64* start, int bytes_forward) {
    return reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(start) + bytes_forward);
}

struct RGB {
    u32 r, g, b;
};
RGB hex_to_rgb(std::string hex_str) {
    std::erase(hex_str, '#');
    std::stringstream buffer;
    u32 r, g, b;
    buffer << std::hex << hex_str.substr(0, 2);
    buffer >> r;
    buffer.clear();
    buffer << std::hex << hex_str.substr(2, 2);
    buffer >> g;
    buffer.clear();
    buffer << std::hex << hex_str.substr(4, 2);
    buffer >> b;
    return {r, g, b};
}

typedef u64*(__fastcall* Parse_PlayerColorParam_t)(u64*);
Parse_PlayerColorParam_t Parse_PlayerColorParam_original;

u64* __fastcall Parse_PlayerColorParam(u64* a1) {
    // Variable definitions.
    u64* result;            // To be returned at end of function.
    u64* nuccBinary_data;   // Data from XFBIN chunk, exactly as-is.
    u32 entry_count;        // Number of entries.
    u64* note_pointer;      // Initial pointer, pointing to start of entries. Can be used to skip over notes.
    u64* entries_start;     // Start of entries.
    struct {
        u64* start;                 // Start of entry.
        struct {
            u64* pointer;           // Pointer to character ID later in the data.
            const char* string;     // Character's ID (e.g. "1jnt01").
            u32 encrypted;          // Encrypted by CRC-32.
        } character_id;             // Character's ID (e.g. "1jnt01").
        u32 costume_index;          // Costume number (e.g. 3 = Special C).
        u32 rgb;                    // Red, Green, Blue colour value.
        i128 rgb_float;             // RGB value as a float.
    } entry;                // All data belonging to only one entry.
    U128 buffer;
    u128* buffer_ptr;
    std::string alt_id;
    RGB color;
    std::unordered_map<std::string, int> tint_tracker;

    // Load data from XFBIN file.
    nuccBinary_data = Load_nuccBinary("data/param/battle/PlayerColorParam.bin.xfbin", "PlayerColorParam");
    result = nuccBinary_data; // In case the pointer is 0.

    if (nuccBinary_data) {
        entry_count = get_offset_value<u32>(nuccBinary_data, 4);
        note_pointer = get_offset_ptr<u64>(nuccBinary_data, 8); // 8 for all vanilla files.
        if (note_pointer) {
            entries_start = get_offset_ptr<u64>(note_pointer, *note_pointer);

            // Iterate through each entry.
            if (entries_start) {
                for (int i = 0; i < entry_count; i++) {
                    entry.start = get_offset_ptr<u64>(entries_start, 24 * i);
                    entry.character_id.pointer = entry.start;

                    // Get character ID from pointer.
                    if (*entry.start) {
                        entry.character_id.string = get_offset_ptr<const char>(entry.character_id.pointer, *entry.character_id.pointer);
                    } else {
                        entry.character_id.string = 0;
                    }
                    // Encrypt character ID.
                    entry.character_id.encrypted = NUCC_Encrypt(entry.character_id.string);

                    // Get costume index and alt ID.
                    entry.costume_index = get_offset_value<u32>(entry.start, 8);
                    alt_id = entry.character_id.string;
                    alt_id.replace(4, 1, std::to_string(entry.costume_index));
                    alt_id += "col" + std::to_string(tint_tracker[alt_id]++);
                    JAPI_LogInfo(alt_id);
                    auto& json_rgb = json_data[alt_id];

                    // Load and apply RGB data from JSON.
                    if (json_rgb.empty()) {
                        // Load from XFBIN data.
                        color.b = get_offset_value<u32>(entry.start, 20);
                        color.g = get_offset_value<u32>(entry.start, 16);
                        color.r = get_offset_value<u32>(entry.start, 12);
                    } else {
                        // Load from JSON data.
                        if (json_rgb.type() == JSON::value_t::string) {
                            color = hex_to_rgb(json_rgb);
                        } else if (json_rgb.type() == JSON::value_t::array) {
                            color.r = json_rgb[0];
                            color.g = json_rgb[1];
                            color.b = json_rgb[2];
                        } else {
                            JAPI_LogError("Invalid RGB input. Must be hex code or integer array.");
                            return 0;
                        }
                    }

                    entry.rgb = 
                        (color.b | 
                        ((color.g | 
                        (color.r 
                        << 8)) << 8)) << 8;
                    result = (u64*)RGBA_Int_to_Float((float*)&entry.rgb_float, entry.rgb | 0xFFu);

                    // Buffer data into whatever for use in-game.
                    buffer.part0 = entry.character_id.encrypted;
                    buffer.part1 = entry.costume_index;
                    buffer_ptr = (u128*)a1[5];
                    if ((u128*)a1[6] == buffer_ptr) {
                        result = (u64*)sub_47EB58(a1 + 1, buffer_ptr, &buffer.value);
                    } else {
                        *buffer_ptr = buffer.value;
                        buffer_ptr[1] = entry.rgb_float;
                        a1[5] += 32;
                    }
                }
            }
        }
    }
    return result;
}

Hook Parse_PlayerColorParam_hook = {
    (void*)0x47F114, // Address of the function we want to hook
    (void*)Parse_PlayerColorParam, // Address of our hook function
    (void**)&Parse_PlayerColorParam_original, // Address of the variable that will store the original function address
    "Parse_PlayerColorParam" // Name of the function we want to hook
};

// This function is called when the mod is loaded.
void __stdcall ModInit() {
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

    // Create directory for JSON files if not already existing.
    if (!fs::exists(json_directory))
        fs::create_directory(json_directory);

    fs::path priority_path{json_directory / "_priority.json"};
    JSON priority_json;

    // Read existing priority data, if it exists.
    if (fs::exists(priority_path)) {
        std::ifstream priority_file(priority_path);
        priority_json = JSON::parse(priority_file);
        priority_file.close();
    }

    // Refresh priority data.
    for (auto& entry : fs::directory_iterator(json_directory)) {
        if (entry.path().extension() == ".json" && entry.path().filename() != "_priority.json")
            if (priority_json[entry.path().filename().stem().string()] == nullptr)
                priority_json[entry.path().filename().stem().string()] = 0;
    }

    // Create new priority JSON file.
    std::ofstream priority_file(priority_path);
    priority_file << priority_json.dump(2);
    priority_file.close();

    // Merge JSON files together.
    std::ifstream json_file_buffer(json_directory / "test.json");
    json_data = JSON::parse(json_file_buffer);

    JAPI_LogInfo("Loaded!");
    JAPI_LogWarn("This plugin will only work up until the July 2024 version of JoJoAPI.");
}