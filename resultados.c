#define _GNU_SOURCE
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include  <stdio.h>
#include  <stdlib.h>
#include <pthread.h>
#include  "Estructuras.h"
#include <limits.h> 
#include <semaphore.h>

// VARIABLES GLOBALES
struct Datos  *datos;
int distance[MAX][MAX], pred[MAX][MAX];
int frecuencia[MAX];
int cores = 1;
cpu_set_t cpuset;
pthread_t threads[MAX];

// Las siguientes variables/estructuras globales son utilizadas en el ordenamiento.
struct index{int p,r;};

 struct nodo  {
    int elemento;        // valor de frecuencia de cada nodo
    int nodo_original;   // indice del nodo original
 };

struct nodo a[MAX];
struct nodo b[MAX];     // arreglo para guardar resultados de ordenamiento temporal

/*
 *  Función que realiza el calculo de la distancia más corta desde el nodo inicio al resto de nodos.
 * @param G matriz M*M que representa un grafo.
 * @param n indica la cantidad de nodos que existen.
 * @param startnode indica el nodo inicio 
 * @return 0 en caso de que todo se ejecute correctamente.
 */
int dijkstra(int G[MAX][MAX],int n,int startnode){
     int cost[MAX][MAX];
     int visited[MAX],count,mindistance,nextnode,i,j;
     
     //pred[] stores the predecessor of each node count gives the number of nodes seen so far
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
               if(distance[startnode][i]<mindistance && visited[i]== 0){
                    mindistance=distance[startnode][i];
                    nextnode=i;
               }
               
               //check if a better path exists through nextnode            
               visited[nextnode]=1;
               for(i=0;i<n;i++){
                    if(!visited[i]){
                         if(mindistance+cost[nextnode][i]<distance[startnode][i]){
                              distance[startnode][i]=mindistance+cost[nextnode][i];
                              pred[startnode][i]=nextnode;
                         }
                    }
               }  
          count++;       
     }
     return 0;
}

/*
 *  Función a ejecutar por cada nodo del grafo, de forma concurrente y paralela en caso de existir multiples cores.
 * @param parametro que indica el core id asignado para definir su afinidad.
 */
void *thread_routine(void *arg){
     int coreIndex = ((int)arg % cores);
     pthread_t thread = pthread_self();
     cpu_set_t cpusetThread;
     CPU_ZERO(&cpusetThread);
     CPU_SET(coreIndex, &cpusetThread);
     int resultAffinity = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpusetThread);
     if (resultAffinity != 0){
          printf("resultado->thread_routine, error al setear la afinidad al hilo\n");
     }
     dijkstra(datos->grafo,datos->cantidadNodos,(int)arg);
     return NULL;
}

/*
 *  Función que obtiene la cantidad de veces que se pasa por esa intersección. Lo guarda en un arreglo global llamado frecuencia.
 */
void obtenerFrecuencia(){
     int k, startnode;
     for(int j=0;j<MAX;j++){
          startnode = j;     
          for(int i=0;i<j;i++){
               if(i!=startnode){
                    //printf("\nDistance of node%d=%d \nPath=%d",i,distance[startnode][i], i);
                    k=i;
                    do{
                         k=pred[startnode][k];
                         if(k!=startnode){
                              frecuencia[k]+=1;
                         }
                         //printf("<-%d.",k);
                    }while(k!=startnode);
               }
          }
     }
}

/*****************************************************************************

      La siguiente función fueron obtenidas inicialmente del siguiente sitio. Se modificaron para adaptarse a lo que se necesitaba  
      y para que sea multithilo.
       Fuente: https://www.geeksforgeeks.org/merge-sort/ 

*****************************************************************************/

/*
 *  Función para ordenar un arreglo.
 *  @param vpid estructura con el indice de inicio/fin de un subarreglo. 
 *  @return .
 */
void* merge_sort(void* param){
     struct index* pr = (struct index*) param;
     int p = pr->p,  r = pr->r , ret1,ret2;
     if (p==r)
          pthread_exit(0);

     pthread_t thread1,thread2;
     struct index pr1,pr2;
     int q = (p+r)/2;
     pr1.p = p;    pr1.r = q;
     pr2.p = q+1;  pr2.r = r;

     ret1 = pthread_create(&thread1,NULL,merge_sort,(void*) &pr1);
     if (ret1>0)
          printf("failed to create new thread 1\n");

     ret2 = pthread_create(&thread2,NULL,merge_sort,(void*) &pr2);
     if (ret2>0)
          printf("failed to create new thread 2\n");

     pthread_join(thread1,NULL);
     pthread_join(thread2,NULL);

     int k = p ,i = p ,j = q+1;
     while (i<=q && j<=r){
          if (a[i].elemento < a[j].elemento){
               b[k].elemento = a[i].elemento;
               b[k++].nodo_original = a[i++].nodo_original;
          }else{
               b[k].elemento = a[j].elemento;
               b[k++].nodo_original = a[j++].nodo_original;
          }
     }
     for (; i<=q ; i++){
          b[k].elemento = a[i].elemento;
          b[k++].nodo_original = a[i].nodo_original;
     }
     for (; j<=r ; j++){
          b[k].elemento = a[j].elemento;
          b[k++].nodo_original = a[j].nodo_original;
     }

     for (i= p ; i <= r ;i++){
          a[i].elemento = b[i].elemento;
          a[i].nodo_original = b[i].nodo_original;
     }

     pthread_exit(0);
     return NULL;
}

/*
 *  Función que presenta los 3 mejores lugares para estacionamientos.
 */
void resultados(){
     for(int i=0;i<MAX;i++){
          frecuencia[i] = 0;
     }
     obtenerFrecuencia();
     struct index start;
     start.p = 0;    start.r =MAX-1;
     for(int i = 0; i < MAX; i++){
          struct nodo startS;
          startS.elemento = frecuencia[i];
          startS.nodo_original = i;
          a[i] = startS;
          b[i] = startS;
     }
     
     pthread_t start_thread;
     pthread_create(&start_thread,NULL,merge_sort,&start);
     pthread_join(start_thread,NULL);
     /* para probar:
     for (int i = 0; i < MAX; i++)
          printf("%d ",a[i].elemento);
     printf("\n");
     for (int i = 0; i < MAX; i++)
          printf("%d ",a[i].nodo_original);
     printf("\n");
     */
     printf("Los mejores lugares son:\n");
     for (int i = 1; i < 4; i++){
          printf("%d.-Nodo %d\n", i,a[MAX-i].nodo_original);
     }
     printf("\n");
}

/*
 *  Función que finaliza adecuadamente el proceso.
 */
void finalizar(){
     long pid = syscall(SYS_gettid);
     actualizarInfoProceso(PROGRAMA_RESULTADO, pid, DISABLE);
     shmdt((void *) datos);
     printf("resultado->finalizar, Desvinculado memoria compartida\n");
     exit(1);
}

/*
 *  Función que realiza los calculos creando hilos..
 */
void consumir(){
     int status;
     void *estado_hilo;
     printf("resultado->consumir, Contenido listo para realizar calculos.\n");
     for(int i=0;i<MAX;i++){
          status = pthread_create(&threads[i], NULL, thread_routine, (void*) i);
          if (status != 0){
               printf("resultado->consumir,error Create thread");
          }
          frecuencia[i]=0;
     }
     for(int i=0;i<MAX;i++){
        status = pthread_join(threads[i], &estado_hilo);
        if (status != 0) {
            printf("resultado->consumir, ERROR uno de los hijos falló.");
        }
     }
}

/*
 *  Función que inicia la operación de consumir.
 */
void iniciarConsumir(){
     sem_wait(&datos->mutex);
     consumir();
     sem_post(&datos->mutex);
     resultados();
}

/*
 *  Función que intenta acceder a la memoria compartida.
 */
int iniciarMemoriaCompartida(){
     int mcID = shmget(ID_MC, sizeof(struct Datos), 0666);
     if (mcID < 0) {
          printf("resultado->iniciarMemoriaCompartida, error. inválido identificador de memoria compartido.\n");
          return -1;
     }
     printf("resultado->iniciarMemoriaCompartida, Obtenido un válido identificador de memoria compartido.\n");
     
     datos = (struct Datos *) shmat(mcID, NULL, 0);
     if ((int) datos == -1) {
          printf("resultado->iniciarMemoriaCompartida, shmat error.\n");
          return -1;
     }
     printf("resultado->iniciarMemoriaCompartida, Mapeo a memoria compartida aceptada.\n");
     return 1;
}

/*
 *  Función que maneja la señal SIGNIT. Finaliza adecuadamente el programa,
 */
void manejadorSIGINT(int signum, siginfo_t *info, void *ptr){ 
     finalizar();
}


/*
 *  Función que maneja la señal que indica que el programa cargador ha realizado actualizaciones. 
 *  y se debe realizar los calculos respectivos.
 */
void manejadorConsumir(int signum, siginfo_t *info, void *ptr){ 
     printf("resultado->manejadorConsumir, Actualización de datos recibida.\n");
     iniciarConsumir();
     printf("resultado->manejadorConsumir, Finalización de cálculos.\n");
}

/*
 *  Función que maneja la señal que indica que el programa cargador ha finalizado. 
 *  y ya no se puede acceder a la memoria compartida.
 */
void manejadorExitCargador(int signum, siginfo_t *info, void *ptr){ 
     printf("resultado->manejadorExitCargador, Programa para recibir cargar/actualizar grafo ha finalizado.\n");
     printf("resultado->manejadorExitCargador, Ultimos resultados:\n");
     resultados();
}

/*
 *  Función que maneja la señal que indica que el programa cargador ha iniciado. 
 *  y se puede acceder a la memoria compartida.
 */
void manejadorInitCargador(int signum, siginfo_t *info, void *ptr){ 
     printf("resultado->manejadorInitCargador, Programa para recibir cargar/actualizar grafo se ha iniciado.\n");
     printf("resultado->manejadorInitCargador, Obteniendo resultados:\n");
     int estadoMC = iniciarMemoriaCompartida();
     if(estadoMC != -1){
          iniciarConsumir();  
     }else{
          printf("resultado->manejadorInitCargador, Actualmente no se tiene acceso a la memoria compartida Verificar que proceso cargadorDatos este ejecutándose.\n");
     }
}

int  main(void){
     CPU_ZERO(&cpuset);
     cores = numeroNucleos();

     // afinidad del programa
     if(cores>1){
          pthread_t thread = pthread_self();
          cpu_set_t cpusetThread;
          CPU_ZERO(&cpusetThread);
          CPU_SET(PROGRAMA_RESULTADO, &cpusetThread);
          int resultAffinity = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpusetThread);
          if (resultAffinity != 0){
               printf("resultado->thread_routine, error al setear la afinidad al hilo\n");
          }
     }
     long pid = syscall(SYS_gettid);
     actualizarInfoProceso(PROGRAMA_RESULTADO, pid, ENABLE);

     // manejadores de senales
     struct sigaction act;
     memset(&act, 0, sizeof(act));
     act.sa_sigaction = manejadorSIGINT;
     act.sa_flags = SIGINT;
     sigaction(SIGINT, &act, NULL);

     memset(&act, 0, sizeof(act));
     act.sa_sigaction = manejadorConsumir;
     act.sa_flags = SIGNEWDATA;
     sigaction(SIGNEWDATA, &act, NULL);

     memset(&act, 0, sizeof(act));
     act.sa_sigaction = manejadorInitCargador;
     act.sa_flags = INITPROGRAMA_CARGADOR;
     sigaction(INITPROGRAMA_CARGADOR, &act, NULL);

     memset(&act, 0, sizeof(act));
     act.sa_sigaction = manejadorExitCargador;
     act.sa_flags = EXITPROGRAMA_CARGADOR;
     sigaction(EXITPROGRAMA_CARGADOR, &act, NULL);


     int estadoMC = iniciarMemoriaCompartida();
     if(estadoMC != -1){
          iniciarConsumir(); 
     }else{
          printf("resultado->main, Actualmente no se tiene acceso a la memoria compartida Verificar que proceso cargadorDatos este ejecutándose.\n");
     }
     
     while(1){
          pause();
     }
     finalizar();
}
