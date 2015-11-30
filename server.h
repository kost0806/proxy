#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#define true 1
#define BUF_SIZE 1024

void init(unsigned int port);		// 이니셜라이징 함수
int communicate(int client_fd);	// 클라이언트와 통신하는 함수
int check_file(char *path);			// 파일의 존재 유무와 접근 권한을 확인하는 함수
char *get_status(int code);			// check_file이 주는 아웃풋에 따른 헤더에 들어갈 스테이터스를 반환하는 함수
char *get_mime(char *format);		// 보내는 파일의 확장자에 따른 mime을 리턴하는 함수
void print_help(char *path);		// 도움말 페이지를 출력하는 함수
int header_len(char *status, char *connection, char *mime);	// 헤더의 길이를 계산하는 함수
char *create_header(char *header, char *status, char *connection, char *mime);	// 헤더를 완성하는 함수

static char *DOCUMENT_ROOT = "./www"; // 서버의 루트 디렉터리
static char *header = "HTTP/1.1 %s\r\n"	// 헤더파일의 포맷
"Connection: %s\r\n"
"Content-Type: %s\r\n"
"\r\n";

static char *NOT_FOUND = 	"<!DOCTYPE HTML>"	// 404에러시 보여줄 페이지
"<html>"
"	<head></head>"
"	<body>"
"		<h1>Not Found</h1>"
"		<p>The requested URL %s was not found on this server.</p>"
"	</body>"
"</html>";

static char *FORBIDDEN = 	"<!DOCTYPE HTML>"	// 403 에러시 보여줄 페이지
"<html>"
"	<head></head>"
"	<body>"
"		<h1>Forbidden</h1>"
"		<p>You don't have permission to access %s on this server.</p>"
"	</body>"
"</html>";


int count = 0;

void init(unsigned int port) {
	int soc_fd, client_fd;

	struct sockaddr_in server;
	struct sockaddr_in client;
	int listen_ret;
	int client_addr_size;

	soc_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (soc_fd == -1) {
		fprintf(stderr, "Socket create error\n");
		exit(3); // socket create error
	}

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(soc_fd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
		fprintf(stderr, "Socket bind fail\n");
		exit(4); // bind fail
	}

	if (listen(soc_fd, 5) == -1) {
		fprintf(stderr, "Socket listen fail\n");
		exit(5); // listening fail
	}

	while (true) {
		client_addr_size = sizeof(struct sockaddr);
		if ((client_fd = accept(soc_fd, (struct sockaddr*)&client, &client_addr_size)) == -1) {
			fprintf(stderr, "Socket accept error\n");
			exit(6); // accepting fail
		}

		communicate(client_fd);
		close(client_fd);
	}
}

int communicate(int client_fd) {	
	char op[5];
	char file_name[20];
	char file_full_name[100];
	FILE *file;
	unsigned long long file_size = 0;
	int code;
	char file_format[10];
	char *tmp;
	int i = 0,n;
	char buf[BUF_SIZE];

	char *file_buf;
	char *c_header;
	int c_header_len;
	char *response;
	printf("count: %d\n", count);
	n = read(client_fd, buf, BUF_SIZE);	// 클라이언트로부터 리퀘스트를 읽음
	printf("read: %d\n", n);
	buf[n] = '\0';
	printf("%s\n", buf);
	sscanf(buf, "%s %s", op, file_name);	// 받은 리퀘스트로부터 첫 두 단어를 분리함. 각 각 GET같은 요청과 파일 이름으로 분리
	tmp = (char *)malloc(sizeof(char) * 20);	// 파일의 확장자를 구하기 위한 임시 변수 사용후 free
	strcpy(tmp, file_name);

	while(tmp[i] != '.') {	// i가 .의 위치를 가르킬때까지 i값을 증가
		if (i == strlen(tmp)) {
			break;
		}
		i++;
	}

	if (i == strlen(tmp))	// i가 tmp의 길이와 같은 경우는 확장자가 없다고 판단.
		strcpy(file_format, "none");
	else	// 아닐경우는 .의 뒷부분부터 확장자를 저장하는 변수에 저장
		strcpy(file_format,tmp + i + 1);
	free(tmp);

	printf("file_type: %s\n", file_format);
	printf("file_name: %s\n", file_name);
	if (!strcmp(file_name, "/")) {	// 요청하는 파일을 명시하지 않았을 경우는 index.html을 요구하는것으로 판단
		printf("redirect to index.html\n");
		strcpy(file_name, "/index.html");
	}

	if (!strncmp(op, "GET", 3)) {	// GET일 경우 실행
		strcpy(file_full_name, DOCUMENT_ROOT);	// 오픈할 파일의 이름을 서버 루트에서 찾기 위한 과정
		strcat(file_full_name, file_name);
		code = check_file(file_full_name);	// 파일의 권한과 존재 유무를 판단
		printf("code: %d\n", code);
		switch(code) {
			case 200:	// 200 OK
				file = fopen(file_full_name, "rb");
				if (file == NULL) {
					fprintf(stderr, "file open error\n");
					return -1;
				}

				fseek(file, 0, SEEK_END);	// 파일의 크기를 구함
				file_size = ftell(file);
				fseek(file, 0, SEEK_SET);
				file_buf = (char *)malloc(sizeof(char) * (file_size + 1));
				memset(file_buf, 0, file_size);
				fread(file_buf, 1, file_size, file);
				break;
			case 403:	// 403 Forbidden // 403과 404의 경우는 미리 스태틱 변수로 정의해놓은 각 페이지를 전달
				file_size = strlen(FORBIDDEN) - 2 + strlen(file_name + 1);
				file_buf = (char *)malloc(sizeof(char) * (file_size));
				sprintf(file_buf, FORBIDDEN, file_name);
				break;
			case 404:	// 404 Not Found
				file_size = strlen(NOT_FOUND) - 2 + strlen(file_name);
				file_buf = (char *)malloc(sizeof(char) * (file_size + 1));
				sprintf(file_buf, NOT_FOUND, file_name);
				break;
		}
		c_header_len = header_len(get_status(code), "close", get_mime(file_format)); 	// 헤더의 길이를 계산
		c_header = (char *)malloc(sizeof(char) * (c_header_len + 1));									// 헤더의 길이를 바탕으로 헤더를 만들 변수 생성
		create_header(c_header, get_status(code), "close", get_mime(file_format));		// 헤더 생성
		response = (char *)malloc(sizeof(char) * (c_header_len + file_size + 1));			// 헤더와 데이터를 담을 변수
		memset(response, 0, c_header_len + file_size);
		strcpy(response, c_header);	// 헤더를 리스폰스에 복사
		memcpy(response+c_header_len, file_buf, file_size);	// 데이터를 헤더 다음에 복사
		free(c_header);
		free(file_buf);
		printf("write!\n");
		write(client_fd, response, c_header_len + file_size);
		free(response);
		printf("write done\n");
	}
}

int check_file(char *path) {
	int rd = access(path, R_OK);
	int ex = access(path, F_OK);

	if (!ex && !rd)
		return 200;
	else if (!ex && rd)
		return 403;
	else
		return 404;
}

char *get_status(int code) {
	switch(code) {
		case 200:
			return "200 OK";
		case 403:
			return "403 Forbidden";
		case 404:
			return "404 Not Found";
	}
}

char *get_mime(char *format) {
	if (!strcmp(format, "jpg"))
		return "image/jpeg";
	else if (!strcmp(format, "gif"))
		return "image/gif";
	else if (!strcmp(format, "html"))
		return "text/html";
	else if (!strcmp(format, "txt"))
		return "text/plain";
	else if (!strcmp(format, "mp3"))
		return "audio/mpeg";
	else if (!strcmp(format, "mpeg"))
		return "video/mpeg";
	else if (!strcmp(format, "avi"))
		return "video/x-msvideo";
	else if (!strcmp(format, "mp4"))
		return "video/mp4";
	else if (!strcmp(format, "wmv"))
		return "video/x-mv-wmv";
	else if (!strcmp(format, "pdf"))
		return "application/pdf";
	else
		return "text/html";
}

void print_help(char *path) {
	char *help = "Usage: %s [port]\n";
	printf(help, path);
}

int header_len(char *status, char *connection, char *mime) {
	int imcomplete_header_len = strlen(header);
	int complete_header_len = imcomplete_header_len - (2 + 2 + 2);
	complete_header_len += strlen(status) + strlen(connection) + strlen(mime);

	return complete_header_len;
}

char *create_header(char *c_header, char *status, char *connection, char *mime) {
	sprintf(c_header, header, status, connection, mime);

	return c_header;
}
