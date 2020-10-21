#include "speech_socket.h"
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/tcp.h>
#include <csignal>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/endian.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>

#include "vrlog.h"
#include "json.h"

#define MAX_BUFFER 1024         //数据缓冲区最大值



using namespace std;
static int connectfd;
static bool logedIn;

int SpeechSocket::BUFFERSIZE       = 128;
int SpeechSocket::RCVSIZE          =   0;
int SpeechSocket::READSIZE         =   0;
int SpeechSocket::dataSize         =   0;
int SpeechSocket::connectfd        = -1 ;
bool SpeechSocket::logedIn         = false;
const std::string outputpah = "./test/auth/";
const std::string uuid = "AISPEECHTEST001";

#define SOCKET_PATH "/vrservice/speechtts"

SpeechSocket::SpeechSocket():
            mTtsEventCallback(NULL),
            mTtsPcmCallback(NULL)

{
    // pthread_mutex_init (&gMutex,NULL);
}

SpeechSocket::~SpeechSocket()
{


}

SpeechSocket& SpeechSocket::GetInstance()
{
    static SpeechSocket instance;
    return instance;
}

EState SpeechSocket::StartEngineSystem()
{
    SpeechSocket::creatThread(); // 连接socket

    // remove("./tmp.pcm");
    // int tag = 999;// TODO
    // int res = TtsInit(&mhandle, TtsPcmCallback, &tag);
    // VR_LOG("tts init res:%d", res);
    // Json::Value auth_cfg;
    // Json::Value devInfo;
    // auth_cfg["productId"] = "278583351";
    // auth_cfg["savedProfile"] = outputpah + "278583351.txt";
    // auth_cfg["productKey"] = "d9623e1764526af8b776386cfa31dbac";
    // auth_cfg["productSecret"] = "79db3e6c971990cbcc8112d31743ed65";

    // devInfo["deviceName"] = uuid;
    // auth_cfg["devInfo"] = devInfo;

    // // std::string auth = auth_cfg.toStyledString();
    // std::string auth = Json::FastWriter().write(auth_cfg);
    // auth = auth.substr(0, auth.size() - 1);

    // VR_LOG("tts auth:%s size:%d", auth.data(), auth.size());
    // res = TtsAuth(mhandle, auth.data());
    // VR_LOG("tts auth res:%d", res);

    // usleep(1000*1000*5);

    return EState::SUCCESS;
}

void SpeechSocket::creatThread()
{
    thread_attribute pull_attr;
    memset(&pull_attr, 0, sizeof(thread_attribute));
    snprintf(pull_attr.name, 16, "push_cloud");
    pull_attr.policy = sched_get_priority_max(SCHED_RR) - 10;
    pull_attr.detachstate = PTHREAD_CREATE_DETACHED;
    threadCreate(SpeechSocket::connectSpeechServer, NULL, &pull_attr);
}

void * SpeechSocket::connectSpeechServer(void *arg)
{
    while (1) {
        if (SpeechSocket::connectfd < 0) {
            VR_LOG("Sockect did not connect and try to connect socket again");
            VR_LOG("liuzhen1");
            SpeechSocket::GetInstance().connectSocket();
                VR_LOG("liuzhen0");

            if (SpeechSocket::connectfd >= 0) {
                VR_LOG("liuzhen1");
                int errLogin = SpeechSocket::GetInstance().onLoginServer();
                // 0:发送成功; -1:socket没有连接; -2:token获取失败; -3:其他错误
                if (0 != errLogin) {
                    // 登录失败了，断开连接，过3秒重试
                    VR_LOGW("login failed, we would reconnect 3 seconds later...");
                    SpeechSocket::GetInstance().disconnectSocket();
                    sleep(3);
                }
            } else {

                VR_LOG("liuzhen2");
                // 连接失败，过2s再重试
                sleep(2);
            }
        } else {
            char buffer[BUFFERSIZE];
            RCVSIZE = recv(SpeechSocket::connectfd, buffer, sizeof(buffer), 0);
            if (RCVSIZE > 0) {
                SpeechSocket::GetInstance().parseRecMsg(buffer, RCVSIZE);
            } else {
                VR_LOG("It seems that socket disconnect");
                close(SpeechSocket::connectfd);
                SpeechSocket::connectfd = -1;
                SpeechSocket::logedIn = false;
                // 有的时候，刚刚连上就会被踢掉，然后再连，然后再踢，重试太频繁，所以这里也 sleep 一下
                sleep(2);
            }
        }
    }

    return (void*)0;
}

void SpeechSocket::connectSocket()
{
    VR_LOG("Try to connect socket");

    // SpeechSocket::connectfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // while (SpeechSocket::connectfd < 0) {
    //     VR_LOG("Failed to create a socket and we will try it again: %s", strerror(errno));
    //     SpeechSocket::connectfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //     usleep(100*1000);
    // }


    // string serverAddr = "/vrservice/speechtts";
    // int serverPort = 30082;
    // VR_LOG("cloud voice host address is %s:%d", serverAddr.data(), serverPort);
    // struct hostent* hosinfo = gethostbyname(serverAddr.data());
    // if (hosinfo == NULL) {
    //     VR_LOG("Get IP Address failed!");
    //     disconnectSocket();
    //     return;
    // }
    // struct sockaddr_in sndAdr;
    // bzero(&sndAdr, sizeof(sndAdr));
    // sndAdr.sin_family = hosinfo->h_addrtype;
    // struct in_addr **addr_list = (struct in_addr **)(hosinfo->h_addr_list);
    // sndAdr.sin_addr.s_addr = inet_addr(inet_ntoa(*addr_list[0]));
    // VR_LOG("ip address is %s", inet_ntoa(*addr_list[0]));
    // sndAdr.sin_port   = htons(serverPort);
    SpeechSocket::connectfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    while (SpeechSocket::connectfd < 0) {
        VR_LOG("Failed to create a socket and we will try it again: %s", strerror(errno));
        SpeechSocket::connectfd = socket(AF_LOCAL, SOCK_STREAM, 0);
        usleep(100*1000);
    }
    struct sockaddr_un sndAdr;
    bzero(&sndAdr, sizeof(sndAdr));
    strcpy(sndAdr.sun_path + 1, SOCKET_PATH);
    sndAdr.sun_family = AF_LOCAL;
    socklen_t addrlen = sizeof(sndAdr.sun_family) + strlen(SOCKET_PATH) + 1;

    int ret = connect(SpeechSocket::connectfd, (struct sockaddr*)&sndAdr, addrlen);
    if (ret < 0) {
        // 连不上就失败了，外面会循环重试的
        VR_LOGE("Connected to server failed and ret is %d", ret);
        SpeechSocket::connectfd = -1;
        return;
    }

    VR_LOG("Socket connected");
}

void SpeechSocket::disconnectSocket()
{
    if (SpeechSocket::connectfd < 0) {
        return;
    }
    VR_LOG("try to disconnect the connection...");
    shutdown(SpeechSocket::connectfd, SHUT_RD);
    close(SpeechSocket::connectfd);
    SpeechSocket::logedIn = false;
    SpeechSocket::connectfd = -1;
    VR_LOG("connection disconnected and buffer cleared.");
}

int SpeechSocket::onLoginServer()
{
    if (SpeechSocket::connectfd < 0) {
        return -1;
    }
    if（sendEventMsg(STARTENGINESYSTEM)) {
        char buffer[MAX_BUFFER];
        memset(buffer, 0, sizeof(buffer));
        int size = 0;
        if ((size = recv(SpeechSocket::connectfd, buffer, sizeof(buffer), 0)) <= 0) {
            VR_LOG("It seems that socket disconnect");
            return -1;
        } else {
            EventType type = EventType(buffer[0]);
            if (STARTENGINESYSTEM == type) {
                SpeechSocket::logedIn = true;
                VR_LOG("start engine system successfully!\n");
                return 0;
            } else {
                VR_LOG("start engine system failed!type is :%d\n", type);
                return -1;                    
            }
        }
    } else {
        VR_LOGE("Failed to start engine system");
        return -1;
    }
}

bool SpeechSocket::sendEventMsg(EventType type, char* msg, int size)
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
    if ((ret = send(SpeechSocket::connectfd, buffer, totalSize, 0)) != totalSize) {
        VR_LOG("Failed to send message and ret is %d with error id %d with message %s", ret, errno, strerror(errno));
        return false;
    } else {
        return true;
    }
}
