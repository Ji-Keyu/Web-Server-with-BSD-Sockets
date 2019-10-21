//https://www.tutorialspoint.com/unix_sockets/socket_server_example.htm
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>

void process(int sockfd)
{
	/* If connection is established then start communicating */
	int n;
	char buf[1024];
	memset(buf, 0, 1024);

	n = read(sockfd, buf, 1024);
	if (n < 0)
	{
		perror("ERROR reading from socket");
		exit(1);
	}
	write(1, buf, 1024);

	int type = -1;
	int version = -1;

	/* Get file name */
	char fileName[256];
	memset(fileName, 0, 256);
	int i = 0;
	int j = 0;
	for (i = 5; i < 256; i++) 
	{
		if (buf[i] == ' ') break;  /* end of the path */
		if (strncmp(&buf[i], "%20",3) == 0) {  /* convert the space in the file name */
			fileName[j++] = ' ';
			i += 2;
		}
		else
			fileName[j++] = tolower(buf[i]);  /* case-insensitive */
	}
	fileName[j] = '\0';
	if (strlen(fileName) == 0) {
        write(sockfd, "HTTP/1.1 200 OK\n", 16);
        write(sockfd, "Content-Length: 77\n", 19);
        write(sockfd, "Content-Type: text/html\n\n", 25);
        write(sockfd, "<html><head><title>Default Page</title></head><body>Hello World</body></html>", 77);
        close(sockfd);
        return;
    }

	/* Get file type */
	int hasType = 0;
	int k = 0;
	char fileType[64];
	memset(fileType, 0, 64);
	for (n = 0; n < j; n++) {
		if (fileName[n] == '.') {
			hasType = 1;
			continue;
		}
		if (hasType == 1)
			fileType[k++] = tolower(fileName[n]);
	}
	if (strncmp(&fileType[0], "html", 4) == 0)
		type = 0;
	else if (strncmp(&fileType[0], "htm", 3) == 0)
		type = 0;
	else if (strncmp(&fileType[0], "txt", 3) == 0)
		type = 1;
	else if (strncmp(&fileType[0], "jpeg", 4) == 0)
		type = 2;
	else if (strncmp(&fileType[0], "jpg", 3) == 0)
		type = 2;
	else if (strncmp(&fileType[0], "png", 3) == 0)
		type = 3;
	else if (strncmp(&fileType[0], "gif", 3) == 0)
		type = 4;


	/* Search the requested file */
	DIR *Dir;
    struct dirent *DirEnt;
    Dir = opendir(".");  /* any suitable directory name  */
    FILE* fd = 0;
    while (DirEnt = readdir (Dir))
    {
        char* dirFile;
        //dirFile = strdup(DirEnt->d_name);
       /*  compare file name in directory entry with your name  */
        if (strncasecmp(DirEnt->d_name, &fileName[0], strlen(fileName)) == 0) {
            fd = fopen(DirEnt->d_name, "r");
            if (!fd)  /* can't find the requested file */
            {
                write(sockfd, "HTTP/1.1 404 Not Found\n", 23);
                write(sockfd, "Content-Length: 77\n", 19);
                write(sockfd, "Content-Type: text/html\n\n", 25);
                write(sockfd, "<html><head><title>Error Page</title></head><body>404 Not Found</body></html>", 77);
                close(sockfd);
                return;
            }
        }
    }

    if (!fd)  /* can't find the requested file */
    {
        write(sockfd, "HTTP/1.1 404 Not Found\n", 23);
        write(sockfd, "Content-Length: 77\n", 19);
        write(sockfd, "Content-Type: text/html\n\n", 25);
        write(sockfd, "<html><head><title>Error Page</title></head><body>404 Not Found</body></html>", 77);
        close(sockfd);
        return;
    }

	/* Content length */
	struct stat fileInfo;
	stat(fileName, &fileInfo);
	int fileLength = fileInfo.st_size;


	/* Write a response to the client */
	/* Message strings */
	char ver_msg[32];
	memset(ver_msg, 0, 32);
	sprintf(ver_msg, "HTTP/1.1 200 OK\n");
	char len_msg[32];
	memset(len_msg, 0, 32);
	sprintf(len_msg, "Content-Length: %d\n", fileLength);
	time_t date;
	time(&date);
	struct tm* tm_info;
	tm_info = gmtime(&date);
	char time_msg[128];
	memset(time_msg, 0, 128);
	sprintf(time_msg, "Date: %s", asctime(tm_info));

	write(sockfd, ver_msg, strlen(ver_msg));
	write(sockfd, time_msg, strlen(time_msg));
	write(sockfd, len_msg, strlen(len_msg));
	write(sockfd, "Content-Type: ", 14);
	switch(type) {
		case 0: {
			write(sockfd, "text/html\n", 10);
			break;
		}
		case 1: {
			write(sockfd, "text/plain\n", 11);
			break;
		}
		case 2: {
			write(sockfd, "image/jpeg\n", 11);
			break;
		}
		case 3: {
			write(sockfd, "image/png\n", 10);
			break;
		}
		case 4: {
			write(sockfd, "image/gif\n", 10);
			break;
		}
		default: {
			write(sockfd, "text/plain\n", 11);
			break;
		}
	}
	write(sockfd, "Accept-Ranges: bytes\n", 21);
	write(sockfd, "Connection: close\n\n", 19);
	
	
	/* Send file contents */
	int input_file = fileno(fd);
	while (1) // use to send file
	{
		// Read data into buffer.  We may not have enough to fill up buffer, so we
		// store how many bytes were actually read in bytes_read.
		int bytes_read = read(input_file, buf, sizeof(buf));
		if (bytes_read == 0) // We're done reading from the file
			break;

		if (bytes_read < 0)
		{
			perror("ERROR on read");
			close(input_file);
			exit(1);
		}

		// You need a loop for the write, because not all of the data may be written
		// in one call; write will return how many bytes were written. p keeps
		// track of where in the buffer we are, while we decrement bytes_read
		// to keep track of how many bytes are left to write.
		void *p = buf;
		while (bytes_read > 0)
		{
			int bytes_written = write(sockfd, p, bytes_read);
			if (bytes_written <= 0)
			{
				perror("ERROR on write");
				close(input_file);
				exit(1);
			}
			bytes_read -= bytes_written;
			p += bytes_written;
		}
	}
	close(sockfd);
	if (input_file)
		close(input_file);

}

int stoi(char* str) {
    char* endptr = NULL;
    int ret = (int) strtol(str, &endptr, 10);
    if (endptr == str || ret < 0)
        return -1;
    return ret;
}

int main(int argc, char* argv[])
{

	int sockfd, newsockfd, portno, clilen;
	struct sockaddr_in serv_addr, cli_addr;

	struct pollfd fds;
	fds.fd = STDIN_FILENO;
	fds.events = POLLIN;
	/* First call to socket() function */
	sockfd = socket(AF_INET, SOCK_STREAM, 0); //PF_UNIX for Unix socket?
	if (sockfd < 0)
	{
		perror("ERROR opening socket");
		exit(1);
	}

	/* Get port number */
	if (argc > 1)
		portno = stoi(argv[1]);
	if (portno < 0)
	{
		perror("ERROR invalid port number");
		exit(1);
	}

	/* Initialize socket structure */
	bzero((char *)&serv_addr, sizeof(serv_addr));
	//portno = 2174;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY; //inet_addr("127.0.0.1"); //bind the socket to all available interfaces?
	serv_addr.sin_port = htons(portno);

	/* Now bind the host address using bind() call.*/
	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("ERROR on binding");
		exit(1);
	}

	/* Now start listening for the clients, here process will
      * go in sleep mode and will wait for the incoming connection
   */
	listen(sockfd, 5); //maximum number of conncetions allowed
	clilen = sizeof(cli_addr);

	while (1)
	{
		/* Accept actual connection from the client */
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
		if (newsockfd < 0)
		{
			perror("ERROR on accept");
			exit(1);
		}
		int pid = fork();
		if (pid < 0)
		{
			perror("ERROR on fork");
			exit(1);
		}
		if (pid == 0)
		{ // child
			close(sockfd);
			process(newsockfd);
			exit(0);
		}
		else
		{ // parent
			close(newsockfd);
		}

	}
	close(sockfd);
	return 0;
}
