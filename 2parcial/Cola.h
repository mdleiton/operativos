//
// Mauricio Leiton Lázaro(mdleiton)
// Fecha: 12/1/20.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    struct elemento *primero;
    struct elemento *final;
};

/**
 * Permite agregar un elemento a una cola específica
 * @param elemento que se desea agregar a la cola.
 * @param cola especifica cola a la que se desea agregar el elemento.
 */
void enqueue(char valor[MAX], struct Cola *cola){
    struct elemento *nuevo = (struct elemento*)malloc(sizeof(struct elemento));
    memset(nuevo->data,'\0', MAX* sizeof(char));
    memcpy(nuevo->data, valor, MAX* sizeof(char));
    nuevo->siguiente = NULL;
    if (cola->final == NULL){
        cola->primero = nuevo;
        cola->final = nuevo;
    }else{
        cola->final->siguiente = nuevo;
        cola->final = cola->final->siguiente;
    }
}

/**
 * Permite presenta los elementos de una cola
 * @param cola
 */
void display(struct Cola *cola){
    struct elemento *temporal;
    temporal = cola->primero;
    printf("\n");
    while (temporal != NULL){
        printf("%s.\t", temporal->data);
        temporal = temporal->siguiente;
    }
}

/**
 * Permite eliminar un elemento de una cola específica
 * @param cola
 */
int dequeue(struct Cola *cola, char data[MAX]){
    if (cola->primero == NULL){
        printf("\n\nLa cola esta vacia.\n");
        return 0;
    }else{
        struct elemento *temporal;
        temporal = cola->primero;
        cola->primero = cola->primero->siguiente;
        sprintf(data,"%s", temporal->data);
        printf("\n\n%s. deleted", temporal->data);
        free(temporal);
        return 1;
    }
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
