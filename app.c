#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"
#include "load_test_data.h"
#include "file_io.h"

char current_user_uuid[37] = {0};
char current_username[100] = {0};
char current_user_class[50] = {0};
int current_user_num = 0;
int current_user_level = -1;

/**
 * @brief 显示主菜单
 */
void show_main_menu() {
    printf("\n");
    if (current_user_uuid[0] == '\0') {
        printf("====== Vocabulary Scale ======\n");
        printf("1. 注册\n");
        printf("2. 登录\n");
        printf("0. 退出\n");
    } else {
        printf("欢迎，%s（等级 %d）\n", current_username, current_user_level);
        printf("2. 注销\n");
        printf("3. 删除账户\n");
        if (current_user_level <= 1) {
            printf("4. 题目管理\n");
            printf("5. 按 姓名 / 学号 / 班级 查询成绩\n");
            printf("6. 按班级输出统计数据\n");
                printf("7. 文件导出\n");
        }
        if (current_user_level == 2) {
            printf("4. 开始测验\n");
            printf("5. 查看我的成绩\n");
        }
        printf("0. 退出\n");
    }
    printf("选择：");
}

/**
 * @brief 注册用户
 */
void register_user() {
    char username[100], password[100], classname[50];
    int level, num;
    
    printf("\n=== 注册用户 ===\n");
    printf("用户名：");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\r\n")] = 0;
    
    printf("密码：");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\r\n")] = 0;
    
    printf("用户等级（0=管理员, 1=教师, 2=学生）：");
    scanf("%d", &level);
    getchar();
    
    if (level < 0 || level > 2) {
        printf("[错误] 等级无效\n");
        return;
    }
    
    printf("班级：");
    fgets(classname, sizeof(classname), stdin);
    classname[strcspn(classname, "\r\n")] = 0;
    
    printf("学号（非学生请填0）：");
    scanf("%d", &num);
    getchar();
    
    char* uuid = createUser(username, password, level, classname, num, NULL);
    if (uuid) {
        printf("[成功] 用户注册成功\n");
        free(uuid);
    }
}

/**
 * @brief 用户登录
 */
void login_user() {
    char username[100], password[100];
    printf("\n=== 登录 ===\n");
    printf("用户名：");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\r\n")] = 0;
    printf("密码：");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\r\n")] = 0;
    
    char* uuid = loginUser(username, password);
    if (uuid) {
        int level = getUserLevel(uuid);
        strncpy(current_user_uuid, uuid, 36);
        strncpy(current_username, username, 99);
        current_user_level = level;
        printf("[成功] 登录成功（等级 %d）\n", level);
        free(uuid);
    } else {
        printf("[错误] 登录失败\n");
    }
}

/**
 * @brief 用户登出
 */
void logout_user() {
    memset(current_user_uuid, 0, 37);
    memset(current_username, 0, 100);
    current_user_level = -1;
    printf("[成功] 已注销\n");
}

/**
 * @brief 题目管理菜单
 */
void manage_questions_menu() {
    int subchoice;
    while (1) {
        printf("\n=== 题目管理 ===\n");
        printf("1. 添加题目\n");
        printf("2. 删除题目\n");
        printf("3. 查看所有题目\n");
        printf("0. 返回\n");
        printf("选择：");
        scanf("%d", &subchoice);
        getchar();
        
        if (subchoice == 1) {
            char word[100], trans[200];
            printf("单词（word）：");
            fgets(word, sizeof(word), stdin);
            word[strcspn(word, "\r\n")] = 0;
            printf("翻译（translate）：");
            fgets(trans, sizeof(trans), stdin);
            trans[strcspn(trans, "\r\n")] = 0;
            
            if (addQuestion(word, trans)) {
                printf("[成功] 添加成功\n");
            }
        } else if (subchoice == 2) {
            int qid;
            printf("题目ID：");
            scanf("%d", &qid);
            getchar();
            
            if (deleteQuestion(qid)) {
                printf("[成功] 删除成功\n");
            }
        } else if (subchoice == 3) {
            int count = 0;
            struct Question* q = getQuestions(&count);
            if (q && count > 0) {
                printf("\n=== 所有题目（%d） ===\n", count);
                printf("%-5s %-25s %-50s\n", "编号", "英文", "翻译");
                for (int i = 0; i < count; i++) {
                    printf("%-5d %-25s %-50s\n", q[i].qid, q[i].word, q[i].translate);
                }
                free(q);
            } else {
                printf("[提示] 无题目\n");
            }
        } else if (subchoice == 0) {
            break;
        }
    }
}

/**
 * @brief 查询成绩菜单
 */
void query_grades_menu() {
    int subchoice;
    while (1) {
        printf("\n=== 成绩查询 ===\n");
        printf("1. 按姓名查询\n");
        printf("2. 按班级查询\n");
        printf("3. 按学号范围查询\n");
        printf("0. 返回\n");
        printf("选择：");
        scanf("%d", &subchoice);
        getchar();
        
        if (subchoice == 1) {
            char name[100];
            printf("学生姓名：");
            fgets(name, sizeof(name), stdin);
            name[strcspn(name, "\r\n")] = 0;
            
            int count = 0;
            struct GradeInfo* grades = getGradesByName(name, &count);
            if (grades && count > 0) {
                printf("\n=== 结果 ===\n");
                printf("%-20s %-15s %-10s %-10s %-10s\n", "姓名", "班级", "学号", "成绩", "正确率");
                for (int i = 0; i < count; i++) {
                    printf("%-20s %-15s %-10d %-10d %-10.1f%%\n", 
                        grades[i].username, grades[i].class_name, grades[i].student_num, 
                        grades[i].total_score, grades[i].accuracy * 100);
                }
                free(grades);
            } else {
                printf("[提示] 无结果\n");
            }
        } else if (subchoice == 2) {
            char classname[50];
            printf("班级名称：");
            fgets(classname, sizeof(classname), stdin);
            classname[strcspn(classname, "\r\n")] = 0;
            
            int count = 0;
            struct GradeInfo* grades = getGradesByClass(classname, &count);
            if (grades && count > 0) {
                printf("\n=== 结果 ===\n");
                printf("%-20s %-15s %-10s %-10s %-10s\n", "姓名", "班级", "学号", "成绩", "正确率");
                for (int i = 0; i < count; i++) {
                    printf("%-20s %-15s %-10d %-10d %-10.1f%%\n", 
                        grades[i].username, grades[i].class_name, grades[i].student_num, 
                        grades[i].total_score, grades[i].accuracy * 100);
                }
                free(grades);
            } else {
                printf("[提示] 无结果\n");
            }
        } else if (subchoice == 3) {
            int min, max;
            printf("最小学号：");
            scanf("%d", &min);
            printf("最大学号：");
            scanf("%d", &max);
            getchar();
            
            int count = 0;
            struct GradeInfo* grades = getGradesByStudentNumRange(min, max, &count);
            if (grades && count > 0) {
                printf("\n=== 结果 ===\n");
                printf("%-20s %-15s %-10s %-10s %-10s\n", "姓名", "班级", "学号", "成绩", "正确率");
                for (int i = 0; i < count; i++) {
                    printf("%-20s %-15s %-10d %-10d %-10.1f%%\n", 
                        grades[i].username, grades[i].class_name, grades[i].student_num, 
                        grades[i].total_score, grades[i].accuracy * 100);
                }
                free(grades);
            } else {
                printf("[提示] 无结果\n");
            }
        } else if (subchoice == 0) {
            break;
        }
    }
}

/**
 * @brief 
 */
void statistics_menu() {
    char classname[50];
    printf("班级名称：");
    fgets(classname, sizeof(classname), stdin);
    // strcspn()
    classname[strcspn(classname, "\r\n")] = 0;
    statisticsByClass(classname);
}

/**
 * @brief 文件导出菜单
 */
void file_export_menu() {
    int subchoice;
    while (1) {
        printf("\n=== 文件导出 ===\n");
        printf("1. 导出题目到 timu.txt\n");
        printf("2. 导出成绩到 stu.txt\n");
        printf("3. 导出按成绩排序的结果 (sort1.txt)\n");
        printf("4. 导出按班级排序的结果 (sort2.txt)\n");
        printf("0. 返回\n");
        printf("选择：");
        scanf("%d", &subchoice);
        getchar();
        
        if (subchoice == 1) {
            if (exportQuestionsToFile("timu.txt")) {
                printf("[成功] 题目已导出到 timu.txt\n");
            } else {
                printf("[错误] 题目导出失败\n");
            }
        } else if (subchoice == 2) {
            if (exportGradesToFile("stu.txt")) {
                printf("[成功] 成绩已导出到 stu.txt\n");
            } else {
                printf("[错误] 成绩导出失败\n");
            }
        } else if (subchoice == 3) {
            if (exportGradesByScoreToFile("sort1.txt")) {
                printf("[成功] 成绩（按分数排序）已导出到 sort1.txt\n");
            } else {
                printf("[错误] 成绩导出失败\n");
            }
        } else if (subchoice == 4) {
            if (exportGradesByClassToFile("sort2.txt")) {
                printf("[成功] 成绩（按班级排序）已导出到 sort2.txt\n");
            } else {
                printf("[错误] 成绩导出失败\n");
            }
        } else if (subchoice == 0) {
            break;
        } else {
            printf("[错误] 无效选择\n");
        }
    }
}

int main() {
    int choice;
    printf("\n====== Vocabulary Scale ======\n");
    /* 在程序启动时初始化数据库表结构（迁移后的 initDatabase 在 load_test_data.c 中实现） */
    {
        sqlite3 *db = NULL;
        int rc = sqlite3_open("vocab_system.db", &db);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "[ERROR] Cannot open database to init: %s\n", sqlite3_errmsg(db));
            if (db) sqlite3_close(db);
        } else {
            if (!initDatabase(db)) {
                fprintf(stderr, "[ERROR] initDatabase failed\n");
            }
            sqlite3_close(db);
        }
    }

    /* 启动时自动加载测试数据（如果需要可在 load_test_user_data 中做存在性检查） */
    load_test_user_data();
    /* 加载样本题目（只有在题库为空时才会插入） */
    load_sample_questions();
    
    while (1) {
        show_main_menu();
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("[错误] 无效输入\n");
            continue;
        }
        getchar();
        
        if (current_user_uuid[0] == '\0') {
            if (choice == 1) {
                register_user();
            } else if (choice == 2) {
                login_user();
            } else if (choice == 0) {
                break;
            } else {
                printf("[错误] 无效选择\n");
            }
        } else {
            if (choice == 0) {
                break;
            } else if (choice == 2) {
                logout_user();
            } else if (choice == 3) {
                if (deleteUser(current_user_uuid)) {
                    logout_user();
                }
            } else if (choice == 4 && current_user_level <= 1) {
                manage_questions_menu();
            } else if (choice == 5 && current_user_level <= 1) {
                query_grades_menu();
            } else if (choice == 6 && current_user_level <= 1) {
                statistics_menu();
            } else if (choice == 7 && current_user_level <= 1) {
                file_export_menu();
            } else if (choice == 4 && current_user_level == 2) {
                int score = startQuiz(current_user_uuid, current_username, current_user_class, current_user_num);
                /* 答题完成后，自动将成绩追加到 stu.txt */
                addStuGradeToFile("stu.txt", current_user_uuid, current_username, 
                                        current_user_class, current_user_num, score);
            } else if (choice == 5 && current_user_level == 2) {
                int count = 0;
                struct GradeInfo* grades = getGradesByName(current_username, &count);
                if (grades && count > 0) {
                    printf("\n=== 我的成绩 ===\n");
                    printf("%-20s %-10s\n", "姓名", "成绩");
                    printf("%-20s %-10d\n", grades[0].username, grades[0].total_score);
                    free(grades);
                } else {
                    printf("[提示] 暂无成绩\n");
                }
            } else {
                printf("[错误] 无效选择\n");
            }
        }
    }
    
    printf("\nbyebye~!\n");
    return 0;
}
