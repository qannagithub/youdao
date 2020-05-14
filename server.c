#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<sqlite3.h>
#include<time.h>

#define N 32

#define R 1 //用户注册regester
#define L 2 //用户登录login
#define Q 3 //用户查询query
#define H 4//用户历史记录history

#define DATABASE "my.db"

//定义通信双方的信息结构体
typedef struct
{
	int type;
	char name[N];
	char data[256];
}MSG;

int do_clint(int acceptfd,sqlite3 *db);
void do_regster(int acceptfd,MSG *msg,sqlite3 *db);
int do_login(int acceptfd,MSG *msg,sqlite3 *db);
int do_query(int acceptfd,MSG *msg,sqlite3 *db);
int do_history(int acceptfd,MSG *msg,sqlite3 *db);
int history_callback(void * arg,int f_num,char ** f_value,char** f_name);
int do_searchword(int acceptfd,MSG *msg,char word[]);
int get_date(char *date);

int main(int argc,const char *argv[])
{
	int sockfd;
	struct sockaddr_in serveraddr;
	int n;
	MSG msg;
	sqlite3 *db;
	int acceptfd;
	pid_t pid;
	
	if(argc != 3)//应该从键盘输入3个参数   ./server 192.168.125.110，如果不是，就提示
	{
		printf("Usage:%s serverip port.\n",argv[0]);
		return -1;
	}
	
	//打开数据库
	if(sqlite3_open(DATABASE,&db) != SQLITE_OK)
	{
		printf("open %s fail.\n",sqlite3_errmsg(db));
		return -1;
	}
	else
		printf("打开成功\n");
	
	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		perror("socket\n");
		return -1;
	}
	
	bzero(&serveraddr,sizeof(serveraddr));//对定义的结构体清零
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr=inet_addr(argv[1]);//因为从键盘输入的第二个参数是ip
	serveraddr.sin_port = htons(atoi(argv[2]));//atoi是char转为int
	
	if(bind(sockfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)) < 0)
	{
		perror("fail to blind.\n");
		return -1;
	}
	
	//将套接字设为监听模式
	
	if(listen(sockfd,5) < 0)
	{
		perror("fail to listen.\n");
		return -1;
	}
	
	//处理僵尸进程
	
	signal(SIGCHLD,SIG_IGN);//17、SIGCHLD子进程结束时，父进程会收到这个信号,SIG_IGN是忽略信号
	
	//开始写一级菜单
	
	while(1)
	{
		if(acceptfd = accept(sockfd,NULL,NULL) < 0 )
		{
			perror("fail to accept.\n");
			return -1;
		}
		//连接成功了创建进程，让父进程接收请求，子进程处理客户端的需求 
	
		if((pid = fork()) < 0)
		{
			perror("创建进程失败");
			return -1;
		}
		else if(pid==0)//子进程，用来处理具体的消息
		{
			close(sockfd);//子进程也不需要监听套接字
			do_clint(acceptfd,db);
		}
		else//这是父进程,用来接收客户端的请求
		{
			close(acceptfd);//所以客户端接收套接字的acceptfd就不需要了，节约资源
		}
			
	}


		return 0;
	}
	
	int do_clint(int acceptfd,sqlite3 *db)
	{
		MSG msg;
		while(recv(acceptfd,&msg,sizeof(msg),0) > 0)
		{
			printf("type:%d\n",msg.type);
			switch(msg.type)
			{
				case R:
					do_regster(acceptfd,&msg,db);
					break;
				case L:
					do_login(acceptfd,&msg,db);
					break;
				case Q:
					do_query(acceptfd,&msg,db);
					break;
				case H:
					do_history(acceptfd,&msg,db);
					break;
				default:
					printf("非法消息\n");
				
			}
			
		}
		
		printf("clint exit.\n");
		close(acceptfd);
		exit(0);
		
		return 0;
	}
	
	void do_regster(int acceptfd,MSG *msg,sqlite3 *db)//acceptfd给客户端通信
{
	char * errmsg;
	char sql[128];
	
	sprintf(sql,"insert into usr values('%s',%s);",msg->name,msg->data);
	printf("%s\n",sql);
	
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=SQLITE_OK)	
	{
		printf("%s\n",errmsg);
		strcpy(msg->data,"usr name 已经存在.");
		
	}
	else
		printf("注册成功\n");
	strcpy(msg->data,"OK");


if(send(acceptfd,msg,sizeof(MSG),0)<0)
{
	perror("发送失败\n");
	return ;
}
return;
}
int do_login(int acceptfd,MSG *msg,sqlite3 *db)
{
	char sql[128];
	char *errmsg;
	int nrow;
	int ncloumn;
	char **resultp;
	
	sprintf(sql,"select * from usr where name = '%s' and passwd = '%s';",msg->name,msg->data);
	printf("%s\n",sql);
	
	if(sqlite3_get_table(db,sql,&resultp,&nrow,&ncloumn,&errmsg)!=SQLITE_OK)
	{
		printf("%s\n",errmsg);
		return -1;
	}
	else
	{
		printf("执行查询语句成功\n");
	}
	//查询成功，数据库有次用户
	if(nrow == 1)
	{
		strcpy(msg->data,"OK");
		send(acceptfd,msg,sizeof(MSG),0);
		return 1;
	}
	else//密码或用户名错误
	{
		strcpy(msg->data,"用户名或密码错误");
		send(acceptfd,msg,sizeof(MSG),0);
	}
	
	return 0;
}

int do_searchword(int acceptfd,MSG *msg,char word[])
{
	FILE * fp;
	int len = 0;
	char temp[512]={};
	int result;
	char *p;
	
	//打开文件，读取文件，进行比对
	
	if((fp = fopen("dict.txt","r"))==NULL)
	{
		perror("fail to fopen.\n");
		strcpy(msg->data,"fail to open dict.txt");
		send(acceptfd,msg,sizeof(MSG),0);
		return -1;
	}
	
	//打印出客户端要查询的单词
	len = strlen(word);
	printf("%s,len = %d\n",word,len);
	
	//读文件查询单词
	while(fgets(temp,512,fp)!=NULL)
	{
		result = strncmp(temp,word,len);
		
		if(result < 0)
		{
			continue;
		}
		if(result > 0 || ((result == 0)&& (temp[len] != ' ')))
		{
			break;
		}
		
		// 表示找到了查询的单词
		p = temp + len;// apple   n.苹果
		while(*p ==' ')
		{
			p++;
		}
		
		//找到了注释，跳过所有的空格
		strcpy(msg->data,p);
		
		//拷贝完后，应该关闭文件
		fclose(fp);
		return 1;
	}
		fclose(fp);
		
		return 0;
}

int get_date(char *date)
{
	time_t t;
	struct tm *tp;
	
	time(&t);
	
	//时间的格式转化
	tp = localtime(&t);
	
	sprintf(date,"%d-%d-%d %d:%d:%d",tp->tm_year + 1900,tp->tm_mon + 1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec);
	printf("get date:%s\n",date);
	return 0;
}

int do_query(int acceptfd,MSG *msg,sqlite3 *db)
{
	char word[64];
	int found=0;
	char date[128]={};
	char sql[126]={};
	char *errmsg;
	
	//拿出msg结构体中要查询的单词
	strcpy(word,msg->data);
		
	found = do_searchword(acceptfd,msg,word);
	
	//表示找到了单词,并应该将用户名，时间，单词，插入到历史记录表
	if(found==1)
	{
		//需要获取系统时间
		get_date(date);
		
		sprintf(sql,"insert into record values('%s','%s','%s')",msg->name,date,word);
		printf("%s\n",sql);
		
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=SQLITE_OK)
		{
			printf("%s\n",errmsg);
			return -1;
		}
	}
	else //表示没有找到
	{
		strcpy(msg->data,"not found");
	}
	
	//将查询的结果发送给客户端
	send(acceptfd,msg,sizeof(MSG),0);
	
	return 0;
}

int history_callback(void * arg,int f_num,char ** f_value,char** f_name)
{
	int acceptfd;
	MSG msg;
	
	acceptfd = *((int *)arg);
	
	sprintf(msg.data,"%s,%s",f_value[1],f_value[2]);
	
	send(acceptfd,&msg,sizeof(MSG),0);
	
	return 0;
}

int do_history(int acceptfd,MSG *msg,sqlite3 *db)
{
	char sql[128]={};
	char * errmsg;
	
	sprintf(sql,"select * from record where name = '%s'",msg->name);
	
	//查询数据库
	if(sqlite3_exec(db,sql,history_callback,(void *)&acceptfd,&errmsg)!=SQLITE_OK)
	{
		printf("%s\n",errmsg);
	}
	else
	{
		printf("Query record done.\n");
	}
	
	//所有的记录查询后，给客户端发送结束
	msg->data[0] = '\0';
	
	send(acceptfd,msg,sizeof(MSG),0);
	
	return 0;
}

