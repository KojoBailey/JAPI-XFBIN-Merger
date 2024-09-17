#include "main.h"

JSON get_json_data(std::filesystem::path directory) {
    // Create directory for JSON files if not already existing.
    if (!fs::exists(directory)) {
        JDEBUG("Attempting to create directory at: `{}`", directory.string());
        fs::create_directories(directory);
    }
    if (!fs::exists(directory)) JFATAL("Failed to create directory at:\n{}", directory.string());

    fs::path priority_path{directory / "_priority.json"};
    JSON priority_json;

    // Read existing priority data, if it exists.
    if (fs::exists(priority_path)) {
        std::ifstream priority_file(priority_path);
        priority_json = JSON::parse(priority_file);
        priority_file.close();
    }

    // Refresh priority data.
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.path().extension() == ".json" && entry.path().filename() != "_priority.json")
            if (priority_json[entry.path().filename().stem().string()] == nullptr)
                priority_json[entry.path().filename().stem().string()] = 0;
    }
    for (auto& item : priority_json.items()) {
        if (!fs::exists(directory / (item.key() + ".json")))
            priority_json.erase(item.key());
    }

    // Create new priority JSON file.
    std::ofstream priority_file(priority_path);
    priority_file << priority_json.dump(2);
    priority_file.close();

    // Merge JSON files together.
    JSON json_result;
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
            json_file_buffer.open(directory / filename_buffer);
            if (!json_file_buffer.is_open()) {
                JWARN("{} could not be opened.", filename_buffer);
                continue;
            }

            try {
                json_buffer = JSON::parse(json_file_buffer);
                json_result.merge_patch(json_buffer);
            } catch (const JSON::parse_error& e) {
                JWARN("{}: {}", filename_buffer, e.what());
            }

            json_file_buffer.close();
        }
    }

    return json_result;
}

/* User API */
static kojo::binary live_data;

static nucc::Binary_Data* global_binary_data = nullptr;

typedef u64*(__fastcall* Load_nuccBinary_t)(const char*, const char*);
Load_nuccBinary_t Load_nuccBinary_original;

u64* __fastcall Load_nuccBinary(const char* xfbin_path, const char* chunk_name_buffer) {
    std::string chunk_name = chunk_name_buffer; // Buffer gets changed when original function is called.
    u64* original_data = Load_nuccBinary_original(xfbin_path, chunk_name_buffer);
    if (chunk_name == "SpeakingLineParam") {
        global_binary_data = (nucc::Binary_Data*) new nucc::ASBR::SpeakingLineParam{original_data};

        nucc::ASBR::SpeakingLineParam* speaking_line_param = (nucc::ASBR::SpeakingLineParam*)global_binary_data;

        JSON json_buffer = get_json_data("japi\\merging\\param\\battle\\SpeakingLineParam");
        speaking_line_param->load(json_buffer);

        JAPI_LogInfo(speaking_line_param->write_to_json());
        
        return (u64*)speaking_line_param->write_to_bin();
    }
    return original_data;
}

typedef u64*(__fastcall* Parse_PlayerColorParam_t)(u64*);
Parse_PlayerColorParam_t Parse_PlayerColorParam_original;

u64* __fastcall Parse_PlayerColorParam(u64* a1) {
    // Load data from XFBIN and JSON.
    u64* result = Load_nuccBinary("data/param/battle/PlayerColorParam.bin.xfbin", "PlayerColorParam");
    nucc::ASBR::PlayerColorParam player_color_param{result};
    JSON json_buffer = get_json_data("japi\\merging\\param\\battle\\PlayerColorParam");
    player_color_param.load(json_buffer);

    // Load all data into game.
    u128 buffer;
    u128* buffer_ptr;
    i128 rgba_float_buffer;
    for (auto& [key, value] : player_color_param.entries) {
        buffer = static_cast<u128>(NUCC_Encrypt(value.character_id.c_str()));
        buffer |= static_cast<u128>(value.costume_index) << 32;
        value.color.consolidate();
        result = (u64*)RGBA_Int_to_Float((float*)&rgba_float_buffer, value.color.rgb | 0xFFu);
        buffer_ptr = (u128*)a1[5];
        if (a1[6] == a1[5]) {
            result = (u64*)sub_47EB58(a1 + 1, buffer_ptr, &buffer);
            ((float*)result)[4] = ((float*)&rgba_float_buffer)[0];
            ((float*)result)[5] = ((float*)&rgba_float_buffer)[1];
            ((float*)result)[6] = ((float*)&rgba_float_buffer)[2];
            ((float*)result)[7] = ((float*)&rgba_float_buffer)[3];
            JTRACE("{} needed manual patching.", key);
        } else {
            *buffer_ptr = buffer;
            buffer_ptr[1] = rgba_float_buffer;
            a1[5] += 32;
        }
    }

    live_data.load((char*)result - (32 * 316)); // Calculation to find data start.
    JTRACE("Data stored at {}", (u64)live_data.data());
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

    JINFO("Loaded!");
}