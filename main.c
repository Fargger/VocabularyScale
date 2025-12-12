#include<stdio.h>
#include"lib/sqlite3.h"

#define MAX_WORD_LENGTH 30

// 题面 / 试卷，数据来源是 dict.db
struct Items{
    int num;
    char* word;
    char* word_puzzled;
    char* hint;
};

// 答题卡，数据来源是 dict.db
struct AnsSheet{
    int num;
    char* ans;
    bool isRight;  
};

int main(){
    
}