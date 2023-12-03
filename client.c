// 客户端程序
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include "msg_mycs.h" // 包含消息结构体的头文件

int main() {
    struct msgform msg;
    int pid;

    msgqid = msgget(MSGKEY, 0777); // 获取消息队列

    pid = getpid(); // 获取当前进程的ID
    // 循环等待用户输入操作符和操作数
    for (;;) {
        memset(msg.msg_string, 0, MAX_MSG_STRING_LENGTH);
        printf("Enter expression (q to quit):\t");
        fgets(msg.msg_string, MAX_MSG_STRING_LENGTH, stdin);
        msg.msg_string[strcspn(msg.msg_string, "\n")] = '\0';

        if (strcmp(msg.msg_string, "q") == 0)
            break;

        msg.source_pid = pid; // 设置消息的来源进程ID
        msg.mtype = 1; // 设置消息类型为 1

        // 显示接收到消息的服务器进程ID
        printf("client(pid=%d) => msg_qry(mtype=%ld):\t%s\n", pid, msg.mtype, msg.msg_string);
        msgsnd(msgqid, &msg, msgsize, 0); // 发送请求给服务器
        msgrcv(msgqid, &msg, msgsize, pid, 0); // 接收服务器的响应

        // 显示计算结果或错误消息
        printf("client(pid=%d) <= server(pid=%d):\t%s\n",
               pid, msg.source_pid, msg.msg_string); // 显示接收到服务器的消息
    }
}
