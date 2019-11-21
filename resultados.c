#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include  <stdio.h>
#include  <stdlib.h>

#include  "Estructuras.h"

int  main(void){
     int            mcID;
     struct Datos  *datos;
     
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
     printf("resultado->main, Contenido: %d %d %d %d \n", datos->data[0], datos->data[1], datos->data[2], datos->data[3]);

     datos->status = TAKEN;
     printf("resultado->main, Finalización de cálculos.\n");
     shmdt((void *) datos);
     printf("resultado->main, Desvinculado la memoria compartida\n");
     return 0;
}
