#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lib/sqlite3.h"
#include "database.h"

/* 生成 UUID v4，输出 buffer 至至少 37 字节（含终止符） */
static void generate_uuid_v4(char out[37]) {
    unsigned char b[16];
    for (int i = 0; i < 16; ++i) {
        b[i] = (unsigned char)(rand() & 0xFF);
    }
    /* set version to 4 --- xxxx0100 */
    b[6] = (b[6] & 0x0F) | 0x40;
    /* set variant to 10xxxxxx */
    b[8] = (b[8] & 0x3F) | 0x80;
    sprintf(out,
            "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            b[0], b[1], b[2], b[3],
            b[4], b[5],
            b[6], b[7],
            b[8], b[9],
            b[10], b[11], b[12], b[13], b[14], b[15]);
}

void load_test_data() {
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    int rc;

    srand((unsigned)time(NULL));

    rc = sqlite3_open("vocab_system.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to open Database: %s\n", sqlite3_errmsg(db));
        return;
    }

    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Transaction Failed: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    const char *insert_sql = "INSERT OR REPLACE INTO users "
        "(uuid, username, password_hash, user_level, class_name, student_num, teacher_uuid) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";
    rc = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "准备插入语句失败: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    for (int i = 0; i < 10; ++i) {
        char uuid[37];
        char username[32];
        const char *password_hash = "177621";
        int user_level = 2;
        const char *class_name;
        const char *teacher_uuid;

        generate_uuid_v4(uuid);
        snprintf(username, sizeof(username), "stu%d", i);

        if (i >= 0 && i <= 4) {
            class_name = "1";
            teacher_uuid = "50615c09-71e9-4bc7-875b-62718ec0873d";
        } else {
            class_name = "2";
            teacher_uuid = "238d3617-beea-46df-b2b6-b1f9d213f4d4";
        }

        sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, username, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, password_hash, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, user_level);
        sqlite3_bind_text(stmt, 5, class_name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 6, i);
        sqlite3_bind_text(stmt, 7, teacher_uuid, -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "插入 %s 失败: %s\n", username, sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
            sqlite3_close(db);
            return;
        }

        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    sqlite3_close(db);

    printf("\n测试数据已写入数据库。\n");
}