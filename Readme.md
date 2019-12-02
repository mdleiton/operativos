Proyecto de Sistemas operativos
===============================

2019-2S - Primer Parcial
------------------------

# Descripción del proyecto:

    TaxiStation

Los taxistas amarillos de la ciudad XYZ quieren sacar a UBER del mercado pero no tienen el poder político para cambiar las leyes de tránsito. Entonces se les ocurre crear un APP y re-organizar la logística de su flota de taxis. Para atraer más clientes planean que sus taxis lleguen en menor tiempo que los de UBER. Su idea es tener taxis esperando en “sitios estratégicos” de la ciudad cuando no están atendiendo una llamada, el problema es definir esos sitios. Para eso lo contratan a usted y le dan un grafo con las distancias entre intersecciones de la ciudad. Le piden crear un sistema que:\

1) En el menor tiempo posible determinar los 3 mejores puntos de espera. Un punto de espera es aquel que aparece en como parte de la ruta de multiples viajes\

2) Recalcule automaticamente los 3 puntos si hay un cambio en las distancias entre intersecciones o una interseccion deja de estar habilitada por ejemplo por reparaciones en la via.\

Entonces su sistema debe de tener al menos dos componentes: (a) Un programa que cargue el grafo y permita cambiar sus valores y (b) Un programa que calcule y muestre los 3 puntos de espera al inicio y si el grafo cambia. Las acciones de a y b deben realizarse sin tener que reinicar ningun componente de su sistema.\

# Implementación


1. Diagrama.

![Alt text](demos/diagram.png "Diagrama")

2. Descripción

Se tiene dos procesos principales(de color celeste).\
1. El primer proceso es CargadorCsv y tiene las siguientes tareas al iniciarse:\

* Actualiza un archivo de nombre “0” con información del proceso: identificador del proceso y estado. Vital para que se pueda comunicar con proceso Resultados.\
* Verificar la cantidad de cores del computador y si es mayor a 1. Setea la afinidad al core 0.\
* Crea la memoria compartida que contendrá la matriz que representa el grafo.\
* Carga un grafo desde un archivo csv a una matriz ubicada en una memoria compartida.\
* Notifica vía señal su inicialización al proceso resultados para que acceda a la memoria compartida.\
* Solicita actualizaciones de pesos.\
* Actualiza la matriz que representa el grafo ubicado en la memoria compartida.\
* Notifica vía señal que se ha actualizado algún peso de algún nodo del grafo.\

2. El segundo proceso es Resultados y tiene las siguientes tareas al iniciarse:\
* Actualiza un archivo de nombre “1” con información del proceso: identificador del proceso y estado. Vital para que se pueda comunicar con proceso cargadorCsv por medio de señales.\
* Verificar la cantidad de cores del computador y si es mayor a 1. Setea la afinidad al core 1.\
* Solicita acceso a la memoria compartida que contendrá la matriz que representa el grafo.\
* Maneja señales para saber si el proceso de cargadorCsv se está ejecutando.\
* Al iniciarse o al recibir una señal que se ha actualizado el grafo se realizará:\
        ** Cálculo de distancias:\
            *** Por cada nodo se creará un hilo para que realice el cálculo de las distancias más cortas desde ese nodo hasta el resto de nodo.\
        ** Obtención de las tres mejores ubicaciones:\
            *** Se obtendrán las frecuencias de cada nodo y se almacenarán en un arreglo.\
            *** Con el algoritmo merge sort multihilo se ordena el arreglo con las frecuencias para saber cuáles son los nodos de mayor frecuencia.\

Para controlar el acceso a la matriz que representa el grafo. En la estructura se tiene un semáforo.\
Con el manejo de señales implementado se puedan ejecutar en cualquier orden y ambos saben cuál ha iniciado o finalizado de forma inmediata, presentando en consola dicha información.\

# Limitaciones de la implementación:
* El tamaño de matriz que se recibe es una constante definida en el archivo Estructuras.h(MAX). Para recibir una matriz que represente un grafo de diferente tamaño a 14(tamaño de matriz permitido actualmente) se debe modificar esa constante y compilar el código otra vez.\
* Actualmente de las señales estándares, solo se está manejando la señal SIGINT(control + C). Pero se pueden incorporar más manejadores para otras señales estándares de forma fácil.\

# Ejecución:

Para compilar:\
```
    $ make all
```

Existen dos programas que se deben ejecutar en cualquier orden.\
    1. El programa para cargar la matriz\
```
        $ ./ecargadorDatos matrizDistancia.csv
```
        Este mismo programa pregunta si desea ingresar actualizaciones. El formato para ingresar actualizaciones es:\
            nodoinicio-nodofinal-peso\
            Ejemplo: 11-13-1\
    2. El programa para realizar los cálculos y mostrar los resultados\
```
    $ ./eresultados
```

Para eliminar los binarios/archivos creados:\
```
    $make clean
```

----------------------------------
### Autor ###
* Mauricio Leiton Lázaro [github](https://github.com/mdleiton)