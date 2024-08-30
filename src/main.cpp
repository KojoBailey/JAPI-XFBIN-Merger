#include "main.h"

u64* __fastcall Parse_PlayerColorParam(u64* a1) {
    u64* result;            // To be returned at end of function.
    std::string key_buffer;
    u128 buffer;
    u128* buffer_ptr;
    std::unordered_map<std::string, int> tint_tracker;

    struct Input_Data {
        kojo::binary data;       // Data from XFBIN chunk, exactly as-is.
        u32 entry_count;            // Number of entries.
        u64 note_pointer;          // Initial pointer, pointing to start of entries. Can be used to skip over notes.
        u64* entries_start;         // Start of all entries.
        u64* entry_start;           // Start of one entry.
        u64* character_id_pointer;  // Pointer to character ID later in the data.
        const char* character_id;   // Character's ID (e.g. "1jnt01").
        RGB color;
        u32 costume_index;
    } input;

    struct Entry {
        u32 encrypted_character_id; // Character ID encrypted by CRC32.
        u32 costume_index;          // Costume number (e.g. 3 = Special C).
        i128 rgba_float;            // RGBA value as floats.
    };  // All data belonging to only one entry.
    std::map<std::string, Entry> entries;

    // Load data from XFBIN file.
        input.data.load(Load_nuccBinary("data/param/battle/PlayerColorParam.bin.xfbin", "PlayerColorParam"));
        if (!input.data.data) {
            JAPI_LogError("`PlayerColorParam.bin.xfbin` data could not be loaded.");
            return 0;
        }

        input.data.move(4);
        input.entry_count = input.data.read<u32>(std::endian::big);
        input.note_pointer = input.data.read<u64>(std::endian::big); // 8 for all vanilla files.
        if (!input.note_pointer) {
            JAPI_LogError("`PlayerColorParam.bin.xfbin` is missing an initial pointer.");
            return 0;
        }

        // Iterate through each entry.
        input.entries_start = get_offset_ptr<u64>(&input.note_pointer, input.note_pointer);
        for (int i = 0; i < input.entry_count; i++) {
            input.entry_start = get_offset_ptr<u64>(input.entries_start, 24 * i);

            // Get character ID from pointer.
            input.character_id_pointer = input.entry_start;
            if (*input.entry_start) {
                input.character_id = get_offset_ptr<const char>(input.character_id_pointer, *input.character_id_pointer);
            } else {
                input.character_id = 0;
                continue;
            }

            // Get costume index and alt ID.
            input.costume_index = get_offset_value<u32>(input.entry_start, 8);
            key_buffer = input.character_id;
            key_buffer.replace(4, 1, std::to_string(input.costume_index));
            key_buffer += "col" + std::to_string(tint_tracker[key_buffer]++);
            entries[key_buffer].costume_index = input.costume_index;

            // Encrypt character ID.
            entries[key_buffer].encrypted_character_id = NUCC_Encrypt(input.character_id);

            // Convert colour data to floats.
            input.color.blue = get_offset_value<u8>(input.entry_start, 20);
            input.color.green = get_offset_value<u8>(input.entry_start, 16);
            input.color.red = get_offset_value<u8>(input.entry_start, 12);
            input.color.consolidate();
            result = (u64*)RGBA_Int_to_Float((float*)&entries[key_buffer].rgba_float, input.color.rgb | 0xFFu);
        }
    
    // Load data from JSON files.
        for (const auto& item : json_data.items()) {
            key_buffer = item.key();
            auto& json_rgb = json_data[key_buffer];
            if (json_rgb.type() == JSON::value_t::string) {
                input.character_id = (key_buffer.substr(0, 4) + "0" + key_buffer.at(5)).c_str();
                entries[key_buffer].encrypted_character_id = NUCC_Encrypt(input.character_id);
                entries[key_buffer].costume_index = key_buffer.at(4) - '0';
                input.color.hex_to_rgb(json_rgb);
                result = (u64*)RGBA_Int_to_Float((float*)&entries[key_buffer].rgba_float, input.color.rgb | 0xFFu);
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
                ((float*)result)[4] = ((float*)&value.rgba_float)[0];
                ((float*)result)[5] = ((float*)&value.rgba_float)[1];
                ((float*)result)[6] = ((float*)&value.rgba_float)[2];
                ((float*)result)[7] = ((float*)&value.rgba_float)[3];
                // JAPI_LogDebug(std::format("{} needed manual patching.", key));
            } else {
                *buffer_ptr = buffer;
                buffer_ptr[1] = value.rgba_float;
                a1[5] += 32;
            }
        }

    // JAPI_LogDebug(std::format("Result: {:#010x}", (u64)result));
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
    if (!JAPI_HookASBRFunction(&Parse_PlayerColorParam_hook))
        JAPI_LogError("Failed to hook function `Parse_PlayerColorParam`.");
    if (!JAPI_HookASBRFunction(&Load_nuccBinary_hook))
        JAPI_LogError("Failed to hook function `Load_nuccBinary`.");
    if (!JAPI_HookASBRFunction(&NUCC_Encrypt_hook))
        JAPI_LogError("Failed to hook function `NUCC_Encrypt`.");
    if (!JAPI_HookASBRFunction(&RGBA_Int_to_Float_hook))
        JAPI_LogError("Failed to hook function `RGBA_Int_to_Float`.");
    if (!JAPI_HookASBRFunction(&sub_47EB58_hook))
        JAPI_LogError("Failed to hook function `sub_47EB58`.");

    // Create directory for JSON files if not already existing.
    if (!fs::exists(json_directory)) {
        JAPI_LogDebug("Attempting to create directory...");
        fs::create_directories(json_directory);
    }
    if (!fs::exists(json_directory)) JAPI_LogFatal("Failed to create directory at:\n" + json_directory.string());

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

    JAPI_LogInfo("Loaded!");
}