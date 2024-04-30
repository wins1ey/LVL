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

    /* Create SQL statement */
    sql = "CREATE TABLE IF NOT EXISTS games(" \
          "game_id INTEGER PRIMARY KEY," \
          "game_name TEXT NOT NULL);";

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Table created successfully\n");
    }
}

void insert_game(sqlite3 *db, int game_id, const char *game_name)
{
    char *zErrMsg = 0;
    int rc;
    sqlite3_stmt *stmt;
    const char *sql = "INSERT OR IGNORE INTO games (game_id, game_name) VALUES (?, ?);";

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
