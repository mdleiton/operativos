//
// Mauricio Leiton Lázaro(mdleiton)
// Fecha: 12/1/20.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#define MAX 200

/**
 * Elemento de la cola
 */
struct elemento{
    char data[MAX];
    struct elemento *siguiente;
};

/**
 * Estructura de una cola
 */
struct Cola{
    // mutex y non_zero_item para que la cola funcione como un buffer de tamaño ilimitado.
    pthread_mutex_t mutex;
    pthread_cond_t non_zero_item;
    int contador;
    struct elemento *primero;
    struct elemento *final;
};

/**
 * Permite agregar un elemento a una cola específica
 * @param valor informacion que se desea agregar a la cola.
 * @param cola especifica la cola a la que se desea agregar el elemento.
 */
void enqueue(char valor[MAX], struct Cola *cola){
    struct elemento *nuevo = (struct elemento*)malloc(sizeof(struct elemento));
    memset(nuevo->data,0, MAX* sizeof(char));
    memcpy(nuevo->data, valor, MAX* sizeof(char));
    nuevo->siguiente = NULL;

    pthread_mutex_lock(&cola->mutex);

    if (cola->final == NULL){
        cola->primero = nuevo;
        cola->final = nuevo;
    }else{
        cola->final->siguiente =nuevo;
        cola->final = nuevo;
    }

    cola->contador = cola->contador + 1;
    pthread_cond_signal(&cola->non_zero_item);
    pthread_mutex_unlock(&cola->mutex);
}

/**
 * Permite presentar los elementos de una cola
 * @param cola especifica la cola a la que se desea imprimir sus elementos.
 */
void imprimir(struct Cola *cola){
    pthread_mutex_lock(&cola->mutex);
    printf("%d,\n", cola->contador);
    struct elemento *temporal;
    temporal = cola->primero;
    while (temporal != NULL){
        printf("%s.\t", temporal->data);
        temporal = temporal->siguiente;
    }
    printf("\n");
    pthread_mutex_unlock(&cola->mutex);
}

/**
 * Permite eliminar un elemento de una cola específica
 * @param cola especifica la cola a la que se desea quitar un elemento
 * @param data aqui se almacenará la información que se saque de la cola.
 * @flag si es 1 no va esperar por el mutex condicional.
 * @return
 */
int dequeue(struct Cola *cola, char data[MAX], int flag){
    pthread_mutex_lock(&cola->mutex);
    if (cola->primero == NULL){
        if(flag == 1){
            pthread_mutex_unlock(&cola->mutex);
            return 0;
        }
    }

    while(cola->contador == 0)
        pthread_cond_wait(&cola->non_zero_item, &cola->mutex);
    struct elemento *temporal;
    temporal = cola->primero;
    cola->primero = cola->primero->siguiente;
    if(cola->primero == NULL){
        cola->final = NULL;
    }
    sprintf(data,"%s", temporal->data);
    free(temporal);

    cola->contador = cola->contador - 1;
    pthread_mutex_unlock(&cola->mutex);
    return 1;
}

/* Prueba cola
int main(){
    struct Cola *cola = (struct Cola*)malloc(sizeof(struct Cola));
    cola->primero = NULL;
    cola->final = NULL;
    char data[MAX];
    memset(data, '1', MAX* sizeof(char));
    enqueue(data, cola);
    display(cola);
    dequeue(cola);
    display(cola);
}
*/