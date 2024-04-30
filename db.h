#ifndef __DB_H__
#define __DB_H__

#include <sqlite3.h>

void create_table(sqlite3 *db);
void insert_game(sqlite3 *db, int game_id, const char *game_name);

#endif /* __DB_H__ */
