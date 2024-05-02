#ifndef __DB_H__
#define __DB_H__

#include <sqlite3.h>

typedef void (*DBRowCallback)(int game_id, const char *game_name, const char *install_path, int playtime, void *user_data);
void create_table(sqlite3 *db);
void create_non_steam_table(sqlite3 *db);
void insert_game(sqlite3 *db, int game_id, const char *game_name, int playtime);
void insert_non_steam_game(sqlite3 *db, const char *game_name, const char *install_path, int playtime);
void db_fetch_all_games(const char *db_path, DBRowCallback callback, void *user_data);

#endif /* __DB_H__ */
