#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<iostream>
#include<assert.h>
#include<errno.h>
#include<stdint.h>
#include<stdlib.h>
#include<vector>
#include<string>


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
// static int32_t query(int fd,const char *text)
// {
//     uint32_t len = (uint32_t)strlen(text);
//     if(len>max_msg_size)
//     {
//         return -1;
//     }
//
//     char wbuff[4+max_msg_size];
//     memcpy(wbuff,&len,4);
//     memcpy(&wbuff[4],text,len);
//     if(int32_t err = write_full(fd,wbuff,4+len)<0)
//     {
//         return err;
//     }
//
//     char rbuff[4+max_msg_size+1];
//     errno=0;
//     int32_t err = read_full(fd,rbuff,4);
//     if(err)
//     {
//         if(errno==0)
//         {
//             cout<<"unexpected eof"<<endl;
//         }
//         else
//         {
//             cout<<"error in read"<<endl;
//         }
//     }
//
//     memcpy(&len,rbuff,4);
//     if(len>max_msg_size)
//     {
//         cout<<"message too long"<<endl;
//         return -1;
//     }
//     err = read_full(fd,&rbuff[4],len);
//     if(err)
//     {
//         cout<<"error in read"<<endl;
//         return err;
//     }
//     rbuff[4+len] = '\0';
//     cout<<"received: "<<&rbuff[4]<<endl;
//     return 0;
//
//
// }

// query function split into two 'send_req' and 'recv_res'

// static int32_t send_req(int fd,const char *text)
// {
//     uint32_t len = (uint32_t)strlen(text);
//     if(len>max_msg_size)
//     {
//         return -1;
//     }
//
//     char wbuff[4+max_msg_size];
//     memcpy(wbuff,&len,4);
//     memcpy(&wbuff[4],text,len);
//     return write_full(fd,wbuff,4+len);
// }

static int32_t read_res(int fd)
{
    char rbuff[4+max_msg_size+1];
    errno=0;
    int32_t err = read_full(fd,rbuff,4);
    if(err)
    {
        if(errno==0)
        {
            cout<<"unexpected eof"<<endl;
        }
        else
        {
            cout<<"error in read"<<endl;
        }
        return err;
    }

    uint32_t len=0;
    memcpy(&len,rbuff,4);
    if(len>max_msg_size)
    {
        cout<<"message too long"<<endl;
        return -1;
    }

    //reply body
    err = read_full(fd,&rbuff[4],len);
    if(err)
    {
        cout<<"error in read"<<endl;
        return err;
    }
    // rbuff[4+len] = '\0';
    // cout<<"received: "<<&rbuff[4]<<endl;
    // return 0;

    //print results
    uint32_t rescode = 0;
    if(len<4)
    {
        cout<<"bad response"<<endl;
        return -1;
    }
    memcpy(&rescode,&rbuff[4],4);
    cout<<"server says:["<<rescode<<"]"<<&rbuff[8]<<endl;
    return 0;

}
static int32_t send_req(int fd, const vector<string> &cmd)
{
    uint32_t len =4;
    for(const string &s : cmd)
    {
        len+= 4+s.size();
    }
    if(len > max_msg_size)
    {
        return -1;
    }
    char wbuf[4+max_msg_size];
    memcpy(&wbuf[0],&len,4);
    uint32_t n=cmd.size();
    memcpy(&wbuf[4],&n,4);
    size_t cur = 8;
    for(const string &s : cmd)
    {
        uint32_t p = (uint32_t)s.size();
        memcpy(&wbuf[cur],&p,4);
        memcpy(&wbuf[cur+4],s.data(),s.size());
        cur+= 4+s.size();
    }
    return write_full(fd,wbuf,4+len);
}

int main(int argc, char **argv)
{
    int client = socket(AF_INET, SOCK_STREAM, 0);
    if(client == -1)
    {
        cout<<"Error in creating socket"<<endl;
        return -1;
    }
    cout<<"Client socket created successfully"<<endl;

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(3000); // port number should be same as server's port number
    server_address.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // localhost this needs to be changed to the server's IP address

    int connect_status = connect(client, (sockaddr*)&server_address, sizeof(server_address));
    if(connect_status == -1)
    {
        cout<<"Error in connecting to server"<<endl;
        return -2;
    }
    cout<<"Connected to server successfully"<<endl;

    // char buffer[] = "Hello from client";
    // int send_status = send(client, buffer, sizeof(buffer), 0);
    // if(send_status == -1)
    // {
    //     cout<<"Error in sending data"<<endl;
    //     return -3;
    // }
    // cout<<"Data sent to server"<<endl;

    // multiple messages
    // int32_t err = query(client,"Hello1");
    // if(err)
    // {
    //     goto L_DONE;
    // }
    // err = query(client,"Hello2");
    // if(err)
    // {
    //     goto L_DONE;
    // }
    // err = query(client,"Hello3");
    // if(err)
    // {
    //     goto L_DONE;
    // }

    // const char *query_list[3] = {"Hello1","Hello2","Hello3"};
    // for(size_t i=0;i<3;i++)
    // {
    //     int32_t err = send_req(client,query_list[i]);
    //     if(err)
    //     {
    //         goto L_DONE;
    //     }
    // }
    // for(size_t i=0;i<3;i++)
    // {
    //     int32_t err = read_res(client);
    //     if(err)
    //     {
    //         goto L_DONE;
    //     }
    // }

    vector<string> cmd;
    for(int i=1;i<argc;i++)
    {
        cmd.push_back(argv[i]);
    }

    int32_t err  = send_req(client,cmd);
    if(err)
    {
        goto L_DONE;
    }
    err=read_res(client);
    if(err)
    {
        goto L_DONE;
    }


    L_DONE:
    close(client);
    return 0;
}

// server_address.sin_port = htons(3000); // port number should be same as client's port number
//     server_address.sin_addr.s_addr = INADDR_ANY; // server's IP address