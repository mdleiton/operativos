#define ACTUALIZANDO -1
#define ACTUALIZADO   0
#define ID_MC 123
#define INFINITY 9999
#define MAX 4


struct Datos {
	int  status;
	int grafo[MAX][MAX];
	int cantidadNodos;

};


void imprimirMatriz(int grafo[MAX][MAX]){
	for(int a=0; a<MAX; a++){    
        for(int b=0; b<MAX; b++){
          printf("-%d-",grafo[a][b]);
        }
     printf("\n");
     }

}