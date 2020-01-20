#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "PedidosLista.h"
#include "Util.h"

struct Cola *cola;
struct Pedidos *pedidos;
pthread_mutex_t mutex_pedidos;

/*
 * Parámetros de inicialización
 * */
int n_brazos;
int n_pedidosxbrazo;
int esquema;

struct BrazoRobotico *brazosCola;
pthread_mutex_t mutex;

void *thread_brazo_robotico(void *arg){
    struct BrazoRobotico *brazo = (struct BrazoRobotico*) arg;
    printf("Brazo con id: %d iniciado.\n", brazo->id);
    pthread_t thread = pthread_self();
    // asignarlo a un especifico core

    char data[100];
    int resultado = 2;
    char* elemento;
    int id;
    int totalItems;
    while(resultado != 0){
        memset(data, 0, sizeof(data));
        resultado = dequeue(brazo->cola, data, 0);
        if(resultado == 1 ){
            char* ptr = data;
            elemento = strtok_r(ptr, "-", &ptr);
            if(elemento != NULL){
                id = atoi(elemento);  // id - pedido
                totalItems = 0;
                elemento = strtok_r(ptr, "-", &ptr);
                while(elemento != NULL){
                    //que pasa si viene al final de un arreglo grande
                    int items = atoi(elemento);
                    if(items == 0){
                        printf("Brazo %d | pedido:%d finalizado.\n", brazo->id, id);

                        pthread_mutex_lock(&mutex_pedidos);
                        struct Pedido *pedido = find(id, pedidos);
                        pedido->estado = PEDIDO_FINALIZADO;
                        pedido->brazo= NULL;
                        //delete(id, pedidos);
                        pthread_mutex_unlock(&mutex_pedidos);

                        pthread_mutex_lock(&mutex);
                        brazo->cantPedidos -= 1;
                        for (int i = 0; i < n_pedidosxbrazo; i++) {
                            if(brazo->pedidos[i].id == id){
                                brazo->pedidos[i].id = -1;
                                brazo->pedidos[i].totalPendientes = 0;
                                break;
                            }
                        }
                        //brazo-> cola = getCola(&brazosCola, brazo->id);
                        //brazo = updateBrazo(&brazosCola, brazo->id, esquema);
                        pthread_mutex_unlock(&mutex);
                        break;
                    }else{
                        totalItems+=items;
                    }
                    elemento = strtok_r(ptr, "-", &ptr);
                }
                if(totalItems == 0){
                    continue;
                }
                brazo->pendientesItem-=totalItems;
                for (int i = 0; i < n_pedidosxbrazo; i++) {
                    if(brazo->pedidos[i].id == id){
                        brazo->pedidos[i].totalPendientes -= totalItems;
                        printf("NUEVO ITEM -> Brazo %d | pedido:%d | items agregados/pendientes :%d/%d  \n",
                                brazo->id, id, totalItems, brazo->pedidos[i].totalPendientes);
                    }
                }
            }
        }else if(resultado == 2){
            //printf("Brazo: %d, cantitems:%d \n",brazo->id, brazo->cola->contador);
            sleep(1);
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
            pthread_mutex_lock(&mutex_pedidos);   // ojo
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
            //siguiente = brazo->siguiente;
            pthread_mutex_unlock(&mutex_pedidos);  // ojo
            pthread_mutex_unlock(&mutex);
            return 1;
        }
    /*
        struct BrazoRobotico* brazo;
        if(n_brazos == 1){
            brazo = brazosCola;
        }else{
            brazo = popP(&brazosCola);                      //deberia volver NULL si hay disponibles// o aqui validar deacuerdo al esquema
        }
        if(brazo->cantPedidos >= n_pedidosxbrazo){          //ya no hay brazos disponibles
            brazo->estado = BRAZO_OCUPADO;
            pushBrazo(&brazosCola, brazo, esquema);
        }else{
            brazo->cantPedidos += 1;
            pthread_mutex_lock(&mutex_pedidos);   // ojo
            pedido->brazo = brazo;
            brazo->pendientesItem += pedido->total;
            for (int i = 0; i < n_pedidosxbrazo; i++) {
                if(brazo->pedidos[i].id == -1){
                    brazo->pedidos[i].id = pedido->id;
                    brazo->pedidos[i].totalPendientes = pedido->total;
                    break;
                }
            }
            printf("planificador-> main, Nuevo Pedido: %d | total items:%d | asignado a brazo %d\n",pedido->id, pedido->total, brazo->id);
            pthread_mutex_unlock(&mutex_pedidos);  // ojo
            pushBrazo(&brazosCola, brazo, esquema);
            pthread_mutex_unlock(&mutex);
            return 1;
        }
        pthread_mutex_unlock(&mutex);
        return 0;
    }*/
}

int main(int argc, char *argv[]){
    // verificando ingreso correcto de parámetros
    pthread_attr_t attr;
    pthread_attr_init(&attr);
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

	// definiendo la cola para almacenar los pedidos que lleguen
    cola = (struct Cola*)malloc(sizeof(struct Cola));
    cola->primero = NULL;
    cola->final = NULL;
    cola->contador = 0;
    char data[MAX];

    //construcción y definición de los brazos robóticos
    pthread_t thread;
    brazosCola = nuevaCola(1, n_pedidosxbrazo, esquema);
    int estado_hilo = pthread_create(&thread, &attr, thread_brazo_robotico, (void*) brazosCola);
    if (estado_hilo != 0){
        printf("Error: planificador-> main, error al crear hilo\n");
    }
    for (int id = 2; id <= n_brazos; id++) {
        struct BrazoRobotico *t = pushP(&brazosCola, id, n_pedidosxbrazo, esquema);
        estado_hilo = pthread_create(&thread, &attr, thread_brazo_robotico, (void*) t);
        if (estado_hilo != 0){
            printf("Error: planificador-> main, error al crear hilo\n");
        }
    }
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
    pedidos = (struct Pedidos*)malloc(sizeof(struct Pedidos));
    pedidos->primero = NULL;
    pedidos->final = NULL;

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
                    printf("ERROR: planificador-> main, Intento fallido de asignación de brazo\n");
                }
            }
        }else{
            printf("ERROR: planificador-> main, Incorrecto formato.\n");
            continue;
        }
	}
    printf("planificador-> main, server exiting\n");
    while(1){
        sleep(2);
    }
    imprimirLista(pedidos);
    close(client_sockfd);
    return 0;
}
// ./eplanificadorBrazosRoboticos 10 2 2

// COSAS A CONSIDERAR
/*
    BRAZO FINALIZA UN PEDIDO Y PUEDE SER ASIGNADO OTRO. ACTUALIZAR LISTA DE PEDIDO. PRIMERO ELIMINAR Y LUEGO HACER PUSH
    BRAZO PASA A SUSPENDIDO: SACAR BRAZO Y LUEGO APUSH . AL HACER PUSH CONSIDERAR EL EL ESTADO SUSPENDIDO PARA MANDARLO AL FINAL
    BRAZO PASA A INICIAR: SACAR BRAZO Y LUEGO PUSH


    */