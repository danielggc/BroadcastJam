#include <asm-generic/errno-base.h>
#include <cstddef>
#include <stdio.h> 
#include <string.h>   //strlen 
#include <stdlib.h> 
#include <errno.h> 
#include <string>
#include <strings.h>
#include <sys/select.h>
#include <unistd.h>   //close 
#include <arpa/inet.h>    //close 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h>
#include <iostream>
#include<fcntl.h>

//definiciones par socket

#define SERVER_PORT 9000
#define LISTEN_BACKLOG 5
#define MAX_NAME_SZE 20
#define NO_OF_CLIENTS 10
#define MAX_BUFFER_SIZE 1024

//colores 
#define NC "\e[0m"
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define CYN "\e[0;36m"
#define REDB "\e[41m"



struct client {
    char cname[MAX_NAME_SZE];
    char chatwith[MAX_NAME_SZE];
    int chatwith_fd;
    int file_des;
    int port;
    char ip[INET_ADDRSTRLEN];
};

struct server_data {
    int total_client;
    struct client client_list[NO_OF_CLIENTS];
};



using namespace std;

class SocketOpen{
    private:

        struct server_data server;
        int server_fd, new_socket, valread;
        struct sockaddr_in address;
        int opt = 1;
        int max_sd , sd  , activity;  
        int addrlen = sizeof(address);
        const unsigned port = SERVER_PORT;
        fd_set readfds, writefds , exceptfds ;  
        const char * mse  = "socket Init";
        const char * mse_client_init = "\nconnecion realizada\n";


    public:
    void error(const char *msg){
      perror(msg);
      exit(1);
    }


    void setScoket( ){
        if (setsockopt(server_fd, SOL_SOCKET,
                    SO_REUSEADDR | SO_REUSEPORT, &opt,
                    sizeof(opt))) {
            error("setsockopt");
        }

    }

    void bind_to_port( ){
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = INADDR_ANY;
        int c=bind( server_fd ,(struct sockaddr*) &address, sizeof(address));
        if(c<0){
            error("bind error ");
        }
    }

    int openSKT(){
        server_fd =socket(AF_INET,SOCK_STREAM,0);
        if(server_fd<0){
            error("error iniciando socket");
        }
        return server_fd;
    }

    void init ( ){
        openSKT();
        setScoket();
        bind_to_port( );
    }


    void listenServidor(){
        if( listen( server_fd ,  3) <0 ){
            error("listen");
        }
    }


    void closeSocket( int socket_fd ){
        close(socket_fd );
    }
    int serverSelect( int max_fd ){
        int new_socket_fd  = 0;
        char recv_msgg[MAX_BUFFER_SIZE] ;
        char send_buff[MAX_BUFFER_SIZE] ;
        memset(recv_msgg, 0 ,sizeof(recv_msgg));
        memset(send_buff, 0 ,sizeof(send_buff));
        int action = select( max_fd + 1 , &readfds , &writefds, &exceptfds, NULL);
        if( action < 0 ){
            error( "select " );
        }
        if( FD_ISSET( server_fd , &readfds) ){
            serverNewClient( server_fd , &new_socket_fd);
            printf(RED"nuevo socke creado = %d\n",new_socket_fd);              
        }
        for(int i = 0; i<server.total_client; i++) {    
            if( FD_ISSET( server.client_list[i].file_des, &readfds)) {
                printf("\nhola aque paso algo->");
                serverRecv( server.client_list[i].file_des, recv_msgg ); 
            } 
        } 
        return 1;
    }

    int serverFdsets(  ){
        int maxFd = server_fd;
        FD_ZERO( &readfds );
        FD_SET( server_fd, &readfds );
        FD_SET( STDERR_FILENO, &readfds);
        fcntl( STDERR_FILENO, F_SETFL, &readfds);
        for( int i = 0; i < server.total_client ; i++ ){
            FD_SET( server.client_list[i].file_des , &readfds);
            maxFd++;
        }
        return maxFd;
    }




    int serverNewClient(int listen_fd, int *new_socket_fd) {
        struct sockaddr_in client_addr;
        int len = sizeof(struct sockaddr);
        bzero(&client_addr,sizeof(client_addr));
        if( (*new_socket_fd =accept(listen_fd, (struct sockaddr* ) &client_addr, (socklen_t*)&len ) ) < 0) {
            error("ERROR :accept failed");
            return -1;
        }
        serverAddClient(client_addr ,*new_socket_fd);

        return 1;
    }

    void serverAddClient(struct sockaddr_in client_info, int new_socket_fd) {
        char ip[INET_ADDRSTRLEN] = {0};
        char buffer[MAX_BUFFER_SIZE] = {0};
        serverRecv( new_socket_fd , buffer);
        
        int port = ntohs(client_info.sin_port );
        printf(GRN"[CLIENT-INFO] : [port = %d , ip = %s , name = %s]\n",port, inet_ntoa(client_info.sin_addr), buffer );
        serverSendToClient( new_socket_fd   , mse_client_init  );
        if(server.total_client >=NO_OF_CLIENTS) {
            perror("ERROR :no hay espacio para mas clientes ");
            
        }
        strncpy(server.client_list[server.total_client].cname ,buffer,strlen(buffer));
        server.client_list[server.total_client].port = port;
        strcpy(server.client_list[server.total_client].ip, inet_ntoa(client_info.sin_addr));
        server.client_list[server.total_client].file_des=new_socket_fd;
        server.total_client++;
    }


    int serverRecv( int client_socket , char *recv_msg ){

        int recv_bytes = 0;
        memset(  recv_msg, 0 , strlen( recv_msg ) );
        if( ( recv_bytes =  recv( client_socket , recv_msg, MAX_BUFFER_SIZE, 0 ) ) > 0 ){
            printf("\n%s\n",recv_msg);
            processRecvData(client_socket, recv_msg );
        }
        else if(  recv_bytes == 0){
            printf( RED" cliente desconectado ");
            serverDeleteClient( client_socket );
        }
        else{
            printf(RED"ERROR recv  data del cliente");
        }
        return 0;
    }

    int find_the_client_index_list(int socket) {
        int index = 0;
        for(int i = 0; i<server.total_client; i++) {
            if(server.client_list[i].file_des == socket) {
                index =i;
            }
        }
        return index;
    }

    int verificarEsisteUsuario( char * data ){
        printf(RED"\nveridicando...\n");
        for( int i =0; i < server.total_client ;i++ ){
            if( strncmp( server.client_list[i].cname , data , strlen(data)) == 0 ){
                printf("\n%s.\n",server.client_list[i].cname);
                return i;
            }
        }
        return -1;
    }
    client * getClientServerBysocket( int socket){
        for(int i = 0;  i <  server.total_client ;i ++){
            if(server.client_list[i].file_des == socket ){
                return &server.client_list[i];
            }
        }

        return NULL;
    }

    int processRecvData( int socket,  char* buffer ){

        char chat_c[MAX_BUFFER_SIZE];
        char buffer_send[MAX_BUFFER_SIZE] = {0};
        int index_sender = 0;
        int index_receiver = 0;
        int len = 0;    
        if( strncmp( buffer , "CONNECT", 7  ) == 0 ){
            printf(CYN"\nse desea ahcer una connecion " );
            char * pch;
            pch = strstr (buffer,"NAME:[");    
            if( pch != NULL ){
                char name[20] = {0};
                memset(name, '\0', 20 );
                int i = 6;
                for( ; pch[i]!=']' ;i ++ ){
                    name[i-6] =pch[i];
                }
                printf("con el usuario :[%s]\n",name);
                int posicion = -1;
                if(  ( posicion =  verificarEsisteUsuario(name)  ) >= 0 ) {
                    printf(GRN"\nse encontro el uasuiop que se esta pidiendo\n");
                    pch = strstr( pch , "MSG:");
                    if( pch != NULL ){
                        memset( buffer_send , '\0', MAX_BUFFER_SIZE );
                        client  * data  = getClientServerBysocket(socket);
                        if( data != NULL ){
                            char  msg_r[100] = "DATA:[Name:" ;
                            strcat(msg_r, data->cname );
                            strncpy(buffer_send, msg_r ,  strlen(msg_r ) );
                            strcat( buffer_send , " , MSG : ");
                            int i = 5;
                            char msg_c[MAX_BUFFER_SIZE];
                            memset( msg_c , '\0', MAX_BUFFER_SIZE );
                            for( ; pch[i]!=']' ;i ++ ){
                                msg_c[i-5] =pch[i];
                            }
                            strcat( buffer_send, msg_c);
                            strcat( buffer_send, "]");
                            printf(CYN"\n el  mensaje a enviar es : %s  \n",buffer_send);
                        
                        }
                 
                    }

                    serverSendToClient( server.client_list[posicion].file_des, buffer_send );

                }else{
                    printf(RED"\npero no existe el usuaro o no eta connecta \n");
                }
            }
        }
        else if( strncmp( buffer, "LIST", 4) == 0 ){
            memset( buffer_send,'\0', sizeof(buffer_send) );
            int count = 0;
            int size = 0;
            strcpy( buffer_send, "\nLIST user: [ ");
            const char * data_name  = " [ name : ";
            const char * data_ip    = "  , ip : ";
            const char * data_por   = "  , port : ";
            
            do {
                strcat( buffer_send, data_name );
                strcat( buffer_send,  server.client_list[count].cname );
                strcat( buffer_send, data_ip);
                strcat( buffer_send,  server.client_list[count].ip );
                strcat( buffer_send, data_por);
                strcat( buffer_send, "]");                
                count++;
                
            }while( sizeMax( buffer_send , &size) &&  count < server.total_client  );
            strcat( buffer_send, "\n],");                
            serverSendToClient(socket , buffer_send );
        }
        if(strlen(server.client_list[index_sender].chatwith) != 0){
            snprintf(buffer_send,sizeof(buffer_send),"[%s] : %s",server.client_list[index_sender].cname,buffer);
            printf("Buffer  =%s\n",buffer_send);

        }

        return 1;
    }

    bool sizeMax( char * buffer , int  *d ) {
        int i= *d;
        while( buffer[i] != '\0'){
            i++;
        }
        *d = i;
        if( i < MAX_BUFFER_SIZE ){
            
            return true;
        }
        else {
            return false;
        }
    }

    int serverDeleteClient( int socket_fd ){
        for(int i  =0  ; i < server.total_client; i++ )    {
            if(server.client_list[i].file_des == socket_fd ){
                for( int d = i ;   d < server.total_client-1; d++){
                    server.client_list[d] = server.client_list[ d+1 ];
                }
                break;
            }
        }
        printf( "\nnumero de cleintes %d\n",server.total_client);
        dataClientZero( server.client_list, server.total_client -1  );
        server.total_client--;
        closeSocket( socket_fd );
        printf( GRN"\n[ client %d ]  borrado correcto\n",socket_fd);

        return 1;

    }


    void dataClientZero(  client * clientSocket , int posicion) {
        printf(GRN"\ndata  a borrar : [  port : [%d] ,ip : [%s], ]\n", clientSocket[posicion].port, clientSocket[posicion].ip);
        bzero( clientSocket[posicion].chatwith , sizeof( clientSocket[posicion].chatwith ) ) ;
        bzero( clientSocket[posicion].cname , sizeof( clientSocket[posicion].cname ) ) ;
        bzero( clientSocket[posicion].ip , sizeof( clientSocket[posicion].ip ) ) ;
        clientSocket[posicion].chatwith_fd = 0;
        clientSocket[posicion].file_des = 0;
        clientSocket[posicion].port = 0;
        printf(NC"\nnueva data  : [  port : [%d] ,ip : [%s], ]\n", clientSocket[posicion].port, clientSocket[posicion].ip);
    }


    int serverSendToClient(int client_socket, const char *send_msg ) {
        int write_bytes = 0;
        int len  =strlen(send_msg);
        if((write_bytes = send(client_socket, send_msg, len, 0)) > 0) {
                printf("\n[CLIENT : %d] || nuemro de bits :  [%d]  || BYTES = [%s]\n",client_socket,write_bytes, send_msg);
        }
        else {
                perror("Error : send failed");
                return -1;
        }

        return write_bytes;
    }

};

int main(int argc, char const* argv[]){
    SocketOpen    socketOpen = SocketOpen();
    socketOpen.init();
    socketOpen.listenServidor();
    while (1) {
        int max_fd  = socketOpen.serverFdsets();
        socketOpen.serverSelect(max_fd);
    }
    
}

