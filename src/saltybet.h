#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "curl/curl.h"
#pragma clang diagnostic pop


#define SALTY_BET_POLL_INTERVAL 10.0f


typedef CURLMcode Curl_Multi_Add_Handle(CURLM *multi_handle, CURL *curl_handle);
typedef CURLMcode Curl_Multi_Remove_Handle(CURLM *multi_handle, CURL *curl_handle);
typedef CURLMcode Curl_Multi_Perform(CURLM *multi_handle, int *running_handles);
typedef CURLcode Curl_Easy_Setopt(CURL *curl, CURLoption option, ...);
typedef void Curl_Free(void *p);
typedef char *Curl_Easy_Escape(CURL *handle, const char *string, int length);


struct Salty_Bet_Client
{
    CURLM *handle;
    CURL *req;

    int still_running;
    bool32 attempted;
    bool32 success;

    Growable_Memory_Arena arena;

    char post_fields[128];

    Curl_Multi_Add_Handle *curl_multi_add_handle;
    Curl_Multi_Remove_Handle *curl_multi_remove_handle;
    Curl_Multi_Perform *curl_multi_perform;
    Curl_Easy_Setopt *curl_easy_setopt;
    Curl_Free *curl_free;
    Curl_Easy_Escape *curl_easy_escape;

    f32 poll_counter;
};

internal void
SaltyBetClientReset(Salty_Bet_Client *client)
{
    client->curl_multi_remove_handle(client->handle, client->req);
    GrowableArenaReset(&client->arena);
    client->still_running = false;
    client->attempted = false;
    client->success = false;
}

internal inline bool32
SaltyBetBeginRequest(Salty_Bet_Client *client)
{
    bool32 result;
    if (!client->still_running)
    {
        SaltyBetClientReset(client);
        client->attempted = true;
        client->curl_multi_add_handle(client->handle, client->req);
        result = client->curl_multi_perform(client->handle, &client->still_running) == CURLM_OK;
        if (!result)
        {
            client->curl_multi_remove_handle(client->handle, client->req);
        }
    }
    else
    {
        result = false;
    }

    return result;
}

internal inline bool32
SaltyBetEndRequest(Salty_Bet_Client *client)
{
    bool32 result = !client->still_running;
    if (!result)
    {
        client->success = client->curl_multi_perform(client->handle, &client->still_running) == CURLM_OK;

        if (!client->still_running)
        {
            client->curl_multi_remove_handle(client->handle, client->req);
            MemZero(client->post_fields, sizeof(client->post_fields));
        }
    }

    return result;
}

internal bool32
SaltyBetLogin(Salty_Bet_Client *client, char *email, char *pword)
{
#ifdef USE_EMU
    client->curl_easy_setopt(client->req, CURLOPT_URL, "http://localhost:8000/authenticate?signin=1");
#else
    client->curl_easy_setopt(client->req, CURLOPT_URL, "https://www.saltybet.com/authenticate?signin=1");
#endif


    char *escaped_email = client->curl_easy_escape(0, email, 0);
    char *escaped_pword = client->curl_easy_escape(0, pword, 0);
    sprintf(client->post_fields, "email=%s&pword=%s&authenticate=signin", escaped_email, escaped_pword);
    client->curl_free(escaped_email);
    client->curl_free(escaped_pword);
    client->curl_easy_setopt(client->req, CURLOPT_POSTFIELDS, client->post_fields);

    return SaltyBetBeginRequest(client);
}

internal bool32
SaltyBetGetHome(Salty_Bet_Client *client)
{
#ifdef USE_EMU
    client->curl_easy_setopt(client->req, CURLOPT_URL, "http://localhost:8000/");
#else
    client->curl_easy_setopt(client->req, CURLOPT_URL, "https://www.saltybet.com/");
#endif
    client->curl_easy_setopt(client->req, CURLOPT_POST, 0);

    return SaltyBetBeginRequest(client);
}

internal bool32
SaltyBetGetState(Salty_Bet_Client *client, u32 timestamp)
{
    char url[64];
#ifdef USE_EMU
    sprintf(url, "http://localhost:8000/state.json?t=%u", timestamp);
#else
    sprintf(url, "https://www.saltybet.com/state.json?t=%u", timestamp);
#endif

    client->curl_easy_setopt(client->req, CURLOPT_URL, url);
    client->curl_easy_setopt(client->req, CURLOPT_POST, 0);

    return SaltyBetBeginRequest(client);
}

internal bool32
SaltyBetGetZdata(Salty_Bet_Client *client, u32 timestamp)
{
    char url[64];
#if USE_EMU
    sprintf(url, "http://localhost:8000/zdata.json?t=%u", timestamp);
#else
    sprintf(url, "https://www.saltybet.com/zdata.json?t=%u", timestamp);
#endif

    client->curl_easy_setopt(client->req, CURLOPT_URL, url);
    client->curl_easy_setopt(client->req, CURLOPT_POST, 0);

    return SaltyBetBeginRequest(client);
}

// NOTE: Player is 1 or 2.
internal bool32
SaltyBetPlaceBet(Salty_Bet_Client *client, u32 player, i64 wager)
{
#ifdef USE_EMU
    client->curl_easy_setopt(client->req, CURLOPT_URL, "http://localhost:8000/ajax_place_bet.php");
#else
    client->curl_easy_setopt(client->req, CURLOPT_URL, "https://www.saltybet.com/ajax_place_bet.php");
#endif

    sprintf(client->post_fields, "selectedplayer=player%u&wager=%lld", player, wager);
    client->curl_easy_setopt(client->req, CURLOPT_POSTFIELDS, client->post_fields);

    return SaltyBetBeginRequest(client);
}

internal bool32
SaltyBetGetTournamentStart(Salty_Bet_Client *client)
{
#ifdef USE_EMU
    client->curl_easy_setopt(client->req, CURLOPT_URL, "http://localhost:8000/ajax_tournament_start.php");
#else
    client->curl_easy_setopt(client->req, CURLOPT_URL, "https://www.saltybet.com/ajax_tournament_start.php");
#endif
    client->curl_easy_setopt(client->req, CURLOPT_POST, 0);

    return SaltyBetBeginRequest(client);
}

internal bool32
SaltyBetGetTournamentEnd(Salty_Bet_Client *client)
{
#ifdef USE_EMU
    client->curl_easy_setopt(client->req, CURLOPT_URL, "http://localhost:8000/ajax_tournament_end.php");
#else
    client->curl_easy_setopt(client->req, CURLOPT_URL, "https://www.saltybet.com/ajax_tournament_end.php");
#endif
    client->curl_easy_setopt(client->req, CURLOPT_POST, 0);

    return SaltyBetBeginRequest(client);
}

enum Salty_Bet_Call
{
    SaltyBetCall_Login = 0,
    SaltyBetCall_GetHome,
    SaltyBetCall_GetState,
    SaltyBetCall_GetZdata,
    SaltyBetCall_PlaceBet,
    SaltyBetCall_GetTournamentStart,
    SaltyBetCall_GetTournamentEnd,
};

union Call_Arg
{
    i64 num;
    char *chr;

    constexpr Call_Arg(i64 n) : num(n) {}
    constexpr Call_Arg(char *c) : chr(c) {}
};

struct Client_Get_Result
{
    char *data;
    mem_t size;
    bool32 failed;
};

internal inline void
SaltyBetClientCooldown(Salty_Bet_Client *client)
{
    client->poll_counter = SALTY_BET_POLL_INTERVAL;
}

internal Client_Get_Result
SaltyBetClientGet(Salty_Bet_Client *client, f32 time_delta,
                  Salty_Bet_Call call, Call_Arg *args = 0, bool32 auto_cooldown = true)
{
    Client_Get_Result result = {};

    if (client->attempted)
    {
        if (SaltyBetEndRequest(client))
        {
            if (client->success)
            {
                // NOTE: Adds null terminator to end of response.
                GrowableArenaAlloc(&client->arena, 1, true);
                result.data = (char *)client->arena.base;
                result.size = client->arena.used;
            }
            else
            {
                result.failed = true;
                if (auto_cooldown)
                    SaltyBetClientCooldown(client);
            }
        }
    }
    else
    {
        client->poll_counter -= time_delta;
        if (client->poll_counter <= 0.0f)
        {
            switch (call)
            {
                case SaltyBetCall_Login: SaltyBetLogin(client, args[0].chr, args[1].chr); break;
                case SaltyBetCall_GetHome: SaltyBetGetHome(client); break;
                case SaltyBetCall_GetState: SaltyBetGetState(client, (u32)args[0].num); break;
                case SaltyBetCall_GetZdata: SaltyBetGetZdata(client, (u32)args[0].num); break;
                case SaltyBetCall_PlaceBet: SaltyBetPlaceBet(client, (u32)args[0].num, args[1].num); break;
                case SaltyBetCall_GetTournamentStart: SaltyBetGetTournamentStart(client); break;
                case SaltyBetCall_GetTournamentEnd: SaltyBetGetTournamentEnd(client); break;
            }
        }
    }

    return result;
}
