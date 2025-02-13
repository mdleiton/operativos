//
// Created by mdleiton on 12/1/20.
//
#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "BrazoRobotico.h"

/**
 * Lista de pedidos */
struct Pedido{
    int data;
    int estado;
    int id;
    int totalPendientes;
    int total;
    struct BrazoRobotico* brazo;
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
void imprimirLista(struct Pedidos *pedidos) {
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
struct Pedido* insertarPrimero(int id, int data, struct Pedidos *pedidos) {
    struct Pedido *nuevoPedido = (struct Pedido*) malloc(sizeof(struct Pedido));
    nuevoPedido->id = id;
    nuevoPedido->data = data;
    nuevoPedido->total = data;
    nuevoPedido->totalPendientes = data;
    nuevoPedido->brazo = NULL;
    nuevoPedido->estado = PEDIDO_PARCIAL;
    nuevoPedido->siguiente = pedidos->primero;  // primer elemento ahora es segundo
    pedidos->primero = nuevoPedido;        // link ahora es el primer elemento
    return nuevoPedido;
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
 * @return referencia al pedido buscado
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
 * @return referencia al pedido eliminar
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