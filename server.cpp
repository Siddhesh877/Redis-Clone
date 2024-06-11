#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<iostream>
#include<assert.h>
#include<errno.h>

using namespace std;

const size_t max_msg_size = 4096;

static int32_t read_full(int fd,char *buff,size_t n)
{
    while(n>0)
    {
        ssize_t rv = read(fd,buff,n);
        if(rv<=0)
        {
            return -1; // error or unexpected eof
        }
        assert(size_t(rv)<=n);
        n -= (size_t)rv;
        buff += rv;
    }
    return 0;
}

static int32_t write_full(int fd,const char *buff,size_t n)
{
    while(n>0)
    {
        ssize_t rv = write(fd,buff,n);
        if(rv<=0)
        {
            return -1; // error or unexpected eof
        }
        assert(size_t(rv)<=n);
        n -= (size_t)rv;
        buff += rv;
    }
    return 0;
}

static int32_t one_request(int connfd)
{
    char rbuff[4+max_msg_size+1];
    errno=0;
    int32_t err = read_full(connfd,rbuff,4);
    if(err)
    {
        if(errno==0)
        {
            cout<<"eof"<<endl;
        }
        else
        {
            cout<<"read error"<<endl;
        }
        return err;
    }

    uint32_t len=0;
    memcpy(&len,rbuff,4);
    cout<<"received message length: "<<len<<endl;
    // len = ntohl(len);
    if(len>max_msg_size)
    {
        cout<<"message too long"<<endl;
        return -1;
    }

    err = read_full(connfd,&rbuff[4],len);
    if(err)
    {
        cout<<"read error"<<endl;
        return err;
    }

    rbuff[4+len] = '\0';
    cout<<"rbuff: "<<&rbuff[0]<<endl;
    cout<<"received message from client: "<<&rbuff[4]<<endl;

    const char *reply = "Hello from server";
    char wbuff[4+strlen(reply)];
    len = strlen(reply);
    memcpy(wbuff,&len,4);
    memcpy(&wbuff[4],reply,len);

    return write_full(connfd,wbuff,4+len);

}

int main()
{
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if(server == -1)
    {
        cout<<"Error in creating socket"<<endl;
        return -1;
    }
    cout<<"Server socket created successfully"<<endl;

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(3000); // port number should be same as client's port number
    server_address.sin_addr.s_addr = INADDR_ANY; // server's IP address

        int val = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    int bind_status = bind(server, (sockaddr*)&server_address, sizeof(server_address));
    if(bind_status == -1)
    {
        cout<<"Error in binding socket to IP and port"<<endl;
        return -2;
    }
    cout<<"Server socket binded successfully"<<endl;

    int listen_status = listen(server, 5);
    if(listen_status == -1)
    {
        cout<<"Error in listening"<<endl;
        return -3;
    }
    cout<<"Server is listening..."<<endl;

    while(1){
        sockaddr_in client_address;
        socklen_t socklen = sizeof(client_address);
    // NULL, NULL are used to get client's IP and port number if needed and client is the socket descriptor for the client
    int client = accept(server, (struct sockaddr*)&client_address, &socklen); 
    if(client == -1)
    {
        cout<<"Error in accepting client"<<endl;
        return -4;
    }
    cout<<"Client connected successfully"<<endl;
    while(1)
    {
        // read and write to a single client
        int32_t err = one_request(client);
        if(err)
        {
            break;
        }
    }
    close(client);
}
    return 0;
}