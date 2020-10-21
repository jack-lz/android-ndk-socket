#ifndef SPEECH_SERVER_H
#define SPEECH_SERVER_H

#pragma once

#include "common.h"

#define MYSOCKET_PATH "/vrservice/speechtts"
#define PORT 8888               //服务器端监听端口号,这个端口号可以自行定义
#define MAX_BUFFER 128         //数据缓冲区最大值


class SpeechServer 
{
public:
    SpeechServer();
    ~SpeechServer();

    static SpeechServer& GetInstance();
    
    static void creatThread();
    static void * InitServer(void * arg);
    static void * msgReceive(void * arg);

    static int serverfd;
    static int TtsClientfd;
    static char* dataBuffer[MAX_CLIENT_NUM];

    // void msgProc(SpeechResponseMessage msg);
    void connectSocket();
    void disconnectSocket();    
    int onLoginServer();
    

    pthread_mutex_t gMutex;


};

#endif // SPEECH_SERVER_H