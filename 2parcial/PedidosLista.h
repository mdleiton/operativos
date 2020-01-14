//
// Created by mdleiton on 12/1/20.
//
#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <stdbool.h>

/**
 * Lista de pedidos */
struct Pedido{
    int data;
    int estado;
    int id;
    int totalPendientes;
    int total;
    int brazoId;
    struct Pedido *siguiente;
};

/**
 * Estructura de la lista de pedidos
 */
struct Pedidos{
    struct Pedido *primero;
    struct Pedido *final;
};

/**
 * Imprime todos los pedidos desde el inicio
 */
void printList(struct Pedidos *pedidos) {
    struct Pedido *ptr = pedidos->primero;
    printf("\n[ ");
    while(ptr != NULL) {
        printf("(%d,%d) ",ptr->id,ptr->data);
        ptr = ptr->siguiente;
    }
    printf(" ]");
}

/**
 * Inserta un elemento en la lista al inicio de la lista
 * @param id identificador de pedido
 * @param data contenido del pedido
 */
void insertarPrimero(int id, int data, struct Pedidos *pedidos) {
    struct Pedido *link = (struct Pedido*) malloc(sizeof(struct Pedido));
    link->id = id;
    link->data = data;
    link->siguiente = pedidos->primero;  // primer elemento ahora es segundo
    pedidos->primero = link;        // link ahora es el primer elemento
}

/**
 * Verifica si la lista esta vacia
 * @return True si esta vacia. False si tiene algún elemento.
 */
bool vacioLista(struct Pedidos *pedidos) {
    return pedidos->primero == NULL;
}

/**
 * Permite conocer la cantidad de pedidos
 * @return entero
 */
int length(struct Pedidos *pedidos) {
    int length = 0;
    struct Pedido *current;
    for(current = pedidos->primero; current != NULL; current = current->siguiente) {
        length++;
    }
    return length;
}

/**
 * Permite obtener un pedido de la lista
 * @param id identificador del pedido que se desea obtener
 * @return
 */
struct Pedido* find(int id, struct Pedidos *pedidos) {
    struct Pedido* current = pedidos->primero;
    if(pedidos->primero == NULL) {
        return NULL;
    }
    while(current->id != id) {
        if(current->siguiente == NULL) {  // si es el ultimo elemento de la lista
            return NULL;
        } else {
            current = current->siguiente;
        }
    }
    return current;
}

/**
 * Elimina un pedido a partir de su identificador
 * @param id identificador del pedido
 * @return
 */
struct Pedido* delete(int id, struct Pedidos *pedidos) {
    struct Pedido* current = pedidos->primero;
    struct Pedido* previous = NULL;
    if(pedidos->primero == NULL) {
        return NULL;
    }
    while(current->id != id) {
        if(current->siguiente == NULL) {
            return NULL;
        } else {
            previous = current;
            current = current->siguiente;
        }
    }
    if(current == pedidos->primero) {
        pedidos->primero = pedidos->primero->siguiente;
    } else {
        previous->siguiente = current->siguiente;
    }
    return current;
}

/*
void main() {
    struct Pedidos *pedidos = (struct Pedidos*)malloc(sizeof(struct Pedidos));
    pedidos->primero = NULL;
    pedidos->final = NULL;

    printList(pedidos);
    insertFirst(1,10, pedidos);
    insertFirst(2,20, pedidos);
    insertFirst(3,30, pedidos);
    printf("\nLista: ");
    printList(pedidos);
    printf("\n");
    struct Pedido *foundLink = find(4, pedidos);
    if(foundLink != NULL) {
        printf("Element found: ");
        printf("(%d,%d) ",foundLink->id,foundLink->data);
        printf("\n");
    } else {
        printf("Element not found.");
    }

    delete(4, pedidos);
    printf("List after deleting an item: ");
    printList(pedidos);
    printf("\n");
    foundLink = find(4, pedidos);

    if(foundLink != NULL) {
        printf("Element found: ");
        printf("(%d,%d) ",foundLink->id,foundLink->data);
        printf("\n");
    } else {
        printf("Element not found.");
    }
    printf("\n");
}
 */

