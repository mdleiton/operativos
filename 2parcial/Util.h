//
// Created by mdleiton on 13/1/20.
//

#include <sys/syscall.h>
#include <signal.h>

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

/* SEÑALES	*/
#define EXITPROGRAMA_PLANIFICADOR 31
#define EXITPROGRAMA_GENERADOR 32
#define INITPROGRAMA_PLANIFICADOR 40
#define INITPROGRAMA_GENERADOR 34

#define ID_MC 124

/**
 * Estructura de información del sisteam
 */
struct Informacion{
    int pedidosFinalizados;
};

/**
 * Estructura de comunicacion entre procesos
 */
struct Proceso{
    long pidGenerador;
    long pidPlanificador;
    sem_t mutex;
};

/*
 *  Función que envia una señal a un proceso.
 *  @param pid identificador del programa.
 *  @param signalValue entero que se puede enviar en la señal.
 *  @param signalId el tipo de señal que se envia.
 *  @return 0 en caso de que se envie y se reciba la señal, -1 cuando ocurra algún error.
 */
int enviarSenal(long pid, int signalValue, int signalId){
    if (pid>0){
        int estado;
        union sigval mysigval;
        mysigval.sival_int = signalValue;
        int send = sigqueue(pid, signalId, mysigval);
        return send;
    }
    return -1;
}