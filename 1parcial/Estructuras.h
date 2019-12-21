#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

#define PROGRAMA_CARGADOR 0
#define PROGRAMA_RESULTADO 1
#define TRUE 1
#define FALSE 0
#define ENABLE 1
#define DISABLE 0
#define ID_MC 124
#define INFINITY 9999
#define MAX 14
/* SEÑALES	*/
#define SIGNEWDATA 30
#define EXITPROGRAMA_CARGADOR 31
#define EXITPROGRAMA_RESULTADO 32
#define INITPROGRAMA_CARGADOR 40
#define INITPROGRAMA_RESULTADO 34

/*
 *  Estructura de datos que contiene información que compartirá los dos procesos.
 */
struct Datos {
	sem_t mutex;
	int grafo[MAX][MAX];
	int cantidadNodos;
};

/*
 *  Función que una matriz que representa un grafo.
 *  @param grafo matriz de dimensión MxM que representa un grafo.
 */
void imprimirMatriz(int grafo[MAX][MAX]){
	for(int a=0; a<MAX; a++){    
        for(int b=0; b<MAX; b++){
        	printf("%d-",grafo[a][b]);
        }
    printf("\n");
    }
}

/*
 *  Función que retorna la cantidad de núcleo que posee tu computadora
 *  @return un número entero positivo.
 */
int numeroNucleos(){
	return sysconf(_SC_NPROCESSORS_ONLN);
}

/*
 *  Función que actualiza la información de un proceso(pid, estado) en un archivo.
 *  @param programaId identificador del programa. 
 *  @param pId identificador del proceso.
 *  @param estado del programa.
 *  @return 1 en caso de que todo se ejecute correctamente.
 */
int actualizarInfoProceso(int programaId, long pId, int estado){
   char file[1];
   sprintf(file, "%d", programaId);
   FILE *fp = fopen(file, "w+");
   fprintf(fp, "%ld,%d\n", pId, estado);
   fclose(fp);
   return 1;
}

/*
 *  Función que obtiene la información de un proceso(pid, estado) desde un archivo.
 *  @param programaId identificador del programa. 
 *  @param pId identificador del proceso.
 *  @param estado del programa.
 *  @return un 1 en caso de que todo se ejecute correctamente, -1 en caso de algún error.
 */
int obtenerInfoProceso(int programaId, long *pId, int *estado){
	char buffer[1024];
    char *record,*line;
    char file[1];
    sprintf(file, "%d", programaId);
    FILE *fstream = fopen(file,"r");
    if(fstream == NULL){
    	printf("obtenerInfoProceso, error al abrir el archivo. %s\n", file);
        return -1;
    }
    while((line=fgets(buffer,sizeof(buffer),fstream))!=NULL){ 
     	record = strtok(line,",");
     	if(record != NULL){
     		*pId = atoi(record);
     	}else{
     		fclose(fstream);
     		return -1;
     	}
     	record = strtok(NULL,",");
     	if(record != NULL){
     		*estado = atoi(record);
     		if(*estado == DISABLE){
     			fclose(fstream);
     			return -1;
     		}
     	}else{
     		fclose(fstream);
     		return -1;
     	}
        break; 
    }     
    fclose(fstream);
    return 1;
}

/*
 *  Función que envia una señal a un proceso.
 *  @param programaId identificador del programa. 
 *  @param signalValue entero que se puede enviar en la señal.
 *  @param signalId el tipo de señal que se envia.
 *  @return 0 en caso de que se envie y se reciba la señal, -1 cuando ocurra algún error.
 */
int enviarSenal(int programId, int signalValue, int signalId){
	int estado;
	long pid;
	union sigval mysigval;
    obtenerInfoProceso(programId, &pid, &estado);
    mysigval.sival_int = signalValue;
    int send = sigqueue(pid, signalId, mysigval);
    return send;
}