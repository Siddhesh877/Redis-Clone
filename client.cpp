#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<iostream>

using namespace std;

int main()
{
    int client = socket(AF_INET, SOCK_STREAM, 0);
    if(client == -1)
    {
        cout<<"Error in creating socket"<<endl;
        return -1;
    }
    cout<<"Client socket created successfully"<<endl;

    sockaddr_in client_address;
    client_address.sin_family = AF_INET;
    client_address.sin_port = htons(3000); // port number should be same as server's port number
    client_address.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // localhost this needs to be changed to the server's IP address

    int connect_status = connect(client, (sockaddr*)&client_address, sizeof(client_address));
    if(connect_status == -1)
    {
        cout<<"Error in connecting to server"<<endl;
        return -2;
    }
    cout<<"Connected to server successfully"<<endl;

    char buffer[] = "Hello from client";
    int send_status = send(client, buffer, sizeof(buffer), 0);
    if(send_status == -1)
    {
        cout<<"Error in sending data"<<endl;
        return -3;
    }
    cout<<"Data sent to server"<<endl;
    return 0;
}