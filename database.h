#ifndef DATABASE_H
#define DATABASE_H

#include <stdbool.h>
#include "lib/sqlite3.h"

#define MAX_WORD_LENGTH 100
#define MAX_TRANS_LENGTH 200
#define DB_NAME "vocab_system.db"

/* 题目 */
struct Question {
    int qid;
    char word[MAX_WORD_LENGTH];
    char translate[MAX_TRANS_LENGTH];
    char word_puzzled[MAX_WORD_LENGTH];
};

/* 学生的成绩信息 */
struct GradeInfo {
    char uuid[37];
    char username[100];
    char class_name[50];
    int student_num;
    int total_score;
    int total_questions;
    double accuracy;
};

/* 用户管理函数 */
char* createUser(const char* username, const char* password, int user_level, 
                 const char* class_name, int student_num, const char* teacher_uuid);
char* loginUser(const char* username, const char* password);
int getUserLevel(const char* uuid);
int deleteUser(const char* uuid);

/* 题目管理函数 */
int addSingleQuestion(const char* word, const char* translate);
int addQuestion(const char* source);
int deleteSingleQuestion(int qid);
struct Question* getQuestions(int* count);

/* 回答问题的相关函数 */
int saveAnswerRecord(const char* student_uuid, int qid, const char* user_answer, int is_correct, int score);
struct GradeInfo* getGradesByName(const char* username, int* count);
struct GradeInfo* getGradesByClass(const char* class_name, int* count);
struct GradeInfo* getGradesByStudentNumRange(int min_num, int max_num, int* count);
void statisticsByClass(const char* class_name);
int startQuiz(const char* student_uuid, const char* student_name, const char* class_name, int student_num);

/* 内存管理 */
void freeGrades(struct GradeInfo* grades);
void freeQuestions(struct Question* questions);

#endif
