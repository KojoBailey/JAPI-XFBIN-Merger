#include "main.h"

static struct {
    std::string steam;
    std::string path3; // e.g. "eng", "jpn", "spa", etc.
} game_language;

int error_handler(nucc::Error e) {
    JERROR("\n\tError Code: {:03} - {}\n\t{}\n\tSuggestion: {}", e.number(), e.generic(), e.specific(), e.suggestion());
    return e.number();
}

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
        priority_json = JSON::parse(priority_file, nullptr, true, true);
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
                json_buffer = JSON::parse(json_file_buffer, nullptr, true, true);
                json_result.merge_patch(json_buffer);
            } catch (const JSON::parse_error& e) {
                JWARN("{}: {}", filename_buffer, e.what());
            }

            json_file_buffer.close();
        }
    }

    return json_result;
}

typedef std::uint64_t*(__fastcall* Get_Game_Language_t)(std::uint64_t*, unsigned int*);
Get_Game_Language_t Get_Game_Language_original;

std::uint64_t* __fastcall Get_Game_Language(std::uint64_t* a1, unsigned int* language_index) {
    switch (*language_index) {
        case 0: 
            game_language.steam = "japanese";
            game_language.path3 = "jpn";
            break;
        case 2:
            game_language.steam = "french";
            game_language.path3 = "fre";
            break;
        case 3:
            game_language.steam = "spanish";
            game_language.path3 = "spa";
            break;
        case 4:
            game_language.steam = "german";
            game_language.path3 = "ger";
            break;
        case 5:
            game_language.steam = "italian";
            game_language.path3 = "ita";
            break;
        case 9:
            game_language.steam = "koreana";
            game_language.path3 = "kor";
            break;
        case 10:
            game_language.steam = "tchinese";
            game_language.path3 = "cht";
            break;
        case 11:
            game_language.steam = "schinese";
            game_language.path3 = "chs";
            break;
        default: // English is 1
            game_language.steam = "english";
            game_language.path3 = "eng";
    }
    return Get_Game_Language_original(a1, language_index);
}

static nucc::Binary_Data* global_binary_data = nullptr;
static nucc::Binary_Data* global_messageInfo_data = nullptr;

typedef u64**(__fastcall* Get_Chunk_t)(u64*, const char*, nucc_hash_string*);
Get_Chunk_t Get_Chunk_original;

u64** Get_Chunk(u64* a1, const char* type, nucc_hash_string* name) {
    std::string name_str;
    if (name->string) name_str = name->string;
    u64** original_data = Get_Chunk_original(a1, type, name);

    if (name_str == "messageInfo") {
        global_messageInfo_data = (nucc::Binary_Data*) new nucc::ASBR::messageInfo{original_data[2]};
        nucc::ASBR::messageInfo* message_info = (nucc::ASBR::messageInfo*)global_messageInfo_data;
        JSON json_data = get_json_data("japi\\merging\\messageInfo\\" + game_language.path3);
        message_info->load(json_data);
        original_data[2] = message_info->write_to_bin();
    }

    return original_data;
}

typedef u64*(__fastcall* Load_nuccBinary_t)(const char*, const char*);
Load_nuccBinary_t Load_nuccBinary_original;

u64* __fastcall Load_nuccBinary(const char* xfbin_path, const char* chunk_name_buffer) {
    std::string chunk_name = chunk_name_buffer; // Buffer gets changed when original function is called.
    u64* original_data = Load_nuccBinary_original(xfbin_path, chunk_name_buffer);

    if (chunk_name == "SpeakingLineParam") {
        global_binary_data = (nucc::Binary_Data*) new nucc::ASBR::SpeakingLineParam{original_data};
        nucc::ASBR::SpeakingLineParam* speaking_line_param = (nucc::ASBR::SpeakingLineParam*)global_binary_data;
        JSON json_data = get_json_data("japi\\merging\\param\\battle\\SpeakingLineParam");
        speaking_line_param->load(json_data);
        return (u64*)speaking_line_param->write_to_bin();
    } else if (chunk_name == "MainModeParam") {
        global_binary_data = (nucc::Binary_Data*) new nucc::ASBR::MainModeParam{original_data};
        nucc::ASBR::MainModeParam* main_mode_param = (nucc::ASBR::MainModeParam*)global_binary_data;
        JSON json_data = get_json_data("japi\\merging\\param\\main_mode\\MainModeParam");
        main_mode_param->load(json_data, true);
        return (u64*)main_mode_param->write_to_bin();
    }

    return original_data;
}

typedef u64*(__fastcall* Parse_PlayerColorParam_t)(nucc_vector*);
Parse_PlayerColorParam_t Parse_PlayerColorParam_original;

u64* __fastcall Parse_PlayerColorParam(nucc_vector_handle* PlayerColorParam_storage) {
    // Load data from XFBIN and JSON.
    u64* result = Load_nuccBinary_original("data/param/battle/PlayerColorParam.bin.xfbin", "PlayerColorParam");
    nucc::ASBR::PlayerColorParam data{result};
    JSON json_buffer = get_json_data("japi\\merging\\param\\battle\\PlayerColorParam");
    data.load(json_buffer);

    // Load all data into game.
    PlayerColorParam_Game game_entry;
    u64* buffer_ptr;
    for (auto& [key, value] : data.entries) {
        game_entry.character_id_hash = NUCC_Hash(value.character_id.c_str());
        game_entry.costume_index = value.costume_index;
        result = (u64*)RGBA_Int_to_Float((float*)&game_entry.color, value.color.consolidate() | 0xFFu);
        buffer_ptr = (u64*)PlayerColorParam_storage->vector.position;
        if (PlayerColorParam_storage->vector.end == (char*)buffer_ptr) {
            result = Allocate_PlayerColorParam_Data(&PlayerColorParam_storage->vector, (PlayerColorParam_Game*)buffer_ptr, &game_entry);
        } else {
            *(PlayerColorParam_Game*)buffer_ptr = game_entry;
            PlayerColorParam_storage->vector.position += 32;
        }
    }

    return result;
}

Hook Get_Game_Language_hook = {
    (void*)0x6F1970,
    (void*)Get_Game_Language,
    (void**)&Get_Game_Language_original,
    "Get_Game_Language"
};

Hook Get_Chunk_hook = {
    (void*)0x6E3290, // Address of the function we want to hook
    (void*)Get_Chunk, // Address of our hook function
    (void**)&Get_Chunk_original, // Address of the variable that will store the original function address
    "Get_Chunk" // Name of the function we want to hook
};

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
    nucc::error_handler = error_handler;

    if (!JAPI_HookASBRFunction(&Get_Game_Language_hook))
        JERROR("Failed to hook function `{}`.", Get_Game_Language_hook.name);
    if (!JAPI_HookASBRFunction(&Get_Chunk_hook))
        JERROR("Failed to hook function `{}`.", Get_Chunk_hook.name);
    if (!JAPI_HookASBRFunction(&Load_nuccBinary_hook))
        JERROR("Failed to hook function `{}`.", Load_nuccBinary_hook.name);
    if (!JAPI_HookASBRFunction(&Parse_PlayerColorParam_hook))
        JERROR("Failed to hook function `{}`.", Parse_PlayerColorParam_hook.name);

    JINFO("Loaded!");
}