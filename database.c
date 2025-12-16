#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include"lib/sqlite3.h"

#define MAX_WORD_LENGTH 30

/**
 * @brief 生成 UUID
 * @return UUID 的字符串
 */
char* generateUUID(){
    char* uuid = (char*)malloc(37);
    if (!uuid) return NULL;
    srand(time(NULL));
    const char *chars = "0123456789abcdef";

    for(int i=0; i<8; i++) uuid[i] = chars[rand() % 16];
    uuid[8] = '-';
    for(int i=9; i<13; i++) uuid[i] = chars[rand() % 16];
    uuid[13] = '-';
    for(int i=14; i<18; i++) uuid[i] = chars[rand() % 16];
    uuid[18] = '-';
    for(int i=19; i<23; i++) uuid[i] = chars[rand() % 16];
    uuid[23] = '-';
    for(int i=24; i<36; i++) uuid[i] = chars[rand() % 16];
    uuid[36] = '\0';

    return uuid;
}

/**
 * @brief 对密码进行简单的 hash 加密
 * @param password 原始密码
 * @return hash 后的密码字符串（需要调用方释放）
 */
char* hashPassword(const char* password) {
    if (!password) return strdup("");
    
    char* hash = (char*)malloc(101);
    if (!hash) return NULL;
    memset(hash, 0, 101);
    
    unsigned int h = 5381;
    for (int i = 0; password[i]; i++) {
        h = ((h << 5) + h) + password[i];
    }
    
    snprintf(hash, 101, "%u", h);
    return hash;
}

/**
 * @brief 创建新用户，并自动分配 uuid
 * @param name 用户昵称
 * @param pswd 用户密码，转换为 hash 值后再存入数据库
 * @param level 用户级别 | 0 - admin | 1 - teacher | 2 - student |
 * @param class_name 班级名称（仅student有，可为空）
 * @param num 学号 / 工号
 * @param belong_to 所属教师的uuid（仅student有，可为空）
 * @param showInfo 是否打印新创建用户的信息
 * 
 * @return 若创建成功，返回 1. 否则返回 0
 */
int createUser(char* name, char* pswd, int level, char* class_name, int num, char* belong_to, bool showInfo){
    if (!name || !pswd) return 0;
    
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_open("data.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DEBUG] createUser: sqlite3_open failed: %s (rc=%d)\n", sqlite3_errmsg(db), rc);
        sqlite3_close(db);
        return 0;
    }
    
    char* uuid = generateUUID();
    char* pswd_hash = hashPassword(pswd);
    
    if (!uuid || !pswd_hash) {
        fprintf(stderr, "[DEBUG] createUser: memory alloc failed (uuid=%p, pswd_hash=%p)\n", (void*)uuid, (void*)pswd_hash);
        if (stmt) sqlite3_finalize(stmt);
        sqlite3_close(db);
        free(uuid);
        free(pswd_hash);
        return 0;
    }
    const char* sql = "INSERT INTO usr (uuid, usr_name, pswd_hash, usr_level, class_name, num, belong_to) VALUES (?, ?, ?, ?, ?, ?, ?);";
    
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DEBUG] createUser: sqlite3_prepare_v2 failed: %s (rc=%d)\n", sqlite3_errmsg(db), rc);
        free(uuid);
        free(pswd_hash);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, pswd_hash, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, level);
    sqlite3_bind_text(stmt, 5, class_name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, num);
    sqlite3_bind_text(stmt, 7, belong_to, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[DEBUG] createUser: sqlite3_step failed: %s (rc=%d)\n", sqlite3_errmsg(db), rc);
    }
    int result = (rc == SQLITE_DONE) ? 1 : 0;
    
    if (result && showInfo) {
        printf("[SUCCESS] User created successfully!\n");
        printf("UUID: %s\n", uuid);
        printf("用户名: %s\n", name);
        printf("密码哈希: %s\n", pswd_hash);
        printf("用户级别: %d\n", level);
        printf("班级: %s\n", class_name ? class_name : "");
        printf("学号/工号: %d\n", num);
        printf("所属教师: %s\n", belong_to ? belong_to : "");
    } else if (!result) {
        fprintf(stderr, "[DEBUG] createUser: insertion failed\n");
    }
    
    sqlite3_finalize(stmt);
    free(uuid);
    free(pswd_hash);
    sqlite3_close(db);
    
    return result;
}

/**
 * @brief 根据uuid删除usr表中指定的用户，需二次确认
 * @param uuid 要删除的用户的uuid
 * 
 * @return 若删除成功，返回 1. 否则返回 0. 若用户取消删除，返回 -1
 */
int deleteUser(char* uuid){
    if (!uuid) return 0;
    
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_open("data.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DEBUG] deleteUser: sqlite3_open failed: %s (rc=%d)\n", sqlite3_errmsg(db), rc);
        sqlite3_close(db);
        return 0;
    }
    
    // 首先查询要删除的用户信息
    const char* select_sql = "SELECT uuid, usr_name, usr_level, class_name, num, belong_to FROM usr WHERE uuid = ?;";
    rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DEBUG] deleteUser: sqlite3_prepare_v2 (SELECT) failed: %s (rc=%d)\n", sqlite3_errmsg(db), rc);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_STATIC);
    
    // 获取用户信息
    char* user_name = NULL;
    int user_level = -1;
    char* class_name = NULL;
    int num = -1;
    char* belong_to = NULL;
    bool user_found = false;
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        user_found = true;
        const unsigned char* name_ptr = sqlite3_column_text(stmt, 1);
        const unsigned char* class_ptr = sqlite3_column_text(stmt, 3);
        const unsigned char* belong_ptr = sqlite3_column_text(stmt, 5);
        
        user_name = strdup((const char*)(name_ptr ? name_ptr : (const unsigned char*)""));
        user_level = sqlite3_column_int(stmt, 2);
        class_name = strdup((const char*)(class_ptr ? class_ptr : (const unsigned char*)""));
        num = sqlite3_column_int(stmt, 4);
        belong_to = strdup((const char*)(belong_ptr ? belong_ptr : (const unsigned char*)""));
    } else if (rc != SQLITE_DONE) {
        fprintf(stderr, "[DEBUG] deleteUser: sqlite3_step (SELECT) failed: %s (rc=%d)\n", sqlite3_errmsg(db), rc);
    }
    
    sqlite3_finalize(stmt);
    stmt = NULL;
    
    // 如果用户不存在，返回 0
    if (!user_found) {
        fprintf(stderr, "[DEBUG] deleteUser: user not found (uuid=%s)\n", uuid);
        sqlite3_close(db);
        return 0;
    }
    
    // 二次确认：显示用户信息并要求确认
    printf("\n========== 删除用户确认 ==========\n");
    printf("UUID: %s\n", uuid);
    printf("用户名: %s\n", user_name ? user_name : "");
    printf("用户级别: %d\n", user_level);
    printf("班级: %s\n", class_name ? class_name : "");
    printf("学号/工号: %d\n", num);
    printf("所属教师: %s\n", belong_to ? belong_to : "");
    printf("================================\n");
    printf("确定要删除此用户吗？ (y/n): ");
    char confirm[10];
    scanf("%s", confirm);
    
    
    if (!confirm) {
        printf("输入错误，删除已取消。\n");
        free(user_name);
        free(class_name);
        free(belong_to);
        sqlite3_close(db);
        return -1;
    }
    
    // 检查确认输入
    if (confirm[0] != 'y' && confirm[0] != 'Y') {
        printf("删除已取消。\n");
        free(user_name);
        free(class_name);
        free(belong_to);
        sqlite3_close(db);
        return -1;
    }
    
    /*
    // 第二次确认
    printf("请再次输入 'DELETE' 确认删除: ");
    fflush(stdout);
    
    char confirm2[20];
    if (!fgets(confirm2, sizeof(confirm2), stdin)) {
        printf("输入错误，删除已取消。\n");
        free(user_name);
        free(class_name);
        free(belong_to);
        sqlite3_close(db);
        return -1;
    }
    
    // 去掉换行符
    size_t len = strcspn(confirm2, "\r\n");
    confirm2[len] = '\0';
    
    // 检查第二次确认
    if (strcmp(confirm2, "DELETE") != 0) {
        printf("确认字符不匹配，删除已取消。\n");
        free(user_name);
        free(class_name);
        free(belong_to);
        sqlite3_close(db);
        return -1;
    }*/
    
    // 执行删除操作
    const char* delete_sql = "DELETE FROM usr WHERE uuid = ?;";
    rc = sqlite3_prepare_v2(db, delete_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DEBUG] deleteUser: sqlite3_prepare_v2 (DELETE) failed: %s (rc=%d)\n", sqlite3_errmsg(db), rc);
        free(user_name);
        free(class_name);
        free(belong_to);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[DEBUG] deleteUser: sqlite3_step (DELETE) failed: %s (rc=%d)\n", sqlite3_errmsg(db), rc);
    }
    int result = (rc == SQLITE_DONE) ? 1 : 0;
    
    if (result) {
        printf("\n[SUCCESS] 用户已成功删除！\n");
        printf("已删除的用户信息:\n");
        printf("  UUID: %s\n", uuid);
        printf("  用户名: %s\n", user_name ? user_name : "");
        printf("  用户级别: %d\n", user_level);
        printf("  班级: %s\n", class_name ? class_name : "");
        printf("  学号/工号: %d\n", num);
        printf("  所属教师: %s\n\n", belong_to ? belong_to : "");
    } else {
        fprintf(stderr, "[DEBUG] deleteUser: deletion failed\n");
    }
    
    sqlite3_finalize(stmt);
    free(user_name);
    free(class_name);
    free(belong_to);
    sqlite3_close(db);
    
    return result;
}



/** 题面 / 试卷，数据来源是 dict.db
 * @param num 编号
 * @param word 单词
 * @param word_puzzled 被随机抹去若干个字母的单词
 * @param hint 中文翻译
 * @param next 链表的next指针
 * @param fromDB 来自哪个数据库
 * @param fromTable 来自哪个表
 */ 
struct Items{
    int num;
    char* word;
    char* word_puzzled;
    char* hint;
    struct Items* next;

    char* fromDB;
    char* fromTable;
};

/** 
 * @brief 答题卡，数据来源是 dict.db
 * @param num 编号（与试卷对应）
 * @param ans 用户的答案
 * @param isRight <boolean>是否正确
 * @param fromItem <Items*> 指向对应试卷节点的指针
 * @param next 链表的 next 指针
 *  */ 
struct AnsSheet{
    int num;
    char* ans;
    bool isRight;
    struct Items* fromItem;
    struct AnsSheet* next;  
};


/**
 * @brief 从数据库中调取数据，生成quiz到链表中
 * @param count 生成题目的数量
 * @param db 从哪个数据库调用数据
 * @return 试卷链表的头部节点
 */
struct Items* generateQuiz(int count, sqlite3* db, const char* table){
    if(count <= 0 || db == NULL || table == NULL) return NULL;

    // 动态 SQL 查询
    char statement_table[256];
    snprintf(statement_table, sizeof(statement_table), "SELECT word, translate FROM %s ORDER BY RANDOM() LIMIT ?;", table);
    
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(db, statement_table, -1, &stmt, NULL);
    if(rc != SQLITE_OK) {
        if(stmt) sqlite3_finalize(stmt);
        return NULL;
    }

    rc = sqlite3_bind_int(stmt, 1, count);
    if(rc != SQLITE_OK){
        sqlite3_finalize(stmt);
        return NULL;
    }

    // 利用时间生成随机数
    srand((unsigned)time(NULL));

    struct Items* head = NULL;
    struct Items* tail = NULL;
    int idx = 0;

    while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
        const unsigned char* w = sqlite3_column_text(stmt, 0); // column: word
        const unsigned char* t = sqlite3_column_text(stmt, 1); // column: translate
        if(!w) continue;

        struct Items* node = (struct Items*)malloc(sizeof(struct Items)); // 给节点分配内存
        if(!node) break;
        memset(node, 0, sizeof(*node)); // 将节点初始化为 0 

        idx++;
        node->num = idx;

        // copy word
        const char* word_src = (const char*)w;
        node->word = strdup(word_src ? word_src : "");
        if(!node->word){ free(node); break; }

        // 将src的任一字母换成下划线（出题）
        size_t len = strlen(node->word);
        node->word_puzzled = (char*)malloc(len + 1);
        if(node->word_puzzled){
            strcpy(node->word_puzzled, node->word);
            if(len > 0){
                size_t i = (size_t)(rand() % len);
                node->word_puzzled[i] = '_';
            }
        }

        // hint from translate column
        {
            const char* hint_src = (const char*)t;
            node->hint = strdup(hint_src ? hint_src : "");
        }
        node->next = NULL;

        if(head == NULL) head = tail = node;
        else { tail->next = node; tail = node; }
    }

    sqlite3_finalize(stmt);
    return head;
}

/**
 * 打印试卷 / 预览试卷
 * @param head 传入头部节点
 * @param showAns <boolean> 是否显示答案
 */
void printItems(const struct Items* head, bool showAns){
    const struct Items* cur = head; // cur is for current node
    while(cur){
        printf("  No.%d: %s\n", cur->num, cur->word_puzzled ? cur->word_puzzled : "");
        printf("  hint: %s\n\n", cur->hint ? cur->hint : "");
        if(showAns) printf("  ans: %s\n", cur->word ? cur->word : "");
        cur = cur->next;
    }
}

/**
 * @brief 释放链表结构体成员被动态分配的内存，并将传入的 head 指针设置为 NULL
 * @param head_ptr 链表的头部节点的地址
 */
void freeItems(struct Items** head_ptr){
    struct Items* cur = *head_ptr;
    while(cur){
        struct Items* next = cur->next;
        free(cur->word);
        free(cur->word_puzzled);
        free(cur->hint);
        free(cur);
        cur = next;
    }
    *head_ptr = NULL;
}

/**
 * @brief 根据某个试卷记录答案（不再打印详细答题卡，返回答题卡链表）
 * @param fromItem <Items*> 指向试卷特定节点的指针
 * @return 答题卡链表头指针（需要调用方负责释放）
 */
struct AnsSheet* recordAns(struct Items* fromItem){
    if(fromItem == NULL) return NULL;

    struct AnsSheet* head = NULL;
    struct AnsSheet* tail = NULL;

    struct Items* item = fromItem;
    int idx = 0;
    char buf[MAX_WORD_LENGTH + 2]; // 留一个位置给 '\0' 

    // 逐题询问并构建答题卡链表
    while(item){
        idx++;
        // 显示题号、被抹去的单词与提示
        printf("? No.%d: %s\n", idx, item->word_puzzled ? item->word_puzzled : "");
        printf("  hint: %s\n", item->hint ? item->hint : "");
        printf("  your ans: ");

        if(!fgets(buf, sizeof(buf), stdin)){
            // 读取失败，结束输入循环
            break;
        }
        // 去掉尾部换行
        size_t l = strcspn(buf, "\r\n");
        buf[l] = '\0';

        // 分配并初始化答题卡节点
        struct AnsSheet* node = (struct AnsSheet*)malloc(sizeof(struct AnsSheet));
        if(!node) break;
        memset(node, 0, sizeof(*node));

        node->num = idx;
        node->ans = strdup(buf ? buf : "");
        node->fromItem = item;
        node->isRight = (node->ans && item->word && strcmp(node->ans, item->word) == 0);
        node->next = NULL;

        if(head == NULL) head = tail = node;
        else { tail->next = node; tail = node; }

        item = item->next;
    }

    // 汇总并展示结果（不再展示每题详细信息）
    int total = 0, correct = 0;
    struct AnsSheet* cur = head;
    while(cur){
        total++;
        if(cur->isRight) correct++;
        cur = cur->next;
    }
    printf("\n正确率: %d/%d \n\n", correct, total);

    return head;
}

/**
 * @brief 展示答题卡的详细内容（不释放）
 * @param head 答题卡链表头指针
 */
void showData(const struct AnsSheet* head){
    const struct AnsSheet* cur = head;
    while(cur){
        printf("%s No.%d: 你的答案：'%s' 正确答案：'%s'\n",
               cur->isRight ? "√" : "×",
               cur->num,
               cur->ans ? cur->ans : "",
               cur->fromItem && cur->fromItem->word ? cur->fromItem->word : ""
        );
        cur = cur->next;
    }
}

/**
 * @brief 释放答题卡链表内存并将指针设定为 NULL
 * @param head_ptr 答题卡链表头指针的地址
 */
void freeAnsSheet(struct AnsSheet** head_ptr){
    if(!head_ptr || !*head_ptr) return;
    struct AnsSheet* cur = *head_ptr;
    while(cur){
        struct AnsSheet* next = cur->next;
        free(cur->ans);
        free(cur);
        cur = next;
    }
    *head_ptr = NULL;
}

int main(){
    int rc1 = SQLITE_ERROR;
    sqlite3* db1 = NULL;
    rc1 = sqlite3_open("dict.db", &db1); 
    if(rc1 == SQLITE_ERROR){
        sqlite3_log(sqlite3_errcode(db1), "Failed to open\n");
        return -1;
    }

    
    createUser("huarun", "6609695", 2, "306", 30616, NULL, true);

    struct Items* quiz = generateQuiz(1, db1, "CET4"); // 生成链表，quiz是链表头部节点

    //printItems(quiz, false);

    struct AnsSheet* sheet = recordAns(quiz);

    showData(sheet);

    freeAnsSheet(&sheet);

    freeItems(&quiz);

    // 删除用户-开始
    printf("请输入要删除的用户的uuid: \n");
    char delete_uuid[50];
    scanf("%s", delete_uuid);
    deleteUser(delete_uuid);
    //删除用户-结束

    sqlite3_close(db1);
}