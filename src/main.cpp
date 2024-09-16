#include "main.h"

fs::path json_directory{"japi\\merging\\param\\battle\\PlayerColorParam"};
JSON json_data;

struct RGB {
    u32 red, green, blue, rgb;

    void consolidate() {
        rgb = (blue | ((green | (red << 8)) << 8)) << 8;
    }

    RGB hex_to_rgb(std::string hex_str) {
        std::erase(hex_str, '#');
        std::stringstream buffer;
        buffer << std::hex << hex_str.substr(0, 2);
        buffer >> red;
        buffer.clear();
        buffer << std::hex << hex_str.substr(2, 2);
        buffer >> green;
        buffer.clear();
        buffer << std::hex << hex_str.substr(4, 2);
        buffer >> blue;
        this->consolidate();
        return *this;
    }
};

/* User API */
static kojo::binary live_data;

typedef u64*(__fastcall* Load_nuccBinary_t)(const char*, const char*);
Load_nuccBinary_t Load_nuccBinary_original;

u64* __fastcall Load_nuccBinary(const char* xfbin_path, const char* chunk_name_buffer) {
    std::string chunk_name = chunk_name_buffer; // Buffer gets changed when original function is called.
    u64* original_data = Load_nuccBinary_original(xfbin_path, chunk_name_buffer);
    if (chunk_name == "SpeakingLineParam") {
        // nucc::ASBR::SpeakingLineParam speaking_line_param{binary_data.data()};
    }
    return original_data;
}

typedef u64*(__fastcall* Parse_PlayerColorParam_t)(u64*);
Parse_PlayerColorParam_t Parse_PlayerColorParam_original;

u64* __fastcall Parse_PlayerColorParam(u64* a1) {
    u64* result;            // To be returned at end of function.
    std::string key_buffer;
    u128 buffer;
    u128* buffer_ptr;
    std::unordered_map<std::string, int> tint_tracker;

    struct Input_Data {
        kojo::binary data;          // Data from XFBIN chunk, exactly as-is.
        u32 entry_count;            // Number of entries.
        u64 note_pointer;           // Initial pointer, pointing to start of entries. Can be used to skip over notes.
        u64 character_id_pointer;   // Pointer to character ID later in the data.
        std::string character_id;   // Character's ID (e.g. "1jnt01").
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
        nucc::ASBR::PlayerColorParam player_color_param{
            Load_nuccBinary("data/param/battle/PlayerColorParam.bin.xfbin", "PlayerColorParam")
        };
        JAPI_LogInfo(player_color_param.write_to_json());
        
        input.data.load(Load_nuccBinary("data/param/battle/PlayerColorParam.bin.xfbin", "PlayerColorParam"));
        if (!input.data.data()) {
            JERROR("`PlayerColorParam.bin.xfbin` data could not be loaded.");
            return 0;
        }
        JTRACE("Loaded PlayerColorParam data from XFBIN.");

        input.data.change_pos(4); // Version (should be 1000)
        input.entry_count = input.data.read<u32>(kojo::endian::little);
        JTRACE("Entry Count from XFBIN: {}", input.entry_count);
        input.note_pointer = input.data.read<u64>(kojo::endian::little); // 8 for all vanilla files.
        if (input.note_pointer == 0) {
            JERROR("`PlayerColorParam.bin.xfbin` is missing an initial pointer.");
            return 0;
        }

        // Iterate through each entry.
        for (int i = 0; i < input.entry_count; i++) {
            // Get character ID from pointer.
            input.character_id_pointer = input.data.read<u64>(kojo::endian::little);
            if (input.character_id_pointer != 0)
                input.character_id = input.data.read<string>(0, input.character_id_pointer - 8);
            else
                input.character_id = "";

            // Get costume index and alt ID.
            input.costume_index = input.data.read<u32>(kojo::endian::little);
            key_buffer = input.character_id;
            key_buffer.replace(4, 1, std::to_string(input.costume_index));
            key_buffer += "col" + std::to_string(tint_tracker[key_buffer]++);
            entries[key_buffer].costume_index = input.costume_index;

            // Encrypt character ID.
            entries[key_buffer].encrypted_character_id = NUCC_Encrypt(input.character_id.c_str());

            // Convert colour data to floats.
            input.color.red = input.data.read<u32>(kojo::endian::little);
            input.color.green = input.data.read<u32>(kojo::endian::little);
            input.color.blue = input.data.read<u32>(kojo::endian::little);
            input.color.consolidate();
            result = (u64*)RGBA_Int_to_Float((float*)&entries[key_buffer].rgba_float, input.color.rgb | 0xFFu);
        }
    
    // Load data from JSON files.
        for (const auto& item : json_data.items()) {
            key_buffer = item.key();
            auto& json_rgb = json_data[key_buffer];
            if (json_rgb.type() == JSON::value_t::string) {
                input.character_id = key_buffer.substr(0, 4) + "0" + key_buffer.at(5);
                entries[key_buffer].encrypted_character_id = NUCC_Encrypt(input.character_id.c_str());
                entries[key_buffer].costume_index = key_buffer.at(4) - '0';
                input.color.hex_to_rgb(json_rgb);
                result = (u64*)RGBA_Int_to_Float((float*)&entries[key_buffer].rgba_float, input.color.rgb | 0xFFu);
            } else {
                JERROR("Invalid RGB input. Must be a valid 6-digit hex code.");
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
                JTRACE("{} needed manual patching.", key);
            } else {
                *buffer_ptr = buffer;
                buffer_ptr[1] = value.rgba_float;
                a1[5] += 32;
            }
        }

    live_data.load((char*)result - (32 * 316)); // Calculation to find data start.
    JTRACE("Data stored at {}", (u64)live_data.data());
    /* TO DO: Calculate final entry count */

    return result;
}

Hook Load_nuccBinary_hook = {
    (void*)0x671C30, // Address of the function we want to hook
    (void*)Load_nuccBinary, // Address of our hook function
    (void**)&Load_nuccBinary_original, // Address of the variable that will store the original function address
    "Load_nuccBinary" // Name of the function we want to hook
};

Hook Parse_PlayerColorParam_hook = {
    (void*)0x47F114, // Address of the function we want to hook
    (void*)Parse_PlayerColorParam, // Address of our hook function
    (void**)&Parse_PlayerColorParam_original, // Address of the variable that will store the original function address
    "Parse_PlayerColorParam" // Name of the function we want to hook
};

// This function is called when the mod is loaded.
void __stdcall ModInit() {
    if (!JAPI_HookASBRFunction(&Parse_PlayerColorParam_hook))
        JERROR("Failed to hook function `{}`.", Parse_PlayerColorParam_hook.name);
    if (!JAPI_HookASBRFunction(&Load_nuccBinary_hook))
        JERROR("Failed to hook function `{}`.", Load_nuccBinary_hook.name);

    // Create directory for JSON files if not already existing.
    if (!fs::exists(json_directory)) {
        JDEBUG("Attempting to create directory...");
        fs::create_directories(json_directory);
    }
    if (!fs::exists(json_directory)) JFATAL("Failed to create directory at:\n{}", json_directory.string());

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
                JWARN("{} could not be opened.", filename_buffer);
                continue;
            }

            try {
                json_buffer = JSON::parse(json_file_buffer);
                json_data.merge_patch(json_buffer);
            } catch (const JSON::parse_error& e) {
                JWARN("{}: {}", filename_buffer, e.what());
            }

            json_file_buffer.close();
        }
    }

    JINFO("Loaded!");
}