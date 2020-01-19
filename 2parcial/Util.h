//
// Created by mdleiton on 13/1/20.
//
/**
 * Esquemas de funcionamiento
 * */
#define ESQUEMA_PRIMERO_DISPONIBLE 1
#define ESQUEMA_IGUAL_X_PEDIDOS 2
#define ESQUEMA_MENOR_ITEM_PENDIENTES 3

#define PRIORIDAD_ESQUEMA_PRIMERO_DISPONIBLE 1
#define PRIORIDAD_ESQUEMA_PRIMERO_OCUPADO 0


/**
 * Estados de un Brazo Robótico
 * */
#define BRAZO_DISPONIBLE 1 //INICIADO
#define BRAZO_SUSPENDIDO 2
#define BRAZO_TERMINADO 3
#define BRAZO_OCUPADO 4

/**
 * Estados de un Pedido
 * */
#define PEDIDO_PARCIAL 1
#define PEDIDO_FINALIZADO 2