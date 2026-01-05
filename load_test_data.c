#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lib/sqlite3.h"
#include "database.h"

/**
 * @brief 初始化数据库（创建必要的表）
 * @return 成功返回 1，失败返回 0
 */
int initDatabase(sqlite3* db) {
    /* 创建 users 表 */
    const char* sql_users = "CREATE TABLE IF NOT EXISTS users (uuid TEXT PRIMARY KEY, username TEXT NOT NULL UNIQUE, password_hash TEXT NOT NULL, user_level INTEGER, class_name TEXT, student_num INTEGER, teacher_uuid TEXT)";
    /* 创建 questions 表 */
    const char* sql_questions = "CREATE TABLE IF NOT EXISTS questions (qid INTEGER PRIMARY KEY AUTOINCREMENT, word TEXT NOT NULL UNIQUE, translate TEXT NOT NULL, difficulty INTEGER DEFAULT 1)";
    /* 创建 answer_records 表 */
    const char* sql_answers = "CREATE TABLE IF NOT EXISTS answer_records (aid INTEGER PRIMARY KEY AUTOINCREMENT, student_uuid TEXT, qid INTEGER, user_answer TEXT, is_correct INTEGER, score INTEGER)";

    char* errmsg = 0;
    int rc = sqlite3_exec(db, sql_users, 0, 0, &errmsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[ERROR] Create users table failed: %s\n", errmsg);
        return 0;
    }
    rc = sqlite3_exec(db, sql_questions, 0, 0, &errmsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[ERROR] Create questions table failed: %s\n", errmsg);
        return 0;
    }
    rc = sqlite3_exec(db, sql_answers, 0, 0, &errmsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[ERROR] Create answers table failed: %s\n", errmsg);
        return 0;
    }
    return 1;
}

/**
 * @brief 生成 UUID v4，输出 buffer 至至少 37 字节（含终止符）
 * @param out 输出生成结果到 out 中
 */
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

/**
 * @brief 生成测试用户数据。包含 stu0 - stu9。stu0-stu4 班级为 1，stu4-stu9 班级为 2。 
 */
void load_test_user_data() {
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    int rc;

    // 以时间为种子生成随机数
    srand((unsigned)time(NULL));

    rc = sqlite3_open("vocab_system.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to open Database: %s\n", sqlite3_errmsg(db));
        return;
    }

    const char *sql = "BEGIN TRANSACTION";
    rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
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

    // 插入 teacher0 的测试数据

    char teacher0_uuid[37];
    generate_uuid_v4(teacher0_uuid);

    sqlite3_bind_text(stmt, 1, teacher0_uuid, -1, SQLITE_TRANSIENT); // uuid
    sqlite3_bind_text(stmt, 2, "teacher0", -1, SQLITE_TRANSIENT); // username
    sqlite3_bind_text(stmt, 3, "177621", -1, SQLITE_TRANSIENT); // password_hash
    sqlite3_bind_int(stmt, 4, 1); // user_level = 1 (teacher)
    sqlite3_bind_text(stmt, 5, "1", -1, SQLITE_TRANSIENT); // class_name
    sqlite3_bind_null(stmt, 6); // student_num = NULL for teacher
    sqlite3_bind_null(stmt, 7); // teacher_uuid = NULL for teacher

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "插入 teacher0 失败: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        sqlite3_close(db);
        return;
    }
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    // 插入 teacher1 的测试数据

    char teacher1_uuid[37];
    generate_uuid_v4(teacher1_uuid);

    sqlite3_bind_text(stmt, 1, teacher1_uuid, -1, SQLITE_TRANSIENT); // uuid
    sqlite3_bind_text(stmt, 2, "teacher1", -1, SQLITE_TRANSIENT); // username
    sqlite3_bind_text(stmt, 3, "177621", -1, SQLITE_TRANSIENT); // password_hash
    sqlite3_bind_int(stmt, 4, 1); // user_level = 1 (teacher)
    sqlite3_bind_text(stmt, 5, "1", -1, SQLITE_TRANSIENT); // class_name
    sqlite3_bind_null(stmt, 6); // student_num = NULL for teacher
    sqlite3_bind_null(stmt, 7); // teacher_uuid = NULL for teacher

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "插入 teacher1 失败: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        sqlite3_close(db);
        return;
    }
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);


    // 插入 stu0 - stu9 的测试数据

    for (int i = 0; i < 10; ++i) {
        
        char uuid[37];
        char username[32];
        const char *password_hash = "177621"; // stu0 - stu9 的密码均为 0， hash 值为 177621
        int user_level = 2;
        const char *class_name;
        const char *teacher_uuid;

        generate_uuid_v4(uuid);
        snprintf(username, sizeof(username), "stu%d", i);

        if (i >= 0 && i <= 4) {
            class_name = "1";
            teacher_uuid = teacher0_uuid;
        } else {
            class_name = "2";
            teacher_uuid = teacher1_uuid;
        }

        sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, username, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, password_hash, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, user_level);
        sqlite3_bind_text(stmt, 5, class_name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 6, i+100); // 测试数据学号统一为 10x
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

/**
 * @brief 加载样本题目（至少 5 道）到数据库
 * 样本题目：常见英文单词及其翻译
 */
void load_sample_questions(void) {
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    int rc;

    rc = sqlite3_open("vocab_system.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "无法打开数据库: %s\n", sqlite3_errmsg(db));
        return;
    }

    /* 若数据库中已经存在题目，则不载入测试数据。 */
    const char *check_sql = "SELECT COUNT(*) FROM questions";
    sqlite3_stmt *check_stmt = NULL;
    rc = sqlite3_prepare_v2(db, check_sql, -1, &check_stmt, NULL);
    if(rc == SQLITE_OK && sqlite3_step(check_stmt) == SQLITE_ROW){
        int count = sqlite3_column_int(check_stmt, 0);
        sqlite3_finalize(check_stmt);
        if(count > 0){
            sqlite3_close(db);
            printf("在当前数据库读取到 %d 道题目。\n", count);
            return;
        }
    }


    const char *transaction = "BEGIN TRANSACTION;";
    rc = sqlite3_exec(db, transaction, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to begin transaction: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    const char *insert_sql = "INSERT INTO questions (word, translate) VALUES (?, ?)";
    rc = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to insert data: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    /* 10 个样本题目 */
    const char *samples[][2] = {
        {"apple", "苹果"},
        {"book", "书"},
        {"cat", "猫"},
        {"dog", "狗"},
        {"elephant", "象"},
        {"friend", "朋友"},
        {"house", "房子"},
        {"idea", "主意"},
        {"journey", "旅程"},
        {"kingdom", "王国"}
    };

    for (int i = 0; i < 10; ++i) {
        sqlite3_bind_text(stmt, 1, samples[i][0], -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, samples[i][1], -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "插入题目 %s 失败: %s\n", samples[i][0], sqlite3_errmsg(db));
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

    printf("检测到当前题目数据库为空。已自动加载 10 道样本题目。\n");
}