#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include "PedidosLista.h"

struct Cola *cola;
struct Pedidos *pedidos;
pthread_mutex_t mutex_pedidos;

/**
 * Parámetros de inicialización
 **/
int n_brazos;
int n_pedidosxbrazo;
int esquema;

struct Informacion *informacion;

struct BrazoRobotico *brazosCola;
pthread_mutex_t mutex;

/** Estructura que contiene la informacion de los pid de los procesos */
struct Proceso* procesos;
int mcID;
int mcID2;

void *thread_brazo_robotico(void *arg){
    struct BrazoRobotico *brazo = (struct BrazoRobotico*) arg;
    printf("Brazo con id: %d iniciado.\n", brazo->id);
    pthread_t thread = pthread_self();
    // asignarlo a un especifico core

    char data[100];
    char* elemento;
    int id, totalItems, resultado = 2;
    while(resultado != 0){
        memset(data, 0, sizeof(data));
        resultado = dequeue(brazo->cola, data, 0);
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
                                    printf("Brazo %d | pedido: %d finalizado.\n", brazo->id, id);
                                    brazo->cantPedidos -= 1;
                                    // deberia actualizar brazos si es otro esquema , y en la funcion no hacer nada de ser el caso
                                    brazo->pendientesItem-=1;
                                    brazo->pedidos[i].id = -1;
                                    brazo->pedidos[i].totalPendientes = 0;
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
                            printf("NUEVO ITEM -> Brazo %d | pedido:%d | items agregados/pendientes :%d/%d  \n",
                                   brazo->id, id, totalItems, brazo->pedidos[i].totalPendientes);
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

int asignarBrazo(struct Pedido** pedido){
    pthread_mutex_lock(&mutex);
    struct BrazoRobotico* brazo = getBrazo(&brazosCola, esquema, n_pedidosxbrazo);
    if(brazo == NULL){
        pthread_mutex_unlock(&mutex);
        return 0;
    }else{
        brazo->cantPedidos += 1;
        pthread_mutex_lock(&mutex_pedidos);
        (*pedido)->brazo = brazo;
        brazo->pendientesItem += (*pedido)->total;
        for (int i = 0; i < n_pedidosxbrazo; i++) {
            if (brazo->pedidos[i].id == -1) {
                brazo->pedidos[i].id = (*pedido)->id;
                brazo->pedidos[i].totalPendientes = (*pedido)->total;
                break;
            }
        }
        printf("planificador-> main, Nuevo Pedido: %d | total items:%d | asignado a brazo %d\n", (*pedido)->id,
               (*pedido)->total, brazo->id);
        pthread_mutex_unlock(&mutex_pedidos);  // ojo
        pthread_mutex_unlock(&mutex);
        return 1;
    }
}

/**
 *  Función que finaliza adecuadamente el proceso.
 */
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
 *  Función que maneja la señal SIGINT. Notifica su finalización al proceso admin en caso de estar ejecutándose.
 */
void manejadorSIGINT(int signum, siginfo_t *info, void *ptr){
    enviarSenal(procesos->pidAdmin, 1, EXITPROGRAMA_PLANIFICADOR);
    finalizar();  // finaliza debidamente
}

/**
 *  Función que maneja la señal CREAR_BRAZO.
 */
void manejadorSIGCREAR_BRAZO(int signum, siginfo_t *info, void *ptr){
    printf("manejadorSIGCREAR_BRAZO, señal recibida para crear nuevo brazo.");
}

/**
 *  Función que maneja la señal SUSPENDER_BRAZO.
 */
void manejadorSIGSUSPENDER_BRAZO(int signum, siginfo_t *info, void *ptr){
    printf("manejadorSIGSUSPENDER_BRAZO, señal recibida para suspender el brazo xx.");
}

/**
 *  Función que maneja la señal REANUDAR_BRAZO.
 */
void manejadorSIGREANUDAR_BRAZO(int signum, siginfo_t *info, void *ptr){
    printf("manejadorSIGSUSPENDER_BRAZO, señal recibida para reanudar el brazo xx.");
}

/**
 *  Función que maneja la señal EXITPROGRAMA_ADMIN.
 */
void manejadorSIGEXITPROGRAMA_ADMIN(int signum, siginfo_t *info, void *ptr){
    printf("manejadorSIGEXITPROGRAMA_ADMIN, señal recibida para finalizar.");
}

int main(int argc, char *argv[]){
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

    // manejadores de senales.
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = manejadorSIGINT;
    act.sa_flags = SIGINT;
    sigaction(SIGINT, &act, NULL);

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = manejadorSIGCREAR_BRAZO;
    act.sa_flags = CREAR_BRAZO | SA_SIGINFO;
    sigaction(CREAR_BRAZO, &act, NULL);

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = manejadorSIGSUSPENDER_BRAZO;
    act.sa_flags = SUSPENDER_BRAZO | SA_SIGINFO;
    sigaction(SUSPENDER_BRAZO, &act, NULL);

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = manejadorSIGREANUDAR_BRAZO;
    act.sa_flags = REANUDAR_BRAZO | SA_SIGINFO;
    sigaction(REANUDAR_BRAZO, &act, NULL);

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = manejadorSIGEXITPROGRAMA_ADMIN;
    act.sa_flags = EXITPROGRAMA_ADMIN | SA_SIGINFO;
    sigaction(EXITPROGRAMA_ADMIN, &act, NULL);

    // definiendo la cola para almacenar los pedidos que lleguen
    cola = (struct Cola*)malloc(sizeof(struct Cola));
    cola->primero = NULL;
    cola->final = NULL;
    cola->contador = 0;
    char data[MAX];

    // definiendo la lista para almacenar los pedidos que lleguen
    pedidos = (struct Pedidos*)malloc(sizeof(struct Pedidos));
    pedidos->primero = NULL;
    pedidos->final = NULL;

    //construcción y definición de los brazos robóticos
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_t thread;
    brazosCola = nuevaCola(1, n_pedidosxbrazo, esquema);
    int estado_hilo = pthread_create(&thread, &attr, thread_brazo_robotico, (void*) brazosCola);
    if (estado_hilo != 0){
        printf("Error: planificador-> main, error al crear hilo.\n");
    }
    for (int id = 2; id <= n_brazos; id++) {
        struct BrazoRobotico *t = pushP(&brazosCola, id, n_pedidosxbrazo, esquema);
        estado_hilo = pthread_create(&thread, &attr, thread_brazo_robotico, (void*) t);
        if (estado_hilo != 0){
            printf("Error: planificador-> main, error al crear hilo.\n");
        }
    }

    // Obtiene los pedidos por socket
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

    // recibiendo pedidos
	while(1){
		memset(buffer,0,sizeof(buffer));
		rc = read(client_sockfd, &buffer,sizeof(buffer));
		if (rc == 0) break;
        memset(data, 0, sizeof(data));
        sprintf(data,"%s",buffer);
        enqueue(data, cola);
		//printf("[Data = %s rc=%d]\n",buffer,rc);
	}

    // desencolando mensajes. esto debe ir en otro hilo
    int resultado;
    char* elemento;
    int id;
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
                //printf(" planificador-> main, Pedido ya registrado: (%d,%s) \n",pedido->id,ptr);
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
    printf("planificador-> main, server exiting\n");
    while(1){
        sleep(2);
    }
    close(client_sockfd);
    return 0;
}

/* necesito crear hilos que encolen la informacion y dejar al hilo principal que reciba senales.
 * no puede esperar mucho esperar en bind del socket.*/