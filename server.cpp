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


    // NULL, NULL are used to get client's IP and port number if needed and client is the socket descriptor for the client
    int client = accept(server, NULL, NULL); 
    if(client == -1)
    {
        cout<<"Error in accepting client"<<endl;
        return -4;
    }
    cout<<"Client connected successfully"<<endl;
    size_t buffer_size = 100;
    char buffer[buffer_size];
    int receive_status = recv(client, buffer, buffer_size, 0);
    if(receive_status == -1)
    {
        cout<<"Error in receiving data"<<endl;
        return -5;
    }
    cout<<"Data received from client: "<<buffer<<endl;
    return 0;
}