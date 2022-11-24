#pragma once


#include "paprika.h"


internal mem_t
WriteMemoryCallback(void *contents, mem_t size, mem_t nmemb, void *userp)
{
    mem_t real_size = size * nmemb;
    Growable_Memory_Arena *arena = (Growable_Memory_Arena *)userp;
    MemCopy(contents, GrowableArenaAllocAndGet(arena, real_size), real_size);
    return real_size;
}

// NOTE: This should only be called once, from the platform layer. This allows us
// to use the Salty_Bet_Client from a dynamically loaded library since it sets
// pointers to libcurl API functions.
internal void
SaltyBetClientInit(Salty_Bet_Client *client)
{
    curl_global_init(CURL_GLOBAL_ALL);

    client->handle = curl_multi_init();
    client->req = curl_easy_init();

#ifdef INTERNAL_BUILD
    curl_easy_setopt(client->req, CURLOPT_VERBOSE, 1);
#endif

    curl_easy_setopt(client->req, CURLOPT_COOKIEFILE, "");
    curl_easy_setopt(client->req, CURLOPT_FOLLOWLOCATION, 0);
    curl_easy_setopt(client->req, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(client->req, CURLOPT_USERAGENT, "paprika/2.0");
    curl_easy_setopt(client->req, CURLOPT_WRITEDATA, &client->arena);
    curl_easy_setopt(client->req, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    client->still_running = false;
    client->attempted = false;
    client->success = false;
    client->poll_counter = 0.0f;
    client->arena = {};
    
    client->curl_multi_add_handle = curl_multi_add_handle;
    client->curl_multi_remove_handle = curl_multi_remove_handle;
    client->curl_multi_perform = curl_multi_perform;
    client->curl_easy_setopt = curl_easy_setopt;
    client->curl_free = curl_free;
    client->curl_easy_escape = curl_easy_escape;
}
