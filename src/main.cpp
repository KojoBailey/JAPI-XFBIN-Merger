#include "main.h"

// This function is called when the mod is loaded.
// It should return a ModMeta struct with the mod's information.
ModMeta __stdcall GetModInfo() {
    static ModMeta meta = {
        "PlayerColorParam JSON Loader", // Name
        "PlayerColorParam_JSON_Loader", // GUID
        "1.0.0",                        // Version
        "Kojo Bailey"                   // Author
    };

    return meta;
}

fs::path json_directory{"japi\\mods\\PlayerColorParam"};
JSON json_data;

u64* __fastcall Parse_PlayerColorParam(u64* a1) {
    // Variable definitions.
    u64* result;            // To be returned at end of function.
    u64* nuccBinary_data;   // Data from XFBIN chunk, exactly as-is.
    u32 entry_count;        // Number of entries.
    u64* note_pointer;      // Initial pointer, pointing to start of entries. Can be used to skip over notes.
    u64* entries_start;     // Start of entries.

    u64* entry_start;                 // Start of entry.
    u64* character_id_pointer;           // Pointer to character ID later in the data.
    const char* character_id;     // Character's ID (e.g. "1jnt01").
    RGB color;
    u32 costume_index_buffer;
    std::string key_buffer;

    u128 buffer;
    u128* buffer_ptr;
    std::unordered_map<std::string, int> tint_tracker;

    struct Entry {
        u32 encrypted_character_id;
        u32 costume_index;          // Costume number (e.g. 3 = Special C).
        i128 rgb_float;             // RGB value as a float.
    };                      // All data belonging to only one entry.
    std::map<std::string, Entry> entries;

    // Load data from XFBIN file.
    nuccBinary_data = Load_nuccBinary("data/param/battle/PlayerColorParam.bin.xfbin", "PlayerColorParam");
    if (!nuccBinary_data) {
        JAPI_LogError("`PlayerColorParam.bin.xfbin` data could not be loaded.");
        return 0;
    }

    entry_count = get_offset_value<u32>(nuccBinary_data, 4);
    note_pointer = get_offset_ptr<u64>(nuccBinary_data, 8); // 8 for all vanilla files.
    if (!note_pointer) {
        JAPI_LogError("`PlayerColorParam.bin.xfbin` is missing an initial pointer.");
        return 0;
    }

    // Iterate through each entry.
    entries_start = get_offset_ptr<u64>(note_pointer, *note_pointer);
    for (int i = 0; i < entry_count; i++) {
        entry_start = get_offset_ptr<u64>(entries_start, 24 * i);

        // Get character ID from pointer.
        character_id_pointer = entry_start;
        if (*entry_start) {
            character_id = get_offset_ptr<const char>(character_id_pointer, *character_id_pointer);
        } else {
            character_id = 0;
            continue;
        }

        // Get costume index and alt ID.
        costume_index_buffer = get_offset_value<u32>(entry_start, 8);
        key_buffer = character_id;
        key_buffer.replace(4, 1, std::to_string(costume_index_buffer));
        key_buffer += "col" + std::to_string(tint_tracker[key_buffer]++);
        entries[key_buffer].costume_index = costume_index_buffer;

        // Encrypt character ID.
        entries[key_buffer].encrypted_character_id = NUCC_Encrypt(character_id);

        // Convert colour data to floats.
        color.blue = get_offset_value<u8>(entry_start, 20);
        color.green = get_offset_value<u8>(entry_start, 16);
        color.red = get_offset_value<u8>(entry_start, 12);
        color.consolidate();
        result = (u64*)RGBA_Int_to_Float((float*)&entries[key_buffer].rgb_float, color.rgb | 0xFFu);
    }
    
    for (const auto& item : json_data.items()) {
        key_buffer = item.key();
        auto& json_rgb = json_data[key_buffer];
        if (json_rgb.type() == JSON::value_t::string) {
            character_id = (key_buffer.substr(0, 4) + "0" + key_buffer.at(5)).c_str();
            entries[key_buffer].encrypted_character_id = NUCC_Encrypt(character_id);
            entries[key_buffer].costume_index = key_buffer.at(4) - '0';
            color.hex_to_rgb(json_rgb);
            result = (u64*)RGBA_Int_to_Float((float*)&entries[key_buffer].rgb_float, color.rgb | 0xFFu);
        } else {
            JAPI_LogError("Invalid RGB input. Must be a valid 6-digit hex code.");
        }
    }

    // Load all data into game.
    for (const auto& [key, value] : entries) {
        buffer = static_cast<u128>(value.encrypted_character_id);
        buffer |= static_cast<u128>(value.costume_index) << 32;
        buffer_ptr = (u128*)a1[5];
        if (a1[6] == a1[5]) {
            result = (u64*)sub_47EB58(a1 + 1, buffer_ptr, &buffer);
            float* buffer_float = (float*)result;
            float* rgb_float_buffer = (float*)&value.rgb_float;
            buffer_float[4] = rgb_float_buffer[0];
            buffer_float[5] = rgb_float_buffer[1];
            buffer_float[6] = rgb_float_buffer[2];
            buffer_float[7] = rgb_float_buffer[3];
            JAPI_LogInfo(std::format("{} needed manual patching.", key));
        } else {
            *buffer_ptr = buffer;
            buffer_ptr[1] = value.rgb_float;
            a1[5] += 32;
        }
    }
    JAPI_LogDebug(std::format("Result: {:#010x}", (u64)result));
    return result;
}

typedef u64*(__fastcall* sub_47F250_t)(u64*, u64*, i32);
sub_47F250_t sub_47F250_original;

u64* __fastcall sub_47F250(u64* a1, u64* a2, i32 a3) {
    JAPI_LogInfo(std::format("a1 = {:#010x}\na2 = {:#010x}\n", (u64)a1, (u64)a2));
    return sub_47F250_original(a1, a2, a3);
}

Hook Parse_PlayerColorParam_hook = {
    (void*)0x47F114, // Address of the function we want to hook
    (void*)Parse_PlayerColorParam, // Address of our hook function
    (void**)&Parse_PlayerColorParam_original, // Address of the variable that will store the original function address
    "Parse_PlayerColorParam" // Name of the function we want to hook
};

Hook sub_47F250_hook = {
    (void*)0x47F250, // Address of the function we want to hook
    (void*)sub_47F250, // Address of our hook function
    (void**)&sub_47F250_original, // Address of the variable that will store the original function address
    "sub_47F250" // Name of the function we want to hook
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
    if(!JAPI_HookASBRFunction(&sub_47F250_hook))
        JAPI_LogError("Failed to hook sub_47F250!");

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
    for (const auto& entry : fs::directory_iterator(json_directory)) {
        if (entry.path().extension() == ".json" && entry.path().filename() != "_priority.json")
            if (priority_json[entry.path().filename().stem().string()] == nullptr)
                priority_json[entry.path().filename().stem().string()] = 0;
    }
    for (auto& item : priority_json.items()) {
        if (!fs::exists(json_directory / (item.key() + ".json")))
            priority_json.erase(item.key());
    }

    // Create new priority JSON file.
    std::ofstream priority_file(priority_path);
    priority_file << priority_json.dump(2);
    priority_file.close();

    // Merge JSON files together.
    std::map<int, std::vector<std::string>> priority_list;
    std::ifstream json_file_buffer;
    JSON json_buffer;
    std::string filename_buffer;
    for (auto& item : priority_json.items()) {
        priority_list[item.value()].push_back(item.key());
    }
    for (const auto& [key, value] : priority_list) {
        for (const auto& str : value) {
            filename_buffer = str + ".json";
            json_file_buffer.open(json_directory / filename_buffer);
            if (!json_file_buffer.is_open()) {
                JAPI_LogWarn(filename_buffer + " could not be opened.");
                continue;
            }

            try {
                json_buffer = JSON::parse(json_file_buffer);
                json_data.merge_patch(json_buffer);
            } catch (const JSON::parse_error& e) {
                JAPI_LogWarn(filename_buffer + ": " + e.what());
            }

            json_file_buffer.close();
        }
    }

    // JAPI_LogInfo(json_data.dump(2));
    JAPI_LogInfo("Loaded!");
    JAPI_LogWarn("This plugin will only work up until the July 2024 version of JoJoAPI.");
}