#define MSGKEY 1183 // 定义消息队列的键值
#define MAX_MSG_STRING_LENGTH 1024 // 设置最大消息长度

// 定义消息的结构体
struct msgform {
    long mtype;           // 消息类型
    int source_pid;       // 消息来源的进程ID
    char msg_string[MAX_MSG_STRING_LENGTH]; // 存储传输消息的数组
};

int msgsize = sizeof(struct msgform) - sizeof(long); // 计算消息结构体的大小
int msgqid;  // 消息队列ID
