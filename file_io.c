#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_io.h"
#include "database.h"

/**
 * @brief 将所有题目导出到 timu.txt 文件
 */
int exportQuestionsToFile(const char* filename) {
    if (!filename) {
        fprintf(stderr, "[错误] 文件名为空\n");
        return 0;
    }
    
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "[错误] 无法打开文件 %s\n", filename);
        return 0;
    }
    
    int count = 0;
    struct Question* questions = getQuestions(&count);
    
    if (!questions || count == 0) {
        fprintf(fp, "暂无题目\n");
        fclose(fp);
        printf("[信息] 题目文件已创建: %s (暂无题目)\n", filename);
        return 1;
    }

    /* 严格的 timu.txt 格式：仅包含题号和英文单词（学生做题时不应看到翻译） */
    fprintf(fp, "# TIMU 文件 (题号. 英文单词)\n");
    fprintf(fp, "# 题目总数: %d\n", count);
    fprintf(fp, "# 每题分值: %.2f\n", 100.0 / count);
    fprintf(fp, "\n");

    /* 写入每道题目为一行："1. word" */
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d. %s\n", i + 1, questions[i].word);
    }
    
    fclose(fp);
    freeQuestions(questions);
    
    printf("[成功] 题目已导出到 %s (%d 道题目)\n", filename, count);
    return 1;
}

/**
 * @brief 将所有学生成绩导出到 stu.txt 文件
 */
int exportGradesToFile(const char* filename) {
    if (!filename) {
        fprintf(stderr, "[错误] 文件名为空\n");
        return 0;
    }
    
    FILE* fp = fopen(filename, "a");  /* 追加模式 */
    if (!fp) {
        fprintf(stderr, "[错误] 无法打开文件 %s\n", filename);
        return 0;
    }
    
    /* 写入时间戳或头部 (首次写入) */
    fprintf(fp, "\n========== 学生成绩记录 ==========\n");
    fprintf(fp, "记录时间: 系统自动生成\n");
    fprintf(fp, "格式: 学号, 姓名, 班级, 成绩, 正确率\n");
    fprintf(fp, "====================================\n");
    
    fclose(fp);
    printf("[成功] 成绩文件已初始化: %s\n", filename);
    return 1;
}

/**
 * @brief 将学生答题记录追加到 stu.txt 文件
 */
int addStuGradeToFile(const char* filename, const char* student_uuid, 
                              const char* student_name, const char* class_name,
                              int student_num, int total_score) {
    if (!filename || !student_name || !class_name) {
        fprintf(stderr, "[错误] 参数不完整\n");
        return 0;
    }
    
    FILE* fp = fopen(filename, "a");
    if (!fp) {
        fprintf(stderr, "[错误] 无法打开文件 %s\n", filename);
        return 0;
    }
    
    fprintf(fp, "%d, %s, %s, %d, %.0f%%\n",
            student_num, student_name, class_name, total_score, (total_score / 100.0) * 100);
    
    fclose(fp);
    return 1;
}

int addStuAnsToFile(const char* filename, const char* student_name,
                               const char* class_name, int student_num,
                               int question_count, const char** questions,
                               const char** user_answers, const char** correct_answers) {
    if (!filename || !student_name) return 0;
    FILE* fp = fopen(filename, "a");
    if (!fp) return 0;

    fprintf(fp, "\n--- 答题详情: %s (学号: %d, 班级: %s) ---\n", student_name, student_num, class_name ? class_name : "N/A");
    for (int i = 0; i < question_count; ++i) {
        fprintf(fp, "Q%d. %s\n", i+1, questions[i] ? questions[i] : "");
        fprintf(fp, "  学生答案: %s\n", user_answers[i] ? user_answers[i] : "");
        fprintf(fp, "  正确答案: %s\n\n", correct_answers[i] ? correct_answers[i] : "");
    }

    fclose(fp);
    return 1;
}

/**
 * @brief 获取按班级分组的所有学生，用于导出
 */
static struct GradeInfo** getGradesByAllClasses(int* class_count, int** counts) {
    /* 简化版：先获取所有成绩，然后按班级分组 */
    sqlite3 *db;
    int rc = sqlite3_open("vocab_system.db", &db);
    if (rc) {
        *class_count = 0;
        return NULL;
    }
    
    const char* sql_classes = "SELECT DISTINCT class_name FROM users WHERE class_name IS NOT NULL AND class_name != '' ORDER BY class_name";
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql_classes, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        *class_count = 0;
        return NULL;
    }
    
    char classes[20][50];
    *class_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && *class_count < 20) {
        strncpy(classes[*class_count], (const char*)sqlite3_column_text(stmt, 0), 49);
        (*class_count)++;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    return NULL;  /* 简化处理，直接在函数内遍历 */
}

/**
 * @brief 将按成绩排序的结果导出到 sort1.txt 文件
 */
int exportGradesByScoreToFile(const char* filename) {
    if (!filename) {
        fprintf(stderr, "[错误] 文件名为空\n");
        return 0;
    }
    
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "[错误] 无法打开文件 %s\n", filename);
        return 0;
    }
    
    /* 从数据库获取所有学生，按成绩排序 */
    sqlite3 *db;
    int rc = sqlite3_open("vocab_system.db", &db);
    if (rc) {
        fprintf(stderr, "[错误] 无法打开数据库\n");
        fclose(fp);
        return 0;
    }
    
    const char* sql = "SELECT u.username, u.class_name, u.student_num, "
                      "COALESCE(SUM(ar.score), 0) as total_score, "
                      "COALESCE(COUNT(ar.aid), 0) as question_count, "
                      "COALESCE(CAST(SUM(ar.is_correct) AS FLOAT) / NULLIF(COUNT(ar.aid), 0), 0.0) as accuracy "
                      "FROM users u "
                      "LEFT JOIN answer_records ar ON u.uuid = ar.student_uuid "
                      "WHERE u.user_level = 2 "
                      "GROUP BY u.uuid "
                      "ORDER BY total_score DESC, u.student_num ASC";
    
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[错误] 准备 SQL 语句失败\n");
        sqlite3_close(db);
        fclose(fp);
        return 0;
    }
    
    fprintf(fp, "========== 学生成绩统计 (按成绩排序) ==========\n");
    fprintf(fp, "排序方式: 成绩降序\n");
    fprintf(fp, "%-20s %-15s %-10s %-10s %-10s\n", "姓名", "班级", "学号", "成绩", "正确率");
    fprintf(fp, "%-20s %-15s %-10s %-10s %-10s\n", 
            "--------------------", "---------------", "----------", "----------", "----------");
    
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* username = (const char*)sqlite3_column_text(stmt, 0);
        const char* class_name = (const char*)sqlite3_column_text(stmt, 1);
        int student_num = sqlite3_column_int(stmt, 2);
        int total_score = sqlite3_column_int(stmt, 3);
        double accuracy = sqlite3_column_double(stmt, 5);
        
        fprintf(fp, "%-20s %-15s %-10d %-10d %-9.1f%%\n",
                username, class_name ? class_name : "N/A", student_num, total_score, accuracy * 100);
        count++;
    }
    
    fprintf(fp, "\n总计: %d 名学生\n", count);
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    fclose(fp);
    
    printf("[成功] 按成绩排序的结果已导出到 %s (%d 名学生)\n", filename, count);
    return 1;
}

/**
 * @brief 将按班级排序的结果导出到 sort2.txt 文件
 */
int exportGradesByClassToFile(const char* filename) {
    if (!filename) {
        fprintf(stderr, "[错误] 文件名为空\n");
        return 0;
    }
    
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "[错误] 无法打开文件 %s\n", filename);
        return 0;
    }
    
    /* 从数据库获取所有学生，按班级和成绩排序 */
    sqlite3 *db;
    int rc = sqlite3_open("vocab_system.db", &db);
    if (rc) {
        fprintf(stderr, "[错误] 无法打开数据库\n");
        fclose(fp);
        return 0;
    }
    
    const char* sql = "SELECT u.username, u.class_name, u.student_num, "
                      "COALESCE(SUM(ar.score), 0) as total_score, "
                      "COALESCE(COUNT(ar.aid), 0) as question_count, "
                      "COALESCE(CAST(SUM(ar.is_correct) AS FLOAT) / NULLIF(COUNT(ar.aid), 0), 0.0) as accuracy "
                      "FROM users u "
                      "LEFT JOIN answer_records ar ON u.uuid = ar.student_uuid "
                      "WHERE u.user_level = 2 "
                      "GROUP BY u.uuid "
                      "ORDER BY u.class_name ASC, total_score DESC";
    
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[错误] 准备 SQL 语句失败\n");
        sqlite3_close(db);
        fclose(fp);
        return 0;
    }
    
    fprintf(fp, "========== 学生成绩统计 (按班级排序) ==========\n");
    fprintf(fp, "排序方式: 班级升序, 成绩降序\n");
    fprintf(fp, "%-20s %-15s %-10s %-10s %-10s\n", "姓名", "班级", "学号", "成绩", "正确率");
    fprintf(fp, "%-20s %-15s %-10s %-10s %-10s\n", 
            "--------------------", "---------------", "----------", "----------", "----------");
    
    char current_class[50] = "";
    int count = 0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* username = (const char*)sqlite3_column_text(stmt, 0);
        const char* class_name = (const char*)sqlite3_column_text(stmt, 1);
        int student_num = sqlite3_column_int(stmt, 2);
        int total_score = sqlite3_column_int(stmt, 3);
        double accuracy = sqlite3_column_double(stmt, 5);
        
        /* 班级变化时打印班级标题 */
        if (strcmp(current_class, (class_name ? class_name : "N/A")) != 0) {
            if (count > 0) fprintf(fp, "\n");
            fprintf(fp, "--- 班级: %s ---\n", class_name ? class_name : "N/A");
            strncpy(current_class, class_name ? class_name : "N/A", 49);
        }
        
        fprintf(fp, "%-20s %-15s %-10d %-10d %-9.1f%%\n",
                username, class_name ? class_name : "N/A", student_num, total_score, accuracy * 100);
        count++;
    }
    
    fprintf(fp, "\n总计: %d 名学生\n", count);
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    fclose(fp);
    
    printf("[成功] 按班级排序的结果已导出到 %s (%d 名学生)\n", filename, count);
    return 1;
}
