#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "database.h"
#include "lib/sqlite3.h"
#include "load_test_data.h"

/**
 * @brief 生成 UUID
 * @return UUID 字符串
 */
char* generateUUID() {
    char* uuid = (char*)malloc(37);
    if (!uuid) return NULL;
    const char chars[] = "0123456789abcdef";
    srand(time(NULL));
    for(int i = 0; i < 8; i++) uuid[i] = chars[rand() % 16];
    uuid[8] = '-';
    for(int i = 9; i < 13; i++) uuid[i] = chars[rand() % 16];
    uuid[13] = '-';
    for(int i = 14; i < 18; i++) uuid[i] = chars[rand() % 16];
    uuid[18] = '-';
    for(int i = 19; i < 23; i++) uuid[i] = chars[rand() % 16];
    uuid[23] = '-';
    for(int i = 24; i < 36; i++) uuid[i] = chars[rand() % 16];
    uuid[36] = '\0';
    return uuid;
}


/**
 * @brief 对用户输入的密码进行简单的 hash 加密
 * @return hash 算法之后的密码
 */
char* hashPassword(const char* password) {
    unsigned int hash = 5381;
    for(int i = 0; password[i]; i++) {
        // 递推式 h[i + 1] = h[i] * 33 + password[i]
        hash = ((hash << 5) + hash) + password[i];
    }
    // 分配 11 字节的内存，最大 4294967295
    char* result = (char*)malloc(11);
    if (!result) return NULL;
    sprintf(result, "%u", hash);
    return result;
}

/* initDatabase 已迁移到 load_test_data.c */

/**
 * @brief 创建用户
 * @return 新创建用户的 UUID
 */
char* createUser(const char* username, const char* password, int level, const char* class_name, int num, const char* teacher_uuid) {
    sqlite3 *db;
    int rc = sqlite3_open("vocab_system.db", &db);
    if (rc) {
        fprintf(stderr, "[ERROR] Cannot open database\n");
        return NULL;
    }
    if (!initDatabase(db)) {
        sqlite3_close(db);
        return NULL;
    }
    
    char* uuid = generateUUID();
    if (!uuid) {
        sqlite3_close(db);
        return NULL;
    }
    
    char* pswd_hash = hashPassword(password);
    if (!pswd_hash) {
        free(uuid);
        sqlite3_close(db);
        return NULL;
    }
    
    const char* sql = "INSERT INTO users VALUES(?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[ERROR] Prepare SQL failed: %s\n", sqlite3_errmsg(db));
        free(pswd_hash);
        free(uuid);
        sqlite3_close(db);
        return NULL;
    }
    
    sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, pswd_hash, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, level);
    sqlite3_bind_text(stmt, 5, class_name ? class_name : "", -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, num);
    sqlite3_bind_text(stmt, 7, teacher_uuid ? teacher_uuid : "", -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "[ERROR] Insert user failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        free(pswd_hash);
        free(uuid);
        sqlite3_close(db);
        return NULL;
    }
    
    sqlite3_finalize(stmt);
    free(pswd_hash);
    sqlite3_close(db);
    
    printf("[SUCCESS] User created: %s\n", username);
    return uuid;
}

/**
 * @brief 用户登录
 */
char* loginUser(const char* username, const char* password) {
    sqlite3 *db;
    int rc = sqlite3_open("vocab_system.db", &db);
    if (rc) {
        fprintf(stderr, "[ERROR] Cannot open database\n");
        return NULL;
    }
    
    const char* sql = "SELECT uuid, password_hash FROM users WHERE username = ?";
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[ERROR] Prepare SQL failed\n");
        sqlite3_close(db);
        return NULL;
    }
    
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        fprintf(stderr, "[ERROR] User not found\n");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return NULL;
    }
    
    const char* stored_uuid = (const char*)sqlite3_column_text(stmt, 0);
    const char* stored_hash = (const char*)sqlite3_column_text(stmt, 1);
    
    char* input_hash = hashPassword(password);
    if (!input_hash || strcmp(stored_hash, input_hash) != 0) {
        fprintf(stderr, "[ERROR] Password incorrect\n");
        if (input_hash) free(input_hash);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return NULL;
    }
    
    char* result = strdup(stored_uuid);
    free(input_hash);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    return result;
}

/**
 * @brief 询问用户权限等级
 * @param uuid 用户的 UUID
 * @return 权限等级 0=Admin 1=Teacher 2=Student
 */
int getUserLevel(const char* uuid) {
    sqlite3 *db;
    int rc = sqlite3_open("vocab_system.db", &db);
    if (rc) return -1;
    
    const char* sql = "SELECT user_level FROM users WHERE uuid = ?";
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_STATIC);
    
    int level = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        level = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return level;
}

/**
 * @brief 删除用户
 * @return 若删除成功，返回 1
 */
int deleteUser(const char* uuid) {
    sqlite3 *db;
    int rc = sqlite3_open("vocab_system.db", &db);
    if (rc) return 0;
    
    const char* sql = "DELETE FROM users WHERE uuid = ?";
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    printf("[SUCCESS] User deleted\n");
    return 1;
}

/**
 * @brief 添加题目
 * @param word 英文单词
 * @param translate 对应的翻译
 * @return 若添加成功，返回 1
 */
int addQuestion(const char* word, const char* translate) {
    sqlite3 *db;
    int rc = sqlite3_open("vocab_system.db", &db);
    if (rc) {
        fprintf(stderr, "[ERROR] Cannot open database\n");
        return 0;
    }
    
    const char* sql = "INSERT INTO questions (word, translate) VALUES (?, ?)";
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[ERROR] Prepare SQL failed\n");
        sqlite3_close(db);
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, word, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, translate, -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "[ERROR] Add question failed\n");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    } else {
        printf("[SUCCESS] Question added\n");
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 1;
}

/**
 * @brief 删除题目
 * @return 若删除成功，返回 1
 */
int deleteQuestion(int qid) {
    sqlite3 *db;
    int rc = sqlite3_open("vocab_system.db", &db);
    if (rc) return 0;
    
    const char* sql = "DELETE FROM questions WHERE qid = ?";
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, qid);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "[ERROR] Delete question failed\n");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    } else {
        printf("[SUCCESS] Question deleted\n");
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 1;
}

/**
 * @brief 
 * @return 将生成的试卷放入链表
 */
struct Question* getQuestions(int* count) {
    sqlite3 *db;
    int rc = sqlite3_open("vocab_system.db", &db);
    if (rc) {
        *count = 0;
        return NULL;
    }
    
    const char* sql = "SELECT COUNT(*) FROM questions";
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK || sqlite3_step(stmt) != SQLITE_ROW) {
        *count = 0;
        if (stmt) sqlite3_finalize(stmt);
        sqlite3_close(db);
        return NULL;
    }
    
    *count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    
    if (*count == 0) {
        sqlite3_close(db);
        return NULL;
    }
    
    // 为试卷（链表）分配内存
    struct Question* questions = (struct Question*)malloc(sizeof(struct Question) * (*count));
    sql = "SELECT qid, word, translate FROM questions";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    int idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < *count) {
        questions[idx].qid = sqlite3_column_int(stmt, 0);
        strncpy(questions[idx].word, (const char*)sqlite3_column_text(stmt, 1), MAX_WORD_LENGTH - 1);
        strncpy(questions[idx].translate, (const char*)sqlite3_column_text(stmt, 2), MAX_TRANS_LENGTH - 1);
        idx++;
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return questions;
}

/**
 * @brief 释放试卷所占的内存
 */
void freeQuestions(struct Question* q) {
    if (q) free(q);
}

/** 
 * @brief 生成带下划线的单词提示（puzzled），例如 "conversation" -> "conver_ation"
 */
static void make_puzzled(const char* word, char* out, int out_size) {
    if (!word || !out || out_size <= 0) return;
    size_t len = strlen(word);
    if (len + 1 > (size_t)out_size) {
        // 如果输出缓冲区太小，截断复制
        strncpy(out, word, out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }
    strcpy(out, word);
    if (len <= 3) {
        /* 对短单词也随机替换一个字符为 '_' */
        int idx = (int)(rand() % (int)len);
        out[idx] = '_';
        return;
    }
    /* 选择中间附近的一个字符替换为 '_'，避免替换首尾 */
    int mid = (int)len / 2;
    int shift = rand() % 3 - 1; // -1,0,1
    int idx = mid + shift;
    if (idx <= 0) idx = 1;
    if (idx >= (int)len - 1) idx = (int)len - 2;
    out[idx] = '_';
}

/**
 * @brief 保存答题记录
 * @param student_uuid 学生的 UUID
 * @param qid 题目 ID
 * @param user_answer 用户的答案
 * @param is_correct 是否正确
 * @param score 该题分数
 * @return 若保存成功，返回 1
 */
int saveAnswerRecord(const char* student_uuid, int qid, const char* user_answer, int is_correct, int score) {
    sqlite3 *db;
    int rc = sqlite3_open("vocab_system.db", &db);
    if (rc) {
        fprintf(stderr, "[ERROR] Cannot open database\n");
        return 0;
    }
    
    const char* sql = "INSERT INTO answer_records (student_uuid, qid, user_answer, is_correct, score) VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[ERROR] Prepare SQL failed\n");
        sqlite3_close(db);
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, student_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, qid);
    sqlite3_bind_text(stmt, 3, user_answer, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, is_correct);
    sqlite3_bind_int(stmt, 5, score);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "[ERROR] Save answer failed\n");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 1;
}

/**
 * @brief 按姓名获取成绩
 */
struct GradeInfo* getGradesByName(const char* username, int* count) {
    sqlite3 *db;
    int rc = sqlite3_open("vocab_system.db", &db);
    if (rc) {
        *count = 0;
        return NULL;
    }
    
    const char* sql = "SELECT u.uuid, u.username, u.class_name, u.student_num, SUM(ar.score), COUNT(ar.aid), CAST(SUM(ar.is_correct) AS FLOAT) / COUNT(ar.aid) FROM users u LEFT JOIN answer_records ar ON u.uuid = ar.student_uuid WHERE u.username LIKE ? GROUP BY u.uuid";
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[ERROR] Prepare SQL failed\n");
        sqlite3_close(db);
        *count = 0;
        return NULL;
    }
    
    char pattern[110];
    snprintf(pattern, sizeof(pattern), "%%%s%%", username);
    sqlite3_bind_text(stmt, 1, pattern, -1, SQLITE_STATIC);
    
    int capacity = 10;
    struct GradeInfo* grades = (struct GradeInfo*)malloc(sizeof(struct GradeInfo) * capacity);
    *count = 0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (*count >= capacity) {
            capacity *= 2;
            grades = (struct GradeInfo*)realloc(grades, sizeof(struct GradeInfo) * capacity);
        }
        
        strncpy(grades[*count].uuid, (const char*)sqlite3_column_text(stmt, 0), 36);
        strncpy(grades[*count].username, (const char*)sqlite3_column_text(stmt, 1), 99);
        strncpy(grades[*count].class_name, (const char*)sqlite3_column_text(stmt, 2), 49);
        grades[*count].student_num = sqlite3_column_int(stmt, 3);
        grades[*count].total_score = sqlite3_column_int(stmt, 4);
        grades[*count].total_questions = sqlite3_column_int(stmt, 5);
        grades[*count].accuracy = sqlite3_column_double(stmt, 6);
        
        (*count)++;
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return grades;
}

/**
 * @brief 按班级获取成绩
 */
struct GradeInfo* getGradesByClass(const char* class_name, int* count) {
    sqlite3 *db;
    int rc = sqlite3_open("vocab_system.db", &db);
    if (rc) {
        *count = 0;
        return NULL;
    }
    
    // u - users; ar - arswer_records
    const char* sql = "SELECT u.uuid, u.username, u.class_name, u.student_num, SUM(ar.score), COUNT(ar.aid), CAST(SUM(ar.is_correct) AS FLOAT) / COUNT(ar.aid) FROM users u LEFT JOIN answer_records ar ON u.uuid = ar.student_uuid WHERE u.class_name = ? AND u.user_level = 2 GROUP BY u.uuid ORDER BY SUM(ar.score) DESC";
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[ERROR] Prepare SQL failed\n");
        sqlite3_close(db);
        *count = 0;
        return NULL;
    }
    
    sqlite3_bind_text(stmt, 1, class_name, -1, SQLITE_STATIC);
    
    int capacity = 20;
    struct GradeInfo* grades = (struct GradeInfo*)malloc(sizeof(struct GradeInfo) * capacity);
    *count = 0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (*count >= capacity) {
            capacity *= 2;
            grades = (struct GradeInfo*)realloc(grades, sizeof(struct GradeInfo) * capacity);
        }
        
        strncpy(grades[*count].uuid, (const char*)sqlite3_column_text(stmt, 0), 36);
        strncpy(grades[*count].username, (const char*)sqlite3_column_text(stmt, 1), 99);
        strncpy(grades[*count].class_name, (const char*)sqlite3_column_text(stmt, 2), 49);
        grades[*count].student_num = sqlite3_column_int(stmt, 3);
        grades[*count].total_score = sqlite3_column_int(stmt, 4);
        grades[*count].total_questions = sqlite3_column_int(stmt, 5);
        grades[*count].accuracy = sqlite3_column_double(stmt, 6);
        
        (*count)++;
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return grades;
}

/**
 * @brief 按学生学号返回获取成绩
 */
struct GradeInfo* getGradesByStudentNumRange(int min_num, int max_num, int* count) {
    sqlite3 *db;
    int rc = sqlite3_open("vocab_system.db", &db);
    if (rc) {
        *count = 0;
        return NULL;
    }
    
    const char* sql = "SELECT u.uuid, u.username, u.class_name, u.student_num, SUM(ar.score), COUNT(ar.aid), CAST(SUM(ar.is_correct) AS FLOAT) / COUNT(ar.aid) FROM users u LEFT JOIN answer_records ar ON u.uuid = ar.student_uuid WHERE u.student_num >= ? AND u.student_num <= ? AND u.user_level = 2 GROUP BY u.uuid ORDER BY u.student_num ASC";
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[ERROR] Prepare SQL failed\n");
        sqlite3_close(db);
        *count = 0;
        return NULL;
    }
    
    sqlite3_bind_int(stmt, 1, min_num);
    sqlite3_bind_int(stmt, 2, max_num);
    
    int capacity = 20;
    struct GradeInfo* grades = (struct GradeInfo*)malloc(sizeof(struct GradeInfo) * capacity);
    *count = 0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (*count >= capacity) {
            capacity *= 2;
            grades = (struct GradeInfo*)realloc(grades, sizeof(struct GradeInfo) * capacity);
        }
        
        strncpy(grades[*count].uuid, (const char*)sqlite3_column_text(stmt, 0), 36);
        strncpy(grades[*count].username, (const char*)sqlite3_column_text(stmt, 1), 99);
        strncpy(grades[*count].class_name, (const char*)sqlite3_column_text(stmt, 2), 49);
        grades[*count].student_num = sqlite3_column_int(stmt, 3);
        grades[*count].total_score = sqlite3_column_int(stmt, 4);
        grades[*count].total_questions = sqlite3_column_int(stmt, 5);
        grades[*count].accuracy = sqlite3_column_double(stmt, 6);
        
        (*count)++;
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return grades;
}

/**
 * @brief 按班级统计数据，并按分数段输出
 * @param class_name 
 */
void statisticsByClass(const char* class_name) {
    sqlite3 *db;
    int rc = sqlite3_open("vocab_system.db", &db);
    if (rc) {
        fprintf(stderr, "[ERROR] Cannot open database\n");
        return;
    }
    
    // 统计特定班级中所有学生的成绩，并计算出正确率
    // 按总得分降序排序
    const char* sql = "SELECT u.uuid, u.username, u.class_name, u.student_num, SUM(ar.score), COUNT(ar.aid), CAST(SUM(ar.is_correct) AS FLOAT) / COUNT(ar.aid) FROM users u LEFT JOIN answer_records ar ON u.uuid = ar.student_uuid WHERE u.class_name = ? AND u.user_level = 2 GROUP BY u.uuid ORDER BY SUM(ar.score) DESC";
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[ERROR] Prepare SQL failed\n");
        sqlite3_close(db);
        return;
    }
    
    sqlite3_bind_text(stmt, 1, class_name, -1, SQLITE_STATIC);
    
    int count_90 = 0, count_80 = 0, count_70 = 0, count_60 = 0, count_other = 0;
    int total_students = 0;
    
    printf("\n=== Grade Statistics for Class: %s ===\n", class_name);
    printf("%-20s %-15s %-10s %-10s\n", "姓名", "班级", "学号", "分数");
    printf("%-20s %-15s %-10s %-10s\n", "--------------------", "---------------", "----------", "----------");
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* username = (const char*)sqlite3_column_text(stmt, 1);
        const char* cls = (const char*)sqlite3_column_text(stmt, 2);
        int num = sqlite3_column_int(stmt, 3);
        int score = sqlite3_column_int(stmt, 4);
        
        // 固定输出间距 20 15 10 10
        printf("%-20s %-15s %-10d %-10d\n", username, cls, num, score);
        
        total_students++;
        if (score >= 90) count_90++;
        else if (score >= 80) count_80++;
        else if (score >= 70) count_70++;
        else if (score >= 60) count_60++;
        else count_other++;
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    printf("\n=== 分数统计 ===\n");
    printf("90-100: %d students\n", count_90);
    printf("80-89:  %d students\n", count_80);
    printf("70-79:  %d students\n", count_70);
    printf("60-69:  %d students\n", count_60);
    printf("<60:    %d students\n", count_other);
    printf("Total:  %d students\n", total_students);
}

/**
 * @brief 开始答题
 */
int startQuiz(const char* student_uuid, const char* student_name, const char* class_name, int student_num) {
    sqlite3 *db;
    int rc = sqlite3_open("vocab_system.db", &db);
    if (rc) {
        fprintf(stderr, "[ERROR] Cannot open database\n");
        return 0;
    }
    
    int count = 0;
    struct Question* questions = getQuestions(&count);
    if (!questions || count == 0) {
        printf("[ERROR] No questions available\n");
        sqlite3_close(db);
        return 0;
    }
    
    int points_per_question = 100 / count;
    int total_score = 0;
    int correct_count = 0;

    /* 为记录学生答案准备临时数组 */
    char **q_words = (char**)malloc(sizeof(char*) * count);
    char **u_answers = (char**)malloc(sizeof(char*) * count);
    char **c_answers = (char**)malloc(sizeof(char*) * count);
    for (int i = 0; i < count; ++i) { q_words[i] = u_answers[i] = c_answers[i] = NULL; }
    
    printf("\n====== Quiz: Vocabulary Scale ======\n");
    printf("考生信息: %s (班级: %s, 学号: %d)\n", student_name, class_name, student_num);
    printf("题目数量: %d, 每题分数: %d\n", count, points_per_question);
    printf("======================================\n\n");
    
    for (int i = 0; i < count; i++) {
        printf("[Question %d/%d]\n", i + 1, count);
        /* 生成并显示 word_puzzled，并显示翻译，要求输入正确的英文单词 */
        char puzzled[MAX_WORD_LENGTH];
        make_puzzled(questions[i].word, puzzled, sizeof(puzzled));
        printf("No. %d: %s\n", i, puzzled);
        printf("Translation: %s\n", questions[i].translate);
        printf("Your answer: ");

        char user_answer[MAX_TRANS_LENGTH];
        if (!fgets(user_answer, sizeof(user_answer), stdin)) {
            user_answer[0] = '\0';
        }
        user_answer[strcspn(user_answer, "\r\n")] = 0;

        int is_correct = (strcmp(user_answer, questions[i].word) == 0) ? 1 : 0;
        int score = is_correct ? points_per_question : 0;
        total_score += score;
        if (is_correct) correct_count++;
        
    saveAnswerRecord(student_uuid, questions[i].qid, user_answer, is_correct, score);
    /* 保存到临时数组，供导出使用（复制字符串） */
    q_words[i] = strdup(questions[i].translate); // question_words
    u_answers[i] = strdup(user_answer); // user_answers
    c_answers[i] = strdup(questions[i].word); // correct_answers
        
        /* 保存答题详情到 stu.txt（每题的学生答案和正确答案） */
        /* 这里只临时收集问题、学生答案和正确答案到数组，最后一次性写入文件 */
        /* 将在循环外统一处理 */
        
        printf(">> 正确答案: %s [%s]\n\n", questions[i].word, is_correct ? "CORRECT" : "WRONG");
    }
    
    printf("\n====== 测试结果 ======\n");
    printf("考生: %s\n", student_name);
    printf("正确数: %d / %d\n", correct_count, count);
    printf("总分: %d / 100\n", total_score);
    printf("正确率: %.2f%%\n", (correct_count * 100.0) / count);
    printf("========================\n\n");
    
    /* 将答题明细追加到 stu.txt（调用 file_io） */
    /* 检测 addStuAnsToFile 的符号在链接时存在 */
    /* 外部函数在 file_io.c 中实现，直接调用 */
    extern int addStuAnsToFile(const char*, const char*, const char*, int, int, const char**, const char**, const char**);
    addStuAnsToFile("stu.txt", student_name, class_name, student_num, count, (const char**)q_words, (const char**)u_answers, (const char**)c_answers);

    /* 释放临时数组 */
    for (int i = 0; i < count; ++i) {
        if (q_words[i]) free(q_words[i]);
        if (u_answers[i]) free(u_answers[i]);
        if (c_answers[i]) free(c_answers[i]);
    }
    free(q_words); free(u_answers); free(c_answers);
    free(questions);
    sqlite3_close(db);
    return total_score;
}

/**
 * @brief 释放读取成绩时的内存
 */
void freeGrades(struct GradeInfo* grades) {
    if (grades) free(grades);
}
