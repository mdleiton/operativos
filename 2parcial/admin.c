#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>

#include "Util.h"

// variables globales
struct Proceso *procesos;
struct Informacion *informacion;
int mcID;
int mcID2;

/**
 *  Función que finaliza adecuadamente el proceso. */
void finalizar(){
    procesos->pidAdmin = -1;
    shmdt((void *) procesos);
    shmdt((void *) informacion);
    printf("admin -> finalizar, memorias compartidas han sido liberadas.\n");
    exit(1);
}

/**
 *  Función que maneja la señal SIGINT. Notifica su finalización al proceso resultado en caso de estar ejecutándose. */
void manejadorSIGINT(int signum, siginfo_t *info, void *ptr){
    enviarSenal(procesos->pidPlanificador, 1, EXITPROGRAMA_ADMIN);
   finalizar();  // finaliza debidamente
}

/**
 *  Función que maneja la señal que indica que el proceso planificacion acabo de finalizar */
void manejadorEXITPLANIF(int signum, siginfo_t *info, void *ptr){
    printf("\nadmin -> El proceso planificacion acabo de finalizar\n");
    //finalizar();  // finaliza debidamente
}

int  main(int  argc, char *argv[]){
    if (argc != 1) {
        printf("Usar: %s \n", argv[0]);
        exit(1);
    }

    // manejadores de senales.
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = manejadorSIGINT;
    act.sa_flags = SIGINT;
    sigaction(SIGINT, &act, NULL);

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = manejadorEXITPLANIF;
    act.sa_flags = EXITPROGRAMA_PLANIF;
    sigaction(EXITPROGRAMA_PLANIF, &act, NULL);

    // acceso a memoria compartida.
    mcID = shmget(ID_MC, sizeof(struct Proceso), 0666);
    if (mcID < 0) {
        printf("ERROR: admin -> main, inválido identificador de memoria compartido.\n");
        return -1;
    }
    procesos = (struct Proceso *) shmat(mcID, NULL, 0);
    if ((int) procesos == -1) {
        printf("admin -> main, shmat error.\n");
        return -1;
    }
    sem_wait(&procesos->mutex);
    procesos->pidAdmin = syscall(SYS_gettid);
    sem_post(&procesos->mutex);

    // acceso a memoria compartida.
    mcID2 = shmget(ID_MC_INFO, sizeof(struct Informacion), 0666);
    if (mcID2 < 0) {
        printf("ERROR: admin -> main, inválido identificador de memoria compartido.\n");
        return -1;
    }
    informacion = (struct Informacion*) shmat(mcID2, NULL, 0);
    if ((int) informacion == -1) {
        printf("admin -> main, shmat error.\n");
        return -1;
    }

    char instruccion[50];
    int send;
    while(1){
        sem_wait(&informacion->mutex);
        printf("Información del sistema: \nCantidad de pedidos finalizados: %d\n Cantidad de brazos activos: %d \n, Cantidad de brazos suspendidos: %d \n",
                informacion->pedidosFinalizados, informacion->brazosActivos, informacion->brazosSuspendidos);
        sem_post(&informacion->mutex);
        memset(instruccion, 0, sizeof(instruccion));
        printf("OPCIONES:\n 1. Crear un nuevo brazo. \n 2. Suspender un brazo. \n 3. Reanudar brazo. \n Ingrese el # de la opción: ");
        scanf("%s", instruccion);
        int opcion = atoi(instruccion);
        if(opcion <= 0 || opcion > 3) {
            printf("ERROR: Opción inválida. Intente otra vez.\n");
            continue;
        }
        if(opcion == 1){
            sem_wait(&procesos->mutex);
            send = enviarSenal(procesos->pidPlanificador, 1, CREAR_BRAZO);
            sem_post(&procesos->mutex);
            if(send != 0){
                printf("admin->main, no se pudo comunicar con proceso. \n");
            }else{
                printf("admin->main, creando...... espere.......\n");
                //debo devolver para confirmar
            }
            continue;
        }
        sem_wait(&informacion->mutex);
        int nBrazos = informacion->brazosActivos + informacion->brazosSuspendidos;
        printf("Ingreso el número del brazo:(1-%d)", nBrazos);
        sem_post(&informacion->mutex);
        memset(instruccion, 0, sizeof(instruccion));
        scanf("%s", instruccion);
        int idBrazo = atoi(instruccion);
        if(idBrazo > 0 && idBrazo <= nBrazos){
            sem_wait(&procesos->mutex);
            if(opcion == 2){
                send = enviarSenal(procesos->pidPlanificador, 1, SUSPENDER_BRAZO);
            }else{
                send = enviarSenal(procesos->pidPlanificador, 1, REANUDAR_BRAZO);
            }
            sem_post(&procesos->mutex);
            if(send != 0){
                printf("ERROR: admin->main, no se pudo comunicar con proceso. \n");
            }else{
                printf("admin->main, ejecutando operación...... espere.......\n");
                //debo devolver para confirmar
            }
        }else{
            printf("ERROR: admin->main, Identificador del brazo inválido. \n");
        }
    }
    finalizar(); // finaliza debidamente
}
