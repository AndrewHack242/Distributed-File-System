#include <stdio.h>
#include <stdlib.h>
#include <string.h>     /* for fgets */
#include <strings.h>    /* for bzero, bcopy */
#include <unistd.h>     /* for read, write */
#include <sys/socket.h> /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

extern int errno;
#define MAXBUF 500000 /* max I/O buffer size */

int getMod4(char *str);
char *fileEncrypt(char *filename, char *hash, long *size);
void getHash(char *pass, char *hash);
void getFileHash(char *filename, char *hash);
char *parseLists(char *list1, char *list2, char *list3, char *list4, bool do1, bool do2, bool do3, bool do4);
void dechunkifyAndDecrypt(char *filename, char *hash, char *fp1, char *fp2, char *fp3, char *fp4, int fp1s, int fp2s, int fp3s, int fp4s);

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <dfc.conf>\n", argv[0]);
        exit(0);
    }

    char line[300];
    char *ip1, *ip2, *ip3, *ip4;
    char port1[16], port2[16], port3[16], port4[16], *tempport;
    FILE *conf = fopen(argv[1], "r");

    fgets(line, 300, conf); //server 1

    strtok(line, " ");
    strtok(NULL, " ");
    ip1 = strtok(NULL, ":");
    tempport = strtok(NULL, " \r\n");
    strcpy(port1, tempport);

    //printf("%s:%s\n", ip1, port1);

    fgets(line, 300, conf); //server 2

    strtok(line, " ");
    strtok(NULL, " ");
    ip2 = strtok(NULL, ":");
    tempport = strtok(NULL, " \r\n");
    strcpy(port2, tempport);

    //printf("%s:%s\n", ip2, port2);

    fgets(line, 300, conf); //server 3

    strtok(line, " ");
    strtok(NULL, " ");
    ip3 = strtok(NULL, ":");
    tempport = strtok(NULL, " \r\n");
    strcpy(port3, tempport);

    //printf("%s:%s\n", ip3, port3);

    fgets(line, 300, conf); //server 4

    strtok(line, " ");
    strtok(NULL, " ");
    ip4 = strtok(NULL, ":");
    tempport = strtok(NULL, " \r\n");
    strcpy(port4, tempport);

    //printf("%s:%s\n", ip4, port4);
    char tempuser[300];
    char temppass[300];
    fgets(tempuser, 300, conf); //username
    fgets(temppass, 300, conf); //password
    strtok(tempuser, ":");
    char *username = strtok(NULL, " \r\n");
    if (username[0] == ' ') //remove possible space
    {
        username++;
    }

    strtok(temppass, ":");
    char *password = strtok(NULL, " \r\n");
    if (password[0] == ' ')
    {
        password++;
    }
    //printf("%s\n", username);
    //printf("%s\n", password);

    fclose(conf);

    int port = atoi(port1);

    struct sockaddr_in s1addr;
    memset(&s1addr, '\0', sizeof(s1addr)); //0 out the memory
    s1addr.sin_family = AF_INET;
    s1addr.sin_port = htons((unsigned short)port);
    inet_aton(ip1, (struct in_addr *)&s1addr.sin_addr.s_addr); //server 1

    port = atoi(port2);
    struct sockaddr_in s2addr;
    memset(&s2addr, '\0', sizeof(s2addr)); //0 out the memory
    s2addr.sin_family = AF_INET;
    s2addr.sin_port = htons((unsigned short)port);
    inet_aton(ip2, (struct in_addr *)&s2addr.sin_addr.s_addr); //server 2

    port = atoi(port3);
    struct sockaddr_in s3addr;
    memset(&s3addr, '\0', sizeof(s3addr)); //0 out the memory
    s3addr.sin_family = AF_INET;
    s3addr.sin_port = htons((unsigned short)port);
    inet_aton(ip3, (struct in_addr *)&s3addr.sin_addr.s_addr); //server 3

    port = atoi(port4);
    struct sockaddr_in s4addr;
    memset(&s4addr, '\0', sizeof(s4addr)); //0 out the memory
    s4addr.sin_family = AF_INET;
    s4addr.sin_port = htons((unsigned short)port);
    inet_aton(ip4, (struct in_addr *)&s4addr.sin_addr.s_addr); //server 4

    while (1)
    {
        char buf[150] = "";
        /* get a message from the user */
        bzero(buf, 150);
        printf("Please enter get [file_name],  put [file_name], list: \n");
        fgets(buf, 150, stdin);
        if (strlen(buf) == 0)
        {
            continue;
        }

        if (buf[0] == '\n')
        {
            continue;
        }
        char *cmd = strtok(buf, " \r\n");

        if (!strcmp(cmd, "put"))
        {
            //get filename
            char *filename = strtok(NULL, " \r\n");
            char hash[32];
            getHash(password, hash);
            long size = 0;

            //read in file and encrypt,break into pieces,
            char *contents = fileEncrypt(filename, hash, &size);
            if (strlen(contents) == 0)
            {
                continue;
            }
            //printf("%s\n", hash);
            //printf("%s\n", contents);

            size_t checkpoint1 = size / 4;
            size_t checkpoint2 = size / 2;
            size_t checkpoint3 = (size * 3) / 4;
            //add to stringstream with put .d >filename<!>filecontent< (filecontent len + filename len + 8)

            char *f1 = NULL, *f2 = NULL, *f3 = NULL, *f4 = NULL;
            size_t s1 = 0, s2 = 0, s3 = 0, s4 = 0;

            FILE *stream1 = open_memstream(&f1, &s1);
            FILE *stream2 = open_memstream(&f2, &s2);
            FILE *stream3 = open_memstream(&f3, &s3);
            FILE *stream4 = open_memstream(&f4, &s4);

            fwrite("1!", 1, 2, stream1);
            fwrite(filename, 1, strlen(filename), stream1);
            fwrite("!", 1, 1, stream1);
            fwrite(contents, 1, checkpoint1, stream1);
            contents += checkpoint1;

            fwrite("2!", 1, 2, stream2);
            fwrite(filename, 1, strlen(filename), stream2);
            fwrite("!", 1, 1, stream2);
            fwrite(contents, 1, checkpoint2 - checkpoint1, stream2);
            contents += checkpoint2 - checkpoint1;

            fwrite("3!", 1, 2, stream3);
            fwrite(filename, 1, strlen(filename), stream3);
            fwrite("!", 1, 1, stream3);
            fwrite(contents, 1, checkpoint3 - checkpoint2, stream3);
            contents += checkpoint3 - checkpoint2;

            fwrite("4!", 1, 2, stream4);
            fwrite(filename, 1, strlen(filename), stream4);
            fwrite("!", 1, 1, stream4);
            fwrite(contents, 1, size - checkpoint3, stream4);

            fclose(stream1);
            fclose(stream2);
            fclose(stream3);
            fclose(stream4);

            char *login;
            size_t loginsize;
            FILE *logins = open_memstream(&login, &loginsize);

            fwrite(username, 1, strlen(username), logins);
            fwrite(" ", 1, 1, logins);
            fwrite(password, 1, strlen(password), logins);

            fclose(logins);

            char *sizem12 = NULL, *sizem23 = NULL, *sizem34 = NULL, *sizem41 = NULL;
            size_t len12 = 0, len23 = 0, len34 = 0, len41 = 0;

            FILE *stream12 = open_memstream(&sizem12, &len12);
            FILE *stream23 = open_memstream(&sizem23, &len23);
            FILE *stream34 = open_memstream(&sizem34, &len34);
            FILE *stream41 = open_memstream(&sizem41, &len41);

            char temp[100];

            fwrite("put!", 1, 4, stream12);
            sprintf(temp, "%ld", s1);
            fwrite(temp, 1, strlen(temp), stream12);
            fwrite("!", 1, 1, stream12);
            sprintf(temp, "%ld", s2);
            fwrite(temp, 1, strlen(temp), stream12);
            fwrite("!", 1, 1, stream12);

            fwrite("put!", 1, 4, stream23);
            sprintf(temp, "%ld", s2);
            fwrite(temp, 1, strlen(temp), stream23);
            fwrite("!", 1, 1, stream23);
            sprintf(temp, "%ld", s3);
            fwrite(temp, 1, strlen(temp), stream23);
            fwrite("!", 1, 1, stream23);

            fwrite("put!", 1, 4, stream34);
            sprintf(temp, "%ld", s3);
            fwrite(temp, 1, strlen(temp), stream34);
            fwrite("!", 1, 1, stream34);
            sprintf(temp, "%ld", s4);
            fwrite(temp, 1, strlen(temp), stream34);
            fwrite("!", 1, 1, stream34);

            fwrite("put!", 1, 4, stream41);
            sprintf(temp, "%ld", s4);
            fwrite(temp, 1, strlen(temp), stream41);
            fwrite("!", 1, 1, stream41);
            sprintf(temp, "%ld", s1);
            fwrite(temp, 1, strlen(temp), stream41);
            fwrite("!", 1, 1, stream41);

            fclose(stream12);
            fclose(stream23);
            fclose(stream34);
            fclose(stream41);

            //connect and send them over, one send for each file

            int server1fd, server2fd, server3fd, server4fd;
            int s1conn, s2conn, s3conn, s4conn;

            if ((server1fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Error creating socket 1");
                return -1;
            }
            if ((server2fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Error creating socket 2");
                return -1;
            }
            if ((server3fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Error creating socket 3");
                return -1;
            }
            if ((server4fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Error creating socket 4");
                return -1;
            }

            bool down1 = false, down2 = false, down3 = false, down4 = false;

            if ((s1conn = connect(server1fd, (struct sockaddr *)&s1addr, sizeof(struct sockaddr))) < 0) // connect 1
            {
                if (errno == 111)
                {
                    printf("Server 1 is currently down.\n");
                    down1 = true;
                }
                else
                {
                    perror("CONNECT 1 BROKE");
                    return -1;
                }
            }

            if ((s2conn = connect(server2fd, (struct sockaddr *)&s2addr, sizeof(struct sockaddr))) < 0) // connect 2
            {
                if (errno == 111)
                {
                    printf("Server 2 is currently down.\n");
                    down2 = true;
                }
                else
                {
                    perror("CONNECT 2 BROKE");
                    return -1;
                }
            }

            if ((s3conn = connect(server3fd, (struct sockaddr *)&s3addr, sizeof(struct sockaddr))) < 0) // connect 3
            {
                if (errno == 111)
                {
                    printf("Server 3 is currently down.\n");
                    down3 = true;
                }
                else
                {
                    perror("CONNECT 3 BROKE");
                    return -1;
                }
            }

            if ((s4conn = connect(server4fd, (struct sockaddr *)&s4addr, sizeof(struct sockaddr))) < 0) // connect 4
            {
                if (errno == 111)
                {
                    printf("Server 4 is currently down.\n");
                    down4 = true;
                }
                else
                {
                    perror("CONNECT 4 BROKE");
                    return -1;
                }
            }

            if (down1 || down2 || down3 || down4)
            {
                close(server1fd);
                close(server2fd);
                close(server3fd);
                close(server4fd);
                printf("unable to reach at least one server. Unable to put a file. ABORTING\n");
                continue;
            }

            int n;
            //AUTHENTICATION
            n = send(server1fd, login, loginsize, 0);
            if (n < 0)
            {
                perror("auth 1");
            }
            n = send(server2fd, login, loginsize, 0);
            if (n < 0)
            {
                perror("auth 2");
            }
            n = send(server3fd, login, loginsize, 0);
            if (n < 0)
            {
                perror("auth 3");
            }
            n = send(server4fd, login, loginsize, 0);
            if (n < 0)
            {
                perror("auth 4");
            }

            char response;
            n = recv(server1fd, &response, 1, 0);
            if (n < 0)
            {
                perror("auth response 1");
            }
            if (response == 'b')
            {
                printf("authentication failed on server 1. ABORTING\n");
                continue;
            }
            n = recv(server2fd, &response, 1, 0);
            if (n < 0)
            {
                perror("auth response 2");
            }
            if (response == 'b')
            {
                printf("authentication failed on server 2. ABORTING\n");
                continue;
            }
            n = recv(server3fd, &response, 1, 0);
            if (n < 0)
            {
                perror("auth response 3");
            }
            if (response == 'b')
            {
                printf("authentication failed on server 3. ABORTING\n");
                continue;
            }
            n = recv(server4fd, &response, 1, 0);
            if (n < 0)
            {
                perror("auth response 4");
            }
            if (response == 'b')
            {
                printf("authentication failed on server 4. ABORTING\n");
                continue;
            }

            int oneandtwo, twoandthree, threeandfour, fourandone;
            char filehash[32];
            getFileHash(filename, filehash);
            int layout = getMod4(filehash);
            switch (layout)
            {
            case 0:
                oneandtwo = server1fd;
                twoandthree = server2fd;
                threeandfour = server3fd;
                fourandone = server4fd;
                break;
            case 1:
                oneandtwo = server2fd;
                twoandthree = server3fd;
                threeandfour = server4fd;
                fourandone = server1fd;
                break;
            case 2:
                oneandtwo = server3fd;
                twoandthree = server4fd;
                threeandfour = server1fd;
                fourandone = server2fd;
                break;
            case 3:
                oneandtwo = server4fd;
                twoandthree = server1fd;
                threeandfour = server2fd;
                fourandone = server3fd;
                break;
            }

            n = send(oneandtwo, sizem12, len12, 0);
            if (n < 0)
            {
                perror("1 size send");
            }

            n = send(oneandtwo, f1, s1, 0);
            if (n < 0)
            {
                perror("1 send");
            }
            n = send(oneandtwo, f2, s2, 0);
            if (n < 0)
            {
                perror("2 send");
            }

            n = send(twoandthree, sizem23, len23, 0);
            if (n < 0)
            {
                perror("2 size send");
            }

            n = send(twoandthree, f2, s2, 0);
            if (n < 0)
            {
                perror("3 send");
            }
            n = send(twoandthree, f3, s3, 0);
            if (n < 0)
            {
                perror("4 send");
            }

            n = send(threeandfour, sizem34, len34, 0);
            if (n < 0)
            {
                perror("3 size send");
            }

            n = send(threeandfour, f3, s3, 0);
            if (n < 0)
            {
                perror("5 send");
            }
            n = send(threeandfour, f4, s4, 0);
            if (n < 0)
            {
                perror("6 send");
            }

            n = send(fourandone, sizem41, len41, 0);
            if (n < 0)
            {
                perror("4 size send");
            }

            n = send(fourandone, f4, s4, 0);
            if (n < 0)
            {
                perror("7 send");
            }
            n = send(fourandone, f1, s1, 0);
            if (n < 0)
            {
                perror("8 send");
            }

            //recv confirmation or rejection of the file/login

            //close down the connections
            close(server1fd);
            close(server2fd);
            close(server3fd);
            close(server4fd);
        }
        else if (!strcmp(cmd, "get"))
        {
            //get filename
            char *filename = strtok(NULL, " \r\n");

            //connect and authenticate

            int server1fd, server2fd, server3fd, server4fd;
            int s1conn, s2conn, s3conn, s4conn;

            if ((server1fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Error creating socket 1");
                return -1;
            }
            if ((server2fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Error creating socket 2");
                return -1;
            }
            if ((server3fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Error creating socket 3");
                return -1;
            }
            if ((server4fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Error creating socket 4");
                return -1;
            }

            bool down1 = false, down2 = false, down3 = false, down4 = false;

            if ((s1conn = connect(server1fd, (struct sockaddr *)&s1addr, sizeof(struct sockaddr))) < 0) // connect 1
            {
                if (errno == 111)
                {
                    printf("Server 1 is currently down.\n");
                    down1 = true;
                }
                else
                {
                    perror("CONNECT 1 BROKE");
                    return -1;
                }
            }

            if ((s2conn = connect(server2fd, (struct sockaddr *)&s2addr, sizeof(struct sockaddr))) < 0) // connect 2
            {
                if (errno == 111)
                {
                    printf("Server 2 is currently down.\n");
                    down2 = true;
                }
                else
                {
                    perror("CONNECT 2 BROKE");
                    return -1;
                }
            }

            if ((s3conn = connect(server3fd, (struct sockaddr *)&s3addr, sizeof(struct sockaddr))) < 0) // connect 3
            {
                if (errno == 111)
                {
                    printf("Server 3 is currently down.\n");
                    down3 = true;
                }
                else
                {
                    perror("CONNECT 3 BROKE");
                    return -1;
                }
            }

            if ((s4conn = connect(server4fd, (struct sockaddr *)&s4addr, sizeof(struct sockaddr))) < 0) // connect 4
            {
                if (errno == 111)
                {
                    printf("Server 4 is currently down.\n");
                    down4 = true;
                }
                else
                {
                    perror("CONNECT 4 BROKE");
                    return -1;
                }
            }

            char *login;
            size_t loginsize;
            FILE *logins = open_memstream(&login, &loginsize);

            fwrite(username, 1, strlen(username), logins);
            fwrite(" ", 1, 1, logins);
            fwrite(password, 1, strlen(password), logins);

            fclose(logins);

            int n;
            //AUTHENTICATION
            if (!down1)
            {
                n = send(server1fd, login, loginsize, 0);
                if (n < 0)
                {
                    perror("auth 1");
                }
            }
            if (!down2)
            {
                n = send(server2fd, login, loginsize, 0);
                if (n < 0)
                {
                    perror("auth 2");
                }
            }
            if (!down3)
            {
                n = send(server3fd, login, loginsize, 0);
                if (n < 0)
                {
                    perror("auth 3");
                }
            }
            if (!down4)
            {
                n = send(server4fd, login, loginsize, 0);
                if (n < 0)
                {
                    perror("auth 4");
                }
            }

            char response;
            if (!down1)
            {
                n = recv(server1fd, &response, 1, 0);
                if (n < 0)
                {
                    perror("auth response 1");
                }
                if (response == 'b')
                {
                    printf("authentication failed on server 1. ABORTING\n");
                    continue;
                }
            }
            if (!down2)
            {
                n = recv(server2fd, &response, 1, 0);
                if (n < 0)
                {
                    perror("auth response 2");
                }
                if (response == 'b')
                {
                    printf("authentication failed on server 2. ABORTING\n");
                    continue;
                }
            }
            if (!down3)
            {
                n = recv(server3fd, &response, 1, 0);
                if (n < 0)
                {
                    perror("auth response 3");
                }
                if (response == 'b')
                {
                    printf("authentication failed on server 3. ABORTING\n");
                    continue;
                }
            }
            if (!down4)
            {
                n = recv(server4fd, &response, 1, 0);
                if (n < 0)
                {
                    perror("auth response 4");
                }
                if (response == 'b')
                {
                    printf("authentication failed on server 4. ABORTING\n");
                    continue;
                }
            }

            if (down1 && down2 && down3 && down4)
            {
                close(server1fd);
                close(server2fd);
                close(server3fd);
                close(server4fd);
                printf("All servers are offline. Unable to get file. ABORTING\n");
                continue;
            }

            //send requests
            char *command = NULL;
            size_t commandsize = 0;
            FILE *out = open_memstream(&command, &commandsize);
            fwrite("get!", 1, 4, out);
            fwrite(filename, 1, strlen(filename), out);
            fwrite("!", 1, 1, out);
            fclose(out);
            if (!down1)
            {
                n = send(server1fd, command, commandsize, 0);
                if (n < 0)
                {
                    perror("1 size send");
                }
            }
            if (!down2)
            {
                n = send(server2fd, command, commandsize, 0);
                if (n < 0)
                {
                    perror("2 size send");
                }
            }
            if (!down3)
            {
                n = send(server3fd, command, commandsize, 0);
                if (n < 0)
                {
                    perror("3 size send");
                }
            }
            if (!down4)
            {
                n = send(server4fd, command, commandsize, 0);
                if (n < 0)
                {
                    perror("4 size send");
                }
            }
            free(command);
            //recieve files or rejection
            char msize1[50], msize2[50], msize3[50], msize4[50];
            int size1 = 0, size2 = 0, size3 = 0, size4 = 0, size12 = 0, size22 = 0, size32 = 0, size42 = 0;
            bool none1 = false, none2 = false, none3 = false, none4 = false;
            if (!down1) //get size
            {
                n = recv(server1fd, msize1, 50, 0);
                if (n < 0)
                {
                    perror("size response 1");
                }
                else
                {
                    if (msize1[0] == '!')
                        none1 = true;
                    else
                    {
                        char *p1 = strtok(msize1, "!");
                        char *p2 = strtok(NULL, "");
                        size1 = atoi(p1);
                        size12 = atoi(p2);
                    }
                }
            }
            else
            {
                none1 = true;
            }

            if (!down2) //get size
            {
                n = recv(server2fd, msize2, 50, 0);
                if (n < 0)
                {
                    perror("size response 2");
                }
                else
                {
                    if (msize2[0] == '!')
                        none2 = true;
                    else
                    {
                        char *p1 = strtok(msize2, "!");
                        char *p2 = strtok(NULL, "");
                        size2 = atoi(p1);
                        size22 = atoi(p2);
                    }
                }
            }
            else
            {
                none2 = true;
            }

            if (!down3) //get size
            {
                n = recv(server3fd, msize3, 50, 0);
                if (n < 0)
                {
                    perror("size response 3");
                }
                else
                {
                    if (msize3[0] == '!')
                        none3 = true;
                    else
                    {
                        char *p1 = strtok(msize3, "!");
                        char *p2 = strtok(NULL, "");
                        size3 = atoi(p1);
                        size32 = atoi(p2);
                    }
                }
            }
            else
            {
                none3 = true;
            }

            if (!down4) //get size
            {
                n = recv(server4fd, msize4, 50, 0);
                if (n < 0)
                {
                    perror("size response 4");
                }
                else
                {
                    if (msize4[0] == '!')
                        none4 = true;
                    else
                    {
                        char *p1 = strtok(msize4, "!");
                        char *p2 = strtok(NULL, "");
                        size4 = atoi(p1);
                        size42 = atoi(p2);
                    }
                }
            }
            else
            {
                none4 = true;
            }

            if (none1 && none2 && none3 && none4)
            {
                printf("File could not be found: %s\n", filename);
                continue;
            }

            //get 2 files from the servers
            char *m1, *m12, *m2, *m22, *m3, *m32, *m4, *m42; //remember to free
            char *part1, *part12, *part2, *part22, *part3, *part32, *part4, *part42;

            char *message1;
            size_t m1size;
            char *message12;
            size_t m12size;
            if (!none1)
            {
                FILE *stream1 = open_memstream(&message1, &m1size);
                n = 0;
                char * m1 = malloc(size1);
                while (n < size1)
                {
                    int m = recv(server1fd, m1, size1 - n, 0);
                    fwrite(m1, 1, m, stream1);
                    n += m;
                    if (n < 0)
                    {
                        perror("size response 1");
                    }
                }
                fclose(stream1);

                FILE *stream12 = open_memstream(&message12, &m12size);
                char * m12 = malloc(size12);
                n = 0;
                while (n < size12)
                {
                    int m = recv(server1fd, m12, size12 - n, 0);
                    fwrite(m12, 1, m, stream12);
                    n += m;
                    if (n < 0)
                    {
                        perror("size response 12");
                    }
                }
                fclose(stream12);

                part1 = strtok(message1, "!");
                message1 += 2;
                part12 = strtok(message12, "!");
                message12 += 2;
                free(m1);
                free(m12);
            }

            char *message2;
            size_t m2size;
            char *message22;
            size_t m22size;
            if (!none2)
            {
                FILE *stream2 = open_memstream(&message2, &m2size);
                char * m2 = malloc(size2);
                n = 0;
                while (n < size2)
                {
                    int m = recv(server2fd, m2, size2 - n, 0);
                    fwrite(m2, 1, m, stream2);
                    n += m;
                    if (n < 0)
                    {
                        perror("size response 2");
                    }
                }
                fclose(stream2);

                FILE *stream22 = open_memstream(&message22, &m22size);
                char * m22 = malloc(size1);
                n = 0;
                while (n < size22)
                {
                    int m = recv(server2fd, m22, size22 - n, 0);
                    fwrite(m22, 1, m, stream22);
                    n += m;
                    if (n < 0)
                    {
                        perror("size response 22");
                    }
                }
                fclose(stream22);

                part2 = strtok(message2, "!");
                message2 += 2;
                part22 = strtok(message22, "!");
                message22 += 2;
                free(m2);
                free(m22);
            }

            char *message3;
            size_t m3size;
            char *message32;
            size_t m32size;
            if (!none3)
            {
                FILE *stream3 = open_memstream(&message3, &m3size);
                char * m3 = malloc(size1);
                n = 0;
                while (n < size3)
                {
                    int m = recv(server3fd, m3, size3 - n, 0);
                    fwrite(m3, 1, m, stream3);
                    n += m;
                    if (n < 0)
                    {
                        perror("size response 3");
                    }
                }
                fclose(stream3);

                FILE *stream32 = open_memstream(&message32, &m32size);
                char * m32 = malloc(size1);
                n = 0;
                while (n < size32)
                {
                    int m = recv(server3fd, m32, size32 - n, 0);
                    fwrite(m32, 1, m, stream32);
                    n += m;
                    if (n < 0)
                    {
                        perror("size response 32");
                    }
                }
                fclose(stream32);

                part3 = strtok(message3, "!");
                message3 += 2;
                part32 = strtok(message32, "!");
                message32 += 2;
                free(m3);
                free(m32);
            }

            char *message4;
            size_t m4size;
            char *message42;
            size_t m42size;
            if (!none4)
            {
                FILE *stream4 = open_memstream(&message4, &m4size);
                char * m4 = malloc(size1);
                n = 0;
                while (n < size4)
                {
                    int m = recv(server4fd, m4, size4 - n, 0);
                    fwrite(m4, 1, m, stream4);
                    n += m;
                    if (n < 0)
                    {
                        perror("size response 4");
                    }
                }
                fclose(stream4);

                FILE *stream42 = open_memstream(&message42, &m42size);
                char * m42 = malloc(size1);
                n = 0;
                while (n < size42)
                {
                    int m = recv(server4fd, m42, size42 - n, 0);
                    fwrite(m42, 1, m, stream42);
                    n += m;
                    if (n < 0)
                    {
                        perror("size response 42");
                    }
                }
                fclose(stream42);

                part4 = strtok(message4, "!");
                message4 += 2;
                part42 = strtok(message42, "!");
                message42 += 2;
                free(m4);
                free(m42);
            }

            //see if all the parts were accumulated

            char *fp1 = NULL, *fp2 = NULL, *fp3 = NULL, *fp4 = NULL;
            int fp1s = 0, fp2s = 0, fp3s = 0, fp4s = 0;

            if (!none1)
            {
                if (part1[0] == '1')
                {
                    fp1 = message1;
                    fp1s = size1 - 2;
                }
                else if (part1[0] == '2')
                {
                    fp2 = message1;
                    fp2s = size1 - 2;
                }
                else if (part1[0] == '3')
                {
                    fp3 = message1;
                    fp3s = size1 - 2;
                }
                else if (part1[0] == '4')
                {
                    fp4 = message1;
                    fp4s = size1 - 2;
                }
            }
            if ((fp1 == NULL) || (fp2 == NULL) || (fp3 == NULL) || (fp4 == NULL))
            {
                if (!none1)
                {
                    if (part12[0] == '1')
                    {
                        fp1 = message12;
                        fp1s = size12 - 2;
                    }
                    else if (part12[0] == '2')
                    {
                        fp2 = message12;
                        fp2s = size12 - 2;
                    }
                    else if (part12[0] == '3')
                    {
                        fp3 = message12;
                        fp3s = size12 - 2;
                    }
                    else if (part12[0] == '4')
                    {
                        fp4 = message12;
                        fp4s = size12 - 2;
                    }
                }
                if ((fp1 == NULL) || (fp2 == NULL) || (fp3 == NULL) || (fp4 == NULL))
                {
                    if (!none2)
                    {
                        if (part2[0] == '1')
                        {
                            fp1 = message2;
                            fp1s = size2 - 2;
                        }
                        else if (part2[0] == '2')
                        {
                            fp2 = message2;
                            fp2s = size2 - 2;
                        }
                        else if (part2[0] == '3')
                        {
                            fp3 = message2;
                            fp3s = size2 - 2;
                        }
                        else if (part2[0] == '4')
                        {
                            fp4 = message2;
                            fp4s = size2 - 2;
                        }
                    }
                    if ((fp1 == NULL) || (fp2 == NULL) || (fp3 == NULL) || (fp4 == NULL))
                    {
                        if (!none2)
                        {
                            if (part22[0] == '1')
                            {
                                fp1 = message22;
                                fp1s = size22 - 2;
                            }
                            else if (part22[0] == '2')
                            {
                                fp2 = message22;
                                fp2s = size22 - 2;
                            }
                            else if (part22[0] == '3')
                            {
                                fp3 = message22;
                                fp3s = size22 - 2;
                            }
                            else if (part22[0] == '4')
                            {
                                fp4 = message22;
                                fp4s = size22 - 2;
                            }
                        }
                        if ((fp1 == NULL) || (fp2 == NULL) || (fp3 == NULL) || (fp4 == NULL))
                        {
                            if (!none3)
                            {
                                if (part3[0] == '1')
                                {
                                    fp1 = message3;
                                    fp1s = size3 - 2;
                                }
                                else if (part3[0] == '2')
                                {
                                    fp2 = message3;
                                    fp2s = size3 - 2;
                                }
                                else if (part3[0] == '3')
                                {
                                    fp3 = message3;
                                    fp3s = size3 - 2;
                                }
                                else if (part3[0] == '4')
                                {
                                    fp4 = message3;
                                    fp4s = size3 - 2;
                                }
                            }
                            if ((fp1 == NULL) || (fp2 == NULL) || (fp3 == NULL) || (fp4 == NULL))
                            {
                                if (!none3)
                                {
                                    if (part32[0] == '1')
                                    {
                                        fp1 = message32;
                                        fp1s = size32 - 2;
                                    }
                                    else if (part32[0] == '2')
                                    {
                                        fp2 = message32;
                                        fp2s = size32 - 2;
                                    }
                                    else if (part32[0] == '3')
                                    {
                                        fp3 = message32;
                                        fp3s = size32 - 2;
                                    }
                                    else if (part32[0] == '4')
                                    {
                                        fp4 = message32;
                                        fp4s = size32 - 2;
                                    }
                                }
                                if ((fp1 == NULL) || (fp2 == NULL) || (fp3 == NULL) || (fp4 == NULL))
                                {
                                    if (!none4)
                                    {
                                        if (part4[0] == '1')
                                        {
                                            fp1 = message4;
                                            fp1s = size4 - 2;
                                        }
                                        else if (part4[0] == '2')
                                        {
                                            fp2 = message4;
                                            fp2s = size4 - 2;
                                        }
                                        else if (part4[0] == '3')
                                        {
                                            fp3 = message4;
                                            fp3s = size4 - 2;
                                        }
                                        else if (part4[0] == '4')
                                        {
                                            fp4 = message4;
                                            fp4s = size4 - 2;
                                        }
                                    }
                                    if ((fp1 == NULL) || (fp2 == NULL) || (fp3 == NULL) || (fp4 == NULL))
                                    {
                                        if (!none4)
                                        {
                                            if (part42[0] == '1')
                                            {
                                                fp1 = message42;
                                                fp1s = size42 - 2;
                                            }
                                            else if (part42[0] == '2')
                                            {
                                                fp2 = message42;
                                                fp2s = size42 - 2;
                                            }
                                            else if (part42[0] == '3')
                                            {
                                                fp3 = message42;
                                                fp3s = size42 - 2;
                                            }
                                            else if (part42[0] == '4')
                                            {
                                                fp4 = message42;
                                                fp4s = size42 - 2;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (!((fp1 == NULL) || (fp2 == NULL) || (fp3 == NULL) || (fp4 == NULL)))
            {
                //piece file together and decrypt
                char hash[32];
                getHash(password, hash);
                dechunkifyAndDecrypt(filename, hash, fp1, fp2, fp3, fp4, fp1s, fp2s, fp3s, fp4s);
                printf("File was retrieved successfully: %s\n", filename);
            }
            else
            {
                printf("File is incomplete, could not get all parts: %s\n", filename);
            }
        }
        else if (!strcmp(cmd, "list"))
        {
            //connect and authenticate
            int server1fd, server2fd, server3fd, server4fd;
            int s1conn, s2conn, s3conn, s4conn;

            if ((server1fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Error creating socket 1");
                return -1;
            }
            if ((server2fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Error creating socket 2");
                return -1;
            }
            if ((server3fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Error creating socket 3");
                return -1;
            }
            if ((server4fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Error creating socket 4");
                return -1;
            }

            bool down1 = false, down2 = false, down3 = false, down4 = false;

            if ((s1conn = connect(server1fd, (struct sockaddr *)&s1addr, sizeof(struct sockaddr))) < 0) // connect 1
            {
                if (errno == 111)
                {
                    printf("Server 1 is currently down.\n");
                    down1 = true;
                }
                else
                {
                    perror("CONNECT 1 BROKE");
                    return -1;
                }
            }

            if ((s2conn = connect(server2fd, (struct sockaddr *)&s2addr, sizeof(struct sockaddr))) < 0) // connect 2
            {
                if (errno == 111)
                {
                    printf("Server 2 is currently down.\n");
                    down2 = true;
                }
                else
                {
                    perror("CONNECT 2 BROKE");
                    return -1;
                }
            }

            if ((s3conn = connect(server3fd, (struct sockaddr *)&s3addr, sizeof(struct sockaddr))) < 0) // connect 3
            {
                if (errno == 111)
                {
                    printf("Server 3 is currently down.\n");
                    down3 = true;
                }
                else
                {
                    perror("CONNECT 3 BROKE");
                    return -1;
                }
            }

            if ((s4conn = connect(server4fd, (struct sockaddr *)&s4addr, sizeof(struct sockaddr))) < 0) // connect 4
            {
                if (errno == 111)
                {
                    printf("Server 4 is currently down.\n");
                    down4 = true;
                }
                else
                {
                    perror("CONNECT 4 BROKE");
                    return -1;
                }
            }

            if (down1 && down2 && down3 && down4)
            {
                close(server1fd);
                close(server2fd);
                close(server3fd);
                close(server4fd);
                printf("All servers are offline. Unable to list files. ABORTING\n");
                continue;
            }

            char *login;
            size_t loginsize;
            FILE *logins = open_memstream(&login, &loginsize);

            fwrite(username, 1, strlen(username), logins);
            fwrite(" ", 1, 1, logins);
            fwrite(password, 1, strlen(password), logins);

            fclose(logins);

            int n;
            //AUTHENTICATION
            if (!down1)
            {
                n = send(server1fd, login, loginsize, 0);
                if (n < 0)
                {
                    perror("auth 1");
                }
            }
            if (!down2)
            {
                n = send(server2fd, login, loginsize, 0);
                if (n < 0)
                {
                    perror("auth 2");
                }
            }
            if (!down3)
            {
                n = send(server3fd, login, loginsize, 0);
                if (n < 0)
                {
                    perror("auth 3");
                }
            }
            if (!down4)
            {
                n = send(server4fd, login, loginsize, 0);
                if (n < 0)
                {
                    perror("auth 4");
                }
            }

            char response;
            if (!down1)
            {
                n = recv(server1fd, &response, 1, 0);
                if (n < 0)
                {
                    perror("auth response 1");
                }
                if (response == 'b')
                {
                    printf("authentication failed on server 1. ABORTING\n");
                    continue;
                }
            }
            if (!down2)
            {
                n = recv(server2fd, &response, 1, 0);
                if (n < 0)
                {
                    perror("auth response 2");
                }
                if (response == 'b')
                {
                    printf("authentication failed on server 2. ABORTING\n");
                    continue;
                }
            }
            if (!down3)
            {
                n = recv(server3fd, &response, 1, 0);
                if (n < 0)
                {
                    perror("auth response 3");
                }
                if (response == 'b')
                {
                    printf("authentication failed on server 3. ABORTING\n");
                    continue;
                }
            }
            if (!down4)
            {
                n = recv(server4fd, &response, 1, 0);
                if (n < 0)
                {
                    perror("auth response 4");
                }
                if (response == 'b')
                {
                    printf("authentication failed on server 4. ABORTING\n");
                    continue;
                }
            }
            //send requests
            if (!down1)
            {
                n = send(server1fd, "list", 4, 0);
                if (n < 0)
                {
                    perror("auth 1");
                }
            }
            if (!down2)
            {
                n = send(server2fd, "list", 4, 0);
                if (n < 0)
                {
                    perror("auth 2");
                }
            }
            if (!down3)
            {
                n = send(server3fd, "list", 4, 0);
                if (n < 0)
                {
                    perror("auth 3");
                }
            }
            if (!down4)
            {
                n = send(server4fd, "list", 4, 0);
                if (n < 0)
                {
                    perror("auth 4");
                }
            }

            //recieve lists
            char list1[MAXBUF], list2[MAXBUF], list3[MAXBUF], list4[MAXBUF];
            if (!down1)
            {
                n = recv(server1fd, &list1, MAXBUF, 0);
                if (n < 0)
                {
                    perror("response 1");
                }
            }
            if (!down2)
            {
                n = recv(server2fd, &list2, MAXBUF, 0);
                if (n < 0)
                {
                    perror("response 2");
                }
            }
            if (!down3)
            {
                n = recv(server3fd, &list3, MAXBUF, 0);
                if (n < 0)
                {
                    perror("response 3");
                }
            }
            if (!down4)
            {
                n = recv(server4fd, &list4, MAXBUF, 0);
                if (n < 0)
                {
                    perror("response 4");
                }
            }

            //printf("%s\n",list1);

            char *finallist = parseLists(list1, list2, list3, list4, !down1, !down2, !down3, !down4);

            printf("%s\n", finallist);

            //parse lists for each file (use map to determine which parts are had? <filename, 1010>?)

            //output results, incomplete if not 1111
        }
    }
}

int getMod4(char *str)
{
    int len = strlen(str);
    char t = str[len - 1];
    if (t == '0')
    {
        return 0;
    }
    if (t == '1')
    {
        return 1;
    }
    if (t == '2')
    {
        return 2;
    }
    if (t == '3')
    {
        return 3;
    }
    if (t == '4')
    {
        return 0;
    }
    if (t == '5')
    {
        return 1;
    }
    if (t == '6')
    {
        return 2;
    }
    if (t == '7')
    {
        return 3;
    }
    if (t == '8')
    {
        return 0;
    }
    if (t == '9')
    {
        return 1;
    }
    if (t == 'a')
    {
        return 2;
    }
    if (t == 'b')
    {
        return 3;
    }
    if (t == 'c')
    {
        return 0;
    }
    if (t == 'd')
    {
        return 1;
    }
    if (t == 'e')
    {
        return 2;
    }
    if (t == 'f')
    {
        return 3;
    }
    return -1;
}

char *fileEncrypt(char *filename, char *hash, long *size) //size uses pass by reference
{
    char *buffer = NULL;
    char buf[32];
    FILE *file;
    if ((file = fopen(filename, "r")) == NULL)
    {
        printf("File does not exist: %s\n", filename);
        return "";
    }
    size_t bufferSize = 0;
    FILE *output = open_memstream(&buffer, &bufferSize);

    int n;
    while ((n = fread(buf, 1, 32, file)) > 0)
    {
        if (n > 32)
        {
            printf("n > 32\n");
        }
        for (int i = 0; i < n; ++i)
        {
            buf[i] ^= hash[i];
        }
        int m = fwrite(buf, 1, n, output);
        if (m > 32)
        {
            printf("m > 32\n");
        }
        if (n != m)
        {
            printf("n != m\n");
        }
    }
    fclose(file);
    fclose(output);
    *size = bufferSize;

    return buffer;
}

void getHash(char *pass, char *hash)
{
    bzero(hash, 32);
    char cmd[150];
    bzero(cmd, 150);
    strcat(cmd, "echo -n ");
    strcat(cmd, pass);
    strcat(cmd, " | md5sum");
    FILE *fp;
    if ((fp = popen(cmd, "r")) == NULL) // "echo -n Welcome | md5sum"
    {
        printf("Error opening pipe!\n");
        return;
    }
    if (fgets(hash, 32, fp) != NULL)
    {
        //printf("OUTPUT: %s\n", hash);
    }
    else
    {
        printf("HASH FAILED\n");
        return;
    }
    pclose(fp);
    return;
}

void getFileHash(char *filename, char *hash)
{
    bzero(hash, 32);
    char cmd[150];
    bzero(cmd, 150);
    strcat(cmd, "md5sum ");
    strcat(cmd, filename);
    FILE *fp;
    if ((fp = popen(cmd, "r")) == NULL)
    {
        printf("Error opening pipe!\n");
        return;
    }
    if (fgets(hash, 32, fp) != NULL)
    {
        //printf("OUTPUT: %s\n", hash);
    }
    else
    {
        printf("HASH FAILED\n");
        return;
    }
    pclose(fp);
}

struct Map
{
    char filename[100];
    bool part1;
    bool part2;
    bool part3;
    bool part4;
};

void initMap(struct Map *map)
{
    map->part1 = false;
    map->part2 = false;
    map->part3 = false;
    map->part4 = false;
}

char *parseLists(char *list1, char *list2, char *list3, char *list4, bool do1, bool do2, bool do3, bool do4)
{
    int nextpos = 0;
    int arrsize = 8;
    struct Map *map = malloc(arrsize * sizeof(struct Map));
    for (int i = 0; i < arrsize; ++i)
    {
        initMap(&map[i]);
    }

    //printf("1: %d\n", map[0].part1);
    for (int c = 0; c < 4; ++c)
    {
        if (c == 0 && !do1)
            continue;
        if (c == 1 && !do2)
            continue;
        if (c == 2 && !do3)
            continue;
        if (c == 3 && !do4)
            continue;
        char *part;
        char filename[100];
        char num[2];
        if (c == 0)
            part = strtok(list1, "\n");
        if (c == 1)
            part = strtok(list2, "\n");
        if (c == 2)
            part = strtok(list3, "\n");
        if (c == 3)
            part = strtok(list4, "\n");
        bzero(num, 2);
        bzero(filename, 100);
        memcpy(filename, &part[1], strlen(part) - 3);
        //printf("2: %s\n", filename);
        memcpy(num, &part[strlen(part) - 1], 1);
        //printf("3: %s\n", num);
        bool found = false;
        for (int i = 0; i < nextpos; ++i)
        {
            if (!strcmp(map[i].filename, filename))
            {
                found = true;
                switch (num[0])
                {
                case '1':
                    map[i].part1 = true;
                    break;
                case '2':
                    map[i].part2 = true;
                    break;
                case '3':
                    map[i].part3 = true;
                    break;
                case '4':
                    map[i].part4 = true;
                    break;
                }
            }
        }

        if (!found)
        {
            if (nextpos >= arrsize)
            {
                arrsize *= 2;
                struct Map *temp = malloc(arrsize * sizeof(struct Map));
                for (int j = 0; j < nextpos; ++j)
                {
                    temp[j] = map[j];
                }
                free(map);
                map = temp;
            }
            initMap(&map[nextpos]);
            memcpy(map[nextpos].filename, filename, 100);
            switch (num[0])
            {
            case '1':
                map[nextpos].part1 = true;
                break;
            case '2':
                map[nextpos].part2 = true;
                break;
            case '3':
                map[nextpos].part3 = true;
                break;
            case '4':
                map[nextpos].part4 = true;
                break;
            }
            nextpos++;
        }

        while ((part = strtok(NULL, "\n")) != NULL)
        {
            bzero(num, 2);
            bzero(filename, 100);
            memcpy(filename, &part[1], strlen(part) - 3);
            //printf("2: %s\n", filename);
            memcpy(num, &part[strlen(part) - 1], 1);
            //printf("3: %s\n", num);
            bool found = false;
            for (int i = 0; i < nextpos; ++i)
            {
                if (!strcmp(map[i].filename, filename))
                {
                    found = true;
                    switch (num[0])
                    {
                    case '1':
                        map[i].part1 = true;
                        break;
                    case '2':
                        map[i].part2 = true;
                        break;
                    case '3':
                        map[i].part3 = true;
                        break;
                    case '4':
                        map[i].part4 = true;
                        break;
                    }
                }
            }

            if (!found)
            {
                if (nextpos >= arrsize)
                {
                    arrsize *= 2;
                    struct Map *temp = malloc(arrsize * sizeof(struct Map));
                    for (int j = 0; j < nextpos; ++j)
                    {
                        temp[j] = map[j];
                    }
                    free(map);
                    map = temp;
                }
                initMap(&map[nextpos]);
                memcpy(map[nextpos].filename, filename, 100);
                switch (num[0])
                {
                case '1':
                    map[nextpos].part1 = true;
                    break;
                case '2':
                    map[nextpos].part2 = true;
                    break;
                case '3':
                    map[nextpos].part3 = true;
                    break;
                case '4':
                    map[nextpos].part4 = true;
                    break;
                }
                nextpos++;
            }
        }
    }

    char *result = NULL;
    size_t resultsize = 0;

    FILE *r = open_memstream(&result, &resultsize);
    fwrite("\n", 1, strlen("\n"), r);
    for (int i = 0; i < nextpos; ++i)
    {
        if (map[i].part1 && map[i].part2 && map[i].part3 && map[i].part4)
        {
            fwrite(map[i].filename, 1, strlen(map[i].filename), r);
            fwrite("\n", 1, strlen("\n"), r);
        }
        else
        {
            fwrite(map[i].filename, 1, strlen(map[i].filename), r);
            fwrite(" [incomplete]\n", 1, strlen(" [incomplete]\n"), r);
        }
    }
    fclose(r);

    return result;
}

void dechunkifyAndDecrypt(char *filename, char *hash, char *fp1, char *fp2, char *fp3, char *fp4, int fp1s, int fp2s, int fp3s, int fp4s)
{

    FILE *file1 = fmemopen(fp1, fp1s, "r");

    size_t len = 0;

    char buf[32];
    FILE *output = fopen(filename, "w");

    char *buffer = NULL;
    size_t bufferSize = 0;
    FILE *stream = open_memstream(&buffer, &bufferSize);

    int n;
    while ((n = fread(buf, 1, 32, file1)) > 0)
    {
        if (n > 32)
        {
            printf("n > 32\n");
        }

        int m = fwrite(buf, 1, n, stream);
        len += m;
        if (m > 32)
        {
            printf("m > 32\n");
        }
        if (n != m)
        {
            printf("n != m\n");
        }
    }
    fclose(file1);

    FILE *file2 = fmemopen(fp2, fp2s, "r");

    while ((n = fread(buf, 1, 32, file2)) > 0)
    {
        if (n > 32)
        {
            printf("n > 32\n");
        }

        int m = fwrite(buf, 1, n, stream);
        len += m;
        if (m > 32)
        {
            printf("m > 32\n");
        }
        if (n != m)
        {
            printf("n != m\n");
        }
    }
    fclose(file2);

    FILE *file3 = fmemopen(fp3, fp3s, "r");

    while ((n = fread(buf, 1, 32, file3)) > 0)
    {
        if (n > 32)
        {
            printf("n > 32\n");
        }
        int m = fwrite(buf, 1, n, stream);
        len += m;
        if (m > 32)
        {
            printf("m > 32\n");
        }
        if (n != m)
        {
            printf("n != m\n");
        }
    }
    fclose(file3);

    FILE *file4 = fmemopen(fp4, fp4s, "r");

    while ((n = fread(buf, 1, 32, file4)) > 0)
    {
        if (n > 32)
        {
            printf("n > 32\n");
        }
        int m = fwrite(buf, 1, n, stream);
        len += m;
        if (m > 32)
        {
            printf("m > 32\n");
        }
        if (n != m)
        {
            printf("n != m\n");
        }
    }
    fclose(file4);
    fclose(stream);

    for (int i = 0; i < len; ++i)
    {
        buffer[i] ^= hash[i % 32];
    }
    fwrite(buffer, 1, len, output);

    fclose(output);
    free(buffer);
}

/*
parselists crashes if no files are on the servers

*/