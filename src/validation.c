#include <stdio.h>
#include <string.h>
#include <glib.h>

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

    return TRUE;
}
