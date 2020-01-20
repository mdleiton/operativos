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
 * Función que crear un cola de prioridad
 * @param data
 * @param prioridad
 * @return    */
struct BrazoRobotico* nuevaCola(int id, int cantidad_pedidos, int esquema){
    struct BrazoRobotico* temporal = (struct BrazoRobotico*)malloc(sizeof(struct BrazoRobotico));
    temporal->id = id;
    temporal->cantPedidos = 0;
    temporal->pendientesItem = 0;

    temporal->cola = (struct Cola*)malloc(sizeof(struct Cola));
    temporal->cola->primero = NULL;
    temporal->cola->final = NULL;
    temporal->cola->contador = 0;

    temporal->pedidos = (struct pedidosMin*)malloc(cantidad_pedidos * sizeof(struct pedidosMin));
    for (int i = 0; i < cantidad_pedidos; i++) {
        temporal->pedidos[i].id = -1;
        temporal->pedidos[i].totalPendientes = 0;
    }

    temporal->estado = BRAZO_DISPONIBLE;
    temporal->siguiente = NULL;
    return temporal;
}

/**
 * remueve el brazo robótico de más alta prioridad
 * @param brazo
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
 * agrega un nuevo Brazo a la cola de prioridad
 * @param head
 * @param d
 * @param p
 */
struct BrazoRobotico* pushP(struct BrazoRobotico** inicio, int id, int cantidad_pedidos, int esquema){
    struct BrazoRobotico* start = (*inicio);
    struct BrazoRobotico* temp = nuevaCola(id, cantidad_pedidos, esquema);
    if(esquema == ESQUEMA_IGUAL_X_PEDIDOS || esquema == ESQUEMA_PRIMERO_DISPONIBLE ) {
        if ((*inicio)->cantPedidos > cantidad_pedidos) {
            temp->siguiente = *inicio;
            (*inicio) = temp;
        } else {
            while (start->siguiente != NULL && start->siguiente->cantPedidos < cantidad_pedidos) {
                start = start->siguiente;
            }
            temp->siguiente = start->siguiente;
            start->siguiente = temp;
        }
    }
    return temp;
}

void pushBrazo(struct BrazoRobotico** inicio, struct BrazoRobotico* brazo, int esquema){
    struct BrazoRobotico* start = (*inicio);
    if(esquema == ESQUEMA_IGUAL_X_PEDIDOS){
        if ((*inicio)->cantPedidos > brazo->cantPedidos) {
            brazo->siguiente = *inicio;
            (*inicio) = brazo;
        }else{
            while (start->siguiente != NULL && start->siguiente->cantPedidos < brazo->cantPedidos) {
                start = start->siguiente;
            }
            brazo->siguiente = start->siguiente;
            start->siguiente = brazo;
        }
    }
}

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

struct Cola* getCola(struct BrazoRobotico** inicio, int id){
    struct BrazoRobotico* start = (*inicio);
        if ((*inicio)->id == id) {
            return (*inicio)->cola;
        }else{
            while (start !=NULL) {
                if(start->id == id){
                    return start->cola;
                }
                start = start->siguiente;
            }
    }
    return NULL;
}

/**
 * Verifica si está vacia la cola de brazo róboticos
 * @param inicio
 * @return
 */
int vaciaCola(struct BrazoRobotico** inicio){
    return (*inicio) == NULL;
}

void imprimirBrazos(struct BrazoRobotico** inicio){
    while (!vaciaCola(inicio)) {
        printf("%d, %d,  %d\n", (*inicio)->id, (*inicio)->pendientesItem, (*inicio)->cantPedidos);
        popP(inicio);
    }
}

//Prueba de cola de prioridad
/*
int main(){
    struct BrazoRobotico* pq = nuevaCola(4, 1);
    push(&pq, 5, 2);
    push(&pq, 6, 3);
    push(&pq, 7, 0);

    while (!isEmpty(&pq)) {
        printf("%d ", pq->data);
        pop(&pq);
    }
    return 0;
}
 */