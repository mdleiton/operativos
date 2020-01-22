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
    int estado;
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
 * remueve el brazo robótico de más alta prioridad de acuerdo al esquema seleccionado
 * @param inicio referencia al primer brazo robótico de la cola
 * return una referencia al brazo robotico de mayor prioridad. Lo saca de la lista.
 */
struct BrazoRobotico* popP(struct BrazoRobotico** inicio){
    struct BrazoRobotico* temporal = *inicio;
    if((*inicio)->siguiente == NULL){
        printf("ERROR: BrazoRobotico -> popP, No existen más brazos disponibles.");
        return NULL;
    }
    (*inicio) = (*inicio)->siguiente;
    return temporal;
}

/**
 * agrega un nuevo Brazo a la cola
 * @param inicio referencia al primer brazo robótico de la cola
 * @param id identificador del brazo robótico
 * @param cantidadPedidos cantidad de pedidos que puede soportar el brazo robótico
 * @param esquema una referencia al brazo robotico de mayor prioridad. Lo saca de la lista
 * @return una referencia al brazo robotico */
struct BrazoRobotico* pushP(struct BrazoRobotico** inicio, int id, int cantidadPedidos, int esquema){
    struct BrazoRobotico* start = (*inicio);
    struct BrazoRobotico* temp = nuevaCola(id, cantidadPedidos, esquema);
    if(esquema == ESQUEMA_IGUAL_X_PEDIDOS || esquema == ESQUEMA_PRIMERO_DISPONIBLE ) {
        if ((*inicio)->cantPedidos > cantidadPedidos) {
            temp->siguiente = *inicio;
            (*inicio) = temp;
        } else {
            while (start->siguiente != NULL && start->siguiente->cantPedidos < cantidadPedidos) {
                start = start->siguiente;
            }
            temp->siguiente = start->siguiente;
            start->siguiente = temp;
        }
    }
    return temp;
}

/**
 * actualiza la ubicación de un elemento en la cola
 * @param inicio referencia al primer brazo robótico de la cola
 * @param id identificador del brazo robótico
 * @param esquema una referencia al brazo robotico de mayor prioridad. Lo saca de la lista
 * @return una referencia al brazo robotico */
struct BrazoRobotico* updateBrazo(struct BrazoRobotico** inicio, int id, int esquema){
    struct BrazoRobotico* start = (*inicio);
    struct BrazoRobotico* temporal = (*inicio);
    if(esquema == ESQUEMA_IGUAL_X_PEDIDOS){
        if ((*inicio)->id == id) {
            return (*inicio);
            return;
        }else{
            while (start->siguiente != NULL) {
                if(start->siguiente->id == id){
                    temporal = start->siguiente;
                    start->siguiente = temporal->siguiente;
                    break;
                }
                start = start->siguiente;
            }
        }
        struct BrazoRobotico* start2 = (*inicio);
            while (start2->siguiente != NULL && start2->siguiente->cantPedidos < temporal->cantPedidos) {
                start2 = start2->siguiente;
            }
            temporal->siguiente = start2->siguiente;
            start2->siguiente = temporal;
    }
    return temporal;
}


struct BrazoRobotico* getBrazo(struct BrazoRobotico** inicio, int esquema, int n_pedidos){
    struct BrazoRobotico* start = (*inicio);
    if(esquema == ESQUEMA_PRIMERO_DISPONIBLE){
        if ((*inicio)->cantPedidos < n_pedidos) {
            return (*inicio);
        }else{
            while (start !=NULL) {
                if(start->cantPedidos < n_pedidos){
                    return start;
                }
                start = start->siguiente;
            }
        }
    }
    return NULL;
}

/**
 * Verifica si está vacia la cola de brazos róboticos
 * @param inicio referencia al primer brazo robótico de la cola
 * @return 1 si está vacia la cola. */
int vaciaCola(struct BrazoRobotico** inicio){
    return (*inicio) == NULL;
}

/**
 * imprime por cola todos los brazos robóticos disponibles
 * @param inicio referencia al primer brazo robótico de la cola */
void imprimirBrazos(struct BrazoRobotico** inicio){
    while (!vaciaCola(inicio)) {
        printf("%d, %d,  %d\n", (*inicio)->id, (*inicio)->pendientesItem, (*inicio)->cantPedidos);
        popP(inicio);
    }
}