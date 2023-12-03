
// 服务端程序
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h> // 包含信号处理的头文件
#include <ctype.h>

#include "msg_mycs.h" // 包含消息结构体的头文件
#include "my_calculate_expression.h" // 包含计算表达式的头文件

// 信号处理函数，用于清理消息队列
int cleanup() {
    msgctl(msgqid, IPC_RMID, 0); // 删除消息队列
    exit(0); // 退出程序
}

int main() {
    struct msgform msg;
    int i;

    extern int cleanup(); // 声明清理函数
    for (i = 0; i < 20; i++)
        signal(i, cleanup); // 注册信号处理函数

    msgqid = msgget(MSGKEY, 0777 | IPC_CREAT); // 获取或创建消息队列

    // 循环等待客户端的请求
    for (;;) {
        printf("server(pid=%d) is ready (msgqid=%d)... \n", getpid(), msgqid); // 显示服务器准备就绪

        // 接收客户端发送的消息
        memset(msg.msg_string, 0, MAX_MSG_STRING_LENGTH);
        msgrcv(msgqid, &msg, msgsize, 1, 0);
        // 显示接收到的客户端消息
        printf("server(pid=%d) <= client(pid=%d):  %s\n", getpid(), msg.source_pid, msg.msg_string);

        // 计算表达式，并检测错误
        char result_string[MAX_MSG_STRING_LENGTH];
        calculate_expression(msg.msg_string, result_string);
        // 发送结果给客户端
        msg.mtype = msg.source_pid;
        msg.source_pid = getpid();
        memset(msg.msg_string, 0, MAX_MSG_STRING_LENGTH);
        strcpy(msg.msg_string, result_string);
        // 显示发送给客户端的结果
        printf("server(pid=%d) => client(pid=%d):  %s\n", getpid(), msg.source_pid, msg.msg_string);

        msgsnd(msgqid, &msg, msgsize, 0);

    }
}