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
#include <linux/tcp.h>
#include <signal.h>

#define MYSOCKET_PATH "/tmp/mysocket"

void *client_thread(void *arg)
{
    int clifd = *(int *)arg;
	char *s = "hello mysocketclient\n";
	while(1)
    {
        usleep(1000000);
        ret = write(clifd,s,strlen(s)); //send(clifd,s,strlen(s),0);
        if(ret == -1)
        {
            printf("client thread write failed !\n");　　　　　　　close(clifd);
            pthread_exit(NULL);
        }

		// recv_len = recv(clifd, recv_buf, 100, 0);
		// if (recv_len < 0) {
		// 	cout << "接受失败！" << endl;
		// 	break;
		// }
		// else {
		// 	cout << "客户端信息:" << recv_buf << endl;
		// }
		// cout << "请输入回复信息:";
		// cin >> send_buf;
		// send_len = send(clifd, send_buf, 100, 0);
		// if (send_len < 0) {
		// 	cout << "发送失败！" << endl;
		// 	break;

    }
    return (void *)0;
}


int main(int argc,char **arg)
{

    int servfd;
    int ret;
    
    servfd = socket(AF_LOCAL,SOCK_STREAM,0);
    if(-1 == servfd)
    {
        perror("Can not create socket");
        return -1;
    }
    
    struct sockaddr_un servaddr;
    bzero(&servaddr, sizeof(servaddr));
    strcpy(servaddr.sun_path+1, MYSOCKET_PATH);
    servaddr.sun_family = AF_LOCAL;
    socklen_t addrlen = 1 + strlen(MYSOCKET_PATH) + sizeof(servaddr.sun_family);

    ret = bind(servfd, (struct sockaddr *)&servaddr, addrlen);
    if(-1 == ret)
    {
        perror("bind failed");
        return -1;
    }

    ret = listen(servfd, 100);
    if(-1 == ret)
    {
        perror("listen failed");
        return -1;
    }

    int clifd = -1;;
    pthread_t tid;
    struct sockaddr_un cliaddr;
    while(1)
    {
        printf("wait for next client connect \n");
        memset(&cliaddr,0,sizeof(cliaddr));
        clifd = accept(servfd,(struct sockaddr *)&cliaddr,&addrlen);
        if(clifd == -1)
        {
            printf("accept connect failed\n");
            continue;
        }
        ret = pthread_create(&tid,NULL,client_thread,&clifd);
        if(0 != ret)
        {
            printf("pthread_create failed \n");
        }

        ret = pthread_join(tid, NULL);
        if(0 != ret)
        {
            printf("pthread_join failed \n");
        }
        printf("exit thread\n");
        /*
        ret = pthread_detach(tid);
        if(0 != ret)
        {
            printf("pthread_detach failed \n");
            return -1;
        }*/
    }
	signal(SIGPIPE, SIG_IGN);

    return 0;
}