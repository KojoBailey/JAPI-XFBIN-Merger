#include "main.h"

static std::string game_language;
static nucc::ASBR::messageInfo* messageInfo_data;

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

typedef u64*(__fastcall* Get_Steam_Path_Info_t)(u64*, u64*, const char*);
Get_Steam_Path_Info_t Get_Steam_Path_Info_original;

u64* __fastcall Get_Steam_Path_Info(u64* a1, u64* a2, const char* a3) {
    game_language = a3;
    if (game_language.size() == 5)
        game_language = game_language.substr(1, 3);
    return Get_Steam_Path_Info_original(a1, a2, a3);
}

static nucc::Binary_Data* global_binary_data = nullptr;

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
        main_mode_param->load(json_data);
        return (u64*)main_mode_param->write_to_bin();
    }

    return original_data;
}

typedef u64*(__fastcall* Parse_PlayerColorParam_t)(u64*);
Parse_PlayerColorParam_t Parse_PlayerColorParam_original;

u64* __fastcall Parse_PlayerColorParam(u64* a1) {
    // Load data from XFBIN and JSON.
    u64* result = Load_nuccBinary_original("data/param/battle/PlayerColorParam.bin.xfbin", "PlayerColorParam");
    nucc::ASBR::PlayerColorParam player_color_param{result};
    JSON json_buffer = get_json_data("japi\\merging\\param\\battle\\PlayerColorParam");
    player_color_param.load(json_buffer);

    struct PlayerColorParam_Game {
        u32 character_id_hash;
        u32 costume_index;
        u64 padding;
        struct {
            float red, green, blue, alpha;
        } color;
    };

    // Load all data into game.
    PlayerColorParam_Game buffer;
    PlayerColorParam_Game* buffer_ptr;
    for (auto& [key, value] : player_color_param.entries) {
        buffer.character_id_hash = NUCC_Hash(value.character_id.c_str());
        buffer.costume_index = value.costume_index;
        result = (u64*)RGBA_Int_to_Float((float*)&buffer.color.red, value.color.consolidate() | 0xFFu);
        buffer_ptr = (PlayerColorParam_Game*)a1[5];
        if (a1[6] == a1[5]) {
            result = (u64*)sub_47EB58(a1 + 1, buffer_ptr, &buffer);
        } else {
            *buffer_ptr = buffer;
            a1[5] += 32;
        }
    }

    // live_data.load((char*)result - (32 * 316)); // Calculation to find data start.
    // JTRACE("Data stored at {}", (u64)live_data.data());
    return result;
}

typedef const char*(__fastcall* Fetch_String_from_ID_t)(u64*, const char*);
Fetch_String_from_ID_t Fetch_String_from_ID_original;

const char* __fastcall Fetch_String_from_ID(u64 a1, const char* string_id) {
    u32 string_hash = NUCC_Hash(string_id);
    const char* string_container = Fetch_String_from_Hash(a1, string_hash);

    if (string_container && messageInfo_data->entries.contains(string_hash)) {
        const char*& string = *((const char**)((u64*)string_container + 2));
        string = messageInfo_data->entries[string_hash].message.c_str();
    }

    return string_container;
}

// WIP (unused)
typedef u64*(__fastcall* Parse_messageInfo_t)(u64*, const char*, u64*, int);
Parse_messageInfo_t Parse_messageInfo_original;

u64* __fastcall Parse_messageInfo(u64* a1, const char* xfbin_path, u64* a3, int a4) {
    u64* result = (u64*)a1[6];
    u64* v6 = a1;
    u64* v7 = result;
    u64* v8 = (u64*)result[1];

    // IDK
    while (!*((u8*)v8 + 25)) {
        if (*((u8*)v8 + 36) && *((u8*)a3 + 4) && *((u32*)v8 + 8) < *((u32*)a3)) {
            v8 = (u64*)v8[2];
        } else {
            v7 = v8;
            v8 = (u64*)v8[2];
        }
    }

    // IDK
    if (v7 == result || *((u8*)a3 + 4) && *((u8*)v7 + 36) && *(u32*)a3 < *((u32*)v7 + 8)) {
        v7 = (u64*)a1[6];
    }

    nucc::ASBR::messageInfo* binary_data;
    if (v7 == result) {
        u64 qword_1AB8D10;
        JAPI_CopyASBRMem((void*)&qword_1AB8D10, (void*)0x1AB8D10, 8);
        result = Load_XFBIN_Data((u64)&qword_1AB8D10, xfbin_path);
        if (result) {
            result = Get_Chunk_Address(result, "nuccChunkBinary", a3);
            if (result) {
                binary_data = (nucc::ASBR::messageInfo*)result[2];
            }
        }
    }

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

Hook Fetch_String_from_ID_hook = {
    (void*)0x77B0E0, // Address of the function we want to hook
    (void*)Fetch_String_from_ID, // Address of our hook function
    (void**)&Fetch_String_from_ID_original, // Address of the variable that will store the original function address
    "Fetch_String_from_ID" // Name of the function we want to hook
};

Hook Parse_messageInfo_hook = {
    (void*)0x77A8D0, // Address of the function we want to hook
    (void*)Parse_messageInfo, // Address of our hook function
    (void**)&Parse_messageInfo_original, // Address of the variable that will store the original function address
    "Parse_messageInfo" // Name of the function we want to hook
};

Hook Get_Steam_Path_Info_hook = {
    (void*)0x70C9A0,
    (void*)Get_Steam_Path_Info,
    (void**)&Get_Steam_Path_Info_original,
    "Get_Steam_Path_Info"
};

// This function is called when the mod is loaded.
void __stdcall ModInit() {
    nucc::error_handler = error_handler;

    if (!JAPI_HookASBRFunction(&Get_Steam_Path_Info_hook))
        JERROR("Failed to hook function `{}`.", Get_Steam_Path_Info_hook.name);
    if (!JAPI_HookASBRFunction(&Load_nuccBinary_hook))
        JERROR("Failed to hook function `{}`.", Load_nuccBinary_hook.name);
    if (!JAPI_HookASBRFunction(&Parse_PlayerColorParam_hook))
        JERROR("Failed to hook function `{}`.", Parse_PlayerColorParam_hook.name);
    if (!JAPI_HookASBRFunction(&Fetch_String_from_ID_hook))
        JERROR("Failed to hook function `{}`.", Fetch_String_from_ID_hook.name);

    JAPI_LogTrace(game_language);
    JSON messageInfo_json = get_json_data("japi\\merging\\messageInfo\\eng");
    messageInfo_data = new nucc::ASBR::messageInfo;
    messageInfo_data->load(messageInfo_json);

    JINFO("Loaded!");
}