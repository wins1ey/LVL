#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <sqlite3.h>

#include "steam.h"
#include "db.h"

typedef struct {
    char *memory;
    size_t size;
} MemoryStruct;

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t real_size = size * nmemb;
    MemoryStruct *mem = (MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + real_size + 1);
    if(ptr == NULL) {
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->memory[mem->size] = 0;

    return real_size;
}

void fetch_data_from_steam_api(const char *api_key, const char *steam_id, sqlite3 *db)
{
    CURL *curl;
    CURLcode res;
    char player_summary_url[512];
    char owned_games_url[512];
    sprintf(player_summary_url, "http://api.steampowered.com/ISteamUser/GetPlayerSummaries/v0002/?key=%s&steamids=%s", api_key, steam_id);
    sprintf(owned_games_url, "http://api.steampowered.com/IPlayerService/GetOwnedGames/v0001/?key=%s&steamid=%s&format=json&include_appinfo=true", api_key, steam_id);

    curl = curl_easy_init();
    if(curl) {
        MemoryStruct chunk;
        chunk.memory = malloc(1);  // will be grown as needed by the realloc above
        chunk.size = 0;    // no data at this point

        curl_easy_setopt(curl, CURLOPT_URL, owned_games_url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            cJSON *json = cJSON_Parse(chunk.memory);
            cJSON *response = cJSON_GetObjectItemCaseSensitive(json, "response");
            cJSON *games = cJSON_GetObjectItemCaseSensitive(response, "games");
            cJSON *game;

            cJSON_ArrayForEach(game, games) {
                cJSON *name = cJSON_GetObjectItemCaseSensitive(game, "name");
                cJSON *appid = cJSON_GetObjectItemCaseSensitive(game, "appid");
                cJSON *playtime = cJSON_GetObjectItemCaseSensitive(game, "playtime_forever");  // In minutes

                int game_id = appid ? appid->valueint : -1;
                const char *game_name = name ? name->valuestring : "Unknown";
                int game_playtime = playtime ? playtime->valueint : 0;

                insert_game(db, game_id, game_name, game_playtime);
            }
            cJSON_Delete(json);
        }
        free(chunk.memory);
        curl_easy_cleanup(curl);
    }
}
