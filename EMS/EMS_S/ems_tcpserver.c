#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <sqlite3.h>
#include <time.h>
#include <sys/stat.h>


#define IP "192.168.1.99"
#define PORT 8888
#define MAX_USERNAME_LEN 64
#define MAX_USERPWD_LEN 64
#define MAX_INFO_DATE_LEN 512
#define MAX_NAME_LEN 64
#define MAX_EINFO_LEN 64
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
#define DATASENDOVER '9'
#define MAX_THAN_LIMIT_WRONG_TIMES 'm'
#define ADDEINFO 'A'
#define DELETE_EINFO 'D'
#define EINFO_NON_EXISTENT 'N'

typedef struct {
	unsigned int record_id;
	char name[MAX_NAME_LEN];
	char time[MAX_TIME_INFO_LEN];
}R_INFO;

typedef struct {
	int                acceptfd;
	struct sockaddr_in cin;
	sqlite3*           db;
}TM;

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
	R_INFO r_info;
	U_INFO u_info;
	EMPLOYEE_INFO e_info;
	int  flag;
}DATA_INFO;

void* do_communication(void* arg);
int do_register(int acceptfd, DATA_INFO d_info, sqlite3* db);
int do_sign_in(int acceptfd, DATA_INFO d_info, sqlite3* db);
int do_cancel(int acceptfd, DATA_INFO d_info, char* Logoutname, sqlite3* db);
int do_query(int acceptfd, DATA_INFO d_info, char* Loginname, sqlite3* db);
int do_query_userself(int acceptfd, DATA_INFO d_info, sqlite3* db);
int do_modify_userself_info(int acceptfd, DATA_INFO d_info, sqlite3* db);
int do_employee_record(int acceptfd, DATA_INFO d_info, sqlite3* db);
int do_show_eself_record(int acceptfd, DATA_INFO d_info, sqlite3* db);
int do_add_employee_info(int acceptfd, DATA_INFO d_info, sqlite3* db);
int do_modifiy_employee_info(int acceptfd, DATA_INFO d_info, sqlite3* db);
int do_delete_employee_info(int acceptfd, DATA_INFO d_info, sqlite3* db);
int do_show_all_record(int acceptfd, DATA_INFO d_info, sqlite3* db);
int do_show_all_employee_info(int acceptfd, DATA_INFO d_info, sqlite3* db);
int do_init_user(sqlite3* db);



int main(int argc, const char* argv[]) {

	//open info_database
	sqlite3* db = NULL;

	if(sqlite3_open("./EMC_MT.db", &db) != 0) {
		printf("sqlite3_open failed\n");
		printf("sqlite3_open:%s\n", sqlite3_errmsg(db));
		return -1;
	}
	printf("Open EMC_MT info_database Success\n");
	//create three tables
	//table employee_info
 
	char  sql[256] = "";
	char* errmsg;
	sprintf(sql, "create table if not exists employee_info(id int primary key, name char,\
	 sex char, age int, department char, post char, lv char, phone char, email char, address char, salary int)");
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != 0) {
		printf("sqlite3_exec:__%d__ %s\n", __LINE__, errmsg);
		return -1;
	}
	bzero(sql, sizeof(sql));
    printf("Create/Load employee_info Success\n");

	//table user_msg

	sprintf(sql, "create table if not exists user_info(name char primary key, pword char, state char, authority char, bind_id int)");
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != 0) {
		printf("sqlite3_exec:__%d__ %s\n", __LINE__, errmsg);
		return -1;
	}
	bzero(sql, sizeof(sql));
    printf("Create/Load user_info Success\n");

	//table attendance_record

	sprintf(sql, "create table if not exists attendance_record(id int, name char, time char)");
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != 0) {
		printf("sqlite3_exec:__%d__ %s\n", __LINE__, errmsg);
		return -1;
	}
	bzero(sql, sizeof(sql));
    printf("Create/Load attendance_record Success\n");

	do_init_user(db);


	//create socket stream

	int emc_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(emc_fd < 0) {
		ERR_MSG("socket");
		return -1;
	}

	//allow user to reuse port quikly
	int value = 1;
	if(setsockopt(emc_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0) {
		ERR_MSG("setsockopt");
		return -1;
	}

	//bind server's IP and PORT
	struct sockaddr_in sin;
	sin.sin_family      = AF_INET;
	sin.sin_port        = htons(PORT);
	sin.sin_addr.s_addr = inet_addr(IP);

	if(bind(emc_fd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
		ERR_MSG("bind");
		return -1;
	}
	printf("bind success\n");

	//listen
	if(listen(emc_fd, 20) < 0) {
     	ERR_MSG("listen");
    	return -1;
	}
	printf("listen success\n");

	//get new file descriptor
	struct sockaddr_in cin;
	int                acceptfd;
	socklen_t          addrlen = sizeof(cin);
	
	while(1) {
		acceptfd = accept(emc_fd, (struct sockaddr*)&cin, &addrlen);
    	if(acceptfd < 0) {
        	ERR_MSG("accept");
	    	return -1;
    	}
    	printf("acceptd = %d\n", acceptfd);
    	printf("[%s:%d]connect success\n", inet_ntoa(cin.sin_addr), ntohs(cin.sin_port));
	    
	//create thread

		TM info;
		info.acceptfd = acceptfd;
		info.cin      = cin;
		info.db       = db;


		pthread_t tid;
		if(pthread_create(&tid, NULL, do_communication, (void*)&info) != 0) {
			ERR_MSG("pthread_create");
			return -1;
		} 
	}
	close(emc_fd);
	sqlite3_close(db);
	return 0;
}

void* do_communication(void* arg) 
{
	pthread_detach(pthread_self());
	TM                 info     = *(TM*)arg;
	int                acceptfd = info.acceptfd;
	int                ret      = 0;
	int                wptc      = 0;
	sqlite3*           db       = info.db;
	DATA_INFO          d_info;
	char               tmp_authority;
	char               Logoutname[64];
	char               Loginname[64];
	memset(&d_info, 0, sizeof(d_info));
	while(recv(acceptfd, &d_info, sizeof(d_info), 0) > 0) {
		switch(d_info.u_info.type) {
		case LOGIN:

			if(wptc == 3) {
				d_info.u_info.type = MAX_THAN_LIMIT_WRONG_TIMES;
				strcpy(d_info.u_info.info_data, "maxtimes than wrongtime limited, can not login right now, try again later");
				if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
					ERR_MSG("send");
					return NULL;
				}
				break;
			}
			ret = do_sign_in(acceptfd, d_info, db);
			if(ret == 0) {
				strcat(Logoutname, d_info.u_info.name);
				strcat(Loginname, d_info.u_info.name);
				tmp_authority = d_info.u_info.authority;
				printf("%c %d\n", tmp_authority, __LINE__);
			}
			if(ret == -2) {
				wptc++;
			}
			break;

		case REGISTER:

			do_register(acceptfd, d_info, db);
			break;

		case EXIT:

			wptc = 0;
			printf("connect is out!\n");
			close(acceptfd);
			pthread_exit(NULL);

		case CANCEL:

			do_cancel(acceptfd, d_info, Logoutname, db);
			bzero(Logoutname, sizeof(Logoutname));
			bzero(Loginname, sizeof(Loginname));
			wptc = 0;
			break;

		case RECORD:

			printf("name = %s pword = %s state = %c authority = %c bind_id = %d\n",
			d_info.u_info.name, d_info.u_info.pword, d_info.u_info.state,
			 d_info.u_info.authority, d_info.u_info.bind_id);
			 
			do_employee_record(acceptfd, d_info, db);
			break;

		case QUERYUSERSEL:

			printf("name = %s pword = %s state = %c authority = %c bind_id = %d\n",
			d_info.u_info.name, d_info.u_info.pword, d_info.u_info.state,
			 d_info.u_info.authority, d_info.u_info.bind_id);

			do_query_userself(acceptfd, d_info, db);
			break;
			
		case MODIFYUSERSELF:

			do_modify_userself_info(acceptfd, d_info, db);
			break;

		case MODIFYALL:
		
			do_modifiy_employee_info(acceptfd, d_info, db);
			break;

		case SHOWSELFRECORD:
			do_show_eself_record(acceptfd, d_info, db);
			break;

		case ADDEINFO:
			do_add_employee_info(acceptfd, d_info, db);
			break;

		case DELETE_EINFO:
			do_delete_employee_info(acceptfd, d_info, db);
			break;

		case SHOWALLRECORD:
			do_show_all_record(acceptfd, d_info, db);
			break;

		case QUERYALL:
			do_show_all_employee_info(acceptfd, d_info, db);
			break;

		default:
			break;
		}
	}

	close(acceptfd);
	pthread_exit(NULL);
}

int do_register(int acceptfd, DATA_INFO d_info, sqlite3* db) 
{

	char   sql[256] = "";
	char** result0   = NULL;
	char** result1   = NULL;
	char** result2   = NULL;
	int    row0, column0;
	int    row1, column1;
	int    row2, column2;
	char*  errmsg   = NULL;

	bzero(d_info.u_info.info_data, sizeof(d_info.u_info.info_data));

	printf("name = %s pword = %s state = %c authority = %c bind_id = %d %d\n",
	d_info.u_info.name, d_info.u_info.pword, d_info.u_info.state, d_info.u_info.authority, ntohl(d_info.u_info.bind_id), __LINE__);
	
	sprintf(sql, "select * from user_info where name = '%s'", d_info.u_info.name);
	if(sqlite3_get_table(db, sql, &result0, &row0, &column0, &errmsg) !=0) {
		printf("sqlite3_get_table:__%d__ %s\n", __LINE__, errmsg);
		return -1;
	}
	if(row0 == 1) {
		strcpy(d_info.u_info.info_data, "USER ALREADY EXIST!");
		if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
		}
		sqlite3_free_table(result0); 
		return 0;
	}
	if(row0 < 1) {
		bzero(sql,sizeof(sql));
		sprintf(sql, "select * from user_info where bind_id = %d", ntohl(d_info.u_info.bind_id));

		if(sqlite3_get_table(db, sql, &result1, &row1, &column1, &errmsg) !=0) {
			printf("sqlite3_get_table:__%d__ %s\n", __LINE__, errmsg);
			return -1;
		}
		printf("%d %d\n", row1, __LINE__);
		if(row1 == 1) {
			strcpy(d_info.u_info.info_data, "EMPLOYEE ID HAS BEEN BINDED");
			/*
			bzero(sql,sizeof(sql));
			sprintf(sql, "delete from user_info where name = '%s'", u_info.name);
			if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != 0) {
				printf("sqlite3_exec:__%d__ %s\n", __LINE__, errmsg);
			}
			printf("DELETE DONE\n");
			*/
		}
		if(row1 < 1) {
			bzero(sql,sizeof(sql));
			sprintf(sql, "select * from employee_info where id = %d", ntohl(d_info.u_info.bind_id));

			if(sqlite3_get_table(db, sql, &result2, &row2, &column2, &errmsg) !=0) {
				printf("sqlite3_get_table:__%d__ %s\n", __LINE__, errmsg);
				return -1;
			}
			if(row2 == 1) {
				/*
				bzero(sql,sizeof(sql));
				sprintf(sql,"update user_info set bind_id = %d where name = '%s'",ntohl(u_info.bind_id), result2[9]);
        		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	        	printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		        return -1;
          		}
				*/
				bzero(sql,sizeof(sql));
				sprintf(sql, "insert into user_info values('%s', '%s', '%c', '%c', %d)",
				d_info.u_info.name, d_info.u_info.pword, d_info.u_info.state, d_info.u_info.authority, ntohl(d_info.u_info.bind_id));

				if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != 0) {
					printf("sqlite3_exec:__%d__ %s\n", __LINE__, errmsg);
					return -1;
				}
			}
			if(row2 < 1) {
				strcpy(d_info.u_info.info_data, "EMPLOYEE ID DOSE NOT EXIST");
				if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
					ERR_MSG("send");
					return -1;
				}
				sqlite3_free_table(result2); 
				return 0;
				/*
				bzero(sql,sizeof(sql));
				sprintf(sql, "delete from user_info where name = '%s'", u_info.name);
				if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != 0) {
				printf("sqlite3_exec:__%d__ %s\n", __LINE__, errmsg);
				}
				printf("DELETE DONE1\n");
				*/
			}
			printf("REGISTER SUCCESS!\n");
			strcpy(d_info.u_info.info_data, "OK!");
			sqlite3_free_table(result2);
		}
		sqlite3_free_table(result1);
	}
	sqlite3_free_table(result0);
	if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	} 
	return 0;
}

int do_sign_in(int acceptfd, DATA_INFO d_info, sqlite3* db) 
{
	printf("name = %s pword = %s state = %c authority = %c bind_id = %d\n",
	d_info.u_info.name, d_info.u_info.pword, d_info.u_info.state, d_info.u_info.authority, ntohl(d_info.u_info.bind_id));
	//search user's signin info_data
	char   sql[256] = "";
 	char** result   = NULL;
	 char** result1   = NULL;
	int    row, column;
	int    row1, column1;
	char*  errmsg   = NULL;


	sprintf(sql, "select * from user_info where name = '%s'", d_info.u_info.name);

	if(sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) !=0) {
		printf("sqlite3_get_table:__%d__ %s\n", __LINE__, errmsg);
		return -1;
	}
	if(row < 1) {
		d_info.u_info.type = USERUNEXIST;
		strcpy(d_info.u_info.info_data, "User does not exist");
		if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
			ERR_MSG("send");
			return -1;
		}
		sqlite3_free_table(result);
		return -1;
	}

	sqlite3_free_table(result);
	bzero(sql,sizeof(sql));
	sprintf(sql, "select * from user_info where name = '%s' and pword = '%s'", d_info.u_info.name, d_info.u_info.pword);

	if(sqlite3_get_table(db, sql, &result1, &row1, &column1, &errmsg) !=0) {
		printf("sqlite3_get_table:__%d__ %s\n", __LINE__, errmsg);
		return -1;
	}
    

	if(row1 < 1) {
		d_info.u_info.type = WPASSWD;
		strcpy(d_info.u_info.info_data, "Wrong password");
		if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
			ERR_MSG("send");
			return -1;
		}
		return -2;
	}

	if(row1 == 1) {
		d_info.u_info.type = CPASSWD;
		if(OFFLINE == *result1[7]) {
        	strcpy(d_info.u_info.info_data, "LOGIN SUCCESS");
			d_info.u_info.authority = *result1[8];
			d_info.u_info.bind_id = htonl(atoi(result1[9]));
            if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
	        	ERR_MSG("send");
	        	return -1;
			}
            sprintf(sql,"update user_info set state = 'o' where name = '%s'", result1[5]);
        	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	        	printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		        return -1;
          	}
        }else {
			d_info.u_info.type = USERISLOGIN;
			strcpy(d_info.u_info.info_data, "The account has been logged in, please do not log in twice!");
            if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
	        	ERR_MSG("send");
	        	return -1;
			}
			sqlite3_free_table(result1);
			return -1;
		}
	}

	sqlite3_free_table(result1);
	return 0;
}

int do_cancel(int acceptfd, DATA_INFO d_info, char* Logoutname, sqlite3* db) 
{

	char   sql[256] = "";
	char*  errmsg   = NULL;

    sprintf(sql,"update user_info set state = 'f' where name = '%s'", Logoutname);
	printf("Logoutname = %s\n", Logoutname);

	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
		printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		d_info.u_info.type = LOGOUTFAIL;
    	strcpy(d_info.u_info.info_data, "Logout Failed!");
        if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) { 
	    	ERR_MSG("send");
	    	return -1;
    	}
		return -1;
	}

	d_info.u_info.type = LOGOUTSCUS;
	strcpy(d_info.u_info.info_data, "Logout Succeeded!");
    if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) { 
		ERR_MSG("send");
		return -1;
	}
	return 0;
}

int do_query_userself(int acceptfd, DATA_INFO d_info, sqlite3* db)
{
	printf("name = %s pword = %s state = %c authority = %c bind_id = %d %d\n",
	d_info.u_info.name, d_info.u_info.pword, d_info.u_info.state, d_info.u_info.authority, d_info.u_info.bind_id, __LINE__);

	char   sql[256] = "";
 	char** result   = NULL;
	int    row, column;
	char*  errmsg   = NULL;



	sprintf(sql, "select * from employee_info where id = %d", d_info.u_info.bind_id);

	if(sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) !=0) {
		printf("sqlite3_get_table:__%d__ %s\n", __LINE__, errmsg);
		return -1;
	}
	printf("%d\n", d_info.e_info.id = atoi(result[11]));
	strcpy(d_info.e_info.name, result[12]);
	printf("%s\n", d_info.e_info.name);
	d_info.e_info.sex = *result[13];
	printf("%c\n", d_info.e_info.sex);
	d_info.e_info.age = atoi(result[14]);
	printf("%d\n", d_info.e_info.id);
	strcpy(d_info.e_info.department, result[15]);
	printf("%s\n", d_info.e_info.department);
	strcpy(d_info.e_info.post, result[16]);
	printf("%s\n", d_info.e_info.post);
	strcpy(d_info.e_info.lv, result[17]);
	printf("%s\n", d_info.e_info.lv);
	strcpy(d_info.e_info.phone, result[18]);
	printf("%s\n", d_info.e_info.phone);
	strcpy(d_info.e_info.email, result[19]);
	printf("%s\n", d_info.e_info.email);
	strcpy(d_info.e_info.address, result[20]);
	printf("%s\n", d_info.e_info.address);
	d_info.e_info.salary = atoi(result[21]);
	printf("%d\n", d_info.e_info.salary);

	sqlite3_free_table(result);

	if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
			ERR_MSG("send");
			return -1;
	}

	return 0;
}

int do_modify_userself_info(int acceptfd, DATA_INFO d_info, sqlite3* db)
{	
	char   sql[512] = "";
 	char** result   = NULL;
	int    row, column;
	char*  errmsg   = NULL;

	sprintf(sql, "select * from employee_info where id = %d", d_info.u_info.bind_id);

	if(sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) !=0) {
		printf("sqlite3_get_table:__%d__ %s\n", __LINE__, errmsg);
		return -1;
	}
	printf("%d\n", d_info.e_info.id = atoi(result[11]));
	strcpy(d_info.e_info.name, result[12]);
	printf("%s\n", d_info.e_info.name);
	d_info.e_info.sex = *result[13];
	printf("%c\n", d_info.e_info.sex);
	d_info.e_info.age = atoi(result[14]);
	printf("%d\n", d_info.e_info.id);
	strcpy(d_info.e_info.department, result[15]);
	printf("%s\n", d_info.e_info.department);
	strcpy(d_info.e_info.post, result[16]);
	printf("%s\n", d_info.e_info.post);
	strcpy(d_info.e_info.lv, result[17]);
	printf("%s\n", d_info.e_info.lv);
	strcpy(d_info.e_info.phone, result[18]);
	printf("%s\n", d_info.e_info.phone);
	strcpy(d_info.e_info.email, result[19]);
	printf("%s\n", d_info.e_info.email);
	strcpy(d_info.e_info.address, result[20]);
	printf("%s\n", d_info.e_info.address);
	d_info.e_info.salary = atoi(result[21]);
	printf("%d\n", d_info.e_info.salary);

	sqlite3_free_table(result);

	if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}

	if(recv(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("recv");
		return -1;
	}

	printf("ID      : %d %d\n", d_info.e_info.id, __LINE__);
	printf("NAME    : %s %d\n", d_info.e_info.name, __LINE__);
	printf("SEX     : %c %d\n", d_info.e_info.sex, __LINE__);
	printf("AGE     : %d %d\n", d_info.e_info.age, __LINE__);
	printf("DEPART  : %s %d\n", d_info.e_info.department, __LINE__);
	printf("POST    : %s %d\n", d_info.e_info.post, __LINE__);
	printf("LV      : %s %d\n", d_info.e_info.lv, __LINE__);
	printf("PHONE   : %s %d\n", d_info.e_info.phone, __LINE__);
	printf("EMAIL   : %s %d\n", d_info.e_info.email, __LINE__);
	printf("ADDRESS : %s %d\n", d_info.e_info.address, __LINE__);
	printf("SALARY  : %d %d\n", d_info.e_info.salary, __LINE__);

	bzero(sql,sizeof(sql));
	/*
	sprintf(sql,"update employee_info set name = '%s' and sex = '%s' and age = %d and\
	 phone = '%s' and email = '%s' and address = '%s' where id = '%d'", 
	 d_info.e_info.name, d_info.e_info.sex, d_info.e_info.age, d_info.e_info.phone,
	  d_info.e_info.email, d_info.e_info.address, d_info.e_info.id);
	*/
	bzero(sql,sizeof(sql));
	sprintf(sql,"update employee_info set name = '%s' where id = %d", d_info.e_info.name, d_info.e_info.id);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	    printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		return -1;
    }

	bzero(sql,sizeof(sql));
	sprintf(sql,"update employee_info set sex = '%c' where id = %d", d_info.e_info.sex, d_info.e_info.id);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	    printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		return -1;
    }

	bzero(sql,sizeof(sql));
	sprintf(sql,"update employee_info set age = %d where id = %d", d_info.e_info.age, d_info.e_info.id);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	    printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		return -1;
    }

	bzero(sql,sizeof(sql));
	sprintf(sql,"update employee_info set phone = '%s' where id = %d", d_info.e_info.phone, d_info.e_info.id);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	    printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		return -1;
    }

	bzero(sql,sizeof(sql));
	sprintf(sql,"update employee_info set email = '%s' where id = %d", d_info.e_info.email, d_info.e_info.id);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	    printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		return -1;
    }

	bzero(sql,sizeof(sql));
	sprintf(sql,"update employee_info set address = '%s' where id = %d", d_info.e_info.address, d_info.e_info.id);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	    printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		return -1;
    }

	strcpy(d_info.u_info.info_data, "MODIFY SUCCESS!");

	if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}

	return 0;
}

int do_employee_record(int acceptfd, DATA_INFO d_info, sqlite3* db)
{	
	time_t t;
	time(&t);
	char   sql[512] = "";
	char** result   = NULL;
	int    row, column;
	char*  errmsg   = NULL;

	sprintf(sql, "select name from employee_info where id = %d", d_info.u_info.bind_id);

	if(sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) !=0) {
		printf("sqlite3_get_table:__%d__ %s\n", __LINE__, errmsg);
		return -1;
	}

	strcpy(d_info.e_info.name, result[1]);

	printf("%s\n", d_info.e_info.name);

	sqlite3_free_table(result);

	bzero(sql,sizeof(sql));
	sprintf(sql, "insert into attendance_record values(%d, '%s', '%s')", d_info.u_info.bind_id, d_info.e_info.name, ctime(&t));

	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != 0) {
		printf("sqlite3_exec:__%d__ %s\n", __LINE__, errmsg);
		return -1;
	}

	strcpy(d_info.u_info.info_data, "RECORD SUCCESS!");

	if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}

	return 0;
}

int do_show_eself_record(int acceptfd, DATA_INFO d_info, sqlite3* db)
{
	int i;
	char   sql[512] = "";
	char** result   = NULL;
	int    row, column;
	char*  errmsg   = NULL;

	sprintf(sql, "select * from attendance_record where id = %d", d_info.u_info.bind_id);

	if(sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) !=0) {
		printf("sqlite3_get_table:__%d__ %s\n", __LINE__, errmsg);
		return -1;
	}
	for(i=1;i<=row;i++) {
		bzero(d_info.r_info.name, sizeof(d_info.r_info.name));
		bzero(d_info.r_info.time, sizeof(d_info.r_info.time));
		d_info.r_info.record_id = htonl(atoi(result[i*column]));
		strcpy(d_info.r_info.name, result[(i*column) + 1]);
		strcpy(d_info.r_info.name, result[(i*column) + 2]);
		if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
			ERR_MSG("send");
			return -1;
		}
	}
	sqlite3_free_table(result);

	d_info.u_info.type = DATASENDOVER;
	if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}
	return 0;
}

int do_add_employee_info(int acceptfd, DATA_INFO d_info, sqlite3* db)
{	
	char   sql[1024] = "";
	char** result   = NULL;
	int    row, column;
	char*  errmsg   = NULL;

	bzero(d_info.u_info.info_data, sizeof(d_info.u_info.info_data));

	sprintf(sql, "select name from employee_info where id = %d", ntohl(d_info.e_info.id));
	if(sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) !=0) {
		printf("sqlite3_get_table:__%d__ %s\n", __LINE__, errmsg);
		return -1;
	}
	if(row == 1) {
		strcpy(d_info.u_info.info_data, "EID ALREADY USED!");
		if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
			ERR_MSG("send");
			return -1;
		}
	}else {

		bzero(sql,sizeof(sql));
		sprintf(sql, "insert into employee_info values(%d, '%s', '%c', %d, '%s', '%s', '%s', '%s', '%s', '%s', %d)",
		d_info.e_info.id, d_info.e_info.name, d_info.e_info.sex, d_info.e_info.age, 
		d_info.e_info.department, d_info.e_info.post, d_info.e_info.lv, d_info.e_info.phone, d_info.e_info.email, 
		d_info.e_info.address, d_info.e_info.salary);

		if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != 0) {
			printf("sqlite3_exec:__%d__ %s\n", __LINE__, errmsg);
			return -1;
		}

		strcpy(d_info.u_info.info_data, "ADD EMPLOYEE SUCCESS!");
		if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
			ERR_MSG("send");
			return -1;
		}
	}
	sqlite3_free_table(result);
	return 0;
}

int do_modifiy_employee_info(int acceptfd, DATA_INFO d_info, sqlite3* db)
{
	char   sql[512] = "";
 	char** result   = NULL;
	int    row, column;
	char*  errmsg   = NULL;

	printf("%d %d\n", d_info.e_info.id, __LINE__);

	sprintf(sql, "select * from employee_info where id = %d", d_info.e_info.id);

	if(sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) !=0) {
		printf("sqlite3_get_table:__%d__ %s\n", __LINE__, errmsg);
		return -1;
	}
	printf("%d\n", d_info.e_info.id = atoi(result[11]));
	strcpy(d_info.e_info.name, result[12]);
	printf("%s\n", d_info.e_info.name);
	d_info.e_info.sex = *result[13];
	printf("%c\n", d_info.e_info.sex);
	d_info.e_info.age = atoi(result[14]);
	printf("%d\n", d_info.e_info.id);
	strcpy(d_info.e_info.department, result[15]);
	printf("%s\n", d_info.e_info.department);
	strcpy(d_info.e_info.post, result[16]);
	printf("%s\n", d_info.e_info.post);
	strcpy(d_info.e_info.lv, result[17]);
	printf("%s\n", d_info.e_info.lv);
	strcpy(d_info.e_info.phone, result[18]);
	printf("%s\n", d_info.e_info.phone);
	strcpy(d_info.e_info.email, result[19]);
	printf("%s\n", d_info.e_info.email);
	strcpy(d_info.e_info.address, result[20]);
	printf("%s\n", d_info.e_info.address);
	d_info.e_info.salary = atoi(result[21]);
	printf("%d\n", d_info.e_info.salary);

	sqlite3_free_table(result);

	if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}

	if(recv(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("recv");
		return -1;
	}

	printf("ID      : %d %d\n", d_info.e_info.id, __LINE__);
	printf("NAME    : %s %d\n", d_info.e_info.name, __LINE__);
	printf("SEX     : %c %d\n", d_info.e_info.sex, __LINE__);
	printf("AGE     : %d %d\n", d_info.e_info.age, __LINE__);
	printf("DEPART  : %s %d\n", d_info.e_info.department, __LINE__);
	printf("POST    : %s %d\n", d_info.e_info.post, __LINE__);
	printf("LV      : %s %d\n", d_info.e_info.lv, __LINE__);
	printf("PHONE   : %s %d\n", d_info.e_info.phone, __LINE__);
	printf("EMAIL   : %s %d\n", d_info.e_info.email, __LINE__);
	printf("ADDRESS : %s %d\n", d_info.e_info.address, __LINE__);
	printf("SALARY  : %d %d\n", d_info.e_info.salary, __LINE__);

	bzero(sql,sizeof(sql));
	/*
	sprintf(sql,"update employee_info set name = '%s' and sex = '%s' and age = %d and\
	 phone = '%s' and email = '%s' and address = '%s' where id = '%d'", 
	 d_info.e_info.name, d_info.e_info.sex, d_info.e_info.age, d_info.e_info.phone,
	  d_info.e_info.email, d_info.e_info.address, d_info.e_info.id);
	*/
	bzero(sql,sizeof(sql));
	sprintf(sql,"update employee_info set name = '%s' where id = %d", d_info.e_info.name, d_info.e_info.id);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	    printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		return -1;
    }

	bzero(sql,sizeof(sql));
	sprintf(sql,"update employee_info set sex = '%c' where id = %d", d_info.e_info.sex, d_info.e_info.id);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	    printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		return -1;
    }

	bzero(sql,sizeof(sql));
	sprintf(sql,"update employee_info set age = %d where id = %d", d_info.e_info.age, d_info.e_info.id);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	    printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		return -1;
    }

	bzero(sql,sizeof(sql));
	sprintf(sql,"update employee_info set department = '%s' where id = %d", d_info.e_info.department, d_info.e_info.id);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	    printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		return -1;
    }

	bzero(sql,sizeof(sql));
	sprintf(sql,"update employee_info set post = '%s' where id = %d", d_info.e_info.post, d_info.e_info.id);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	    printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		return -1;
    }

	bzero(sql,sizeof(sql));
	sprintf(sql,"update employee_info set lv = '%s' where id = %d", d_info.e_info.lv, d_info.e_info.id);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	    printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		return -1;
    }

	bzero(sql,sizeof(sql));
	sprintf(sql,"update employee_info set phone = '%s' where id = %d", d_info.e_info.phone, d_info.e_info.id);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	    printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		return -1;
    }

	bzero(sql,sizeof(sql));
	sprintf(sql,"update employee_info set email = '%s' where id = %d", d_info.e_info.email, d_info.e_info.id);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	    printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		return -1;
    }

	bzero(sql,sizeof(sql));
	sprintf(sql,"update employee_info set address = '%s' where id = %d", d_info.e_info.address, d_info.e_info.id);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	    printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		return -1;
    }

	bzero(sql,sizeof(sql));
	sprintf(sql,"update employee_info set salary = %d where id = %d", d_info.e_info.salary, d_info.e_info.id);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0){
	    printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		return -1;
    }

	strcpy(d_info.u_info.info_data, "MODIFY SUCCESS!");

	if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}
	return 0;
}

int do_delete_employee_info(int acceptfd, DATA_INFO d_info, sqlite3* db)
{
	char   sql[256] = "";
	char** result   = NULL;
	int    row, column;
	char*  errmsg   = NULL;

	bzero(d_info.u_info.info_data, sizeof(d_info.u_info.info_data));

	printf("%d %d\n", d_info.e_info.id, __LINE__);

	sprintf(sql, "select name from employee_info where id = %d", d_info.e_info.id);
	if(sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) !=0) {
		printf("sqlite3_get_table:__%d__ %s\n", __LINE__, errmsg);
		return -1;
	}
	if(row < 1) {
		strcpy(d_info.u_info.info_data, "THIS EMPLOYEE DOSE NOT EXIST!");
		if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
			ERR_MSG("send");
			return -1;
		}
	}else {
		bzero(sql,sizeof(sql));
		sprintf(sql, "delete from employee_info where id = %d", d_info.e_info.id);
	
		if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != 0) {
			printf("sqlite3_exec:__%d__ %s\n", __LINE__, errmsg);
			return -1;
		}
		strcpy(d_info.u_info.info_data, "DELETE SUCCESS!");
		if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
			ERR_MSG("send");
			return -1;
		}
	}
	return 0;
}

int do_show_all_record(int acceptfd, DATA_INFO d_info, sqlite3* db)
{
	int i;
	char   sql[512] = "";
	char** result   = NULL;
	int    row, column;
	char*  errmsg   = NULL;

	sprintf(sql, "select * from attendance_record ");

	if(sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) !=0) {
		printf("sqlite3_get_table:__%d__ %s\n", __LINE__, errmsg);
		return -1;
	}
	for(i=1;i<=row;i++) {
		bzero(d_info.r_info.name, sizeof(d_info.r_info.name));
		bzero(d_info.r_info.time, sizeof(d_info.r_info.time));
		d_info.r_info.record_id = htonl(atoi(result[i*column]));
		strcpy(d_info.r_info.name, result[(i*column) + 1]);
		strcpy(d_info.r_info.name, result[(i*column) + 2]);
		if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
			ERR_MSG("send");
			return -1;
		}
	}
	sqlite3_free_table(result);

	d_info.u_info.type = DATASENDOVER;
	if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}
	return 0;
}

int do_show_all_employee_info(int acceptfd, DATA_INFO d_info, sqlite3* db)
{
	int i;
	char   sql[512] = "";
	char** result   = NULL;
	int    row, column;
	char*  errmsg   = NULL;

	sprintf(sql, "select * from employee_info");

	if(sqlite3_get_table(db, sql, &result, &row, &column, &errmsg) !=0) {
		printf("sqlite3_get_table:__%d__ %s\n", __LINE__, errmsg);
		return -1;
	}

	if(row < 1) {
		d_info.u_info.type = EINFO_NON_EXISTENT;
		strcpy(d_info.u_info.info_data, "THERE IS NO E_INFO!");
		if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
			ERR_MSG("send");
			return -1;
		}
		sqlite3_free_table(result);
		return 0;
	}

	for(i=1;i<=row;i++) {

		strcpy(d_info.r_info.name, result[(i*column) + 1]);
		strcpy(d_info.r_info.name, result[(i*column) + 2]);

		bzero(d_info.e_info.name, sizeof(d_info.e_info.name));
		bzero(d_info.e_info.department, sizeof(d_info.e_info.department));
		bzero(d_info.e_info.post, sizeof(d_info.e_info.post));
		bzero(d_info.e_info.lv, sizeof(d_info.e_info.lv));
		bzero(d_info.e_info.phone, sizeof(d_info.e_info.phone));
		bzero(d_info.e_info.email, sizeof(d_info.e_info.email));
		bzero(d_info.e_info.address, sizeof(d_info.e_info.address));

		d_info.e_info.id = htonl(atoi(result[i*column]));
		strcpy(d_info.e_info.name, result[(i*column) + 1]);
		d_info.e_info.sex = *result[(i*column) + 2];
		d_info.e_info.age = htonl(atoi(result[(i*column) + 3]));
		strcpy(d_info.e_info.department, result[(i*column) + 4]);
		strcpy(d_info.e_info.post, result[(i*column) + 5]);
		strcpy(d_info.e_info.lv, result[(i*column) + 6]);
		strcpy(d_info.e_info.phone, result[(i*column) + 7]);
		strcpy(d_info.e_info.email, result[(i*column) + 8]);
		strcpy(d_info.e_info.address, result[(i*column) + 9]);
		d_info.e_info.salary = htonl(atoi(result[(i*column) + 10]));


		if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
			ERR_MSG("send");
			return -1;
		}
	}
	sqlite3_free_table(result);

	d_info.u_info.type = DATASENDOVER;
	if(send(acceptfd, &d_info, sizeof(d_info), 0) < 0) {
		ERR_MSG("send");
		return -1;
	}

	return 0;
}

int do_init_user(sqlite3* db) 
{

	char *errmsg = NULL;
	char sql[256] = "";

    sprintf(sql,"update user_info set state = 'f'");

	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != 0) {
		printf("sqlite3_exec:__%d__%s\n",__LINE__,errmsg);
		return -1;
	}
	return 0;
}
