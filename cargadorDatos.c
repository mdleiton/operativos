#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  "Estructuras.h"

int  main(int  argc, char *argv[]){
     int            mcID;
     struct Datos  *datos;

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
     datos->data[0] = 1;
     datos->data[1] = 1;
     datos->data[2] = 1;
     datos->data[3] = 1;
     printf("cargadorDatos->main, has filled %d %d %d %d to shared memory...\n",
            datos->data[0], datos->data[1], 
            datos->data[2], datos->data[3]);
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