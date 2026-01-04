#ifndef FILE_IO_H
#define FILE_IO_H

#include "database.h"

/**
 * @brief 将所有题目导出到 timu.txt 文件
 * @param filename 输出文件名（通常为 "timu.txt"）
 * @return 成功返回 1，失败返回 0
 */
int exportQuestionsToFile(const char* filename);

/**
 * @brief 将所有学生成绩导出到 stu.txt 文件
 * @param filename 输出文件名（通常为 "stu.txt"）
 * @return 成功返回 1，失败返回 0
 */
int exportGradesToFile(const char* filename);

/**
 * @brief 将按成绩排序的结果导出到 sort1.txt 文件
 * @param filename 输出文件名（通常为 "sort1.txt"）
 * @return 成功返回 1，失败返回 0
 */
int exportGradesByScoreToFile(const char* filename);

/**
 * @brief 将按班级排序的结果导出到 sort2.txt 文件
 * @param filename 输出文件名（通常为 "sort2.txt"）
 * @return 成功返回 1，失败返回 0
 */
int exportGradesByClassToFile(const char* filename);

/**
 * @brief 将学生答题记录追加到 stu.txt 文件
 * @param student_uuid 学生 UUID
 * @param student_name 学生姓名
 * @param class_name 班级名称
 * @param student_num 学号
 * @param total_score 总分
 * @return 成功返回 1，失败返回 0
 */
int addStuGradeToFile(const char* filename, const char* student_uuid, 
                              const char* student_name, const char* class_name,
                              int student_num, int total_score);

/* 将单次答题的每道题及学生答案追加到 stu.txt （包含正确答案） */
int addStuAnsToFile(const char* filename, const char* student_name,
                               const char* class_name, int student_num,
                               int question_count, const char** questions,
                               const char** user_answers, const char** correct_answers);

#endif /* FILE_IO_H */
