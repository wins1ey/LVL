#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

#include "validation.h"
#include "steam.h"

gboolean validate_steam_credentials(const char *api_key, const char *steam_id)
{
    // Example validation: Check if the lengths are within a reasonable range and if all characters are alphanumeric
    // Steam API key is typically 32 characters long
    if (api_key == NULL || strlen(api_key) != 32) {
        fprintf(stderr, "Invalid API Key length.\n");
        return FALSE;
    }

    // Steam ID is typically a numeric value; let's check for a reasonable length
    if (steam_id == NULL || strlen(steam_id) < 17 || strlen(steam_id) > 18 || strspn(steam_id, "0123456789") != strlen(steam_id)) {
        fprintf(stderr, "Invalid Steam ID format.\n");
        return FALSE;
    }

    // If basic checks pass, proceed to validate with a Steam API call
    CURL *curl;
    CURLcode res;
    char url[512];
    sprintf(url, "http://api.steampowered.com/ISteamUser/GetPlayerSummaries/v0002/?key=%s&steamids=%s", api_key, steam_id);

    curl = curl_easy_init();
    if (curl) {
        MemoryStruct chunk = {0};
        chunk.memory = malloc(1);
        chunk.size = 0;

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            free(chunk.memory);
            curl_easy_cleanup(curl);
            return FALSE;
        } else {
            cJSON *json = cJSON_Parse(chunk.memory);
            cJSON *response = cJSON_GetObjectItemCaseSensitive(json, "response");
            cJSON *players = cJSON_GetObjectItemCaseSensitive(response, "players");
            size_t num_players = cJSON_GetArraySize(players);

            // Check if players data actually contains entries
            gboolean valid_credentials = num_players > 0;

            cJSON_Delete(json);
            free(chunk.memory);
            curl_easy_cleanup(curl);

            return valid_credentials;
        }
    }
    return FALSE;
}
