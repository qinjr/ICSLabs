/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS:
 *     Name: 秦佳锐 
 *     ID: 515030910475
 *     Email: qinjr@icloud.com
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */


/*
 * Description:
 * For part1, I just send the requests from client to server, and forward the response to client
 * But I change a little in request, I change "Connection: Keep-Alive" to "Connection: close"
 * For part2, I write a thread safe open_clientfd_ts, and use P and V in writting log file
 * I found that the format_log_entry() function doesn't use size, so I changed it a little to generate 
 * the same log file as descriped on doc. The grade.sh seems not to check the log file
 */ 

#include "csapp.h"

#define MAX_LINE 256
/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
int open_clientfd_ts(char *hostname, int port);
void *thread(void *vargp);

//mutex1 used in writting log file
sem_t mutex1;
//mutex2 used in open_clientfd_ts
sem_t mutex2;

//argument passed to thread
struct args2th {
    int threadid;
    int connfd;
    struct sockaddr_in clientaddr; 
};

/*
 * Rio - new Rio functions
 */
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes) {
    ssize_t n;
  
    if ((n = rio_readn(fd, ptr, nbytes)) < 0) {
        n = 0;
        printf("Rio_readn error");
    }
    return n;
}

void Rio_writen_w(int fd, void *usrbuf, size_t n) 
{
    if (rio_writen(fd, usrbuf, n) != n) {
        printf("Rio_writen error");
    }
}

ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0) {
        rc = 0;
        printf("Rio_readlineb error");
    }
    return rc;
}

/*
 * doit - transfer data between client and server
 */
void doit(struct args2th *args) {
    int connfd = ((struct args2th *)args)->connfd;
    struct sockaddr_in clientaddr = ((struct args2th *)args)->clientaddr;
    int threadid = ((struct args2th *)args)->threadid;
    Free(args);
    printf("this is thread%d\n", threadid);

    char uri[MAXLINE];
    rio_t rio_client;
    char hostname[MAXLINE], pathname[MAXLINE], method[MAXLINE], version[MAXLINE];
    int serverport;
    Rio_readinitb(&rio_client, connfd);
    char buf[MAXLINE];
    Rio_readlineb_w(&rio_client, buf, MAXLINE);

    bzero(uri, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    parse_uri(uri, hostname, pathname, &serverport);
    int serverfd = open_clientfd_ts(hostname, serverport);

    char toServer[3 * MAXLINE];
    bzero(toServer, 3 * MAXLINE);
    strcat(toServer, method);
    strcat(toServer, " /");
    strcat(toServer, pathname);
    strcat(toServer, " ");
    strcat(toServer, version);
    strcat(toServer, "\r\n");
    while (strcmp(buf, "\r\n")) {
        bzero(buf, MAXLINE);
        Rio_readlineb_w(&rio_client, buf, MAXLINE);
        if (!strncmp("Connection", buf, 10))
            strcat(toServer, "Connection: close\r\n");
        else if(!strncmp("Proxy-Connection", buf, 16))
            strcat(toServer, "Proxy-Connection: close\r\n");
        else
            strcat(toServer, buf);
    }
    Rio_writen_w(serverfd, toServer, strlen(toServer));

    rio_t rio_server;
    Rio_readinitb(&rio_server, serverfd);
    int n = 0;
    int receive_amt = 0;
    bzero(buf, MAXLINE);

    while ((n = Rio_readlineb_w(&rio_server, buf, MAXLINE) > 0)) {
        receive_amt += n;
        Rio_writen_w(connfd, buf, strlen(buf));
        bzero(buf, MAXLINE);
    }
    char logstring[MAXLINE];

    
    P(&mutex1);
    FILE *fp = fopen("proxy.log", "a");
    format_log_entry(logstring, &clientaddr, uri, receive_amt);
    strcat(logstring, "\n");
    fputs(logstring, fp);
    fclose(fp);
    V(&mutex1);

    Close(serverfd);
    Close(connfd);
    return;
}

void *thread(void *vargp) {
    Pthread_detach(pthread_self());
    doit(vargp);
    return NULL;
}

/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{
    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }

    /* As a server */
    int listenfd, port, clientlen;
    struct sockaddr_in clientaddr;
    struct args2th *args;
    port = atoi(argv[1]);

    listenfd = open_listenfd(port);
    clientlen = sizeof(clientaddr);
    pthread_t tid;
    Sem_init(&mutex1, 0, 1);
    Sem_init(&mutex2, 0, 1);
    signal(SIGPIPE, SIG_IGN);

    int threadid = 0;
    while (1) {
        threadid ++;
        args = Malloc(sizeof(struct args2th));
        args->connfd = Accept(listenfd, (SA *)(&(args->clientaddr)), &clientlen);
        args->threadid = threadid;
        Pthread_create(&tid, NULL, thread, args);
    }
    Close(listenfd);
    exit(0);
}


/*
 * parse_uri - URI parser
 * 
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
    hostname[0] = '\0';
    return -1;
    }

    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';
    
    /* Extract the port number */
    *port = 80; /* default */
    if (*hostend == ':')   
    *port = atoi(hostend + 1);
    
    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
    pathname[0] = '\0';
    }
    else {
    pathbegin++;    
    strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
              char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s %d", time_str, a, b, c, d, uri, size);
}

/*
 * open_clientfd_ts - thread safe function
 */
int open_clientfd_ts(char *hostname, int port) {    
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;
    if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;
    bzero((char * )&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    P(&mutex2);
    if((hp = gethostbyname(hostname)) == NULL)
        return -2;
    bcopy((char *)hp->h_addr_list[0], (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    V(&mutex2);
    serveraddr.sin_port = htons(port);
    if(connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
        return -1;
    return clientfd;
}

