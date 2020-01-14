//
// Mauricio Leiton Lázaro(mdleiton)
// Fecha: 12/1/20.
//
#include <stdio.h>
#include <stdlib.h>
#include "Cola.h"
#include "Util.h"

/**
 * Estructura de lista de Brazos robóticos
 */
struct BrazoRobotico {
    int id;
    int data;
    int pendientesItem;
    int cantPedidos;
    int prioridad;  // valores bajos indican alta prioridad
    int estado;
    //struct Pedidos *pedidos; // sin usar
    struct Cola *cola;
    struct BrazoRobotico* siguiente;
};

/**
 * Función que crear un cola de prioridad
 * @param data
 * @param prioridad
 * @return    */
struct BrazoRobotico* nuevaCola(int data, int id, int prioridad, int cantidad_pedidos){
    struct BrazoRobotico* temporal = (struct BrazoRobotico*)malloc(sizeof(struct BrazoRobotico));
    temporal->data = data;
    temporal->id = id;
    temporal->prioridad = prioridad;
    temporal->cantPedidos = 0;
    temporal->pendientesItem = 0;

    temporal->cola = (struct Cola*)malloc(sizeof(struct Cola));
    temporal->cola->primero = NULL;
    temporal->cola->final = NULL;

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
    (*inicio) = (*inicio)->siguiente;
    return temporal;
    //que pasa cuando esta vacio
}

/**
 * agrega un nuevo Brazo a la cola de prioridad
 * @param head
 * @param d
 * @param p
 */
struct BrazoRobotico* pushP(struct BrazoRobotico** inicio, int data, int id, int prioridad, int cantidad_pedidos){
    struct BrazoRobotico* start = (*inicio);
    struct BrazoRobotico* temp = nuevaCola(data, id, prioridad, cantidad_pedidos);
    if ((*inicio)->prioridad > prioridad) {
        temp->siguiente = *inicio;
        (*inicio) = temp;
    }else{
        while (start->siguiente != NULL && start->siguiente->prioridad < prioridad) {
            start = start->siguiente;
        }
        temp->siguiente = start->siguiente;
        start->siguiente = temp;
    }
    return temp;
}

void pushBrazo(struct BrazoRobotico** inicio, struct BrazoRobotico *brazo){
    struct BrazoRobotico* start = (*inicio);
    if ((*inicio)->prioridad > brazo->prioridad) {
        brazo->siguiente = *inicio;
        (*inicio) = brazo;
    }else{
        while (start->siguiente != NULL && start->siguiente->prioridad < brazo->prioridad) {
            start = start->siguiente;
        }
        brazo->siguiente = start->siguiente;
        start->siguiente = brazo;
    }
}

/**
 * Verifica si está vacia la cola de brazo róboticos
 * @param inicio
 * @return
 */
int vaciaCola(struct BrazoRobotico** inicio){
    return (*inicio) == NULL;
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