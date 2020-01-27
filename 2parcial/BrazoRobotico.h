//
// Mauricio Leiton Lázaro(mdleiton)
// Fecha: 12/1/20.
//
#include <stdio.h>
#include <stdlib.h>
#include "Cola.h"
#include "Util.h"

/**
 * Informacion minima requerida que un brazo debe saber de un pedido
 */
struct pedidosMin{
    int id;
    int totalPendientes;
};

/**
 * Estructura de lista de Brazos robóticos
 */
struct BrazoRobotico {
    int id;
    int pendientesItem;
    int cantPedidos;  // sera mi prioridad
    pthread_mutex_t mutex;
    int totalPaquetesProcesados;
    int estado;                 //
    pthread_cond_t estadoCon;
    struct Cola *cola;
    struct pedidosMin* pedidos;
    struct BrazoRobotico* siguiente;
};

/**
 * Función que crea una cola de brazos roboticos
 * @param id identificador del brazo robotico
 * @param cantidadPedidos cantidad de pedidos máxima que puede soporta un brazo robotico
 * @param esquema metodo de funcionamiento y prioridad de la cola
 * @return  una referencia al nuevo brazo robotico construido  */
struct BrazoRobotico* nuevaCola(int id, int cantidadPedidos, int esquema){
    struct BrazoRobotico* temporal = (struct BrazoRobotico*)malloc(sizeof(struct BrazoRobotico));
    temporal->id = id;
    temporal->cantPedidos = 0;
    temporal->pendientesItem = 0;
    temporal->totalPaquetesProcesados = 0;

    temporal->cola = (struct Cola*)malloc(sizeof(struct Cola));
    temporal->cola->primero = NULL;
    temporal->cola->final = NULL;
    temporal->cola->contador = 0;

    temporal->pedidos = (struct pedidosMin*)malloc(cantidadPedidos * sizeof(struct pedidosMin));
    for (int i = 0; i < cantidadPedidos; i++) {
        temporal->pedidos[i].id = -1;
        temporal->pedidos[i].totalPendientes = 0;
    }

    temporal->estado = BRAZO_DISPONIBLE;
    temporal->siguiente = NULL;
    return temporal;
}

/**
 * agrega un nuevo Brazo a la cola
 * @param inicio referencia al primer brazo robótico de la cola
 * @param id identificador del brazo robótico
 * @param cantidadPedidos cantidad de pedidos que puede soportar el brazo robótico
 * @param esquema metodo de funcionamiento y prioridad de la cola
 * @return una referencia al brazo robotico */
struct BrazoRobotico* pushP(struct BrazoRobotico** inicio, int id, int cantidadPedidos, int esquema){
    struct BrazoRobotico* temp = nuevaCola(id, cantidadPedidos, esquema);
    temp->siguiente = *inicio;
    (*inicio) = temp;
    return temp;
}

/**
 * actualiza la ubicación de un elemento en la cola
 * @param inicio referencia al primer brazo robótico de la cola
 * @param id identificador del brazo robótico
 * @param esquema metodo de funcionamiento y prioridad de la cola
 * @return una referencia al brazo robotico */
void updateBrazo(struct BrazoRobotico** inicio, int id, int esquema){
    if(esquema == ESQUEMA_PRIMERO_DISPONIBLE) {
        return; // no es necesario hacer algo
    }
    if(esquema == ESQUEMA_MENOR_ITEM_PENDIENTES){
        struct BrazoRobotico* start = (*inicio);
        struct BrazoRobotico* temporal = NULL;
        struct BrazoRobotico* prev = NULL;
        if ((*inicio)->id != id) {
            while (start != NULL) {
                if(start->id == id){
                    temporal = start;
                    break;
                }
                prev = start;
                start = start->siguiente;
            }
        }else{
            temporal = (*inicio);
        }
        if(temporal == NULL){
            printf("ERROR: BrazoRobotico -> updateBrazo, no existe brazo con ese id.");
            return;
        }
        start = (*inicio);
        if(temporal -> estado == BRAZO_SUSPENDIDO){ //LO ENVIO AL FINAL
            if((*inicio)->id == temporal->id){
                (*inicio) = (*inicio)->siguiente;
            }
            while (start->siguiente != NULL) {
                start = start->siguiente;
            }
            if(prev !=  NULL){
                prev->siguiente = temporal->siguiente;
            }
            temporal->siguiente = start->siguiente;  // a NULL
            start->siguiente = temporal;
        }else{
            if((*inicio)->id == temporal->id){
                start = (*inicio);
                if(start->siguiente != NULL){
                    if(start->pendientesItem <= start->siguiente->pendientesItem){
                        return;
                    }
                }
                (*inicio) = (*inicio)->siguiente;
                while (start->siguiente != NULL && start->siguiente->pendientesItem < temporal->pendientesItem) {
                    start = start->siguiente;
                }
                temporal->siguiente = start->siguiente;
                start->siguiente = temporal;
                return;
            }else{
                start = (*inicio);
                if(start->pendientesItem >= temporal->pendientesItem){
                    if(prev != NULL){
                        prev->siguiente = temporal->siguiente;
                    }
                    temporal->siguiente = (*inicio);
                    (*inicio) = temporal;
                    return;
                }
                while (start != NULL && start->pendientesItem < temporal->pendientesItem) {
                    if(start->siguiente == NULL){
                        temporal->siguiente = start->siguiente;
                        start->siguiente = temporal;
                        return;
                    }
                    prev = start;
                    start = start->siguiente;
                }
                temporal->siguiente = start;
                start->siguiente = temporal;
                return;
            }
        }
    }
}

/**
 * obtiene un brazo deacuerdo al esquema definido
 * @param inicio referencia al primer brazo robótico de la cola
 * @param esquema metodo de funcionamiento y prioridad de la cola
 * @param n_pedidos cantidad de pedidos
 * return una referencia al brazo robotico de mayor prioridad. Lo saca de la lista.
 */
struct BrazoRobotico* getBrazo(struct BrazoRobotico** inicio, int esquema, int n_pedidos){
    struct BrazoRobotico* start = (*inicio);
    if(esquema == ESQUEMA_PRIMERO_DISPONIBLE){
        if ((*inicio)->cantPedidos < n_pedidos && (*inicio)->estado == BRAZO_DISPONIBLE) {
            return (*inicio);
        }else{
            while (start != NULL) {
                if(start->cantPedidos < n_pedidos && start->estado == BRAZO_DISPONIBLE){
                    return start;
                }
                start = start->siguiente;
            }
        }
    }else if(esquema == ESQUEMA_IGUAL_X_PEDIDOS || esquema == ESQUEMA_MENOR_ITEM_PENDIENTES){
        if ((*inicio)->cantPedidos < n_pedidos && start->estado == BRAZO_DISPONIBLE) return (*inicio);
    }
    return NULL;
}

/**
 * permite obtener una referencia a un brazo robotico con el id
 * @param inicio referencia al primer brazo robótico de la cola
 * @param id identificador del brazo robótico
 * @param esquema metodo de funcionamiento y prioridad de la cola
 * @return una referencia al brazo robotico */
struct BrazoRobotico* getBrazobyId(struct BrazoRobotico** inicio, int id, int esquema){
    struct BrazoRobotico* start = (*inicio);
    struct BrazoRobotico* temporal = NULL;
    if ((*inicio)->id == id) {
        return (*inicio);
    }else{
        while (start != NULL) {
            if(start->id == id){
                return start;
            }
            start = start->siguiente;
        }
    }
    return temporal;
}

void display(struct BrazoRobotico** inicio){
    struct BrazoRobotico* start = (*inicio);
    printf("Estado Brazo \n");
    while (start != NULL) {
        printf("brazo %d, total pedientes: %d\n", start->id, start->pendientesItem);
        start = start->siguiente;
    }
}