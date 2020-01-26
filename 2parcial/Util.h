//
// Mauricio Leiton Lázaro(mdleiton)
// Fecha: 12/1/20.
//
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

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
#define EXITPROGRAMA_PLANIF 36
#define EXITPROGRAMA_GENERADOR 37
#define INITPROGRAMA_PLANIFICADOR 38
#define INITPROGRAMA_GENERADOR 39
#define EXITPROGRAMA_ADMIN 40
#define INITPROGRAMA_ADMIN 41

#define CREAR_BRAZO 42
#define SUSPENDER_BRAZO 43
#define REANUDAR_BRAZO 44

#define CORE_BRAZOS 0
#define CORE_PLANIFICACION 1
#define CORE_ADMIN 2
#define CORE_RECEPCION 3

#define ID_MC 124
#define ID_MC_INFO 224
#define MAX_BUFFER 100

/**
 * Estructura de información del sisteam
 */
struct Informacion{
    int pedidosFinalizados;
    int brazosActivos;
    int brazosSuspendidos;
    sem_t mutex;
};

/**
 * Estructura de comunicacion entre procesos
 */
struct Proceso{
    long pidGenerador;
    long pidPlanificador;
    long pidAdmin;
    sem_t mutex;
};

/**
 *  Función que envia una señal a un proceso.
 *  @param pid identificador del proceso.
 *  @param signalValue entero que se puede enviar en la señal.
 *  @param signalId el tipo de señal que se envia.
 *  @return 0 en caso de que se envie y se reciba la señal, -1 cuando ocurra algún error.
 */
int enviarSenal(long pid, int signalValue, int signalId){
    if (pid>0){
        union sigval mysigval;
        mysigval.sival_int = signalValue;
        int send = sigqueue(pid, signalId, mysigval);
        return send;
    }
    return -1;
}

/**
 *  Función que retorna la cantidad de núcleo que posee tu computadora
 *  @return un número entero positivo.
 */
int numeroNucleos(){
    return sysconf(_SC_NPROCESSORS_ONLN);
}

/**
 * permite setear la afinidad a un hilo
 * @param core  id del core
 * @param cpuset
 * @return
 */
void setAffinity(int core, cpu_set_t cpuset){
    CPU_ZERO(&cpuset);
    pthread_t thread = pthread_self();
    cpu_set_t cpusetThread;
    CPU_ZERO(&cpusetThread);
    CPU_SET(core, &cpusetThread);
    int resultAffinity = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpusetThread);
    if (resultAffinity != 0){
        printf("resultado->thread_routine, error al setear la afinidad al hilo\n");
    }
}