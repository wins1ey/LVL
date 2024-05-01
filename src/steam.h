#ifndef __STEAM_H__
#define __STEAM_H__

#include <stdio.h>
#include <stdlib.h>

#include <curl/curl.h>
#include <sqlite3.h>

typedef struct {
    char *memory;
    size_t size;
} MemoryStruct;

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
void fetch_data_from_steam_api(const char *api_key, const char *steam_id, sqlite3 *db);

#endif /* __STEAM_H__ */
