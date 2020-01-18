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
struct BrazoRobotico *brazosCola;
struct Pedidos *pedidos;
int n_brazos;
int n_pedidosxbrazo;
int esquema;
pthread_mutex_t mutex;


void *thread_brazo_robotico(void *arg){
    struct BrazoRobotico *brazo = (struct BrazoRobotico*) arg;
    //struct Cola* cola = brazo->cola;
    int brazoiD=brazo->id;
    printf("Brazo con id: %d iniciado.\n", brazo->id);
    char data[100];
    memset(data, 0, sizeof(data));
    int resultado = 2;

    while(resultado != 0){
        resultado = dequeue(brazo->cola, data, 0);
        if(resultado == 1 ){
            printf("brazo %d-%d, Nuevo item recibido:%s \n",brazoiD,brazo->id, data);
            memset(data, 0, sizeof(data));
        }else if(resultado == 2){
            sleep(1);
        }
    }

    pthread_t thread = pthread_self();
    // procesar datos de hilo
    return NULL;
}

int asignarBrazo(struct Pedido* pedido){
    pthread_mutex_lock(&mutex);
    struct BrazoRobotico* brazo = popP(&brazosCola);
    printf("Pedido sacado de cola:%d,%d\n", brazo->id, brazo->cantPedidos);
    if(brazo->cantPedidos >= n_pedidosxbrazo){
        //ya no hay brazos disponibles
        pushBrazo(&brazosCola, brazo);
        //repetir ?
    }else{
        brazo->cantPedidos +=1;
        pedido->brazo = brazo;
        pushBrazo(&brazosCola, brazo);
        brazo->pendientesItem += pedido->total;
        /*
        for (int i = 0; i < n_pedidosxbrazo; i++) {
            if(brazo->pedidos[i].id == -1){
                //llenar nuevos datos
                brazo->pedidos[i].id = pedido->id;
                brazo->pedidos[i].totalPendientes = pedido->total;
            }
            // validar si ya esta llena la cola
        }*/
        // aqui debo llenar pedidos min del brazo.

        pthread_mutex_unlock(&mutex);
        return 1;
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}


int main(int argc, char *argv[]){
    // verificando ingreso correcto de parametros
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (argc != 4) {
        printf("Usar: %s N_BRAZOS N_PEDIDOSXBRAZO ESQUEMAPLANIFICACION\n", argv[0]);
        exit(1);
    }
    n_brazos = atoi(argv[1]);
    if(n_brazos == 0){
        printf("Parámetro N_BRAZOS inválido.\n");
        exit(1);
    }
    n_pedidosxbrazo = atoi(argv[2]);
    if(n_pedidosxbrazo == 0){
        printf("Parámetro N_PEDIDOSXBRAZO inválido.\n");
        exit(1);
    }
    esquema = atoi(argv[3]);
    if(esquema == 0 || esquema > 3){
        printf("Parámetro ESQUEMAPLANIFICACION inválido.\n");
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
    //sem_init(&cola->mutex, 0, 1);
    char data[MAX];

    //construccion y definicion de los brazos roboticos
    //DEBO CREAR HILOS
    pthread_t thread;
    int estado_hilo;
    if(esquema == ESQUEMA_PRIMERO_DISPONIBLE){
        // SETEAR VALORES A LOS DEMAS ATRIBUTOS DE CADA BRAZO DE LA COLA como cantidad de pedidos
        brazosCola = nuevaCola(0, 0, n_pedidosxbrazo);
        estado_hilo = pthread_create(&thread, &attr, thread_brazo_robotico, (void*) brazosCola);
        if (estado_hilo != 0){
            printf("error al crear thread\n");
        }
        for (int id = 1; id < n_brazos; id++) {
            struct BrazoRobotico *t = pushP(&brazosCola, 0, id, n_pedidosxbrazo);
            estado_hilo = pthread_create(&thread, &attr, thread_brazo_robotico, (void*) t);
            if (estado_hilo != 0){
                printf("error al crear thread\n");
            }
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
		printf("[Data = %s rc=%d]\n",buffer,rc);
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
	    resultado = dequeue(cola, data, 1);
        if(resultado==0) break;
        char* ptr = data;
        elemento = strtok_r(ptr, "-", &ptr);
        if(elemento != NULL){
            id = atoi(elemento);
            //validar el 0 al finalizar un pedido
            struct Pedido *pedido = find(id, pedidos);
            //elemento = strtok_r(ptr, "-", &ptr);
            if(pedido != NULL) {
                printf("Pedido ya registrado: (%d,%s) \n",pedido->id,ptr);
                sprintf(data,"%d-%s",id,ptr);
                pthread_mutex_lock(&mutex);
                if(pedido->brazo == NULL){
                    // no tiene asignado un brazo
                    continue;
                }
                enqueue(data, pedido->brazo->cola);
                pthread_mutex_unlock(&mutex);
            }else{
                printf("NUevo Pedido: %d - total) \n",id);
                pedido = insertarPrimero(id, 1, pedidos);
                if( asignarBrazo(pedido) == 0){
                    // no se pudo asignar pedido.
                    // debo encolar
                }
            }
        }else{
            printf("Incorrecto formato.\n");
            continue;
        }
	}
    //imprimir(cola);
    //
    while(1){
        sleep(2);

    }
	printf("server exiting\n");
    //imprimirLista(pedidos);
	close(client_sockfd);
	return 0;
}
