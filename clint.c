#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>

#define N 32

#define R 1 //用户注册regester
#define L 2 //用户登录login
#define Q 3 //用户查询query
#define H 4//用户历史记录history

//定义通信双方的信息结构体
typedef struct
{
	int type;
	char name[N];
	char data[256];
}MSG;

int do_regester(int sockfd,MSG *msg)
{
	//把用户输入的注册数据发后台
	msg->type = R;
	
	printf("input name: ");
	scanf("%s",msg->name);
	getchar();
	
	printf("input password: ");
	scanf("%s",msg->data);
	
	if(send(sockfd,msg,sizeof(MSG),0)<0)//msg是地址，MSG是存msg地址的内容
	{
		printf("发送失败\n");
		return -1;
	}

//发送成功后等待后台回复，接收后台的回复
if(recv(sockfd,msg,sizeof(MSG),0)<0)
{
	printf("读取失败\n");
	return -1;
}
//如果传入的是OK或文件已存在就代表传入成功
printf("%s\n",msg->data);
return 0;
}

int do_login(int sockfd,MSG *msg)
{
	msg->type = L;
	
	printf("input name: ");
	scanf("%s",msg->name);
	getchar();
	
	printf("input password: ");
	scanf("%s",msg->data);
	
	if(send(sockfd,msg,sizeof(MSG),0)<0)//msg是地址，MSG是存msg地址的内容
	{
		printf("发送失败\n");
		return -1;
	}

//发送成功后等待后台回复，接收后台的回复
if(recv(sockfd,msg,sizeof(MSG),0)<0)
{
	printf("读取失败\n");
	return -1;
}
	if(strncmp(msg->data,"OK",3) == 0)
	{
		printf("登录成功\n");
		return 1;
	}
	else
	{
		printf("%s\n",msg->data);
	}
	
	return 0;
}



int do_query(int sockfd,MSG *msg)
{
	msg->type = Q;
	puts("---------------");
	
	while(1)
	{
		printf("inout word: ");
		scanf("%s",msg->data);
		getchar();
		
		//客户端输入#返回到上一级菜单
		if(strncmp(msg->data,"#",1)==0)
			break;
		
		//将要查询的单词发送给服务器
		if(send(sockfd,msg,sizeof(MSG),0)<0)
		{
			printf("fail to send.\n");
			return -1;
		}
		
		//等待接收服务器传递回来单词的注释信息
		
		if(recv(sockfd,msg,sizeof(MSG),0)<0)
		{
			printf("fail to recv.\n");
			return -1;
		}
		printf("%s\n",msg->data);
	}
	
	return 0;
}

int do_history(int sockfd,MSG *msg)
{
	msg->type = H;
	
	send(sockfd,msg,sizeof(MSG),0);
	
	//接收服务器传递回来的历史信息
	while(1)
	{
		recv(sockfd,msg,sizeof(MSG),0);
		
		if(msg->data[0] == '\0')
			break;
		//打印历史信息
		printf("%s\n",msg->data);
	}
	
	//close(sockfd);
	return 0;
}

int main(int argc,const char *argv[])
{
	int sockfd;
	struct sockaddr_in serveraddr;
	int n;
	MSG msg;
	
	if(argc != 3)//应该从键盘输入3个参数   ./server 192.168.125.110，如果不是，就提示
	{
		printf("Usage:%s serverip port.\n",argv[0]);
		return -1;
	}
	
	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		perror("socket\n");
		return -1;
	}
	
	bzero(&serveraddr,sizeof(serveraddr));//对定义的结构体清零
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr=inet_addr(argv[1]);//因为从键盘输入的第二个参数是ip
	serveraddr.sin_port = htons(atoi(argv[2]));//atoi是char转为int
	
	if(connect(sockfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)) < 0)
	{
		perror("连接失败");
		return -1;
	}
	
	//开始写一级菜单
	
	while(1)
	{
		printf("********************************\n");
		printf("* 1:register   2: login 3: quit *\n");
		printf("********************************\n");
		printf("please choose : ");
		scanf("%d",&n);//获取命令
		
		getchar();//去掉垃圾字符
		
		switch(n)
		{
			case 1:
				do_regester(sockfd,&msg);
				break;
			case 2:
				if(do_login(sockfd,&msg)==1)
					goto next;
				break;
			case 3:
				close(sockfd);
				exit(0);
				break;
			default:
				printf("请输入正确命令\n");
				
		}
	}

next:
	while(1)
	{
		printf("*************************************************\n");
		printf("* 1 :  query_word  2 :  history_record 3 ：退出 *\n");
		printf("**************************************************\n");
		printf("please choose (#退出): ");
		scanf("%d",&n);//获取命令
		
		getchar();
		
		switch(n)
		{
			case 1:
				do_query(sockfd,&msg);
				break;
			case 2:
				do_history(sockfd,&msg);
				break;
			case 3:
				close(sockfd);
				exit(0);
				break;
			default:
				printf("请输入正确命令\n");				
		}
		
		
	}
		return 0;
	}