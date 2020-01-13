#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "Cola.h"

struct Cola *cola;

int main(int argc, char *argv[]){
    // Obtiene los pedidos por socket

	//Declaring process variables.
	int server_sockfd, client_sockfd;
	int server_len;
	int rc;
	unsigned client_len;
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;
	char buffer[50]; 

	//Remove any old socket and create an unnamed socket for the server.
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htons(INADDR_ANY);
	server_address.sin_port = htons(7734) ; 
	server_len = sizeof(server_address);

	rc = bind(server_sockfd, (struct sockaddr *) &server_address, server_len);
	printf("RC from bind = %d\n", rc ) ; 
	
	//Create a connection queue and wait for clients
	rc = listen(server_sockfd, 50);
	printf("RC from listen = %d\n", rc ) ; 

	client_len = sizeof(client_address);
	client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_address, &client_len);
	printf("after accept()... client_sockfd = %d\n", client_sockfd);

	// definiendo la cola para almacenar los pedidos que lleguen
    cola = (struct Cola*)malloc(sizeof(struct Cola));
    cola->primero = NULL;
    cola->final = NULL;
    char data[MAX];

    // recibiendo pedidos
	while(1){
		memset(buffer,0,sizeof(buffer));
		rc = read(client_sockfd, &buffer,sizeof(buffer));
		if (rc == 0) break;
        memset(data, '\0', MAX* sizeof(char));
        sprintf(data,"%s",buffer);
        enqueue(data, cola);
		printf("[Data = %s rc=%d]\n",buffer,rc);
	}
    int resultado;
	while(1){
        memset(data, '\0', MAX* sizeof(char));
	    resultado = dequeue(cola, data);
	    if(resultado==0) break;
	    //llenar lista de pedidos
	    //asignar a un brazo robotico


	}

    display(cola);
	printf("server exiting\n");
	close(client_sockfd);
	return 0;
}
