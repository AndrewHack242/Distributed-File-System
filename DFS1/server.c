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
#include <dirent.h>
#include <errno.h>

extern int errno;

#define MAXlINE 8192  /* max text line length */
#define MAXBUF 500000 /* max I/O buffer size */
#define LISTENQ 1024  /* second argument to listen() */

//struct for getting data out of libcurl
struct data
{
    char d[MAXBUF];
    char url[100];
    size_t size;
    size_t s;
};

int open_listenfd(int port); 
void handleRecieved(int connfd);
void *thread(void *vargp);
size_t manageResponse(void *ptr, size_t size, size_t nmemb, struct data *userdata);
bool checkUser(char *username, char *password);

int main(int argc, char **argv)
{
    int listenfd, *connfdp, port;
    unsigned int clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);

    listenfd = open_listenfd(port);
    printf("Server setup complete.\n");
    while (1)
    { //listen constantly and create threads to handle activities while listening
        connfdp = malloc(sizeof(int));
        *connfdp = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        pthread_create(&tid, NULL, thread, connfdp);
    }
}

/* thread routine */
void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self());
    free(vargp);
    handleRecieved(connfd);
    close(connfd);
    return NULL;
}

//login, command and file sizes, file 1, file 2
void handleRecieved(int connfd)
{
    size_t n;
    char buf[MAXBUF];
    char buf2[MAXBUF];
    n = recv(connfd, buf, MAXBUF, 0); //n = chars read//recieve username/password
    if (n < 0)
    {
        perror("recv 1 went and done broke\n");
    }
    if (strlen(buf) == 0)
        return;
    char *username = strtok(buf, " ");
    char *password = strtok(NULL, " \r\n");
    char r = 'b';
    if (!checkUser(username, password))
    {
        send(connfd, &r, 1, 0); //bad login
        printf("user failed authentication: %s\n", username);
        return;
    }
    r = 'g';
    send(connfd, &r, 1, 0);
    if (mkdir(username, 0777) < 0)
    {
        if (errno != 17)
            perror("failed to make directory");
    }
    //printf("1: %s\n", buf);
    recv(connfd, buf2, MAXBUF, 0); //command and file sizes(if relevant) or filename (if relevant)
    if (n < 0)
    {
        perror("recv 2 went and done broke\n");
    }
    //printf("2: %s\n", buf2);
    char *cmd = strtok(buf2, "!");
    //printf(">%s<\n",cmd);

    if (!strcmp(cmd, "put"))
    {
        size_t size1, size2;
        size1 = atol(strtok(NULL, "!"));
        size2 = atol(strtok(NULL, "!"));
        char *m1 = malloc(size1);
        char *m2 = malloc(size1);

        char *message1;
        size_t m1size;
        FILE *stream = open_memstream(&message1, &m1size);
        int n = 0;
        while (n < size1)
        {
            int m = recv(connfd, m1, size1-n, 0); //file1
            fwrite(m1, 1, m, stream);
            n += m;
            if (n < 0)
            {
                perror("put recv 1 went and done broke\n");
            }
        }
        fclose(stream);
        char *message2;
        size_t m2size;
        FILE *stream2 = open_memstream(&message2, &m2size);
        n = 0;
        while (n < size2)
        {
            int m = recv(connfd, m2, size2, 0); //file2
            fwrite(m2,1,m,stream2);
            n += m;
            if (n < 0)
            {
                perror("put recv 2 went and done broke\n");
            }
        }
        fclose(stream2);

        char *file1part = strtok(message1, "!");
        char *file1name = strtok(NULL, "!");
        char *f1contents = strtok(NULL, "");
        int f1contentsize = size1 - (3 + strlen(file1name));
        char f1partname[100];
        strcpy(f1partname, username);
        strcat(f1partname, "/");
        strcat(f1partname, ".");
        strcat(f1partname, file1name);
        strcat(f1partname, ".");
        strcat(f1partname, file1part);

        FILE *file1;
        if ((file1 = fopen(f1partname, "w")) == NULL)
        {
            perror("file open failed");
        }
        fwrite(f1contents, 1, f1contentsize, file1);
        fclose(file1);
        char *file2part = strtok(message2, "!");
        char *file2name = strtok(NULL, "!");
        char *f2contents = strtok(NULL, "");
        int f2contentsize = size2 - (3 + strlen(file2name));
        //char * f2contents = malloc(f2contentsize);
        //memcpy(f2contents,temp,f2contentsize);
        char f2partname[100];
        strcpy(f2partname, username);
        strcat(f2partname, "/");
        strcat(f2partname, ".");
        strcat(f2partname, file2name);
        strcat(f2partname, ".");
        strcat(f2partname, file2part);

        FILE *file2;
        if ((file2 = fopen(f2partname, "w")) == NULL)
        {
            perror("file open failed");
        }
        fwrite(f2contents, 1, f2contentsize, file2);
        fclose(file2);
        free(message1);
        free(message2);
    }
    else if (!strcmp(cmd, "get"))
    {
        char *filename = strtok(NULL, "!");

        char p1[300];
        char p2[300];
        char p3[300];
        char p4[300];
        bool g1 = false, g2 = false, g3 = false, g4 = false;
        strcpy(p1, username);
        strcpy(p2, username);
        strcpy(p3, username);
        strcpy(p4, username);
        strcat(p1, "/.");
        strcat(p2, "/.");
        strcat(p3, "/.");
        strcat(p4, "/.");
        strcat(p1, filename);
        strcat(p2, filename);
        strcat(p3, filename);
        strcat(p4, filename);
        strcat(p1, ".1");
        strcat(p2, ".2");
        strcat(p3, ".3");
        strcat(p4, ".4");

        FILE *file1;
        FILE *file2;
        FILE *temp;
        char f1p[2];
        char f2p[2];

        if ((temp = fopen(p1, "r")) != NULL)
        {
            if (!g1 && !g2 && !g3 && !g4)
            {
                file1 = temp;
                strcpy(f1p, "1");
            }
            else
            {
                file2 = temp;
                strcpy(f2p, "1");
            }
            temp = NULL;
            g1 = true;
        }

        if ((temp = fopen(p2, "r")) != NULL)
        {
            if (!g1 && !g2 && !g3 && !g4)
            {
                file1 = temp;
                strcpy(f1p, "2");
            }
            else
            {
                file2 = temp;
                strcpy(f2p, "2");
            }
            temp = NULL;
            g2 = true;
        }
        if ((temp = fopen(p3, "r")) != NULL)
        {
            if (!g1 && !g2 && !g3 && !g4)
            {
                file1 = temp;
                strcpy(f1p, "3");
            }
            else
            {
                file2 = temp;
                strcpy(f2p, "3");
            }
            temp = NULL;
            g3 = true;
        }
        if ((temp = fopen(p4, "r")) != NULL)
        {
            if (!g1 && !g2 && !g3 && !g4)
            {
                file1 = temp;
                strcpy(f1p, "4");
            }
            else
            {
                file2 = temp;
                strcpy(f2p, "4");
            }
            temp = NULL;
            g4 = true;
        }

        if (!g1 && !g2 && !g3 && !g4)
        {
            send(connfd, "!", 1, 0);
            printf("No files found.");
            return;
        }

        char buf[32];

        char *buffer = NULL;
        size_t bufferSize = 0;
        FILE *stream = open_memstream(&buffer, &bufferSize);

        fwrite(f1p, 1, strlen(f1p), stream);
        fwrite("!", 1, 1, stream);

        int n;
        while ((n = fread(buf, 1, 32, file1)) > 0)
        {
            if (n > 32)
            {
                printf("n > 32\n");
            }

            int m = fwrite(buf, 1, n, stream);
            if (m > 32)
            {
                printf("m > 32\n");
            }
            if (n != m)
            {
                printf("n != m\n");
            }
        }
        fclose(stream);

        char *buffer2 = NULL;
        size_t buffer2Size = 0;
        FILE *stream2 = open_memstream(&buffer2, &buffer2Size);

        fwrite(f2p, 1, strlen(f2p), stream2);
        fwrite("!", 1, 1, stream2);

        while ((n = fread(buf, 1, 32, file2)) > 0)
        {
            if (n > 32)
            {
                printf("n > 32\n");
            }

            int m = fwrite(buf, 1, n, stream2);
            if (m > 32)
            {
                printf("m > 32\n");
            }
            if (n != m)
            {
                printf("n != m\n");
            }
        }
        fclose(stream2);

        fclose(file1);
        fclose(file2);
        char msg[50];
        char s1[25];
        char s2[25];
        sprintf(s1, "%ld", bufferSize);
        sprintf(s2, "%ld", buffer2Size);
        strcpy(msg, s1);
        strcat(msg, "!");
        strcat(msg, s2);
        send(connfd, msg, 50, 0);

        send(connfd, buffer, bufferSize, 0);   //send file 1
        send(connfd, buffer2, buffer2Size, 0); //send file 2

        free(buffer);
    }
    else if (!strcmp(cmd, "list"))
    {
        char *list = NULL;
        size_t listsize = 0;
        FILE *output = open_memstream(&list, &listsize);
        DIR *d;
        struct dirent *dir;
        d = opendir(username);
        if (d)
        {
            while ((dir = readdir(d)) != NULL)
            {
                //printf("%s\n", dir->d_name);
                if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, ".."))
                    continue;
                fwrite(dir->d_name, 1, strlen(dir->d_name), output);
                fwrite("\n", 1, strlen("\n"), output);
            }
            closedir(d);
        }
        fclose(output);
        //printf("%s\n", list);
        send(connfd, list, listsize, 0);
    }
}

/* 
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure 
 */
int open_listenfd(int port)
{
    int listenfd, optval = 1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval, sizeof(int)) < 0)
        return -1;

    /* listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
} /* end open_listenfd */

bool checkUser(char *username, char *password)
{
    char line[300];
    FILE *file = fopen("../dfs.conf", "r");
    if (file == NULL)
    {
        printf("failed to open dfs.conf\n");
        return true;
    }
    while (fgets(line, 300, file))
    {
        char *fuser = strtok(line, " ");
        char *fpass = strtok(NULL, " \r\n");
        //printf(">%s< : >%s<\n", cleaned, host);
        if (!strcmp(fuser, username) && !strcmp(fpass, password))
        {
            printf("Verified user: %s\n", username);
            return true;
        }
    }

    return false;
}