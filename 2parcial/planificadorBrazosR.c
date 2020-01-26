//
// Mauricio Leiton Lázaro(mdleiton)
// Fecha: 12/1/20.
//
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/signal.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "PedidosLista.h"

struct Cola *cola;
struct Pedidos *pedidos;
pthread_mutex_t mutex_pedidos;

/**
 * Parámetros de inicialización */
int n_brazos;
int n_pedidosxbrazo;
int esquema;

struct BrazoRobotico *brazosCola;
struct BrazoRobotico * next;   // se usa en ESQUEMA = ESQUEMA_IGUAL_X_PEDIDOS
pthread_mutex_t mutex;

/**
 * Estructura que son compartidas entre procesos */
struct Informacion *informacion;
struct Proceso* procesos;
int mcID2;
int mcID;

int client_sockfd;
struct sigaction act;

int cores = 1;
cpu_set_t cpuset;

/**
 * rutina a ejecutarse por cada hilo de brazo robótico que se inicie */
void *threadBrazoRobotico(void *arg){
    // afinidad
    CPU_ZERO(&cpuset);
    if(cores>1){
        setAffinity(CORE_BRAZOS, cpuset);
    }
    struct BrazoRobotico *brazo = (struct BrazoRobotico*) arg;
    printf("Brazo con id: %d iniciado.\n", brazo->id);

    char data[100];
    char* elemento;
    int id, totalItems, resultado = 2;
    while(resultado != 0){
        pthread_mutex_lock(&brazo->mutex);
        if(brazo->estado == BRAZO_SUSPENDIDO){
            pthread_cond_wait(&brazo->estadoCon, &brazo->mutex);
        }
        pthread_mutex_unlock(&brazo->mutex);
        memset(data, 0, sizeof(data));
        resultado = dequeue(brazo->cola, data, 0);  // QUE SI ESTA AQUI CUANDO LO VOY A SUSPENDER
        pthread_mutex_lock(&brazo->mutex);
        if(brazo->estado == BRAZO_SUSPENDIDO){
            enqueue(data, brazo->cola);
            pthread_cond_wait(&brazo->estadoCon, &brazo->mutex);
            continue;
        }
        pthread_mutex_unlock(&brazo->mutex);
        if(resultado == 1){
            char* ptr = data;
            elemento = strtok_r(ptr, "-", &ptr);
            if(elemento != NULL){
                id = atoi(elemento);
                totalItems = 0;
                elemento = strtok_r(ptr, "-", &ptr);
                while(elemento != NULL){
                    int items = atoi(elemento);
                    if(items == 0){
                        int ingreso = 0;
                        for (int i = 0; i < n_pedidosxbrazo; i++) {
                            if (brazo->pedidos[i].id == id) {
                                brazo->pedidos[i].totalPendientes -= totalItems;

                                pthread_mutex_lock(&mutex);
                                brazo->pendientesItem-=totalItems;
                                totalItems = 0;
                                if(brazo->pedidos[i].totalPendientes == 1){
                                    brazo->totalPaquetesProcesados += 1;
                                    printf("Brazo %d | total Procesados %d  | pedido: %d finalizado.\n", brazo->id, brazo->totalPaquetesProcesados, id);
                                    brazo->cantPedidos -= 1;
                                    brazo->pendientesItem-=1;
                                    brazo->pedidos[i].id = -1;
                                    brazo->pedidos[i].totalPendientes = 0;
                                    if(brazo->cantPedidos < n_pedidosxbrazo) brazo->estado = BRAZO_DISPONIBLE;
                                    updateBrazo(&brazosCola, brazo->id, esquema);
                                    pthread_mutex_unlock(&mutex);

                                    pthread_mutex_lock(&mutex_pedidos);
                                    struct Pedido *pedido = find(id, pedidos);
                                    pedido->estado = PEDIDO_FINALIZADO;
                                    pedido->brazo = NULL;
                                    delete(id, pedidos);
                                    pthread_mutex_unlock(&mutex_pedidos);

                                    sem_wait(&informacion->mutex);
                                    informacion->pedidosFinalizados++;
                                    sem_post(&informacion->mutex);
                                    ingreso = 1;
                                    break;
                                }else{
                                    pthread_mutex_unlock(&mutex);
                                    char rest[100];
                                    memset(rest, 0, sizeof(rest));
                                    sprintf(rest,"%d-0",id);
                                    enqueue(rest, brazo->cola);
                                    ingreso = 2;
                                    break;
                                }
                            }
                        }
                        if(ingreso >= 1){
                            totalItems=0;
                            break;
                        }
                    }else{
                        totalItems+=items;
                    }
                    elemento = strtok_r(ptr, "-", &ptr);
                }
                if(totalItems == 0){
                    continue;
                }else{
                    brazo->pendientesItem-=totalItems;
                    for (int i = 0; i < n_pedidosxbrazo; i++) {
                        if(brazo->pedidos[i].id == id){
                            brazo->pedidos[i].totalPendientes -= totalItems;
                            brazo->totalPaquetesProcesados += 1;
                            printf("NUEVO ITEM -> Brazo %d | total Procesados %d | pedido:%d | items agregados/pendientes :%d/%d  \n",
                                   brazo->id, brazo->totalPaquetesProcesados, id, totalItems, brazo->pedidos[i].totalPendientes);
                        }
                    }
                }
            }
        }
    }
    pthread_exit(NULL);
}

/**
 * rutina a ejecutarse para recibir nuevos paquetes desde el socket. */
void *threadRecepcionPaquetes(void *arg){
    //afinidad
    CPU_ZERO(&cpuset);
    if(cores>1){
        setAffinity((CORE_RECEPCION % cores), cpuset);
    }

    int server_sockfd;
    int server_len;
    int rc;
    unsigned client_len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    char buffer[50], data[MAX_BUFFER];

    //Remove any old socket and create an unnamed socket for the server.
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htons(INADDR_ANY);
    server_address.sin_port = htons(7734);
    server_len = sizeof(server_address);

    rc = bind(server_sockfd, (struct sockaddr *) &server_address, server_len);
    printf("RC from bind = %d\n", rc);

    //Create a connection queue and wait for clients
    rc = listen(server_sockfd, 50);
    printf("RC from listen = %d\n", rc );

    // recibiendo pedidos
    while(1){
        client_len = sizeof(client_address);
        client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_address, &client_len);
        printf("after accept()... client_sockfd = %d\n", client_sockfd);
        while(1){
            memset(buffer,0,sizeof(buffer));
            rc = read(client_sockfd, &buffer,sizeof(buffer));
            if (rc == 0){
                printf("\n\nEsperando mas mensaje. Ejecute otra vez ./eorder.\n\n");
                break;
            }
            memset(data, 0, sizeof(data));
            sprintf(data,"%s",buffer);

            enqueue(data, cola);
            //printf("[Data = %s rc=%d]\n",buffer,rc);
            // se puede tratar de asignar de una un brazo robotico
        }
    }
    printf("Finalizando hilos de recepción de nuevos paquetes.\n");
    pthread_exit(NULL);
}

/**
 * Función que se encarga de asignarle a un pedido un brazo robótico disponible
 * @param pedido pedido al cual se le desea asignar un brazo robótico
 * @return 1 en caso de que exista brazo robótico disponible.
 */
int asignarBrazo(struct Pedido** pedido){
    struct BrazoRobotico* brazo = NULL;
    pthread_mutex_lock(&mutex);
    if(esquema == ESQUEMA_IGUAL_X_PEDIDOS){
        if(next == NULL){
            next =  brazosCola;
        }else{
            next= next->siguiente;
        }
        while(next != NULL){
            if(next->cantPedidos < n_pedidosxbrazo && next->estado == BRAZO_DISPONIBLE){
                brazo = next;
                next = brazo;
                break;
            }
            next =  next->siguiente;
        }
    }else{
        brazo = getBrazo(&brazosCola, esquema, n_pedidosxbrazo);
    }
    if(brazo == NULL){
        pthread_mutex_unlock(&mutex);
        return 0;
    }else{
        if(brazo->estado == BRAZO_SUSPENDIDO){
            pthread_mutex_unlock(&mutex);
            return 0;
        }
        brazo->cantPedidos += 1;
        if(brazo->cantPedidos == n_pedidosxbrazo) brazo->estado = BRAZO_OCUPADO;
        pthread_mutex_lock(&mutex_pedidos);
        (*pedido)->brazo = brazo;
        brazo->pendientesItem += (*pedido)->totalPendientes;
        for (int i = 0; i < n_pedidosxbrazo; i++) {
            if (brazo->pedidos[i].id == -1) {
                brazo->pedidos[i].id = (*pedido)->id;
                brazo->pedidos[i].totalPendientes = (*pedido)->totalPendientes;
                break;
            }
        }
        if((*pedido)->totalPendientes != (*pedido)->total){
            printf("planificador-> main, Pedido reasignado: %d | total items:%d | total pendientes: %d | asignado a brazo %d\n", (*pedido)->id,
                   (*pedido)->total, (*pedido)->totalPendientes, brazo->id);
        }else{
            printf("planificador-> main, Nuevo Pedido     : %d | total items:%d | total pendientes: %d | asignado a brazo %d\n", (*pedido)->id,
                   (*pedido)->total, (*pedido)->totalPendientes, brazo->id);
        }
        pthread_mutex_unlock(&mutex_pedidos);
        updateBrazo(&brazosCola, brazo->id, esquema);
        pthread_mutex_unlock(&mutex);
        return 1;
    }
}

/**
 * rutina a ejecutarse para desencolar datos. */
void *threadprocesamientoPaquetes(void *arg){
    //afinidad
    CPU_ZERO(&cpuset);
    if(cores>1){
        setAffinity( (CORE_PLANIFICACION % cores), cpuset);
    }
    char* elemento;
    int id, resultado;
    char data[MAX_BUFFER];
    while(1){
        memset(data, 0, sizeof(data));
        resultado = dequeue(cola, data, 0);
        if(resultado==0) break;
        char* ptr = data;
        elemento = strtok_r(ptr, "-", &ptr);
        if(elemento != NULL){
            id = atoi(elemento);
            pthread_mutex_lock(&mutex_pedidos);
            struct Pedido *pedido = find(id, pedidos);
            pthread_mutex_unlock(&mutex_pedidos);
            if(pedido != NULL) {
                sprintf(data,"%d-%s",id,ptr);
                if(pedido->brazo == NULL){
                    enqueue(data, cola);
                    asignarBrazo(&pedido);
                }else{
                    enqueue(data, pedido->brazo->cola);
                }
            }else{
                pthread_mutex_lock(&mutex_pedidos);
                pedido = insertarPrimero(id, atoi(ptr), pedidos);
                pthread_mutex_unlock(&mutex_pedidos);
                if( asignarBrazo(&pedido) == 0){
                    printf("ERROR: planificador-> main, Intento fallido de asignación de brazo, brazos ocupados.\n");
                }
            }
        }else{
            printf("ERROR: planificador-> main, Formato Incorrecto.\n");
            continue;
        }
    }
    pthread_exit(NULL);
}

/**
 *  Función que finaliza adecuadamente el proceso. */
void finalizar(){
    procesos->pidPlanificador = -1;
    sem_destroy(&procesos->mutex);
    shmdt((void *) procesos);
    shmctl(mcID, IPC_RMID, NULL);

    sem_destroy(&informacion->mutex);
    shmdt((void *) informacion);
    shmctl(mcID2, IPC_RMID, NULL);
    printf("planificador -> finalizar, memorias compartidas han sido eliminadas.\n");
    exit(1);
}

/**
 *  Función que maneja la señal SIGINT. Notifica su finalización al proceso admin en caso de estar ejecutándose. */
void manejadorSIGINT(int signum, siginfo_t *info, void *ptr){
    enviarSenal(procesos->pidAdmin, 1, EXITPROGRAMA_PLANIF);
    finalizar();
}

/**
 *  Función que maneja la señal CREAR_BRAZO. */
void manejadorCrear(int signum, siginfo_t *info, void *ptr){
    printf("manejadorCrear, señal recibida para crear nuevo brazo.\n");
    pthread_t thread;
    pthread_mutex_lock(&mutex);
    n_brazos+=1;
    struct BrazoRobotico *t = pushP(&brazosCola, n_brazos, n_pedidosxbrazo, esquema);
    int estado_hilo = pthread_create(&thread, NULL, threadBrazoRobotico, (void*) t);
    if (estado_hilo != 0){
        printf("Error: planificador-> main, error al crear hilo.\n");
    }
    pthread_mutex_unlock(&mutex);
    sem_wait(&informacion->mutex);
    informacion->brazosActivos = n_brazos;
    sem_post(&informacion->mutex);
}

/**
 *  Función que maneja la señal SUSPENDER_BRAZO. */
void manejadorSuspender(int signum, siginfo_t *info, void *ptr){
    int brazoId = info->si_value.sival_int;
    printf("manejadorSuspender, señal recibida para suspender el brazo con id: %d.\n", brazoId);
    pthread_mutex_lock(&mutex);
    struct BrazoRobotico* brazo = getBrazobyId(&brazosCola, brazoId, esquema);
    pthread_mutex_unlock(&mutex);
    if(brazo->estado != BRAZO_SUSPENDIDO){
        pthread_mutex_lock(&brazo->mutex);
        brazo->estado = BRAZO_SUSPENDIDO;
        updateBrazo(&brazosCola, brazo->id, esquema);
        pthread_mutex_unlock(&brazo->mutex);

        pthread_mutex_lock(&mutex_pedidos);
        for (int i = 0; i < n_pedidosxbrazo; i++) {
            if(brazo->pedidos[i].id == -1) continue;
            struct Pedido *pedido = find(brazo->pedidos[i].id, pedidos);
            pedido->brazo = NULL;
            pedido->totalPendientes = brazo->pedidos[i].totalPendientes;
            brazo->pendientesItem -= brazo->pedidos[i].totalPendientes;
            brazo->pedidos[i].id = -1;
            brazo->pedidos[i].totalPendientes = 0;
        }
        brazo->cantPedidos = 0;
        if(brazo->cola->contador > 0){
            // se puede crear una nueva cola
            char data[100];
            int cant = brazo->cola->contador;
            for(int i=0; i< cant; i++){
                memset(data, 0, sizeof(data));
                dequeue(brazo->cola, data, 0);  // QUE SI ESTA AQUI CUANDO LO VOY A SUSPENDER
                enqueue(data, cola);
            }
        }
        pthread_mutex_unlock(&mutex_pedidos);
        printf("manejadorSuspender, el brazo con id: %d, se ha suspendido correctamente.\n", brazoId);

        sem_wait(&informacion->mutex);
        informacion->brazosActivos = informacion->brazosActivos - 1;
        informacion->brazosSuspendidos = informacion->brazosSuspendidos + 1;
        sem_post(&informacion->mutex);
    }else{
        printf("manejadorSuspender, el brazo con id: %d, ya se encuentra en modo suspendido.\n", brazoId);
    }
}

/**
 *  Función que maneja la señal REANUDAR_BRAZO. */
void manejadorReanudar(int signum, siginfo_t *info, void *ptr){
    int brazoId = info->si_value.sival_int;
    printf("manejadorReanudar, señal recibida para reanudar el brazo con id: %d.\n", brazoId);
    pthread_mutex_lock(&mutex);
    struct BrazoRobotico* brazo = getBrazobyId(&brazosCola, brazoId, esquema);
    pthread_mutex_unlock(&mutex);
    if(brazo->estado == BRAZO_SUSPENDIDO){
        pthread_mutex_lock(&brazo->mutex);
        if(brazo->cantPedidos < n_pedidosxbrazo){
            brazo->estado = BRAZO_DISPONIBLE;
        }else{
            brazo->estado = BRAZO_OCUPADO;
        }
        updateBrazo(&brazosCola, brazo->id, esquema);
        pthread_cond_signal(&brazo->estadoCon);
        pthread_mutex_unlock(&brazo->mutex);
        printf("manejadorReanudar, el brazo con id: %d, se ha reanudado correctamente.\n", brazoId);
        sem_wait(&informacion->mutex);
        informacion->brazosActivos = informacion->brazosActivos + 1;
        informacion->brazosSuspendidos = informacion->brazosSuspendidos - 1;
        sem_post(&informacion->mutex);
    }else{
        printf("manejadorReanudar, el brazo con id: %d, ya se encuentra en operativo.\n", brazoId);
    }
}

/**
 *  Función que maneja la señal EXITPROGRAMA_ADMIN. */
void manejadorExitAdmin(int signum, siginfo_t *info, void *ptr){
    printf("manejadorExitAdmin, señal recibida, admin ha finalizado.\n");
}

int main(int argc, char *argv[]){
    // afinidad
    cores = numeroNucleos();

    // manejadores de senales.
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = manejadorCrear;
    act.sa_flags = CREAR_BRAZO;
    sigaction(CREAR_BRAZO, &act, NULL);

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = manejadorSuspender;
    act.sa_flags = SUSPENDER_BRAZO | SA_SIGINFO;
    sigaction(SUSPENDER_BRAZO, &act, NULL);

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = manejadorReanudar;
    act.sa_flags = REANUDAR_BRAZO | SA_SIGINFO;
    sigaction(REANUDAR_BRAZO, &act, NULL);

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = manejadorExitAdmin;
    act.sa_flags = EXITPROGRAMA_ADMIN | SA_SIGINFO;
    sigaction(EXITPROGRAMA_ADMIN, &act, NULL);

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = manejadorSIGINT;
    act.sa_flags = SIGINT;
    sigaction(SIGINT, &act, NULL);

    // verificando ingreso correcto de parámetros
    if (argc != 4) {
        printf("Usar: %s N_BRAZOS N_PEDIDOSXBRAZO ESQUEMAPLANIFICACION\n", argv[0]);
        exit(1);
    }
    n_brazos = atoi(argv[1]);
    if(n_brazos == 0){
        printf("Error: planificador-> main, Parámetro N_BRAZOS inválido.\n");
        exit(1);
    }
    n_pedidosxbrazo = atoi(argv[2]);
    if(n_pedidosxbrazo == 0){
        printf("Error: planificador-> main, Parámetro N_PEDIDOSXBRAZO inválido.\n");
        exit(1);
    }
    esquema = atoi(argv[3]);
    if(esquema == 0 || esquema > 3){
        printf("Error: planificador-> main, Parámetro ESQUEMAPLANIFICACION inválido.\n");
        exit(1);
    }

    // Acceso a memoria compartida que contiene los PID de los procesos.
    mcID = shmget(ID_MC, sizeof(struct Proceso), IPC_CREAT | 0666);
    if (mcID < 0) {
        printf("Error: planificador -> main, Inválido identificador de memoria compartido.\n");
        exit(1);
    }
    procesos = (struct Proceso *) shmat(mcID, NULL, 0);
    if ((int) procesos == -1) {
        printf("Error: planificador->main, shmat error.\n");
        exit(1);
    }
    sem_init(&procesos->mutex, 0, 1);
    sem_wait(&procesos->mutex);
    procesos->pidPlanificador = syscall(SYS_gettid);
    procesos->pidGenerador = -1;
    procesos->pidAdmin = -1;
    sem_post(&procesos->mutex);

    //Acceso a memoria compartida que contiene informacion general a compartir.
    mcID2 = shmget(ID_MC_INFO, sizeof(struct Informacion), IPC_CREAT | 0666);
    if (mcID2 < 0) {
        printf("Error: planificador -> main, Inválido identificador de memoria compartido.\n");
        exit(1);
    }
    informacion = (struct Informacion*) shmat(mcID2, NULL, 0);
    if ((int) informacion == -1) {
        printf("Error: planificador->main, shmat error.\n");
        exit(1);
    }
    sem_init(&informacion->mutex, 0, 1);
    sem_wait(&informacion->mutex);
    informacion->pedidosFinalizados = 0;
    informacion->brazosActivos = n_brazos;
    informacion->brazosSuspendidos = 0;
    sem_post(&informacion->mutex);

    // definiendo la cola para almacenar los pedidos que lleguen
    cola = (struct Cola*)malloc(sizeof(struct Cola));
    cola->primero = NULL;
    cola->final = NULL;
    cola->contador = 0;
    pthread_mutex_init(&cola->mutex, NULL);

    // definiendo la lista para almacenar los pedidos que lleguen
    pedidos = (struct Pedidos*)malloc(sizeof(struct Pedidos));
    pedidos->primero = NULL;
    pedidos->final = NULL;

    //construcción y definición de los brazos robóticos
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_t thread;
    brazosCola = nuevaCola(1, n_pedidosxbrazo, esquema);
    int estado_hilo = pthread_create(&thread, &attr, threadBrazoRobotico, (void*) brazosCola);
    if (estado_hilo != 0){
        printf("Error: planificador-> main, error al crear hilo.\n");
    }
    for (int id = 2; id <= n_brazos; id++) {
        struct BrazoRobotico *t = pushP(&brazosCola, id, n_pedidosxbrazo, esquema);
        estado_hilo = pthread_create(&thread, &attr, threadBrazoRobotico, (void*) t);
        if (estado_hilo != 0){
            printf("Error: planificador-> main, error al crear hilo.\n");
        }
    }
    next = brazosCola;

    // Obtiene los pedidos por socket
    estado_hilo = pthread_create(&thread, NULL, threadRecepcionPaquetes, NULL);
    if (estado_hilo != 0){
        printf("Error: planificador-> main, error al crear hilo.\n");
    }

    // desencolando mensajes
    estado_hilo = pthread_create(&thread, NULL, threadprocesamientoPaquetes, NULL);
    if (estado_hilo != 0){
        printf("Error: planificador-> main, error al crear hilo.\n");
    }

    while(1){
        pause();
    }
    return 0;
}
/*
 * crear brazo
 * implementar esquema 2 y 3
 */