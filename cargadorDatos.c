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
          printf("Use: %s csvFile \n", argv[0]);
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
     
     datos->status  = NOT_READY;
     loadCsv(argv[1]);
     printf("cargadorDatos->main, has filled\n");
     datos->status = FILLED;
     
     printf("Please start the client in another window...\n");
                 
     while (datos->status != TAKEN)
          sleep(1);
          
     printf("Server has detected the completion of its child...\n");
     shmdt((void *) datos);
     printf("Server has detached its shared memory...\n");
     shmctl(mcID, IPC_RMID, NULL);
     printf("Server has removed its shared memory...\n");
     printf("Server exits...\n");
     return 0;
}