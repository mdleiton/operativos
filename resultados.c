#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  "Estructuras.h"
#include <limits.h> 
struct Datos  *datos;

int distance[MAX][MAX], pred[MAX][MAX];
int frecuencia[MAX];

void dijkstra(int G[MAX][MAX],int n,int startnode){
     int cost[MAX][MAX];
     int visited[MAX],count,mindistance,nextnode,i,j;
     
     //pred[] stores the predecessor of each node
     //count gives the number of nodes seen so far
     //create the cost matrix
     for(i=0;i<n;i++)
          for(j=0;j<n;j++)
               if(G[i][j]==0)
                    cost[i][j]=INFINITY;
               else
                    cost[i][j]=G[i][j];
     
     //initialize pred[],distance[] and visited[]
     for(i=0;i<n;i++){
          distance[startnode][i]=cost[startnode][i];
          pred[startnode][i]=startnode;
          visited[i]=0;
     }
     
     distance[startnode][startnode]=0;
     visited[startnode]=1;
     count=1;
     
     while(count<n-1){
          mindistance=INFINITY;
          
          //nextnode gives the node at minimum distance
          for(i=0;i<n;i++)
               if(distance[startnode][i]<mindistance&&!visited[i]){
                    mindistance=distance[startnode][i];
                    nextnode=i;
               }
               
               //check if a better path exists through nextnode            
               visited[nextnode]=1;
               for(i=0;i<n;i++)
                    if(!visited[i])
                         if(mindistance+cost[nextnode][i]<distance[startnode][i]){
                              distance[startnode][i]=mindistance+cost[nextnode][i];
                              pred[startnode][i]=nextnode;
                         }
          count++;
     }
}

void obtenerFrecuencia(){
     int k, startnode;
     for(int j=0;j<MAX;j++){
          startnode = j;     
          for(int i=0;i<j;i++){
               if(i!=startnode){
                    //printf("\nDistance of node%d=%d",i,distance[startnode][i]);
                    frecuencia[i]+=1;
                    //printf("\nPath=%d",i);
                    k=i;
                    do{
                         k=pred[startnode][k];
                         frecuencia[k]+=1;
                         //printf("<-%d",k);
                    }while(k!=startnode);
               }
          }

     }
}


int  main(void){
     int  mcID;
     mcID = shmget(ID_MC, sizeof(struct Datos), 0666);
     if (mcID < 0) {
          printf("resultado->main, shmget error.\n");
          exit(1);
     }
     printf("resultado->main, Obtenido un válido identificador de memoria compartido.\n");
     
     datos = (struct Datos *) shmat(mcID, NULL, 0);
     if ((int) datos == -1) {
          printf("resultado->main, shmat error.\n");
          exit(1);
     }
     printf("resultado->main, Mapeo a memoria compartida aceptada.\n");
     
     while (datos->status != FILLED);
     printf("resultado->main, Contenido listo para realizar calculos.\n");
     
     for(int i=0;i<MAX;i++){
          //varios hilos
          dijkstra(datos->grafo,datos->cantidadNodos,i);
          frecuencia[i]=0;
     }
     obtenerFrecuencia();
     for(int i=0;i<MAX;i++){
          //varios hilos
          printf("%d<-%d \n",i, frecuencia[i]);
     }
     datos->status = TAKEN;
     
     printf("resultado->main, Finalización de cálculos.\n");
     shmdt((void *) datos);
     printf("resultado->main, Desvinculado la memoria compartida\n");

     return 0;
}
