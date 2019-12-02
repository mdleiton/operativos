#define _GNU_SOURCE
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include  <stdio.h>
#include  <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <limits.h> 
#include  "Estructuras.h"


// variables globales
struct Datos  *datos;
int mcID;
int cores = 1;
cpu_set_t cpuset;

/*
 *  Función que finaliza adecuadamente el proceso.
 */
void finalizar(){
    long pid = syscall(SYS_gettid);
    actualizarInfoProceso(PROGRAMA_CARGADOR, pid, DISABLE);
    sem_destroy(&datos->mutex);
    shmdt((void *) datos);
    printf("cargadorDatos->finilizar,  liberar memoria compartido\n");
    shmctl(mcID, IPC_RMID, NULL);
    printf("cargadorDatos->finilizar, memoria copartida a sido eliminada.\n");
    exit(1);
}

/*
 *  Función que carga en un matriz un archivo csv.
 *  @param csvFile nombre del archivo csv que se desea cargar en una matriz.
 *  @return 1 cuando se cargue correctamente, -1 en caso de algún error.
 */
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
     return -1;
   }
   datos->cantidadNodos = j;
   return 1;
}

/*
 *  Función que maneja la señal SIGINT. Notifica su finalización al proceso resultado en caso de estar ejecutándose. 
 */
void manejadorSIGINT(int signum, siginfo_t *info, void *ptr){ 
  // envio de señal a otro proceso para notificar que ha finalizado este proceso.
  int send = enviarSenal(PROGRAMA_RESULTADO, 1, EXITPROGRAMA_CARGADOR);
  if(send != 0){
    printf("\ncargadorDatos->manejadorSIGINT, ERROR: no se pudo comunicar con proceso. \n");
  }
  finalizar();  // finaliza debidamente
}

int  main(int  argc, char *argv[]){
    CPU_ZERO(&cpuset);
    cores = numeroNucleos();
    
    // afinidad del programa
    pthread_t thread = pthread_self();
    cpu_set_t cpusetThread;
    CPU_ZERO(&cpusetThread);
    CPU_SET(PROGRAMA_CARGADOR, &cpusetThread);
    int resultAffinity = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpusetThread);
    if (resultAffinity != 0){
        printf("resultado->thread_routine, error al setear la afinidad al hilo\n");
    }
    


    if (argc != 2) {
        printf("Usar: %s csvFile \n", argv[0]);
        exit(1);
    }

    // guardar información del proceso.
    long pid = syscall(SYS_gettid);
    actualizarInfoProceso(PROGRAMA_CARGADOR, pid, ENABLE);

    // envio de senal a proceso resultado para notificar que se puede obtener nuevos datos.

    // manejadores de senales.
     struct sigaction act;
     memset(&act, 0, sizeof(act));
     act.sa_sigaction = manejadorSIGINT;
     act.sa_flags = SIGINT;
     sigaction(SIGINT, &act, NULL);

     // acceso a memoria compartida.
     mcID = shmget(ID_MC, sizeof(struct Datos), IPC_CREAT | 0666);
     if (mcID < 0) {
          printf("cargadorDatos->main, error. inválido identificador de memoria compartido.\n");
          exit(1);
     }
     printf("cargadorDatos->main, Obtenido un válido identificador de memoria compartido.\n");
     
     datos = (struct Datos *) shmat(mcID, NULL, 0);
     if ((int) datos == -1) {
          printf("cargadorDatos->main, shmat error.\n");
          exit(1);
     }
     sem_init(&datos->mutex, 0, 1);
     printf("cargadorDatos->main, Mapeo a memoria compartida aceptada.\n");
     
     sem_wait(&datos->mutex);

     int estado = loadCsv(argv[1]);
     if(estado<=0){
          printf("cargadorDatos->main, error al cargar la matriz.\n");
          sem_post(&datos->mutex);
          finalizar();
     }
     printf("cargadorDatos->main, Se ha cargado el grafo correctamente.\n");
     sem_post(&datos->mutex);
     int send = enviarSenal(PROGRAMA_RESULTADO, 1, INITPROGRAMA_CARGADOR); // notificar a proceso resultados que se esta ejecutando.
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
          if (nodoA > MAX || nodoB > MAX || nodoA < 0 || nodoB < 0){
               printf("cargadorDatos->main, Error nodos deben valores menores a: %d\n", MAX);
               continue;
          }
          printf("%d-%d=%d\n",nodoA,nodoB,peso);
          printf("cargadorDatos->main, actualizando\n");
          sem_wait(&datos->mutex);
          datos->grafo[nodoA][nodoB]=peso;
          sem_post(&datos->mutex); 
          // envio de señal a otro proceso para notificar que se ha actualiza el grafo
          send = enviarSenal(PROGRAMA_RESULTADO, 1, SIGNEWDATA);
          if(send != 0){
            printf("cargadorDatos->main, no se pudo comunicar con proceso. \n");
          }else{
            printf("cargadorDatos->main, actualizado\n");
          }
     }
     finalizar(); // finaliza debidamente
}