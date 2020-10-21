#ifndef SPEECH_SOCKET_H
#define SPEECH_SOCKET_H

#pragma once

#include "common.h"

class SpeechSocket 
{
public:
    SpeechSocket();
    ~SpeechSocket();

private:
    static void creatThread();
    static void * connectSpeechServer(void * arg);

    void connectSocket();
    void disconnectSocket();    
    int onLoginServer();
    bool sendEventMsg(EventType type, char* msg = NULL, int size = 0);

    static int connectfd;

    pthread_mutex_t gMutex;
};

#endif // SPEECH_SOCKET_H