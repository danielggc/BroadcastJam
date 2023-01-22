#include <asm-generic/socket.h>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <type_traits>
#include <unistd.h>
#include <arpa/inet.h>
#include<fcntl.h>
#include <pthread.h>

//colores 
#define NC "\e[0m"
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define CYN "\e[0;36m"
#define REDB "\e[41m"


using namespace std;
#define MAX_BUFFER_SIZE 1024


class SocketOpen{
    private:
    const unsigned port = 9000;
    int client_fd = 0,   new_socket = 0 , valread;
    struct sockaddr_in serv_addr;
    fd_set readfds, writefds , exceptfds ;  
    char buffer[MAX_BUFFER_SIZE] = { 0 };
    int opt = 1; 
    const char   msg_list[5] = "LIST";

    public:

    void error(const char *msg){
      perror(msg);
      exit(1);
    }




    void setScoket( ){
        if (setsockopt(client_fd, SOL_SOCKET,
                    SO_REUSEADDR | SO_REUSEPORT, &opt,
                    sizeof(opt))) {
            error("setsockopt");
        }

    }
    
    void closeSocket(  ){
        close( client_fd );
    }

  
    
    int clientFdsets(  ){
        int maxFd = client_fd;
        FD_ZERO( &readfds );
        FD_SET( client_fd, &readfds );
        FD_SET( client_fd, &writefds );
        FD_SET( STDERR_FILENO, &readfds);
//                fcntl( STDERR_FILENO, F_SETFL, O_NONBLOCK);

        return 1;
    }


    int cleintCreateSocket( char *client_name ) {
        printf(GRN"\nconfiguradno socket.....\n");

        if((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("ERROR : socket creation failed");
            return -1;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port); 
        serv_addr.sin_addr.s_addr = INADDR_ANY;

        if(0!=connect(client_fd,(struct sockaddr *)&serv_addr,sizeof(struct sockaddr))) {
            perror("ERROR : connect failed");
            return -1;
        }
        printf(GRN"\nconnectado socket servidor\n");

        clientSendToServer(client_fd, client_name);

        return 0;

    }

 
  

 

    int clientSelect(){
        char recv_msgg[MAX_BUFFER_SIZE] ;
        char send_buff[MAX_BUFFER_SIZE] ;
        memset(recv_msgg, 0 ,sizeof(recv_msgg));
        memset(send_buff, 0 ,sizeof(send_buff));
    
        int action = select( client_fd + 1 , &readfds, &writefds, &exceptfds, NULL);
        if( action < 0 ){
            error("select");
        }

        if( FD_ISSET( client_fd, &readfds ) ){
            readServer( recv_msgg );
            return 0;
        }

        if(FD_ISSET(STDIN_FILENO,&readfds)) {

            if(read(0,send_buff,sizeof(send_buff))>0) {
                clientSendToServer(client_fd,send_buff);
            }
                   
        }

        return 1;
    }

    int readServer( char * recv_msg ){
        int read_bytes = 0;
     
        if ((read_bytes = recv(client_fd, recv_msg, MAX_BUFFER_SIZE, 0)) > 0) {
            printf(NC"\nmsg resividos : [ %s ]\n",recv_msg);
        }
        else if(read_bytes == 0) {
            printf("Client Disconnected\n");
            close(client_fd);
        }
        else {
            printf("ERROR: recv failed\n");
        }
        return 1;
    }



    int clientSendToServer(int client_socket, const char *send_msg) {
        int write_bytes = 0;
        int len =strlen(send_msg);
        if((write_bytes = send(client_socket, send_msg, len, 0)) <=0) {
                perror("ERROR : send failed");
                return -1;
            }

        return write_bytes;
    }
    int getListaUSuarios(){
        clientSendToServer( client_fd ,  msg_list );
        return  1;
    }
    int connectUser( char * name , char * buffer_msg ){
        char sendData[MAX_BUFFER_SIZE] = "CONNECT:[NAME:[";
        strcat(sendData, name);
        strcat(sendData, "],MSG:[");
        strcat(sendData,buffer_msg);
        strcat(sendData, "]]");
        clientSendToServer( client_fd ,  sendData );
        return  1;
    }



};
char   msg_Menu [5][30] =   {{"\n\n1. ver lista de usuarios",},{"\n2. connectarse a un usauario"},{" \n3.ver data resivida\n\n"}};

void impimirMenu ( int num ){
        char data[40];
        for( int i = 0 ; i < 3 ; i++){
            bzero(data, sizeof( data ));
            if(num == i ){
                strcpy(data, RED);
            }
            else{
                strcpy(data, CYN);
            }
            strcat(data, msg_Menu[i]);
            printf("%s",data );
        }
}

int main(int argc, char const* argv[]){


    char send_buff[MAX_BUFFER_SIZE] ;
    
    SocketOpen socketOpen;
    printf("\ningrese el nombre de la maquina\n");
    scanf("%s",send_buff);
    socketOpen.cleintCreateSocket(send_buff);
    int option = 1;
    char data =0 ;
    do {
        impimirMenu(option -1 );
        scanf("%d", &option );
        switch (option) {
            case 1 : {
                printf("\nprecione Y para continuar ||  Z para salir\n");
                cin>>data;
                if(data == 'y' )
                    socketOpen.getListaUSuarios();
                system("clear");
                break;
            }
            case 2 : {
                system("clear");
                impimirMenu(1);
                char name[20];
                printf("\nintroduca el nombre a connetar\n");
                bzero(send_buff, MAX_BUFFER_SIZE );
                bzero(name, 20);
                cin>>name;
                printf("\nintrodusca su mensaje\n");
                cin>>send_buff;
                printf("\nprecione Y para connectarse con  %s ||  Z para salir\n",name);
                cin>>data;
                if(data == 'y' ){
                    socketOpen.connectUser( name , send_buff);
          
                }             
                system("clear");
                break;
            }
            case 3: {
                system("clear");
                socketOpen.clientFdsets();
                socketOpen.clientSelect();
                break;
            }
        
        }
    }while (1);

}
