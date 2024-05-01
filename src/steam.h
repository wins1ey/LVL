#ifndef __STEAM_H__
#define __STEAM_H__

#include <sqlite3.h>

void fetch_data_from_steam_api(const char *api_key, const char *steam_id, sqlite3 *db);

#endif /* __STEAM_H__ */
