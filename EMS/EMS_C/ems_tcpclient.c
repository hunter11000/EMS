#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#define IP "192.168.1.99"
#define PORT 8888
#define MAX_USERNAME_LEN 64
#define MAX_USERPWD_LEN 64
#define MAX_NAME_LEN 64
#define MAX_EINFO_LEN 64
#define MAX_INFO_DATE_LEN 512
#define MAX_TIME_INFO_LEN 256

#define ERR_MSG(msg) do{\
	fprintf(stderr, "%d %s %s\n",\
	__LINE__, __func__, __FILE__);\
	perror(msg);\
}while(0)

#define REGISTER 'R'
#define LOGIN 'L'
#define SUCCESS 'S'
#define EXIT 'E'
#define CANCEL 'C'
#define OFFLINE 'f'
#define ONLINE 'o'
#define ADMIN 'a'
#define EMPLOYEE 'e'
#define WPASSWD 'w'
#define CPASSWD 'c'
#define USERUNEXIST 'u'
#define USERISLOGIN 'i'
#define LOGOUTFAIL '0'
#define LOGOUTSCUS '1'
#define QUERYUSERSEL '2'
#define QUERYALL '3'
#define MODIFYUSERSELF '4'
#define MODIFYALL '5'
#define RECORD '6'
#define SHOWSELFRECORD '7'
#define SHOWALLRECORD '8'
#define MAX_THAN_LIMIT_WRONG_TIMES 'm'
#define DATASENDOVER '9'
#define ADDEINFO 'A'
#define DELETE_EINFO 'D'
#define EINFO_NON_EXISTENT 'N'


typedef struct {
	unsigned int id;
	char name[MAX_NAME_LEN];
	char sex;
	int age;
	char department[MAX_EINFO_LEN];
	char post[MAX_EINFO_LEN];
	char lv[MAX_EINFO_LEN];
	char phone[MAX_EINFO_LEN];
	char email[MAX_EINFO_LEN];
	char address[MAX_EINFO_LEN];
	int salary;
}EMPLOYEE_INFO;

typedef struct {
	char type; 
	char name[MAX_USERNAME_LEN];
	char pword[MAX_USERPWD_LEN];
    char info_data[MAX_INFO_DATE_LEN];
	char state;
    char authority;
	unsigned int  bind_id;
}U_INFO;

typedef struct {
	unsigned int record_id;
	char name[MAX_NAME_LEN];
	char time[MAX_TIME_INFO_LEN];
}R_INFO;


typedef struct {
	R_INFO r_info;
	U_INFO u_info;
	EMPLOYEE_INFO e_info;
	int  flag;
}DATA_INFO;

int do_employee_window(DATA_INFO info, int choose, int cfd);
int do_admin_window(DATA_INFO d_info, int choose, int cfd);
int do_main_window(DATA_INFO d_info, DATA_INFO dataout, int choose, int cfd);
int do_register(int cfd, DATA_INFO d_info);
DATA_INFO  do_sign_in(int cfd, DATA_INFO d_info);
int do_quit(int cfd, DATA_INFO d_info);
int do_cancel(int cfd, DATA_INFO d_info);
int do_query_userself_info(int cfd, DATA_INFO d_info);
int do_modify_userself_info(int cfd, DATA_INFO d_info);
int do_employee_record(int cfd, DATA_INFO d_info);
int do_show_eself_record(int cfd, DATA_INFO d_info);
int do_add_employee_info(int cfd, DATA_INFO d_info);
int do_modifiy_employee_info(int cfd, DATA_INFO d_info);
int do_delete_employee_info(int cfd, DATA_INFO d_info);
int do_show_all_record(int cfd, DATA_INFO d_info);
int do_show_all_employee_info(int cfd, DATA_INFO d_info);



int main(int argc, const char *argv[]) {

	
	//create socket stream/

	int cfd = socket(AF_INET, SOCK_STREAM, 0);
	if(cfd < 0) {
		ERR_MSG("socket");
		return -1;
	}
	
    //connect server

	struct sockaddr_in sin;

	sin.sin_family      = AF_INET;
	sin.sin_port        = htons(PORT);
	sin.sin_addr.s_addr = inet_addr(IP);

	if(connect(cfd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
		ERR_MSG("connect");
		return -1;
	}
	printf("connect server success\n");

	DATA_INFO d_info;
	DATA_INFO  dataout = {0};
	int    choose = 0;
	int    ret;

	while(1) {
		ret = do_main_window(d_info, dataout, choose, cfd);
		if(ret == 0) {
			break;
		}
	}
    return 0;
}

int do_main_window(DATA_INFO d_info, DATA_INFO dataout, int choose, int cfd)
{
	system("clear");
	printf("_________________________________________________\n");
	printf("|                                               |\n");
	printf("|                    登录(1)                    |\n"); 
	printf("|_______________________________________________|\n");
	printf("|                                               |\n");
	printf("|                    注册(2)                    |\n");
	printf("|_______________________________________________|\n");
	printf("|                                               |\n");
	printf("|                  退出程序(3)                  |\n");
	printf("|_______________________________________________|\n");

	putchar(10);
	printf("Please Input >>>>>");
	scanf("%d", &choose);
	while(getchar() != 10);
	memset(&d_info, 0, sizeof(d_info));
	memset(&dataout, 0, sizeof(dataout));

	switch(choose) {
	    case 1:
			dataout = do_sign_in(cfd, d_info);
			if(dataout.flag == 1) {
				if(dataout.u_info.authority == ADMIN) {
					if(do_admin_window(dataout, choose, cfd) == 1) {
						do_main_window(d_info, dataout, choose, cfd);
					}
                }
                if(dataout.u_info.authority == EMPLOYEE) {
                    if(do_employee_window(dataout, choose, cfd) == 1) {
						do_main_window(d_info, dataout, choose, cfd);
					}
                }
			}
			break;
    	case 2:
    		do_register(cfd, d_info);
			do_main_window(d_info, dataout, choose, cfd);
	    	break;
	    case 3:
		    do_quit(cfd, d_info);
			break;
	    default:
     		printf("Input Again\n");
		    break;
	}
	return 0;
}

int do_admin_window(DATA_INFO d_info, int choose, int cfd)
{
    while(1) {
		system("clear");
		printf("_______________________________________________________\n");
		printf("|                          |                          |\n");
		printf("|       添加员工信息(1)    |       查看员工信息(4)    |\n"); 
		printf("|__________________________|__________________________|\n");
		printf("|                          |                          |\n");
		printf("|       修改员工信息(2)    |        查看考勤表(5)     |\n");
		printf("|__________________________|__________________________|\n");
		printf("|                          |                          |\n");
		printf("|       删除员工信息(3)    |       注销/切换用户(6)   |\n");
		printf("|__________________________|__________________________|\n");
		putchar(10);

		printf("Please Input >>>>>");	
		scanf("%d", &choose);

		while(getchar() != 10);

		switch (choose)
		{
		case 1:
			do_add_employee_info(cfd, d_info);
			break;
		case 2:
			do_modifiy_employee_info(cfd, d_info);
			break;
		case 3:
			do_delete_employee_info(cfd, d_info);
			break;
		case 4:
			do_show_all_employee_info(cfd, d_info);
			break;
		case 5:
			do_show_all_record(cfd, d_info);
			break;
		case 6:
			if(do_cancel(cfd, d_info) == 1) {
				return 1;
			}
			break;
		
		default:
			break;
		}

		printf("PRESS ANY KEY TO GOBACK!\n");
	    while(getchar() != 10);
	}
}

int do_employee_window(DATA_INFO d_info, int choose, int cfd)
{
    while(1) {
		system("clear");
		printf("__________________________________________________________________\n");
		printf("|                                |                               |\n");
		printf("|         查询本人信息(1)        |          员工打卡(3)          |\n"); 
		printf("|________________________________|_______________________________|\n");
		printf("|	                         |                               |\n");
		printf("|         修改本人信息(2)        |        查看本人打卡记录(4)    |\n");
		printf("|________________________________|_______________________________|\n");
		printf("|                                                                |\n");
		printf("|                          注销/切换用户(5)                      |\n");
		printf("|________________________________________________________________|\n");
		putchar(10);

		printf("Please Input >>>>>");	
		scanf("%d", &choose);

		while(getchar() != 10);

		switch(choose) {
	    	case 1:
		    	do_query_userself_info(cfd, d_info);
	    		break;
    		case 2:
  			    do_modify_userself_info(cfd, d_info);
	    		break;
			case 3:
				do_employee_record(cfd, d_info);
				break;
			case 4:
				do_show_eself_record(cfd, d_info);
				break;
	    	case 5:
	            if(do_cancel(cfd, d_info) == 1) {
					return 1;
				}
		    	break;
	    	default:
     			printf("Input Again\n");
		}
    printf("PRESS ANY KEY TO GOBACK!\n");
	while(getchar() != 10);
	}
}

int do_register(int cfd, DATA_INFO d_info) 
{
    //Filled structure(type, name, pword)
	int temp = 0;
	d_info.u_info.type = REGISTER;

	printf("Please Input Username >>>");
	fgets(d_info.u_info.name, sizeof(d_info.u_info.name), stdin);
	d_info.u_info.name[strlen(d_info.u_info.name)-1] = 0;

	printf("Please Input Password >>>");
	fgets(d_info.u_info.pword, sizeof(d_info.u_info.pword), stdin);
	d_info.u_info.pword[strlen(d_info.u_info.pword)-1] = 0;

	printf("Please Input E_ID >>>");
	scanf("%d", &temp);
	while(getchar() != 10);
    d_info.u_info.bind_id = htonl(temp);
	d_info.u_info.state = OFFLINE;
    d_info.u_info.authority = EMPLOYEE;

	//send register message to server

	if(send(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}

	//receive register message from server

	if(recv(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("recv");
		return -1;
	}

	printf("%s \n", d_info.u_info.info_data);

	printf("按任意键返回!\n");
	while(getchar() != 10);

	return 0;
}

DATA_INFO do_sign_in(int cfd, DATA_INFO d_info) 
{

    //Filled structure(type, name, pword)

	d_info.u_info.type = LOGIN;
	DATA_INFO dataout = {0};
	

	printf("Please Input Username >>>");
	fgets(d_info.u_info.name, sizeof(d_info.u_info.name), stdin);
	d_info.u_info.name[strlen(d_info.u_info.name)-1] = 0;

	printf("Please Input Password >>>");
	fgets(d_info.u_info.pword, sizeof(d_info.u_info.pword), stdin);
	d_info.u_info.pword[strlen(d_info.u_info.pword)-1] = 0;


	//send sign in message to server

	if(send(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		dataout.flag = -1;
		return dataout;
	}

	//receive sign in message from server

	if(recv(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("recv");
		dataout.flag = -1;
		return dataout;
	}
	switch(d_info.u_info.type) {
		case MAX_THAN_LIMIT_WRONG_TIMES:
            printf("%s\n", d_info.u_info.info_data);
            printf("PRESS ANY KEY TO GOBACK!\n");
    	    while(getchar() != 10);
		    dataout.flag = -1;
			return dataout;
		    break;
        case USERUNEXIST:
            printf("%s\n", d_info.u_info.info_data);
            printf("PRESS ANY KEY TO GOBACK!\n");
    	    while(getchar() != 10);
		    dataout.flag = -1;
			return dataout;
		    break;
        case WPASSWD:
            printf("%s\n", d_info.u_info.info_data);
            printf("PRESS ANY KEY TO GOBACK!\n");
    	    while(getchar() != 10);
		    dataout.flag = -1;
			return dataout;
		    break;
	    case CPASSWD:
    	    printf("%s %d\n", d_info.u_info.info_data, __LINE__);
            printf("%c %d\n", d_info.u_info.authority, __LINE__);
			printf("name = %s pword = %s state = %c authority = %c bind_id = %d %d\n",
			d_info.u_info.name, d_info.u_info.pword, d_info.u_info.state, d_info.u_info.authority, ntohl(d_info.u_info.bind_id), __LINE__);
			printf("PRESS ANY KEY TO GOBACK!\n");
    	    //while(getchar() != 10);
			dataout.flag = 1;
			dataout.u_info.authority = d_info.u_info.authority;
			dataout.u_info.bind_id = ntohl(d_info.u_info.bind_id);
			strcpy(dataout.u_info.name, d_info.u_info.name);
		    return dataout;
		    break;
	    case USERISLOGIN:
    	    printf("%s\n", d_info.u_info.info_data);
    	    printf("PRESS ANY KEY TO GOBACK\n!");
    	    while(getchar() != 10);
		    dataout.flag = -1;
			return dataout;
		    break;
	    default:
		    break;
	}
	return dataout;
}

int do_quit(int cfd, DATA_INFO d_info) 
{
	d_info.u_info.type = EXIT;

	if(send(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}
	return 0;
}

int do_cancel(int cfd, DATA_INFO d_info) 
{

	d_info.u_info.type  = CANCEL;

	//send state message to server

	if(send(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}

	//receive cancelmessage from server

	if(recv(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("recv");
		return -1;
	}

	switch(d_info.u_info.type) {
	case LOGOUTSCUS:
    	printf("%s\n", d_info.u_info.info_data);
		return 1;
		break;
	case LOGOUTFAIL:
    	printf("%s\n", d_info.u_info.info_data);
    	printf("PRESS ANY KEY TO GOBACK!\n");
    	while(getchar() != 10);
		return -1;
		break;
	default:
		break;
	}
	return 0;
}

int do_query_userself_info(int cfd, DATA_INFO d_info) 
{
	//printf("name = %s pword = %s state = %c authority = %c bind_id = %d %d\n",
	//d_info.u_info.name, d_info.u_info.pword, d_info.u_info.state, d_info.u_info.authority, 
	//d_info.u_info.bind_id, __LINE__);
	d_info.u_info.type = QUERYUSERSEL;
	if(send(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}

	if(recv(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("recv");
		return -1;
	}
	printf("ID      : %d\n", d_info.e_info.id);
	printf("NAME    : %s\n", d_info.e_info.name);
	printf("SEX     : %c\n", d_info.e_info.sex);
	printf("AGE     : %d\n", d_info.e_info.age);
	printf("DEPART  : %s\n", d_info.e_info.department);
	printf("POST    : %s\n", d_info.e_info.post);
	printf("LV      : %s\n", d_info.e_info.lv);
	printf("PHONE   : %s\n", d_info.e_info.phone);
	printf("EMAIL   : %s\n", d_info.e_info.email);
	printf("ADDRESS : %s\n", d_info.e_info.address);
	printf("SALARY  : %d\n", d_info.e_info.salary);

	return 0;
}

int do_modify_userself_info(int cfd, DATA_INFO d_info)
{	
	//printf("name = %s pword = %s state = %c authority = %c bind_id = %d %d\n",
	//d_info.u_info.name, d_info.u_info.pword, d_info.u_info.state, d_info.u_info.authority, 
	//d_info.u_info.bind_id, __LINE__);
	d_info.u_info.type = MODIFYUSERSELF;

	if(send(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}

	if(recv(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("recv");
		return -1;
	}
	printf("**********************************\n");

	printf("ID      : %d\n", d_info.e_info.id);
	printf("NAME    : %s\n", d_info.e_info.name);
	printf("SEX     : %c\n", d_info.e_info.sex);
	printf("AGE     : %d\n", d_info.e_info.age);
	printf("DEPART  : %s\n", d_info.e_info.department);
	printf("POST    : %s\n", d_info.e_info.post);
	printf("LV      : %s\n", d_info.e_info.lv);
	printf("PHONE   : %s\n", d_info.e_info.phone);
	printf("EMAIL   : %s\n", d_info.e_info.email);
	printf("ADDRESS : %s\n", d_info.e_info.address);
	printf("SALARY  : %d\n", d_info.e_info.salary);

	printf("**********************************\n");
	
	bzero(d_info.e_info.name, sizeof(d_info.e_info.name));
	printf("PLZ INPUT MODIFIED_NAME>>>");
	scanf("%s", d_info.e_info.name);
	getchar();

	printf("%s\n", d_info.e_info.name);

	printf("PLZ INPUT MODIFIED_SEX>>>");
	scanf("%c", &d_info.e_info.sex);
	getchar();

	printf("%c\n", d_info.e_info.sex);

	printf("PLZ INPUT MODIFIED_AGE>>>");
	scanf("%d", &d_info.e_info.age);
	getchar();

	printf("%d\n", d_info.e_info.age);

	bzero(d_info.e_info.phone, sizeof(d_info.e_info.phone));
	printf("PLZ INPUT MODIFIED_PHONE>>>");
	scanf("%s", d_info.e_info.phone);
	getchar();

	printf("%s\n", d_info.e_info.phone);

	bzero(d_info.e_info.email, sizeof(d_info.e_info.email));
	printf("PLZ INPUT MODIFIED_EMAIL>>>");
	scanf("%s", d_info.e_info.email);
	getchar();

	printf("%s\n", d_info.e_info.email);

	bzero(d_info.e_info.address, sizeof(d_info.e_info.address));
	printf("PLZ INPUT MODIFIED_ADDRESS>>>");
	scanf("%s", d_info.e_info.address);
	getchar();

	printf("%s\n", d_info.e_info.address);

	printf("**********************************\n");

	d_info.u_info.type = MODIFYUSERSELF;

	if(send(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}

	if(recv(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("recv");
		return -1;
	}

	printf("           %s\n", d_info.u_info.info_data);

	return 0;
}

int do_employee_record(int cfd, DATA_INFO d_info)
{	
	char c;
	printf("**********************************\n");
	printf("DO YOU WANT TO RECORD NWO\n");
	printf("INPUT (y)RECORD (r)GOBACK\n");
	printf("PLZ INPUT CHOOSE  >>>");
	scanf("%c", &c);
	while(getchar() != 10);

	switch (c)
	{
	case 'y':
		//printf("name = %s pword = %s state = %c authority = %c bind_id = %d %d\n",
		//d_info.u_info.name, d_info.u_info.pword, d_info.u_info.state, d_info.u_info.authority, 
		//d_info.u_info.bind_id, __LINE__);
		d_info.u_info.type = RECORD;

		if(send(cfd, &d_info, sizeof(d_info), 0) < 0) {
			ERR_MSG("send");
			return -1;
		}
	
		if(recv(cfd, &d_info, sizeof(d_info), 0) < 0) {
			ERR_MSG("recv");
			return -1;
		}

		printf("%s\n", d_info.u_info.info_data);

		break;

	case 'r':
		return 0;
		break;
	
	default:
		break;
	}
	return 0;	
}

int do_show_eself_record(int cfd, DATA_INFO d_info)
{
	d_info.u_info.type = SHOWSELFRECORD;

	if(send(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}

	while(1) {
		if(recv(cfd, &d_info, sizeof(d_info), 0) < 0) {
			ERR_MSG("recv");
			return -1;
		}
		if(d_info.u_info.type == DATASENDOVER) {
			break;
		}else {
			printf("**********************************\n");
			printf("**************RECORD**************\n");
			printf("**********************************\n");
			printf("%d  %s  %s\n", ntohl(d_info.r_info.record_id), d_info.r_info.name, d_info.r_info.time);

		}
	}
	return 0;
}

int do_add_employee_info(int cfd, DATA_INFO d_info)
{
	d_info.u_info.type = ADDEINFO;

	printf("PLZ INPUT E_ID>>>");
	scanf("%d", &d_info.e_info.id);
	getchar();

	printf("%d\n", d_info.e_info.id);

	bzero(d_info.e_info.name, sizeof(d_info.e_info.name));
	printf("PLZ INPUT E_NAME>>>");
	scanf("%s", d_info.e_info.name);
	getchar();

	printf("%s\n", d_info.e_info.name);

	printf("PLZ INPUT E_SEX>>>");
	scanf("%c", &d_info.e_info.sex);
	getchar();

	printf("%c\n", d_info.e_info.sex);

	printf("PLZ INPUT E_AGE>>>");
	scanf("%d", &d_info.e_info.age);
	getchar();

	printf("%d\n", d_info.e_info.age);

	bzero(d_info.e_info.department, sizeof(d_info.e_info.department));
	printf("PLZ INPUT E_DPART>>>");
	scanf("%s", d_info.e_info.department);
	getchar();

	printf("%s\n", d_info.e_info.department);

	bzero(d_info.e_info.post, sizeof(d_info.e_info.post));
	printf("PLZ INPUT E_POST>>>");
	scanf("%s", d_info.e_info.post);
	getchar();

	printf("%s\n", d_info.e_info.post);

	bzero(d_info.e_info.lv, sizeof(d_info.e_info.lv));
	printf("PLZ INPUT E_LV>>>");
	scanf("%s", d_info.e_info.lv);
	getchar();

	printf("%s\n", d_info.e_info.lv);

	bzero(d_info.e_info.phone, sizeof(d_info.e_info.phone));
	printf("PLZ INPUT E_PHONE>>>");
	scanf("%s", d_info.e_info.phone);
	getchar();

	printf("%s\n", d_info.e_info.phone);

	bzero(d_info.e_info.email, sizeof(d_info.e_info.email));
	printf("PLZ INPUT E_EMAIL>>>");
	scanf("%s", d_info.e_info.email);
	getchar();

	printf("%s\n", d_info.e_info.email);

	bzero(d_info.e_info.address, sizeof(d_info.e_info.address));
	printf("PLZ INPUT E_ADDRESS>>>");
	scanf("%s", d_info.e_info.address);
	getchar();

	printf("%s\n", d_info.e_info.address);

	printf("PLZ INPUT E_SALARY>>>");
	scanf("%d", &d_info.e_info.salary);
	getchar();

	printf("%d\n", d_info.e_info.salary);



	if(send(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}

	if(recv(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("recv");
		return -1;
	}

	printf("\n");

	printf("%s\n", d_info.u_info.info_data);

	return 0;
}

int do_modifiy_employee_info(int cfd, DATA_INFO d_info)
{
	printf("PLZ INPUT EID OF EMPLOYEE WHICH ONE TO MODIFY >>>");
	scanf("%d", &d_info.e_info.id);
	while(getchar() != 10);

	d_info.u_info.type = MODIFYALL;

	if(send(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}

	if(recv(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("recv");
		return -1;
	}
	printf("**********************************\n");

	printf("ID      : %d\n", d_info.e_info.id);
	printf("NAME    : %s\n", d_info.e_info.name);
	printf("SEX     : %c\n", d_info.e_info.sex);
	printf("AGE     : %d\n", d_info.e_info.age);
	printf("DEPART  : %s\n", d_info.e_info.department);
	printf("POST    : %s\n", d_info.e_info.post);
	printf("LV      : %s\n", d_info.e_info.lv);
	printf("PHONE   : %s\n", d_info.e_info.phone);
	printf("EMAIL   : %s\n", d_info.e_info.email);
	printf("ADDRESS : %s\n", d_info.e_info.address);
	printf("SALARY  : %d\n", d_info.e_info.salary);

	printf("**********************************\n");
	
	bzero(d_info.e_info.name, sizeof(d_info.e_info.name));
	printf("PLZ INPUT MODIFIED_NAME>>>");
	scanf("%s", d_info.e_info.name);
	getchar();

	printf("%s\n", d_info.e_info.name);

	printf("PLZ INPUT MODIFIED_SEX>>>");
	scanf("%c", &d_info.e_info.sex);
	getchar();

	printf("%c\n", d_info.e_info.sex);

	printf("PLZ INPUT MODIFIED_AGE>>>");
	scanf("%d", &d_info.e_info.age);
	getchar();

	printf("%d\n", d_info.e_info.age);

	bzero(d_info.e_info.department, sizeof(d_info.e_info.department));
	printf("PLZ INPUT MODIFIED_DEPART>>>");
	scanf("%s", d_info.e_info.department);
	getchar();

	printf("%s\n", d_info.e_info.department);

	bzero(d_info.e_info.post, sizeof(d_info.e_info.post));
	printf("PLZ INPUT MODIFIED_POST>>>");
	scanf("%s", d_info.e_info.post);
	getchar();

	printf("%s\n", d_info.e_info.post);

	bzero(d_info.e_info.lv, sizeof(d_info.e_info.lv));
	printf("PLZ INPUT MODIFIED_LV>>>");
	scanf("%s", d_info.e_info.lv);
	getchar();

	printf("%s\n", d_info.e_info.lv);

	bzero(d_info.e_info.phone, sizeof(d_info.e_info.phone));
	printf("PLZ INPUT MODIFIED_PHONE>>>");
	scanf("%s", d_info.e_info.phone);
	getchar();

	printf("%s\n", d_info.e_info.phone);

	bzero(d_info.e_info.email, sizeof(d_info.e_info.email));
	printf("PLZ INPUT MODIFIED_EMAIL>>>");
	scanf("%s", d_info.e_info.email);
	getchar();

	printf("%s\n", d_info.e_info.email);

	bzero(d_info.e_info.address, sizeof(d_info.e_info.address));
	printf("PLZ INPUT MODIFIED_ADDRESS>>>");
	scanf("%s", d_info.e_info.address);
	getchar();

	printf("%s\n", d_info.e_info.address);

	printf("PLZ INPUT MODIFIED_SALARY>>>");
	scanf("%d", &d_info.e_info.salary);
	getchar();

	printf("%d\n", d_info.e_info.salary);

	printf("**********************************\n");

	d_info.u_info.type = MODIFYALL;

	if(send(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}

	if(recv(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("recv");
		return -1;
	}

	printf("    %s\n", d_info.u_info.info_data);

	return 0;
}

int do_delete_employee_info(int cfd, DATA_INFO d_info)
{
	printf("PLZ INPUT EID OF EMPLOYEE WHICH ONE TO DELETE >>>");
	scanf("%d", &d_info.e_info.id);
	while(getchar() != 10);

	d_info.u_info.type = DELETE_EINFO;

	if(send(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}

	if(recv(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("recv");
		return -1;
	}

	printf("    %s\n", d_info.u_info.info_data);
	return 0;
}

int do_show_all_record(int cfd, DATA_INFO d_info)
{
	d_info.u_info.type = SHOWALLRECORD;

	if(send(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}

	while(1) {
		if(recv(cfd, &d_info, sizeof(d_info), 0) < 0) {
			ERR_MSG("recv");
			return -1;
		}
		if(d_info.u_info.type == DATASENDOVER) {
			break;
		}else {
			printf("**********************************\n");
			printf("**************RECORD**************\n");
			printf("**********************************\n");
			printf("%d  %s  %s\n", ntohl(d_info.r_info.record_id), d_info.r_info.name, d_info.r_info.time);

		}
	}
	return 0;
}

int do_show_all_employee_info(int cfd, DATA_INFO d_info)
{
	d_info.u_info.type = QUERYALL;
	if(send(cfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}

	printf("***************E_INFO**************\n");

	while(1) {
		if(recv(cfd, &d_info, sizeof(d_info), 0) < 0) {
			ERR_MSG("recv");
			return -1;
		}
		if(d_info.u_info.type == EINFO_NON_EXISTENT) {
			printf("%s\n", d_info.u_info.info_data);
			break;
		}
		if(d_info.u_info.type == DATASENDOVER) {
			break;
		}else {
			printf("%d %s %c %d %s %s %s %s %s %s %d\n", ntohl(d_info.e_info.id), d_info.e_info.name,
			d_info.e_info.sex, ntohl(d_info.e_info.age), d_info.e_info.department, d_info.e_info.post, d_info.e_info.lv, 
			d_info.e_info.phone, d_info.e_info.email, d_info.e_info.address, ntohl(d_info.e_info.salary));

		}
	}
	return 0;
}






