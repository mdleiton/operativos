/*
 * order_generator.c
 *
 * Version sin optimizar de generador de paquetes
 */


#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SIZE_F    10      /* array size */ 
#define SIZE_C    10      /* array size */ 


static int matrix[SIZE_F][SIZE_C]; //en la columan 0 se lleva el numero de items pendientes de enviar
static int status[SIZE_F]; //0 no enviado, 1 en proceso, 2 finalizado

void print_queue (int matrix[SIZE_F][SIZE_C])
{
    int i, j;
    int first;

    for (i = 0; i < SIZE_F; i++) {
        printf ("[");
        first = 1;
        for (j = 0; j < SIZE_C; j++) {
            if (!first)
                printf (",");
            printf ("%x", matrix[i][j]);
            first = 0;
        }
        printf ("]\n");
    }
        
}


void *thread_routine (void *arg) //se usarà en el futuro
{

}

void fill_queue (int matrix[SIZE_F][SIZE_C],int status[SIZE_F])
{
    int i, j;
    int first;
    
    for (i = 0; i < SIZE_F; i++) 
	matrix[i][0]= (rand() % (SIZE_F - 1)+1) ;    


    for (i = 0; i < SIZE_F; i++) 
        for (j = 1; j < matrix[i][j]; j++) 
            matrix[i][j]=1;    

    for (i = 0; i < SIZE_F; i++)   
        status[i]=0;
}
int pending_items(int matrix[SIZE_F][SIZE_C]){
int sum,i;
sum=0;
	for (i = 0; i < SIZE_F; i++) 
		sum=sum+matrix[i][0];
	return(sum);
}

int next_item(int matrix[SIZE_F][SIZE_C])
{
    int i, j;
    int sum;
    
    for (i = 0; i < SIZE_F; i++) 
	sum=sum+matrix[i][0];
    if (sum<=0)
        return (-1);
    while(pending_items(matrix)>0)
    {
	i=(rand() % (SIZE_F - 1));
	return(i);
	
    }
}




int main (int argc, char *argv[])
{
   int i;
   struct timespec tim, tim2;
   tim.tv_sec = 0;
   tim.tv_nsec = 500000000L; //medio segundo
   char buffer[50]; 
   int sockfd;
   int len, rc ;
   struct sockaddr_in address;
   int result;

    //Create socket for client.
   sockfd = socket(PF_INET, SOCK_STREAM, 0);
   if (sockfd == -1) { 
	perror("Socket create failed.\n") ; 
	return -1 ; 
  } 
	
	//Name the socket as agreed with server.
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr("127.0.0.1");
  address.sin_port = htons(7734);
  len = sizeof(address);
  result = connect(sockfd, (struct sockaddr *)&address, len);
  
  if(result == -1){
		perror("Error has occurred");
		exit(-1);
  }



    fill_queue(matrix,status);
    print_queue(matrix);
    printf("%d\n",pending_items(matrix));
    while(pending_items(matrix)>0){
	nanosleep(&tim , &tim2); //proxima version aleatorio
	i=next_item(matrix);
        if (status[i]==2) {
	   while(status[i++]!=2){ //avanzo al sigueente paquete no terminado	
   }

        }
	//printf(" i=%d -- status= %d",i,status[i]);
        memset(buffer,0,sizeof(buffer));
	if (status[i]==0) {
		//envio numero de items del paquete}
		status[i]=1;
		sprintf(buffer,"%d-%d",i,matrix[i][0]);
		//printf("buffer= %s", buffer);
		rc = write(sockfd, &buffer, strlen(buffer));
		continue;		
	}
	if (status[i]==1) {
		//envio otro item del paquete}
 	        matrix[i][0]=matrix[i][0]-1;
		if (matrix[i][0]==0){
			status[i]=2;
			sprintf(buffer,"%d-%d",i,0);
		}
		else
			sprintf(buffer,"%d-%d",i,1);

		rc = write(sockfd, &buffer, strlen(buffer));

        }
    }
    return 0;
}
