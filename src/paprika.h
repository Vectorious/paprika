#pragma once


#include "paprika_types.h"


#ifndef PAPRIKA_TYPES

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef int bool32;

typedef size_t mem_t;

#define internal static

#endif

#define MAX_U64 0xFFFFFFFFFFFFFFFF
#define MAX_I64 0x7FFFFFFFFFFFFFFF
#define MAX_U32 0xFFFFFFFF
#define MAX_I32 0x7FFFFFFF
#define MAX_U16 0xFFFF

#define Abs(a) (((a) < 0) ? (-(a)) : (a))
#define Min(a, b) (((a) < (b)) ? (a) : (b))
#define Max(a, b) (((a) < (b)) ? (b) : (a))
#define Clamp(value, min, max) (Max((min), Min((max), (value))))

#define Kilobytes(amount) ((amount) * 1024LL)
#define Megabytes(amount) (Kilobytes(amount) * 1024LL)
#define Gigabytes(amount) (Megabytes(amount) * 1024LL)
#define Terabytes(amount) (Gigabytes(amount) * 1024LL)

#define Swap(a, b, type) \
    {                    \
        type temp = (a); \
        (a) = (b);       \
        (b) = temp;      \
    }
#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

#ifdef INTERNAL_BUILD

#define Panic(message)          \
    {                           \
        *(volatile int *)0 = 0; \
    }

#define Assert(expression)          \
    if (!(expression))              \
    {                               \
        Panic("Assertion failed."); \
    }

#define DefaultPanic default: { Panic("DefaultPanic"); } break;

#else

#define Panic(message)          \
    {                           \
        *(volatile int *)0 = 0; \
    }

#define Assert(expression)          \
    if (!(expression))              \
    {                               \
        Panic("Assertion failed."); \
    }

#define DefaultPanic default: { Panic("DefaultPanic"); } break;

#endif


#include <math.h>
#include "intrinsics.h"
#include "arena.h"
#include "json.h"
#include "paprika_platform.h"
#include "saltybet.h"
#include "math.h"
#include "paprika_buffers.h"
#include "paprika_log.h"


typedef u16 Character_ID;
typedef u32 Match_ID;


#pragma pack(push, 1)
struct Legacy_Match
{
    u32 winner    : 2;
    u32 player1id : 15;
    u32 player2id : 15;
    u32 timestamp;
};

struct Legacy_Match_Wager
{
    u32 timestamp;
    u32 wager;
    u32 open_balance;
    u32 close_balance;
};
#pragma pack(pop)

struct Match
{
    u64 player1id    : 16;
    u64 player1total : 48;

    u64 player2id    : 16;
    u64 player2total : 48;

    u32 timestamp;

    u32 winner   : 1;
    u32 gamemode : 2;
    u32 _unused  : 29;
};

struct Match_Wager
{
    i64 wager   : 48;
    u64 player  : 1;
    u64 tag     : 15;

    i64 open_balance;
    i64 close_balance;

    u32 timestamp;
};

struct Matches
{
    Growable_Memory_Arena arena;
    u32 count;
};

struct Rank
{
    i16 rank;
    i16 uncertainty;
};

struct Skill_Env
{
    i16 initial_rank;
    i16 initial_uncertainty;
    i16 skill_differential;
    i16 min_uncertainty;
    i16 certainty_gain;
};

struct Character
{
    Arena_Offset name_offset;
    u16 id;
    u16 wins;
    u16 losses;
    Rank rank;
};

struct Character_Map_Entry
{
    u32 hash;
    Character_ID index;
    Arena_Offset next_offset;
};

#define MAP_INVALID MAX_U16

struct Characters
{
    Skill_Env skill_env;
    u16 count;
    Growable_Memory_Arena str_arena;
    Growable_Memory_Arena map_arena;
    Growable_Memory_Arena chr_arena;
    
    Character_Map_Entry map[4096];
};

struct Match_System
{
    Matches matches;
    Characters characters;
};

enum Salty_Bet_Mode
{
    SaltyBetMode_None = 0,

    SaltyBetMode_Matchmaking,
    SaltyBetMode_Tournament,
    SaltyBetMode_Exhibition,
};

internal inline const char *
GetSaltyBetModeName(Salty_Bet_Mode mode)
{
    switch (mode)
    {
        case SaltyBetMode_None:        return "N/A";
        case SaltyBetMode_Matchmaking: return "Matchmaking";
        case SaltyBetMode_Tournament:  return "Tournament";
        case SaltyBetMode_Exhibition:  return "Exhibition";
    }
}

struct Matchup_Comparison
{
    i64 balance;

    f32 red_prediction;

    f32 red_confidence;
    f32 blue_confidence;

    u16 vs_count;
    u16 vs_wins;

    bool32 team_in_matchup;

    Salty_Bet_Mode gamemode;
};

struct Matchup_Frame
{
    Match_ID match_id;
    Matchup_Comparison comparison;
    Match_Wager wager;
};

struct Matchup_History
{
    mem_t count;
    mem_t write_index;
    Matchup_Frame matchup_frames[100];
};


#include "nodes.h"


struct Salt_State
{
    char p1name[64];
    char p2name[64];
    i64 p1total;
    i64 p2total;
    char status[8];
    char alert[32];
    i32 x;
    char remaining[128];

    Salty_Bet_Mode mode;
};

struct Salt_Zdata
{
    Salt_State state;

    i64 balance;
    i32 player;
    i64 wager;
    i32 rank;
    char name[21];
};

struct Paprika_Input
{
    f32 time_delta;
    bool32 place_bet;

    i64 wager;
    u8 player;
};

enum Paprika_Mode
{
    PaprikaMode_Uninitialized = 0,

    PaprikaMode_Disabled,

    PaprikaMode_Login,
    PaprikaMode_Logged_In,
    PaprikaMode_Refresh_Balance,
    PaprikaMode_Await_Open,
    PaprikaMode_On_Open,
    PaprikaMode_Open,
    PaprikaMode_Place_Wager,
    PaprikaMode_Await_Result,
};

internal inline const char *
GetPaprikaModeName(Paprika_Mode mode)
{
    switch (mode)
    {
        case PaprikaMode_Uninitialized:   return "";
        case PaprikaMode_Disabled:        return "Disabled";
        case PaprikaMode_Login:           return "Logging in";
        case PaprikaMode_Logged_In:       return "Logged in";
        case PaprikaMode_Refresh_Balance: return "Refreshing balance";
        case PaprikaMode_Await_Open:      return "Awaiting open";
        case PaprikaMode_On_Open:         return "Open";
        case PaprikaMode_Open:            return "Open";
        case PaprikaMode_Place_Wager:     return "Place wager";
        case PaprikaMode_Await_Result:    return "Awaiting result";
    }
}

typedef u32 Node_System_ID;

struct Node_Systems
{
    u32 count;
    Growable_Memory_Arena arena;

    Node_System_ID matchmaking_id;
    Node_System_ID tournament_id;
    Node_System_ID exhibition_id;
};

struct Paprika_Config
{
    char username[64];
    char password[64];
    i64 maximum_manual_wager;
};

internal inline Node_System *
GetNodeSystem(Node_Systems *systems, Node_System_ID id)
{
    Assert(id <= systems->count);

    if (id)
        return ((Node_System *)systems->arena.base) + (id - 1);
    else
        return 0;
}

internal inline Node_System *
GetActiveNodeSystem(Node_Systems *paprika, Salty_Bet_Mode gamemode);

internal inline Node_System_ID
AddNodeSystem(Node_Systems *systems)
{
    NodeSystemInit(GrowableArenaAllocStructAndGet(&systems->arena, Node_System));
    Node_System_ID result = ++systems->count;
    return result;
}

internal void
RemoveNodeSystem(Node_Systems *systems, Node_System_ID id)
{
    Assert(id && id <= systems->count);

    MemCopy(GetNodeSystem(systems, id + 1), GetNodeSystem(systems, id), (systems->count - id) * sizeof(Node_System));
    MemZero(GetNodeSystem(systems, systems->count), sizeof(Node_System));
    
    if (systems->matchmaking_id == id) systems->matchmaking_id = 0;
    if (systems->tournament_id == id) systems->tournament_id = 0;
    if (systems->exhibition_id == id) systems->exhibition_id = 0;

    if (systems->matchmaking_id > id) --systems->matchmaking_id;
    if (systems->tournament_id > id) --systems->tournament_id;
    if (systems->exhibition_id > id) --systems->exhibition_id;

    --systems->count;
}

struct Player_Info
{
    i64 balance;
    i32 rank;
    // NOTE: SaltyBet does not allow usernames to be longer than 20 characters.
    char name[21];
    char user_id[11];
};

enum MatchCompletion
{
    MatchCompletion_None,
    MatchCompletion_Open,
    MatchCompletion_Locked,
    MatchCompletion_Complete,
};


#include "paprika_imgui.h"


struct Paprika_State
{
    Paprika_Mode mode;
    bool32 running;
    bool32 stopping;

    Paprika_Input pending_input;

    Salt_State state;
    Salt_Zdata zdata;

    Player_Info player;
    MatchCompletion current_match_completion;
    Match current_match;
    Matchup_Comparison current_match_comparison;

    Node_Systems node_systems;

    Matchup_History history;

    Paprika_Windows windows;
    f32 frame_counter;
    f32 frame_rate;

    Match_System mm;

    Salty_Bet_Client client;

    Json_Reader json_reader;
    Json_Writer json_writer;

    Paprika_Platform platform;
    Paprika_Log log;
    Paprika_Config config;

    Memory_Arena arena;
};


internal Character *
GetCharacter(Characters *characters, Character_ID character_id);

internal char *
GetCharacterName(Characters *characters, Arena_Offset name_offset);

// NOTE: This is slower than using the `name_offset`.
internal char *
GetCharacterName(Characters *characters, Character_ID character_id);

internal Rank
InitialRank(Skill_Env skill_env);

internal Character *
AddCharacter(Characters *characters, char *name);

internal Character *
GetOrAddCharacter(Characters *characters, char *name, bool32 force_add = false);

internal Match *
GetMatch(Matches *matches, Match_ID index);

internal Match_ID
AddMatch(Matches *matches, Match match);

internal bool32
LoadLegacyMatches(Paprika_Platform *platform, Match_System *mm);

internal bool32
ConvertLegacyMatchWagers(Paprika_Platform *platform);

internal u32
LoadMatchWagers(Paprika_Platform *platform, Matchup_History *history, Matches *matches);

internal bool32
LoadMatchData(Paprika_Platform *platform, Match_System *mm);

internal bool32
SaveMatchData(Paprika_Platform *platform, Match_System *mm);

internal bool32
LoadNodeSystems(Paprika_Platform *platform, Node_Systems *node_systems);

internal bool32
SaveNodeSystems(Paprika_Platform *platform, Node_Systems *node_systems);

internal bool32
LoadConfig(Paprika_Platform *platform, Json_Reader *reader, Paprika_Config *config);

internal bool32
SaveConfig(Paprika_Platform *platform, Json_Writer *writer, Paprika_Config *config);

internal inline f32
Prediction(i16 a, i16 b, Skill_Env skill_env);

internal inline void
AdjustRanks(Rank *a, Rank *b, u8 winner, Skill_Env skill_env);

internal inline f32
Confidence(i16 uncertainty, Skill_Env skill_env);

internal void
ProcessMatch(Characters *characters, Match *match);


struct Versus_History_Result
{
    u16 wins;
    u16 count;

    Rank red_rank;
    Rank blue_rank;
};


internal Versus_History_Result
GetVersusHistory(Matches *matches, Skill_Env skill_env, Character_ID red_id, Character_ID blue_id, Match_ID until_match = MAX_U32);

internal Matchup_Comparison
GetMatchupComparison(Match_System *mm,
                     Character *red, Character *blue,
                     i64 balance = 0,
                     Salty_Bet_Mode gamemode = SaltyBetMode_None,
                     Match_ID versus_until = MAX_U32);

// NOTE: This is slower than directly providing a `Character` pointer.
internal Matchup_Comparison
GetMatchupComparison(Match_System *mm,
                     Match match, i64 balance = 0, Match_ID versus_until = MAX_U32);

internal char *
FindSubstring(char *s, char *needle);


struct Between_Result
{
    char *start;
    char *end;
};


internal Between_Result
Between(char *s, char *before, char *after);

internal inline bool32
ClientReady(Paprika_State *paprika, f32 time_delta);

internal inline Salty_Bet_Mode
GetSaltyBetMode(char *remaining);

internal void
ParseState(Json_Object_Entry *entry, Salt_State *result);

internal void
ParseZdata(Json_Object_Entry *entry, Salt_Zdata *result, char *user_id);

internal Skill_Env
CreateSkillEnv(i16 initial_rank, i16 initial_uncertainty,
               i16 skill_differential, i16 min_uncertainty,
               i16 certainty_gain);

internal void
CharactersMapInit(Characters *characters);

internal inline void
ChangeMode(Paprika_State *paprika, Paprika_Mode mode);

internal i64
GetBailout(i32 player_rank);

internal void
CircularBufferCopy(void *dst, void *src,
                   mem_t element_size, mem_t element_count,
                   mem_t element_capacity, mem_t write_index);

internal void
CircularBufferPtrCopy(void **dst, void *src,
                      mem_t element_size, mem_t element_count,
                      mem_t element_capacity, mem_t write_index);

internal i64
SimulateNodeSystem(i64 balance, i64 bailout, Node_System *system, Matchup_Frame *matchup, Match *match);

internal void
SimulateNodeSystems(i64 starting_money, i64 bailout, Salty_Bet_Mode mode, Node_Systems *systems, Matches *matches, Matchup_History *history, i64 *results);

internal void
LoadPaprikaData(Paprika_State *paprika, bool32 load_matches = true);

#define EXPORT_FUNC extern "C" __declspec(dllexport)

#define PaprikaUpdateFunction(name) bool32 name(Paprika_State *paprika, Paprika_Input input)
typedef PaprikaUpdateFunction(Paprika_Update_Function);

#define PaprikaStopRunningFunction(name) void name(Paprika_State *paprika)
typedef PaprikaStopRunningFunction(Paprika_Stop_Running_Function);


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

PaprikaUpdateFunction(PaprikaUpdateStub) { return false; }
PaprikaStopRunningFunction(PaprikaStopRunningStub) {}

#pragma clang diagnostic pop
