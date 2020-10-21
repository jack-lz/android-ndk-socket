#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <cutils/sockets.h>
#include <utils/Log.h>
#include <android/log.h>

#define SOCKET_PATH "/tmp/mysocket"

int main(int argc,char **arg)
{
    int clifd;
    int ret;

    clifd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if(-1 == clifd)
    {
        perror("socket create failed\n");
        return -1;
    }

    struct sockaddr_un cileddr;
    bzero(&cileddr, sizeof(cileddr));
    strcpy(cileddr.sun_path + 1, SOCKET_PATH);
    cileddr.sun_family = AF_LOCAL;
    socklen_t addrlen = sizeof(cileddr.sun_family) + strlen(SOCKET_PATH) + 1;

    ret = connect(clifd, (struct sockaddr *)&cileddr, addrlen);
    if(ret == -1) {
        perror("Connect fail\n");
        return -1;
    }

    char getData[100];    
    while(1)
    {
        bzero(&getData,sizeof(getData));
        ret = read(clifd,&getData,sizeof(getData));
        if(ret > 0)
        {
            printf("rcv message: %s\n", getData);
        }

		// cout << "请输入发送信息:";
		// cin >> send_buf;
		// send_len = send(s_server, send_buf, 100, 0);
		// if (send_len < 0) {
		// 	cout << "发送失败！" << endl;
		// 	break;
		// }
		// recv_len = recv(s_server, recv_buf, 100, 0);
		// if (recv_len < 0) {
		// 	cout << "接受失败！" << endl;
		// 	break;
		// }
		// else {
		// 	cout << "服务端信息:" << recv_buf << endl;
		// }

    }

    return 0;
}