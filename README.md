
# [JoJoAPI XFBIN Merger](https://github.com/KojoBailey/JAPI-XFBIN-Merger/)
This is a plugin for [JoJoAPI](https://jojomodding.miraheze.org/wiki/JoJoAPI) that allows for the merging of XFBIN data, currently only via JSON, although as more file support is added, other formats will be used as well.

## How do I merge?
Right now, you can only merge some nuccChunkBinary formats. These are:
- [PlayerColorParam](#PlayerColorParam)
- [SpeakingLineParam](#SpeakingLineParam)
- [MainModeParam](#MainModeParam)

Many more are to come, so sit tight!

Creating merge files is quite simple, as you just need to write the data you want to target in a JSON file. See the [Formats](#Formats) section for how to structure those correctly. You will also see a **Directory** under each section, indicating where those JSON files should go, like so:

![image](https://github.com/user-attachments/assets/00d952f8-2ade-483b-845d-b25019c071c8)

`_priority.json` simply controls which files get loaded in which order, and will update itself to add/remove new/old JSON files whenever you launch the game (and this activate the plugin). A **higher** priority number means that file takes **higher** precedence over others lower than it, since it will get loaded later and therefore overwrite the data that came before it.

## Formats
This section will serve as a reference for how this plugin handles the different formats. Make sure to follow whatever rules are outlined below if you want to avoid errors.

First, some important JSON terminology:
- **Key** → The name of a variable.
- **Field** → A key-value pair.
```json
{
    "This is a key" : "This is a value"
}
```
- **Array** → A collection of values under one key.
```json
{
    "Some Numbers" : [
        1,
        69,
        23
    ]
}
```
- **Object** → A collection of key-value pairs.
```json
{
    "Character HP" : {
        "Jonathan Joestar": 1000,
        "Dio Brando": 950,
        "Bruno Bucciarati": 1100
    }
}
```

And some quick yet **important** general notes:
- In most cases, the strings are **case sensitive**. And be weary of typos! You will get an error from JAPI if you enter an invalid value.
- For characters, you can **either** use their name (e.g. Jotaro, Jotaro Kujo, Noriaki, etc.) or ASBR ID (e.g. `1jnt01`, `3jtr01`, `7dio02`, etc).

### PlayerColorParam
**Directory:** `japi/merging/param/battle/PlayerColorParam`

Each JSON **key** is the character's model code (e.g. `5grn01`) followed by their `col` ID, much like you'd find in `data/spc` for the game.

The hex codes are in the format `#RRGGBB` (see [this site](https://www.rapidtables.com/web/color/RGB_Color.html) for more info if you aren't already familiar).

```json
{
    "1jnt31col1" : "#FF0000",
    "5grn01col0" : "#FF0000",
    "5grn01col1" : "#DA8A00",
    "5grn01col2" : "#D6DA00",
    "5grn01col3" : "#1DDA00",
    "5grn11col0" : "#00DADA",
    "5grn11col1" : "#0700DA",
    "5grn21col0" : "#9900DA",
    "5grn21col1" : "#DA0091"
}
```

Whatever you choose to edit the JSON data with, it's recommended you use something that adds a colour picker over the hex codes, like Visual Studio Code with the [json-color-token](https://marketplace.visualstudio.com/items?itemName=yechunan.json-color-token) extension.

### SpeakingLineParam
**Directory:** `japi/merging/param/battle/SpeakingLineParam`

The key isn't actually used so the format doesn't strictly matter, but it's better to keep a consistent format for the sake of helping others read your stuff.

Maybe in the future, this format will be updated to use the key instead.

The **Interaction Type** *must* be one of the following:
- `"Battle Start"`
- `"Round Win"`
- `"Battle Win"`

```json
{
    "3dio01 vs 3jtr01 - Battle Start": {
        "Interaction Type": "Battle Start",
        "Character 1": {
            "Character": "DIO",
            "Dialogue": "3dio01_btlst_00_3abd01"
        },
        "Character 2": {
            "Character": "Jotaro",
            "Dialogue": "3jtr01_btlst_00_3abd01"
        }
    }
}
```

### MainModeParam
**Directory:** `japi/merging/param/main_mode/MainModeParam`

This one has a lot of parameters, but not all of them are necessary!

The key is important, so make sure it is the panel ID you want to target.

The following fields can be auto-calculated and therefore are not necessary unless you want a custom definition:
- **Index** → Based on entry key (e.g. `"PANEL_04_03"` will produce `4 * 3` which equals `12`)
- **Page** → Based on entry key (e.g. `"PANEL_04_03"` will produce `4`)
- **Boss Panel** → `"PANEL_XX_08"` based on entry key (e.g. `"PANEL_04_03"` will produce `"PANEL_04_08"`)
- **CPU Level** → Matches **Difficulty**

The following fields have default values (blank/nothing) if you do not define them at all (not even `""`).:
- **Adjacent Panels**
- **Type** → `"Normal"` by default
- **Player/Enemy Assist**
- **Special Rules object**
- **Special Rule** (individually)
- **Secret Mission Reward**

There are literally hundreds of possible values for the Special Rules and Secret Mission Condittions, so I'll add a full list of them later once I've had the chance to organise this documentation a bit better. In the meantime, you can check the source code for [MainModeParam.hpp](https://github.com/KojoBailey/nucc-cpp-library/blob/main/include/nucc/chunks/binary/asbr/MainModeParam.hpp).

```json
{
    "PANEL_01_01" : {
        "Index": 1,
        "Part": 1,
        "Page": 1,
        "Boss Panel": "PANEL_01_08",
        "Adjacent Panels": {
            "Up": "",
            "Down": "PANEL_01_02",
            "Left": "",
            "Right": "PANEL_01_08"
        },
        "Type": "Extra",
        "Stars": 3,
        "CPU Level": 3,
        "Gold Reward": 1000,
        "Stage": "Dio's Castle",
        "Player Information": {
            "Character": "Dio Brando",
            "Assist": "",
            "Start Dialogue": "1dio01_story_btlst_2jsp01_00",
            "Win Dialogue": "1dio01_story_btlwin_2jsp01_00" 
        },
        "Enemy Information": {
            "Character": "Joseph",
            "Assist": "Caesar",
            "Start Dialogue": "2jsp01_story_btlst_1dio01_00",
            "Win Dialogue": "2jsp01_story_btlwin_1dio01_00" 
        },
        "First To Speak": "Player",
        "Special Rules": {
            "Rule 1": "OPP_HEALTH_REFILL",
            "Rule 2": "",
            "Rule 3": "",
            "Rule 4": ""
        },
        "Secret Missions": {
            "Mission 1": {
                "Condition": "TAUNT",
                "Reward": "",
                "Gold Reward": 1000
            },
            "Mission 2": {
                "Condition": "USE_MERE_DOG_1DIO01",
                "Reward": "ID_ART_149",
                "Gold Reward": 250
            },
            "Mission 3": {
                "Condition": "LAND_VAPOR_FREEZE_1DIO01",
                "Reward": "CCD_CUSTOM_CARD_ID_27",
                "Gold Reward": 500
            }
        }
    }
}
```

Here's how it'd look without defining unnecessary things:

```json
{
    "PANEL_01_01" : {
        "Part": 1,
        "Adjacent Panels": {
            "Down": "PANEL_01_02",
            "Right": "PANEL_01_08"
        },
        "Type": "Extra",
        "Stars": 3,
        "Gold Reward": 1000,
        "Stage": "Dio's Castle",
        "Player Information": {
            "Character": "Dio Brando",
            "Start Dialogue": "1dio01_story_btlst_2jsp01_00",
            "Win Dialogue": "1dio01_story_btlwin_2jsp01_00" 
        },
        "Enemy Information": {
            "Character": "Joseph",
            "Assist": "Caesar",
            "Start Dialogue": "2jsp01_story_btlst_1dio01_00",
            "Win Dialogue": "2jsp01_story_btlwin_1dio01_00" 
        },
        "Special Rules": {
            "Rule 1": "OPP_HEALTH_REFILL"
        },
        "Secret Missions": {
            "Mission 1": {
                "Condition": "TAUNT",
                "Gold Reward": 1000
            },
            "Mission 2": {
                "Condition": "USE_MERE_DOG_1DIO01",
                "Reward": "ID_ART_149",
                "Gold Reward": 250
            },
            "Mission 3": {
                "Condition": "LAND_VAPOR_FREEZE_1DIO01",
                "Reward": "CCD_CUSTOM_CARD_ID_27",
                "Gold Reward": 500
            }
        }
    }
}
```
