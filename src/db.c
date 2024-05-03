#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <sqlite3.h>

#include "db.h"

int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    int i;
    for(i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

void create_table(sqlite3 *db)
{
    char *zErrMsg = 0;
    int rc;
    char *sql;

    sql = "CREATE TABLE IF NOT EXISTS steam_games(" \
          "game_id INTEGER PRIMARY KEY," \
          "game_name TEXT NOT NULL," \
          "playtime INTEGER DEFAULT 0);";  // playtime in minutes

    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Table created successfully\n");
    }
}

void create_non_steam_table(sqlite3 *db)
{
    char *zErrMsg = 0;
    int rc;
    char *sql = "CREATE TABLE IF NOT EXISTS non_steam_games(" \
                "game_id INTEGER PRIMARY KEY AUTOINCREMENT," \
                "game_name TEXT NOT NULL," \
                "install_path TEXT NOT NULL," \
                "playtime INTEGER DEFAULT 0);";

    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Non-Steam games table created successfully\n");
    }
}

void insert_game(sqlite3 *db, int game_id, const char *game_name, int playtime)
{
    char *zErrMsg = 0;
    int rc;
    sqlite3_stmt *stmt;
    const char *sql = "INSERT OR IGNORE INTO steam_games (game_id, game_name, playtime) VALUES (?, ?, ?);";

    // Prepare the statement
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        return;
    }

    // Bind integer game_id to the first placeholder
    sqlite3_bind_int(stmt, 1, game_id);
    // Bind string game_name to the second placeholder
    sqlite3_bind_text(stmt, 2, game_name, -1, SQLITE_TRANSIENT);
    // Bind integer playtime to the third placeholder
    sqlite3_bind_int(stmt, 3, playtime);

    // Execute the statement
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    } else {
        // Check if a record was inserted
        if (sqlite3_changes(db) > 0) {
            fprintf(stdout, "Record inserted successfully\n");
        } else {
            fprintf(stdout, "Record already exists: ID: %d, Name: %s\n", game_id, game_name);
        }
    }

    // Finalize the statement to prevent memory leaks
    sqlite3_finalize(stmt);
}

void insert_non_steam_game(sqlite3 *db, const char *game_name, const char *install_path, int playtime)
{
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO non_steam_games (game_name, install_path, playtime) VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, game_name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, install_path, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, playtime);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            fprintf(stderr, "SQL error while inserting non-Steam game: %s\n", sqlite3_errmsg(db));
        } else {
            fprintf(stdout, "Non-Steam game inserted successfully\n");
        }

        sqlite3_finalize(stmt);
    } else {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    }
}

void db_fetch_all_games(const char *db_path, DBRowCallback callback, void *user_data) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    const char *sql = "SELECT game_id, game_name, install_path, playtime FROM non_steam_games "
                      "UNION ALL "
                      "SELECT game_id, game_name, NULL AS install_path, playtime FROM steam_games "
                      "ORDER BY game_name ASC";
    int rc;

    rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int game_id = sqlite3_column_int(stmt, 0);
        const char *game_name = (const char *)sqlite3_column_text(stmt, 1);
        const char *install_path = (const char *)sqlite3_column_text(stmt, 2);
        int playtime = sqlite3_column_int(stmt, 3);

        callback(game_id, game_name, install_path, playtime, user_data);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}
