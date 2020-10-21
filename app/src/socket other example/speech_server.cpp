#include "speech_server.h"

#include "vrlog.h"
#include "json.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>

// #include "cyclebuffer.h"


// CCycleBuffer cloudBuffer(128 * 100);

int SpeechServer::serverfd         =  -1;
const std::string outputpah = "./test/auth/";
const std::string uuid = "AISPEECHTEST001";


char* SpeechServer::dataBuffer[MAX_CLIENT_NUM] = {NULL};

SpeechServer::SpeechServer()
{
    pthread_mutex_init (&gMutex,NULL);
}

SpeechServer::~SpeechServer()
{


}

SpeechServer& SpeechServer::GetInstance()
{
    static SpeechServer instance;
    return instance;
}

void SpeechServer::creatThread()
{
    thread_attribute pull_attr;
    memset(&pull_attr, 0, sizeof(thread_attribute));
    snprintf(pull_attr.name, 16, "push_cloud");
    pull_attr.policy = sched_get_priority_max(SCHED_RR) - 10;
    pull_attr.detachstate = PTHREAD_CREATE_DETACHED;
    threadCreate(SpeechServer::InitServer, NULL, &pull_attr);
}

void * SpeechServer::InitServer(void *arg)
{
    VR_LOG("Try to connect socket");
    int size, write_size;
    char buffer[MAX_BUFFER];

    // SpeechServer::serverfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // while (SpeechServer::serverfd < 0) {
    //     VR_LOG("Failed to create a socket and we will try it again: %s", strerror(errno));
    //     SpeechServer::serverfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //     usleep(100*1000);
    // }
    // struct sockaddr_in server_addr, client_addr;
    // bzero(&server_addr, sizeof(server_addr));
    // server_addr.sin_family = AF_INET;
    // server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // server_addr.sin_port = htons(PORT);
    // bzero(&(server_addr.sin_zero), 8);

    SpeechServer::serverfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    while (SpeechServer::serverfd < 0) {
        VR_LOG("Failed to create a socket and we will try it again: %s", strerror(errno));
        SpeechServer::serverfd = socket(AF_LOCAL, SOCK_STREAM, 0);
        usleep(100*1000);
    }
    struct sockaddr_un server_addr, client_addr;
    bzero(&server_addr, sizeof(server_addr));
    strcpy(server_addr.sun_path+1, MYSOCKET_PATH);
    server_addr.sun_family = AF_LOCAL;
    socklen_t serAddrlen = 1 + strlen(MYSOCKET_PATH) + sizeof(server_addr.sun_family);

    int opt = 1;
    int res = setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));    //设置地址复用
    if (res < 0) {
        perror("Server reuse address failed!\n");
        exit(1);
    }

    res = bind(serverfd, (struct sockaddr *)&server_addr, serAddrlen);
    if (-1 == res) {
        perror("Socket Bind Failed!\n");
        exit(1);
    }

    if (listen(serverfd, 5) == -1) {
        perror("Listened Failed!\n");
        exit(1);
    }

    while(1) {
        // socklen_t len = sizeof(client_addr);
        struct sockaddr_un client_addr;
        bzero(&client_addr, sizeof(client_addr));

        perror("waiting connection...\n");
        int client_fd = -1;
        if ((client_fd = accept(serverfd, (struct sockaddr *)&client_addr, &serAddrlen)) == -1) {
            perror("accept connect failed\n");
            continue;
        }
        VR_LOG("client_fd is %d", client_fd);
        thread_attribute rev_attr;
        memset(&rev_attr, 0, sizeof(thread_attribute)); 
        snprintf(rev_attr.name, 16, "parseCloudData");
        rev_attr.policy = sched_get_priority_max(SCHED_RR) - 10;
        rev_attr.detachstate = PTHREAD_CREATE_DETACHED;
        threadCreate(SpeechServer::msgReceive, &client_fd, &rev_attr);
    }
}

void * SpeechServer::msgReceive(void *arg)
{
    char buffer[MAX_BUFFER];
    char leftBuffer[MAX_BUFFER*100];
    int leftSize = 0;
    int clientfd = *(int*)arg;
    int size;
    
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        if ((size = recv(clientfd, buffer, sizeof(buffer), 0)) <= 0) {
            VR_LOG("It seems that socket disconnect");
            close(clientfd);
            break;
        } else {
            memcpy_s(leftBuffer + leftSize, buffer, size);
            leftSize = leftSize + size;
            while(1) {
                int length = 0;
                if (leftSize <= sizeof(int)) {
                    break;
                }
                memcpy_s(&length, leftBuffer, sizeof(int));
                if (leftSize >= length) {
                    char msg[length];
                    memcpy_s(&msg, leftBuffer, length);
                    recMsgParse(clientfd, msg, length);
                    leftSize = leftSize-length;
                    memcpy_s(&leftBuffer, leftBuffer+length, leftSize);
                } else {
                    break;
                }
            }
        }
        
        // usleep(500*1000);

        // thread_attribute parse_attr;
        // memset(&parse_attr, 0, sizeof(thread_attribute));
        // snprintf(parse_attr.name, 16, "parseCloudData");
        // parse_attr.policy = sched_get_priority_max(SCHED_RR) - 10;
        // parse_attr.detachstate = PTHREAD_CREATE_DETACHED;
        // threadCreate(SpeechServer::msgParse, NULL, &parse_attr);
    }

    return (void*)0;
}

void SpeechServer::disconnectSocket()
{
    if (SpeechServer::serverfd < 0) {
        return;
    }
    VR_LOG("try to disconnect the connection...");
    shutdown(SpeechServer::serverfd, SHUT_RD);
    close(SpeechServer::serverfd);
    SpeechServer::serverfd = -1;
    // cloudBuffer.empty();
    VR_LOG("connection disconnected and buffer cleared.");
}

bool SpeechServer::sendEventMsg(int clientfd, EventType type, char* msg, int size)
{
    int lengthSize = sizeof(size);
    int totalSize = size + lengthSize + 1;
    char buffer[totalSize];
    bzero(buffer, totalSize);
    memcpy_s(buffer, &totalSize, lengthSize);

    char msgType = static_cast<unsigned char> type;
    buffer[lengthSize] = msgType;

    memcpy_s(buffer+lengthSize+1, msg, size);
    int ret = 0;
    if ((ret = send(clientfd, buffer, totalSize, 0)) != totalSize) {
        VR_LOG("Failed to send message and ret is %d with error id %d with message %s", ret, errno, strerror(errno));
        return false;
    } else {
        return true;
    }
}