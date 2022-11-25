#include "paprika_types.h"

#ifdef INTERNAL_BUILD
#define IM_VEC2_CLASS_EXTRA                                                     \
        constexpr ImVec2(const Vector2& v) : x(v.x), y(v.y) {}                  \
        operator Vector2() const { return {x,y}; }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_tables.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"

#include "implot/implot.h"
#include "implot/implot_internal.h"
#include "implot/implot.cpp"
#include "implot/implot_items.cpp"
#pragma clang diagnostic pop
#endif

#include "paprika.h"
#include "paprika_imgui.cpp"
#include "saltybet.cpp"


internal char *VERSION_STR = "paprikabot2 R1";


internal inline Node_System_ID
GetActiveNodeSystemID(Node_Systems *systems, Salty_Bet_Mode gamemode)
{
    Node_System_ID result;
    if (gamemode == SaltyBetMode_Tournament)
        result = systems->tournament_id;
    else if (gamemode == SaltyBetMode_Exhibition)
        result = systems->exhibition_id;
    else
        result = systems->matchmaking_id;
    return result;
}

internal inline Node_System *
GetActiveNodeSystem(Node_Systems *systems, Salty_Bet_Mode gamemode)
{
    return GetNodeSystem(systems, GetActiveNodeSystemID(systems, gamemode));
}

internal Character *
GetCharacter(Characters *characters, Character_ID character_id)
{
    Assert(character_id < characters->count);

    Character *result = (Character *)characters->chr_arena.base + character_id;
    return result;
}

internal inline void
MatchSystemReset(Match_System *mm)
{
    GrowableArenaReset(&mm->characters.chr_arena);
    GrowableArenaReset(&mm->characters.map_arena);
    GrowableArenaReset(&mm->characters.str_arena);

    CharactersMapInit(&mm->characters);

    GrowableArenaReset(&mm->matches.arena);

    mm->characters.count = 0;
    mm->matches.count = 0;
}

internal char *
GetCharacterName(Characters *characters, Arena_Offset name_offset)
{
    return GrowableArenaGetString(&characters->str_arena, name_offset);
}

internal char *
GetCharacterName(Characters *characters, Character_ID character_id)
{
    return GetCharacterName(characters, GetCharacter(characters, character_id)->name_offset);
}

internal Rank
InitialRank(Skill_Env skill_env)
{
    return {skill_env.initial_rank, skill_env.initial_uncertainty};
}

internal Character *
AddCharacter(Characters *characters, char *name)
{
    Character *result = GrowableArenaAllocStructAndGet(&characters->chr_arena, Character);

    result->id = (u16)characters->count++;
    result->wins = 0;
    result->losses = 0;
    result->rank = InitialRank(characters->skill_env);
    result->name_offset = GrowableArenaAllocString(&characters->str_arena, name);

    return result;
}

internal Character *
GetOrAddCharacter(Characters *characters, char *name, bool32 force_add)
{
    u32 hash = 0;
    char *read = name;
    while (*read)
    {
        hash = hash * 33 ^ *read++;
    }
    u32 index = hash % ArrayCount(characters->map);

    Character_Map_Entry *node = characters->map + index;
    while (node->index != MAP_INVALID && (
               force_add ||
               node->hash != hash ||
               !StringCmp(name, GetCharacterName(characters, node->index))))
    {
        if (node->next_offset)
        {
            node = GrowableArenaGetStruct(&characters->map_arena, Character_Map_Entry, node->next_offset);
        }
        else
        {
            node->next_offset = GrowableArenaAllocStruct(&characters->map_arena, Character_Map_Entry);
            node = GrowableArenaGetStruct(&characters->map_arena, Character_Map_Entry, node->next_offset);
            node->index = MAP_INVALID;
        }
    }

    if (node->index == MAP_INVALID)
    {
        node->hash = hash;
        node->index = AddCharacter(characters, name)->id;
        node->next_offset = 0;
    }

    Character *result = GetCharacter(characters, node->index);
    return result;
}

internal Match *
GetMatch(Matches *matches, Match_ID index)
{
    Assert(index < matches->count);

    Match *result = (Match *)matches->arena.base + index;
    return result;
}

internal Match_ID
AddMatch(Matches *matches, Match match)
{
    *GrowableArenaAllocStructAndGet(&matches->arena, Match) = match;
    return matches->count++;
}

internal bool32
LoadLegacyMatches(Paprika_Platform *platform, Match_System *mm)
{
    Read_File_Result char_file = platform->ReadFile("chars.idx");
    Read_File_Result match_file = platform->ReadFile("matches.pmh");

    if (char_file.size && match_file.size)
    {
        {
            char *read = (char *)char_file.data;
            char *end = read + char_file.size;
            char *to_add = read;
            while (read < end)
            {
                if (*read == '\r')
                {
                    *read = '\0';
                }
                else if (*read == '\n')
                {
                    *read = '\0';
                    GetOrAddCharacter(&mm->characters, to_add, true);
                    to_add = read + 1;
                }

                ++read;
            }
        }

        if (match_file.size % sizeof(Legacy_Match) == 0)
        {
            mm->matches.count = (u32)(match_file.size / sizeof(Legacy_Match));
            Legacy_Match *legacy_match = (Legacy_Match *)match_file.data;
            Legacy_Match *end = legacy_match + mm->matches.count;
            while (legacy_match < end)
            {
                Match match = {};
                match.player1id = legacy_match->player1id;
                match.player2id = legacy_match->player2id;
                match.winner = legacy_match->winner;
                match.timestamp = legacy_match->timestamp;

                match.gamemode = SaltyBetMode_Matchmaking;

                *GrowableArenaAllocStructAndGet(&mm->matches.arena, Match) = match;

                ++legacy_match;
            }
        }

        LogWrite(platform, "Loaded legacy match data.");
    }
    else
        LogWrite(platform, "Unable to load legacy match data.");

    if (char_file.size)
        platform->MemFree(char_file.data);

    if (match_file.size)
        platform->MemFree(match_file.data);

    return (char_file.size && match_file.size);
}

internal bool32
ConvertLegacyMatchWagers(Paprika_Platform *platform)
{
    Read_File_Result read_file = platform->ReadFile("bets.pbh");
    Write_File_Result write_file = {};

    if (read_file.size)
    {
        write_file = platform->WriteFile("wager_history.pwh");
        if (write_file.handle)
        {
            Legacy_Match_Wager *wager = (Legacy_Match_Wager *)read_file.data;
            Legacy_Match_Wager *end = (Legacy_Match_Wager *)(read_file.data + read_file.size);
            while (wager < end)
            {
                Match_Wager match_wager = {};
                match_wager.timestamp = wager->timestamp;
                match_wager.wager = wager->wager;
                match_wager.open_balance = wager->open_balance;
                match_wager.close_balance = wager->close_balance;

                platform->FileWrite(&match_wager, sizeof(match_wager), 1, write_file.handle);

                ++wager;
            }

            platform->FileClose(write_file.handle);

            LogWrite(platform, "Converted legacy wager data.");
        }
        else
        {
            LogWrite(platform, "Unable to open wager history file for writing.");
        }

        platform->MemFree(read_file.data);
    }
    else
        LogWrite(platform, "Unable to load legacy wager data.");

    return write_file.handle != 0;
}

internal u32
LoadMatchWagers(Paprika_Platform *platform, Matchup_History *history, Matches *matches)
{
    u32 loaded_count = 0;

    Read_File_Result file = platform->ReadFile("wager_history.pwh");
    if (file.size)
    {
        Assert(file.size % sizeof(Match_Wager) == 0);

        mem_t wager_count = file.size / sizeof(Match_Wager);
        Match_Wager *wager = (Match_Wager *)file.data;
        Match_Wager *end = wager + wager_count;

        Matchup_Frame *frames[ArrayCount(history->matchup_frames)];
        CircularBufferPtrCopy((void **)frames, history->matchup_frames, sizeof(history->matchup_frames[0]), history->count, ArrayCount(frames), history->write_index);

        for (mem_t i = 0; i < history->count; ++i)
        {
            Matchup_Frame *frame = frames[i];
            Match *match = GetMatch(matches, frame->match_id);

            while (wager < end && wager->timestamp < match->timestamp)
                ++wager;
            
            if (wager < end && wager->timestamp == match->timestamp)
            {
                frame->wager = *wager;

                frame->comparison.balance = wager->open_balance;

                ++loaded_count;
            }
        }

        platform->MemFree(file.data);

        LogWrite(platform, "Loaded match wager data.");
    }
    else
        LogWrite(platform, "Unable to load match wager data.");

    return loaded_count;
}

internal bool32
SaveMatchWager(Paprika_Platform *platform, Match_Wager match_wager)
{
    Write_File_Result file = platform->AppendFile("wager_history.pwh");
    if (file.handle)
    {
        platform->FileWrite(&match_wager, sizeof(match_wager), 1, file.handle);
        platform->FileClose(file.handle);
    }
    else
    {
        LogWrite(platform, "Unable to save match wager.");
    }

    return (bool32)file.handle;
}

internal bool32
LoadSaltyBetBotMatches(Paprika_Platform *platform, Match_System *mm)
{
    bool32 any_success = false;
    Json_Reader reader = {};

    char filename[32];
    for (i32 i = 0; i < 32; ++i)
    {
        sprintf(filename, "SaltyBet Records %d.json", i);
        Read_File_Result file = platform->ReadFile(filename);
        if (file.size)
        {
            any_success = true;

            Json_Value parsed = JsonParse(&reader, (char *)file.data);
            if (parsed.type == JsonType_Array)
            {
                Json_Array_Entry *entry = parsed.arr.root;
                while (entry)
                {
                    Match match = {};

                    Json_Object_Entry *obj_entry = entry->value.obj.root;
                    Json_Object_Entry *player_entry = obj_entry->value.obj.root;
                    match.player1id = GetOrAddCharacter(&mm->characters, player_entry->value.str)->id;
                    match.player1total = (u64)player_entry->next->value.num.f;

                    obj_entry = obj_entry->next;
                    player_entry = obj_entry->value.obj.root;
                    match.player2id = GetOrAddCharacter(&mm->characters, player_entry->value.str)->id;
                    match.player2total = (u64)player_entry->next->value.num.f;

                    obj_entry = obj_entry->next;
                    if (StringCmp(obj_entry->value.str, "Right"))
                        match.winner = 1;

                    obj_entry = obj_entry->next->next;

                    if (StringCmp(obj_entry->value.str, "Tournament"))
                        match.gamemode = SaltyBetMode_Tournament;
                    else
                        match.gamemode = SaltyBetMode_Matchmaking;
                    
                    obj_entry = obj_entry->next->next->next;
                    
                    match.timestamp = (u32)(obj_entry->value.num.f * 0.001);

                    AddMatch(&mm->matches, match);

                    entry = entry->next;
                }
            }

            platform->MemFree(file.data);
        }
        else
        {
            break;
        }
    }

    free(reader.arena.base);

    return any_success;
}

internal bool32
LoadMatchData(Paprika_Platform *platform, Match_System *mm)
{
    Read_File_Result file = platform->ReadFile("matches.pmd");
    if (file.size)
    {
        u32 version_info;

        Assert(file.size >= sizeof(version_info) + sizeof(mm->matches.count));

        u8 *read = file.data;
        char *end = (char *)file.data + file.size;

        version_info = *(u32 *)read;
        read += sizeof(version_info);
        
        mm->matches.count = *(u32 *)read;
        read += sizeof(mm->matches.count);

        mem_t matches_size = mm->matches.count * sizeof(Match);

        Assert(matches_size <= (mem_t)((u8 *)end - read));

        MemCopy(read, GrowableArenaAllocAndGet(&mm->matches.arena, matches_size), matches_size);
        read += matches_size;

        char *read_ch = (char *)read;
        bool32 last_was_null = true;
        while (read_ch < end)
        {
            if (last_was_null)
                GetOrAddCharacter(&mm->characters, read_ch, true);

            last_was_null = *read_ch++ == '\0';
        }

        LogWrite(platform, "Loaded match data.");

        platform->MemFree(file.data);
    }
    else
        LogWrite(platform, "Unable to load match data.");

    return file.size != 0;
}

internal bool32
SaveMatchData(Paprika_Platform *platform, Match_System *mm)
{
    Write_File_Result file = platform->WriteFile("matches.pmd");
    if (file.handle)
    {
        u32 version_info = 0;
        platform->FileWrite(&version_info, sizeof(version_info), 1, file.handle);

        platform->FileWrite(&mm->matches.count, sizeof(mm->matches.count), 1, file.handle);
        platform->FileWrite(mm->matches.arena.base, mm->matches.arena.used, 1, file.handle);

        platform->FileWrite(mm->characters.str_arena.base, mm->characters.str_arena.used, 1, file.handle);

        LogWrite(platform, "Saved match data.");

        platform->FileClose(file.handle);
    }
    else
        LogWrite(platform, "Unable to save match data.");

    return file.handle != 0;
}

internal bool32
LoadNodeSystems(Paprika_Platform *platform, Node_Systems *node_systems)
{
    Read_File_Result file = platform->ReadFile("strategies.nsys");
    if (file.size)
    {
        u8 *read_byte = file.data;

        u32 version = *(u32 *)read_byte;
        read_byte += sizeof(version);

        node_systems->matchmaking_id = *(Node_System_ID *)read_byte;
        read_byte += sizeof(Node_System_ID);

        node_systems->tournament_id = *(Node_System_ID *)read_byte;
        read_byte += sizeof(Node_System_ID);

        node_systems->exhibition_id = *(Node_System_ID *)read_byte;
        read_byte += sizeof(Node_System_ID);

        node_systems->count = (u32)((file.data + file.size - read_byte) / sizeof(Node_System));

        GrowableArenaReset(&node_systems->arena);
        GrowableArenaResize(&node_systems->arena, node_systems->count * sizeof(Node_System));
        Node_System *read = (Node_System *)read_byte;
        Node_System *end = read + node_systems->count;
        while (read < end)
        {
            *GrowableArenaAllocStructAndGet(&node_systems->arena, Node_System) = *read++;
        }

        LogWrite(platform, "Loaded node systems.");

        platform->MemFree(file.data);
    }
    else
        LogWrite(platform, "Unable to load node systems.");

    return file.size != 0;
}

internal bool32
SaveNodeSystems(Paprika_Platform *platform, Node_Systems *node_systems)
{
    Write_File_Result file = platform->WriteFile("strategies.nsys");
    if (file.handle)
    {
        u32 version = 0;
        platform->FileWrite(&version, sizeof(version), 1, file.handle);

        platform->FileWrite(&node_systems->matchmaking_id, sizeof(Node_System_ID), 1, file.handle);
        platform->FileWrite(&node_systems->tournament_id, sizeof(Node_System_ID), 1, file.handle);
        platform->FileWrite(&node_systems->exhibition_id, sizeof(Node_System_ID), 1, file.handle);

        platform->FileWrite(node_systems->arena.base, sizeof(Node_System), node_systems->count, file.handle);
        LogWrite(platform, "Saved node systems.");

        platform->FileClose(file.handle);
    }
    else
        LogWrite(platform, "Unable to save node systems.");

    return file.handle != 0;
}

internal Paprika_Config
DefaultConfig()
{
    Paprika_Config config = {};

    config.maximum_manual_wager = 100000;

    return config;
}


internal bool32
LoadConfig(Paprika_Platform *platform, Json_Reader *reader, Paprika_Config *config)
{
    *config = DefaultConfig();

    Read_File_Result file = platform->ReadFile("config.json");
    if (file.size)
    {
        Json_Value res = JsonParse(reader, (char *)file.data);
        Assert(res.type != JsonType_Error);
        Json_Object obj = res.obj;

        Json_Object_Entry *entry = obj.root;
        while (entry)
        {
            if (StringCmp(entry->name, "username"))
                StringCopy(entry->value.str, config->username);
            else if (StringCmp(entry->name, "password"))
                StringCopy(entry->value.str, config->password);
            else if (StringCmp(entry->name, "maximum_manual_wager"))
                config->maximum_manual_wager = entry->value.num.i;
            else
                Panic("Invalid config entry.");

            entry = entry->next;
        }

        LogWrite(platform, "Loaded config.");

        platform->MemFree(file.data);
    }
    else
        LogWrite(platform, "Unable to load config.");

    return file.size != 0;
}

internal bool32
SaveConfig(Paprika_Platform *platform, Json_Writer *writer, Paprika_Config *config)
{
    Json_Object_Entry maximum_manual_wager_entry;
    maximum_manual_wager_entry.name = "maximum_manual_wager";
    maximum_manual_wager_entry.value.type = JsonType_Num;
    maximum_manual_wager_entry.value.num.type = JsonNumType_Int;
    maximum_manual_wager_entry.value.num.i = config->maximum_manual_wager;
    maximum_manual_wager_entry.next = 0;

    Json_Object_Entry password_entry;
    password_entry.name = "password";
    password_entry.value.type = JsonType_String;
    password_entry.value.str = config->password;
    password_entry.next = &maximum_manual_wager_entry;

    Json_Object_Entry username_entry;
    username_entry.name = "username";
    username_entry.value.type = JsonType_String;
    username_entry.value.str = config->username;
    username_entry.next = &password_entry;

    Json_Object obj = {};
    obj.count = 2;
    obj.root = &username_entry;

    JsonDump(writer, obj);
    
    Write_File_Result file = platform->WriteFile("config.json");
    if (file.handle)
    {
        platform->FileWrite(writer->arena.base, writer->arena.used, 1, file.handle);
        platform->FileClose(file.handle);

        LogWrite(platform, "Saved config.");
    }
    else
        LogWrite(platform, "Unable to save config.");

    return file.handle != 0;
}

internal inline f32
Prediction(i16 a, i16 b, Skill_Env skill_env)
{
    f64 diff = a - b;
    f64 exponent = -(diff / (f64)skill_env.skill_differential);
    f32 expected = (f32)(1.0 / (1.0 + pow(10.0, exponent)));
    return expected;
}

internal inline void
AdjustRanks(Rank *a, Rank *b, u8 winner, Skill_Env skill_env)
{
    f64 a_prediction = Prediction(a->rank, b->rank, skill_env);
    f64 a_outcome = winner == 0 ? 1.0 : 0.0;
    a->rank += (i16)((f64)a->uncertainty * (a_outcome - a_prediction));

    f64 b_prediction = 1.0 - a_prediction;
    f64 b_outcome = 1.0 - a_outcome;
    b->rank += (i16)((f64)b->uncertainty * (b_outcome - b_prediction));

    a->uncertainty = Max(skill_env.min_uncertainty, a->uncertainty - skill_env.certainty_gain);
    b->uncertainty = Max(skill_env.min_uncertainty, b->uncertainty - skill_env.certainty_gain);
}

internal inline f32
Confidence(i16 uncertainty, Skill_Env skill_env)
{
    f32 result = 1.0f - ((f32)uncertainty / skill_env.initial_uncertainty);
    return result;
}

internal void
ProcessMatch(Characters *characters, Match *match)
{
    Character *a = GetCharacter(characters, match->player1id);
    Character *b = GetCharacter(characters, match->player2id);

    AdjustRanks(&a->rank, &b->rank, match->winner, characters->skill_env);

    a->wins += 1 - match->winner;
    a->losses += match->winner;

    b->wins += match->winner;
    b->losses += 1 - match->winner;
}

internal Versus_History_Result
GetVersusHistory(Matches *matches, Skill_Env skill_env, Character_ID red_id, Character_ID blue_id, Match_ID until_match)
{
    Versus_History_Result result = {};

    result.red_rank = InitialRank(skill_env);
    result.blue_rank = InitialRank(skill_env);

    until_match = Min(until_match, matches->count);
    Match *match = (Match *)matches->arena.base;
    for (Match_ID match_idx = 0; match_idx < until_match; ++match_idx)
    {
        if (match->player1id == red_id && match->player2id == blue_id)
        {
            result.wins += 1 - match->winner;
            ++result.count;
            AdjustRanks(&result.red_rank, &result.blue_rank, match->winner, skill_env);
        }
        else if (match->player1id == blue_id && match->player2id == red_id)
        {
            result.wins += match->winner;
            ++result.count;
            AdjustRanks(&result.blue_rank, &result.red_rank, match->winner, skill_env);
        }

        ++match;
    }

    return result;
}

internal Matchup_Comparison
GetMatchupComparison(Match_System *mm,
                     Character *red, Character *blue,
                     i64 balance, Salty_Bet_Mode gamemode, Match_ID versus_until)
{
    Matchup_Comparison result = {};

    result.balance = (f64)balance;
    result.gamemode = gamemode;

    result.team_in_matchup = 
        StringCmpLen(GetCharacterName(&mm->characters, red->name_offset), "Team ", 5) ||
        StringCmpLen(GetCharacterName(&mm->characters, blue->name_offset), "Team ", 5);

    Versus_History_Result versus_record = GetVersusHistory(&mm->matches, mm->characters.skill_env,
                                                           red->id, blue->id, versus_until);
    if (versus_record.count)
    {
        result.vs_count = versus_record.count;
        result.vs_wins = versus_record.wins;

        result.red_prediction = Prediction(versus_record.red_rank.rank, versus_record.blue_rank.rank, mm->characters.skill_env);
        result.red_confidence = Confidence(versus_record.red_rank.uncertainty, mm->characters.skill_env);
        result.blue_confidence = Confidence(versus_record.blue_rank.uncertainty, mm->characters.skill_env);
    }
    else
    {
        result.red_prediction = Prediction(red->rank.rank, blue->rank.rank, mm->characters.skill_env);
        result.red_confidence = Confidence(red->rank.uncertainty, mm->characters.skill_env);
        result.blue_confidence = Confidence(blue->rank.uncertainty, mm->characters.skill_env);
    }

    return result;
}

internal Matchup_Comparison
GetMatchupComparison(Match_System *mm,
                     Match match, i64 balance, Match_ID versus_until)
{
    Character *red = GetCharacter(&mm->characters, match.player1id);
    Character *blue = GetCharacter(&mm->characters, match.player2id);
    return GetMatchupComparison(mm, red, blue, balance, (Salty_Bet_Mode)match.gamemode, versus_until);
}

internal char *
FindSubstring(char *s, char *needle)
{
    while (*s)
    {
        if (*s == *needle)
        {
            char *c = s + 1;
            char *n = needle + 1;
            bool32 is_match = true;
            while (*n)
            {
                if (*n++ != *c++)
                {
                    is_match = false;
                    break;
                }
            }

            if (is_match)
                return s;
        }
        ++s;
    }

    return 0;
}

internal Between_Result
Between(char *s, char *before, char *after)
{
    Between_Result result = {};

    result.start = FindSubstring(s, before);
    if (result.start)
    {
        result.start += StringLength(before) - 1;
        result.end = FindSubstring(result.start, after);
    }

    return result;
}

internal inline Salty_Bet_Mode
GetSaltyBetMode(char *remaining)
{
    Salty_Bet_Mode result = SaltyBetMode_None;
    if (FindSubstring(remaining, "Tournament mode") || FindSubstring(remaining, "next tournament!"))
        result = SaltyBetMode_Matchmaking;
    else if (FindSubstring(remaining, "FINAL ROUND!") || FindSubstring(remaining, "the bracket!"))
        result = SaltyBetMode_Tournament;
    else
        result = SaltyBetMode_Exhibition;
    return result;
}

internal void
ParseState(Json_Object obj, Salt_State *result)
{
    Json_Object_Entry *entry = obj.root;
    while (entry)
    {
        if (StringCmp(entry->name, "p1name"))
            StringCopy(entry->value.str, result->p1name);
        else if (StringCmp(entry->name, "p2name"))
            StringCopy(entry->value.str, result->p2name);
        else if (StringCmp(entry->name, "p1total"))
            result->p1total = StringToI64(entry->value.str);
        else if (StringCmp(entry->name, "p2total"))
            result->p2total = StringToI64(entry->value.str);
        else if (StringCmp(entry->name, "status"))
            StringCopy(entry->value.str, result->status);
        else if (StringCmp(entry->name, "alert"))
            StringCopy(entry->value.str, result->alert);
        else if (StringCmp(entry->name, "x"))
            result->x = (i32)entry->value.num.i;
        else if (StringCmp(entry->name, "remaining"))
            StringCopy(entry->value.str, result->remaining);

        entry = entry->next;
    }

    if (*result->remaining)
        result->mode = GetSaltyBetMode(result->remaining);
}

internal void
ParseZdata(Json_Object obj, Salt_Zdata *result, char *user_id)
{
    Json_Object_Entry *entry = obj.root;
    while (entry)
    {
        if (StringCmp(entry->name, user_id))
        {
            Json_Object_Entry *u_entry = entry->value.obj.root;
            while (u_entry)
            {
                if (*u_entry->name == 'n')
                    StringCopy(u_entry->value.str, result->name);
                else if (*u_entry->name == 'b')
                    result->balance = StringToI64(u_entry->value.str);
                else if (*u_entry->name == 'p')
                    result->player = StringToI32(u_entry->value.str);
                else if (*u_entry->name == 'w')
                    result->wager = StringToI64(u_entry->value.str);
                else if (*u_entry->name == 'r')
                    result->rank = StringToI32(u_entry->value.str);

                u_entry = u_entry->next;
            }
        }
        else
        {
            if (StringCmp(entry->name, "p1name"))
                StringCopy(entry->value.str, result->state.p1name);
            else if (StringCmp(entry->name, "p2name"))
                StringCopy(entry->value.str, result->state.p2name);
            else if (StringCmp(entry->name, "p1total"))
                result->state.p1total = StringToI64(entry->value.str);
            else if (StringCmp(entry->name, "p2total"))
                result->state.p2total = StringToI64(entry->value.str);
            else if (StringCmp(entry->name, "status"))
                StringCopy(entry->value.str, result->state.status);
            else if (StringCmp(entry->name, "alert"))
                StringCopy(entry->value.str, result->state.alert);
            else if (StringCmp(entry->name, "x"))
                result->state.x = (i32)entry->value.num.i;
            else if (StringCmp(entry->name, "remaining"))
                StringCopy(entry->value.str, result->state.remaining);
        }

        entry = entry->next;
    }

    if (*result->state.remaining)
        result->state.mode = GetSaltyBetMode(result->state.remaining);
}

internal Skill_Env
CreateSkillEnv(i16 initial_rank, i16 initial_uncertainty,
               i16 skill_differential, i16 min_uncertainty,
               i16 certainty_gain)
{
    Skill_Env result;
    result.initial_rank = initial_rank;
    result.initial_uncertainty = initial_uncertainty;
    result.skill_differential = skill_differential;
    result.min_uncertainty = min_uncertainty;
    result.certainty_gain = certainty_gain;
    return result;
}

internal void
CharactersMapInit(Characters *characters)
{
    Character_Map_Entry *entry = characters->map;
    Character_Map_Entry *end = entry + ArrayCount(characters->map);
    while (entry < end)
    {
        entry->index = MAP_INVALID;
        entry->next_offset = 0;
        entry->hash = 0;

        ++entry;
    }
}

internal inline void
ChangeMode(Paprika_State *paprika, Paprika_Mode mode)
{
#if 0
#ifdef INTERNAL_BUILD
    char line[64];
    sprintf(line, "Changing mode to '%s'...", GetPaprikaModeName(mode));
    LogWrite(&paprika->log, line);
#endif
#endif
    SaltyBetClientReset(&paprika->client);
    paprika->client.poll_counter = 0.0f;

    paprika->mode = mode;
}

internal void
DebugDumpData(Paprika_Platform *platform, char *src, mem_t size, char *filename)
{
    Write_File_Result file = platform->WriteFile(filename);
    if (file.handle)
    {
        platform->FileWrite(src, size, 1, file.handle);
        platform->FileClose(file.handle);
    }
}

internal i64
GetBailout(i32 player_rank)
{
    return 1000 + (25 * player_rank);
}

internal i64
SimulateNodeSystem(i64 balance, i64 bailout, Node_System *system, Matchup_Frame *matchup, Match *match)
{
    Matchup_Comparison comparison = matchup->comparison;
    comparison.balance = balance;

    Calc_Nodes_Result calc = CalcNodes(system, &comparison);

    if (calc.all_nodes_resolved)
    {
        f64 red_total = (f64)match->player1total;
        f64 blue_total = (f64)match->player2total;
        f64 win_odds;

        calc.out.wager = Clamp(calc.out.wager, 0, balance);

        if (matchup->wager.player == 0)
            red_total -= matchup->wager.wager;
        else
            blue_total -= matchup->wager.wager;

        if (calc.out.player == 0)
        {
            red_total += calc.out.wager;
            win_odds = red_total == 0.0 ? 0.0 : blue_total / red_total;
        }
        else
        {
            blue_total += calc.out.wager;
            win_odds = blue_total == 0.0 ? 0.0 : red_total / blue_total;
        }

        if (calc.out.player == match->winner)
            balance += (i64)((f64)calc.out.wager * win_odds);
        else
            balance = Max(balance - calc.out.wager, bailout);
    }
    
    return balance;
}

internal void
CircularBufferCopy(void *dst, void *src,
                   mem_t element_size, mem_t element_count,
                   mem_t element_capacity, mem_t write_index)
{
    if (element_count < element_capacity)
    {
        MemCopy(src, dst, element_count * element_size);
    }
    else
    {
        mem_t until_end = element_capacity - write_index;
        MemCopy((u8 *)src + (write_index * element_size), dst, until_end * element_size);
        MemCopy(src, (u8 *)dst + (until_end * element_size), write_index * element_size);
    }
}

internal void
CircularBufferPtrCopy(void **dst, void *src,
                      mem_t element_size, mem_t element_count,
                      mem_t element_capacity, mem_t write_index)
{
    u8 **write = (u8 **)dst;

    if (element_count < element_capacity)
    {
        for (mem_t offset = 0;
             offset < element_count * element_size;
             offset += element_size)
        {
            *write++ = (u8 *)src + offset;
        }
    }
    else
    {
        for (mem_t offset = write_index * element_size;
             offset < element_capacity * element_size;
             offset += element_size)
        {
            *write++ = (u8 *)src + offset;
        }

        for (mem_t offset = 0;
             offset < write_index * element_size;
             offset += element_size)
        {
            *write++ = (u8 *)src + offset;
        }
    }
}

internal void
SimulateNodeSystems(i64 starting_money, i64 bailout, Salty_Bet_Mode mode, Node_Systems *systems, Matches *matches, Matchup_History *history, i64 *results)
{
    Matchup_Frame matchups[ArrayCount(history->matchup_frames)];

    CircularBufferCopy(matchups, history->matchup_frames,
                       sizeof(Matchup_Frame), history->count,
                       ArrayCount(history->matchup_frames), history->write_index);

    for (u32 idx = 0; idx < systems->count; ++idx)
        results[idx] = starting_money;

    for (Matchup_Frame *matchup = matchups;
         matchup < matchups + history->count;
         ++matchup)
    {
        Match *match = GetMatch(matches, matchup->match_id);
        if (match->gamemode == mode && match->player1total && match->player2total)
            for (Node_System_ID id = 1; id <= systems->count; ++id)
                results[id-1] = SimulateNodeSystem(results[id-1], bailout, GetNodeSystem(systems, id), matchup, match);
    }
}

internal void
PushHistoryFrame(Matchup_History *history, Matchup_Frame matchup)
{
    history->matchup_frames[history->write_index] = matchup;
    history->write_index = (history->write_index + 1) % ArrayCount(history->matchup_frames);
    history->count = Min(history->count + 1, ArrayCount(history->matchup_frames));
}

internal void
ProcessMatches(Match_System *mm, Matchup_History *history)
{
    for (Match_ID match_id = 0; match_id < mm->matches.count; ++match_id)
    {
        Match *match = GetMatch(&mm->matches, match_id);

        if ((i32)mm->matches.count - match_id <= ArrayCount(history->matchup_frames))
        {
            Matchup_Frame frame = {};
            frame.match_id = match_id;
            frame.comparison = GetMatchupComparison(mm, *match, 0, match_id);

            PushHistoryFrame(history, frame);
        }

        ProcessMatch(&mm->characters, match);
    }
}

internal void
LoadPaprikaData(Paprika_State *paprika, bool32 load_matches)
{
    if (load_matches)
    {
        MatchSystemReset(&paprika->mm);

        if (LoadMatchData(&paprika->platform, &paprika->mm) ||
            LoadLegacyMatches(&paprika->platform, &paprika->mm))
        {
            char line[128];
            sprintf(line, "Loaded %u matches and %u characters.",
                    paprika->mm.matches.count,
                    paprika->mm.characters.count);
            LogWrite(&paprika->log, line);
        }

        u32 match_count = paprika->mm.matches.count;
        u32 character_count = paprika->mm.characters.count;
        if (LoadSaltyBetBotMatches(&paprika->platform, &paprika->mm))
        {
            char line[128];
            sprintf(line, "Loaded %u matches and %u characters from SaltyBetBot data.",
                    paprika->mm.matches.count - match_count,
                    paprika->mm.characters.count - character_count);
            LogWrite(&paprika->log, line);
        }

        ProcessMatches(&paprika->mm, &paprika->history);

        if (!LoadMatchWagers(&paprika->platform, &paprika->history, &paprika->mm.matches))
            if (ConvertLegacyMatchWagers(&paprika->platform))
                LoadMatchWagers(&paprika->platform, &paprika->history, &paprika->mm.matches);
    }

    LoadNodeSystems(&paprika->platform, &paprika->node_systems);

    if (!LoadConfig(&paprika->platform, &paprika->json_reader, &paprika->config))
        ActivateConfigWindow(paprika);
}

EXPORT_FUNC PaprikaUpdateFunction(PaprikaUpdate)
{
    paprika->platform.log = &paprika->log;

    switch (paprika->mode)
    {
        case PaprikaMode_Uninitialized:
        {
            paprika->log.platform = &paprika->platform;

            mem_t line_buffer_size = Kilobytes(256);
            mem_t write_buffer_size = Kilobytes(1);

            LineBufferInit(&paprika->log.line_buffer,
                           (char *)ArenaAlloc(&paprika->arena, line_buffer_size),
                           line_buffer_size);
            
            ArenaSub(&paprika->arena,
                     &paprika->windows.log_window.view_buffer,
                     line_buffer_size);

            ArenaSub(&paprika->arena,
                     &paprika->log.write_buffer,
                     write_buffer_size);

            ArenaSub(&paprika->arena,
                     &paprika->json_writer.arena,
                     Kilobytes(256));

            ArenaSub(&paprika->arena,
                     &paprika->windows.strategies_list,
                     Kilobytes(16));
            
            LogWrite(&paprika->platform, VERSION_STR);
            LogWrite(&paprika->platform, "===========");

            paprika->mm.characters.skill_env = CreateSkillEnv(1000, 100, 300, 12, 2);

            LoadPaprikaData(paprika);

            ChangeMode(paprika, PaprikaMode_Disabled);
        } break;

        case PaprikaMode_Disabled:
        {
        } break;

        case PaprikaMode_Login:
        {
            if (*paprika->config.username && *paprika->config.password)
            {
                Call_Arg args[] = {Call_Arg(paprika->config.username),
                                   Call_Arg(paprika->config.password)};
                Client_Get_Result res = SaltyBetClientGet(&paprika->client, input.time_delta,
                                                          SaltyBetCall_Login, args);

                if (res.data)
                {
                    ChangeMode(paprika, PaprikaMode_Logged_In);
                }
                else if (res.failed)
                {
                    LogWrite(&paprika->log, "Error logging in.");
                }
            }
            else
            {
                LogWrite(&paprika->log, "No login information found.");
                ActivateConfigWindow(paprika);
                ChangeMode(paprika, PaprikaMode_Disabled);
            }
        } break;

        case PaprikaMode_Logged_In:
        {
            Client_Get_Result res = SaltyBetClientGet(&paprika->client, input.time_delta,
                                                      SaltyBetCall_GetHome);

            if (res.data)
            {
                Between_Result b;
                b = Between(res.data, "<span class=\"navbar-text\">", " ");
                if (b.end)
                {
                    MemZero(paprika->player.name, sizeof(paprika->player.name));
                    MemCopy(b.start, paprika->player.name, b.end - b.start);

                    b = Between(b.end, "<input type=\"hidden\" id=\"u\" name=\"u\" value =\"", "\"");
                    MemZero(paprika->player.user_id, sizeof(paprika->player.user_id));
                    MemCopy(b.start, paprika->player.user_id, b.end - b.start);

                    b = Between(b.end, "<input type=\"hidden\" id=\"b\" name=\"b\" value =\"", "\"");
                    paprika->player.balance = StringToI64(b.start, b.end);

                    char line[128];
                    sprintf(line, "User '%s' logged in! ($%lld)", paprika->player.name, paprika->player.balance);
                    LogWrite(&paprika->log, line);
                    ChangeMode(paprika, PaprikaMode_Await_Open);
                }
                else
                {
                    LogWrite(&paprika->log, "Failed login. Possibly incorrect credentials.");
                    ChangeMode(paprika, PaprikaMode_Disabled);
                }
            }
            else if (res.failed)
            {
                LogWrite(&paprika->log, "Error logging in.");
            }
        } break;

        case PaprikaMode_Refresh_Balance:
        {
            Salty_Bet_Call call =
                paprika->state.mode == SaltyBetMode_Tournament ?
                    SaltyBetCall_GetTournamentStart : SaltyBetCall_GetTournamentEnd;

            Client_Get_Result res = SaltyBetClientGet(&paprika->client, input.time_delta, call);
            if (res.data)
            {
                u64 new_balance = StringToI64(res.data);
                if (new_balance)
                {
                    paprika->player.balance = new_balance;
                    LogWrite(&paprika->platform, "Refreshed balance because of gamemode change.");
                }
                else
                {
                    LogWrite(&paprika->platform, "Error refreshing balance.");
                    ChangeMode(paprika, PaprikaMode_Disabled);
                }

                if (paprika->mode != PaprikaMode_Disabled)
                    ChangeMode(paprika, PaprikaMode_On_Open);
            }
            else if (res.failed)
            {
                LogWrite(&paprika->platform, "Error refreshing balance.");
                ChangeMode(paprika, PaprikaMode_Disabled);
            }
        } break;

        case PaprikaMode_Await_Open:
        {
            Call_Arg args[] = {Call_Arg(paprika->platform.GetTimestamp())};
            Client_Get_Result res = SaltyBetClientGet(&paprika->client, input.time_delta,
                                                      SaltyBetCall_GetState, args);

            if (res.data)
            {
                Salt_State state = {};
                Json_Value parsed = JsonParse(&paprika->json_reader, res.data);
                if (parsed.type == JsonType_Object)
                {
                    ParseState(parsed.obj, &state); 
                }
                else
                {
                    char line[64];
                    sprintf(line, "Error parsing JSON at position %llu.", paprika->json_reader.error_offset);
                    LogWrite(&paprika->platform, line);
                }

                if (parsed.type == JsonType_Object && StringCmp(state.status, "open"))
                {
                    paprika->zdata = {};
                    if (StringCmp(state.alert, "Tournament mode start!") ||
                        StringCmp(state.alert, "Exhibition mode start!"))
                    {
                        ChangeMode(paprika, PaprikaMode_Refresh_Balance);
                    }
                    else
                    {
                        ChangeMode(paprika, PaprikaMode_On_Open);
                    }

                    paprika->state = state;
                }
                else
                {
                    SaltyBetClientReset(&paprika->client);
                    SaltyBetClientCooldown(&paprika->client);
                }
            }
            else if (res.failed)
            {
                LogWrite(&paprika->log, "Error getting state.");
            }
        } break;

        case PaprikaMode_On_Open:
        {
            Character *red = GetOrAddCharacter(&paprika->mm.characters, paprika->state.p1name);
            Character *blue = GetOrAddCharacter(&paprika->mm.characters, paprika->state.p2name);

            Match current_match = {};
            current_match.player1id = red->id;
            current_match.player2id = blue->id;
            current_match.gamemode = paprika->state.mode;

            paprika->current_match = current_match;
            paprika->current_match_comparison =
                GetMatchupComparison(&paprika->mm,
                                     red, blue, paprika->player.balance, paprika->state.mode);

            Node_System_ID active_strategy = GetActiveNodeSystemID(&paprika->node_systems, paprika->state.mode);
            Node_System *node_system = GetNodeSystem(&paprika->node_systems, active_strategy);
            if (node_system)
            {
                Calc_Nodes_Result calc = CalcNodes(node_system, &paprika->current_match_comparison);
                if (calc.all_nodes_resolved)
                {
                    paprika->windows.match_window.selected_strategy = active_strategy;

                    paprika->pending_input.place_bet = true;
                    paprika->pending_input.wager = calc.out.wager;
                    paprika->pending_input.player = calc.out.player + 1;
                    ChangeMode(paprika, PaprikaMode_Place_Wager);
                }
            }

            if (paprika->mode != PaprikaMode_Place_Wager)
            {
                ChangeMode(paprika, PaprikaMode_Open);
            }
        } break;

        case PaprikaMode_Open:
        {
            if (input.place_bet && input.wager)
            {
                paprika->pending_input = input;
                ChangeMode(paprika, PaprikaMode_Place_Wager);
            }
            else
            {
                Call_Arg args[] = {Call_Arg(paprika->platform.GetTimestamp())};
                Client_Get_Result res = SaltyBetClientGet(&paprika->client, input.time_delta,
                                                          SaltyBetCall_GetZdata, args);

                if (res.data)
                {
                    Json_Value parsed = JsonParse(&paprika->json_reader, res.data);
                    if (parsed.type == JsonType_Object)
                    {
                        ParseZdata(parsed.obj, &paprika->zdata, paprika->player.user_id);

                        if (paprika->zdata.rank)
                            paprika->player.rank = paprika->zdata.rank;

                        if (StringCmp(paprika->zdata.state.status, "locked"))
                        {
                            paprika->current_match.player1total = paprika->zdata.state.p1total;
                            paprika->current_match.player2total = paprika->zdata.state.p2total;
                            ChangeMode(paprika, PaprikaMode_Await_Result); 
                        }
                    }
                    else
                    {
                        char line[64];
                        sprintf(line, "Error parsing JSON at position %llu.", paprika->json_reader.error_offset);
                        LogWrite(&paprika->platform, line);
                    }

                    if (paprika->mode == PaprikaMode_Open)
                    {
                        SaltyBetClientReset(&paprika->client);
                        SaltyBetClientCooldown(&paprika->client);
                    }
                }
                else if (res.failed)
                {
                    LogWrite(&paprika->log, "Error getting zdata.");
                }
            }
        } break;

        case PaprikaMode_Await_Result:
        {
            Call_Arg args[] = {Call_Arg(paprika->platform.GetTimestamp())};
            Client_Get_Result res = SaltyBetClientGet(&paprika->client, input.time_delta,
                                                      SaltyBetCall_GetZdata, args);

            if (res.data)
            {
                Json_Value parsed = JsonParse(&paprika->json_reader, res.data);
                if (parsed.type == JsonType_Object)
                {
                    ParseZdata(parsed.obj, &paprika->zdata, paprika->player.user_id);

                    if (paprika->zdata.rank)
                        paprika->player.rank = paprika->zdata.rank;

                    if (*paprika->zdata.state.status == '1' || *paprika->zdata.state.status == '2')
                    {
                        i64 open_balance = paprika->player.balance;
                        if (paprika->zdata.balance)
                        {
                            char line[128];
                            char change_sign = paprika->zdata.balance > paprika->player.balance ? '+' : '-';
                            sprintf(line, "Balance: $%lld (%c$%lld)",
                                    paprika->zdata.balance,
                                    change_sign,
                                    Abs(paprika->zdata.balance - paprika->player.balance));
                            LogWrite(&paprika->log, line);
                            paprika->player.balance = paprika->zdata.balance;
                        }

                        Match match = paprika->current_match;
                        match.timestamp = (u32)args[0].num;
                        match.player1total = paprika->zdata.state.p1total;
                        match.player2total = paprika->zdata.state.p2total;
                        match.winner = *paprika->zdata.state.status == '1' ? 0 : 1;
                        ProcessMatch(&paprika->mm.characters, &match);
                        Match_ID match_id = AddMatch(&paprika->mm.matches, match);
                        
                        Matchup_Frame matchup_frame = {};
                        matchup_frame.match_id = match_id;
                        matchup_frame.comparison = paprika->current_match_comparison;

                        Match_Wager match_wager = {};
                        match_wager.timestamp = match.timestamp;
                        match_wager.open_balance = open_balance;
                        match_wager.close_balance = paprika->player.balance;
                        match_wager.wager = paprika->zdata.wager;
                        match_wager.player = paprika->zdata.player == 2 ? 1 : 0;

                        matchup_frame.wager = match_wager;

                        SaveMatchWager(&paprika->platform, match_wager);
                        PushHistoryFrame(&paprika->history, matchup_frame);

                        if (paprika->stopping)
                        {
                            ChangeMode(paprika, PaprikaMode_Disabled);
                            paprika->stopping = false;
                        }
                        else
                        {
                            ChangeMode(paprika, PaprikaMode_Await_Open);
                        }
                    }
                }
                else
                {
                    char line[64];
                    sprintf(line, "Error parsing JSON at position %llu.", paprika->json_reader.error_offset);
                    LogWrite(&paprika->platform, line);
                }

                if (paprika->mode == PaprikaMode_Await_Result)
                {
                    SaltyBetClientReset(&paprika->client);
                    SaltyBetClientCooldown(&paprika->client);
                }
            }
            else if (res.failed)
            {
                LogWrite(&paprika->log, "Error getting zdata.");
            }
        } break;

        case PaprikaMode_Place_Wager:
        {
            u64 wager = Clamp(paprika->pending_input.wager, 0, paprika->player.balance);
            Call_Arg args[] = {Call_Arg(paprika->pending_input.player),
                               Call_Arg(wager)};
            Client_Get_Result res = SaltyBetClientGet(&paprika->client, input.time_delta,
                                                      SaltyBetCall_PlaceBet, args);

            if (res.data)
            {
                char line[128];
                Character_ID id = paprika->pending_input.player == 1 ?
                    paprika->current_match.player1id : paprika->current_match.player2id;

                sprintf(line, "Wagered $%llu on '%s'.",
                        wager,
                        GetCharacterName(&paprika->mm.characters, id));
                LogWrite(&paprika->log, line);
                ChangeMode(paprika, PaprikaMode_Open);
            }
            else if (res.failed)
            {
                LogWrite(&paprika->log, "Error placing bet.");
            }
        } break;
    }

    bool32 rendered = false;

    paprika->frame_counter += input.time_delta;
    if (paprika->frame_counter > paprika->frame_rate)
    {
        paprika->platform.PrepareFrame();
        HandleWindows(paprika);
        while (paprika->frame_rate && paprika->frame_counter > paprika->frame_rate)
            paprika->frame_counter -= paprika->frame_rate;
        rendered = true;
    }

    return rendered;
}

EXPORT_FUNC PaprikaStopRunningFunction(PaprikaStopRunning)
{
    SaveMatchData(&paprika->platform, &paprika->mm);

    LogFlush(&paprika->log);

    paprika->running = false;
}
