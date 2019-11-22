#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include  <stdio.h>
#include  <stdlib.h>
#include <string.h>
#include  "Estructuras.h"

struct Datos  *datos;

int loadCsv(char* csvFile){
   char buffer[1024];
   char *record,*line;
   int i=0,j=0;
   FILE *fstream = fopen(csvFile,"r");
   if(fstream == NULL){
      printf("cargadorDatos->loadCsv, error al abrir el archivo.\n");
      return -1;
   }
   while((line=fgets(buffer,sizeof(buffer),fstream))!=NULL){ 
     j=0;
     record = strtok(line,",");
     while(record != NULL){
          datos->grafo[i][j] = atoi(record);
          record = strtok(NULL,",");
          j++; 
     }     
     i++;
   }
   if (i!=j){
     printf("cargadorDatos->loadCsv, error matriz no cuadrada.\n");
     exit(1);
   }
   datos->cantidadNodos = j;
   return 1;
}


int  main(int  argc, char *argv[]){
     int mcID;
     if (argc != 2) {
          printf("Usar: %s csvFile \n", argv[0]);
          exit(1);
     }

     mcID = shmget(ID_MC, sizeof(struct Datos), IPC_CREAT | 0666);
     if (mcID < 0) {
          printf("cargadorDatos->main, shmget error.\n");
          exit(1);
     }
     printf("cargadorDatos->main, Obtenido un válido identificador de memoria compartido.\n");
     
     datos = (struct Datos *) shmat(mcID, NULL, 0);
     if ((int) datos == -1) {
          printf("cargadorDatos->main, shmat error.\n");
          exit(1);
     }
     printf("cargadorDatos->main, Mapeo a memoria compartida aceptada.\n");
     
     datos->status  = ACTUALIZANDO;
     int estado = loadCsv(argv[1]);
     if(estado<=0){
          exit(1);
     }
     printf("cargadorDatos->main, Se ha cargado el grafo correctamente.\n");
     datos->status = ACTUALIZADO;
     char actualizacion[20];
     char* elemento;
     int nodoA =-1, nodoB=-1, peso=-1;
     while(1){
          memset(actualizacion, 0, 20 * (sizeof actualizacion[0]) );
          printf("Ingrese alguna actualización de distancias(ejemplo: 1-2-23):");
          scanf("%s",actualizacion);
          elemento = strtok(actualizacion,"-");
          if(elemento != NULL){
               nodoA = atoi(elemento);
               elemento = strtok(NULL,"-");
               if(elemento != NULL){
                    nodoB = atoi(elemento);
                    elemento = strtok(NULL,"-");
                    if(elemento != NULL){
                         peso = atoi(elemento);
                    }else{
                         printf("cargadorDatos->main, Incorrecto valor del peso.\n");
                         continue;
                    }
               }else{
                    printf("cargadorDatos->main, Incorrecto valor del nodo2.\n");
                    continue;
               } 
          }else{
               printf("cargadorDatos->main, Incorrecto valor del nodo1.\n");
               continue;
          }
          if (nodoA > MAX || nodoB > MAX){
               printf("cargadorDatos->main, Error nodos deben valores menores a: %d\n", MAX);
               continue;
          }
          printf("%d-%d=%d\n",nodoA,nodoB,peso);
          printf("actualizando\n");
          datos->status = ACTUALIZANDO;
          datos->grafo[nodoA][nodoB]=peso;
          datos->status = ACTUALIZADO;
          printf("actualizado\n");
     
     }
          
     printf("cargadorDatos->main, Finalizando.\n");
     shmdt((void *) datos);
     printf("cargadorDatos->main,  liberar memoria compartido\n");
     shmctl(mcID, IPC_RMID, NULL);
     printf("cargadorDatos->main, memoria copartida a sido eliminada.\n");
     return 0;
}