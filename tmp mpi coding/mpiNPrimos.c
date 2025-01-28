#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
/** Cantidad de números primos a generar */
//#define VECTOR_LENGTH 100000
#define VECTOR_LENGTH 100
/** Proceso master */
#define MASTER 0
/** Flag que indica el fin de ejecución */
#define END_OF_PROCESSING -1
// Prototipos de funciones


int generatePrimeNumber() {
    static int current = 2; // Empezamos con el primer número primo
    bool isPrime = false;

    while (!isPrime) {
        isPrime = true;
        for (int i = 2; i * i <= current; i++) {
            if (current % i == 0) {
                isPrime = false;
                break;
            }
        }
        if (isPrime) {
            return current++;
        }
        current++;
    }
    return current;
}

void executeMaster (int grainSize, int numProc){
    int vPrimos[VECTOR_LENGTH];
    int nrecv = 0, nsend = 0, nWorkers = numProc - 1;

    MPI_Status status;
    for(int i = 1; i < nWorkers; i++){
        MPI_Send(&vPrimos[nsend * grainSize], grainSize, MPI_INT, i, 0, MPI_COMM_WORLD); //enviamos las prociones a cada worker con tam grainz
        nsend++;
    }


    while(nrecv < VECTOR_LENGTH){

        MPI_Recv(&vPrimos[nrecv * grainSize], grainSize, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        nrecv++;
        int rankWorker = status.MPI_SOURCE;

        if(nsend < VECTOR_LENGTH){
            MPI_Send(&vPrimos[nsend * grainSize], grainSize, MPI_INT, rankWorker, 0, MPI_COMM_WORLD);
            nsend++;
        }else{
            
            MPI_Send(&END_OF_PROCESSING, 1, MPI_INT, rankWorker, 1, MPI_COMM_WORLD);
        }        
    }
    for(int i= 0; i < VECTOR_LENGTH;i++)
        printf("%d, ", vPrimos[i]);

    MPI_Finalize();


}
void executeWorker (int grainSize){

    MPI_Status status;
    int vporcion[grainSize];
    while(1){

        MPI_Recv(&vporcion, grainSize, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if(status.MPI_TAG == 1 && vporcion[0] == END_OF_PROCESSING)
            break;
        for (int i = 0; i < grainSize; i++){
            vporcion[i] = generatePrimeNumber();
        }
        
        MPI_Send(&vporcion, grainSize, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }


}

int main(int argc, char *argv[]){

    int numProc, rank;
    int grain;
    // Init MPI
    MPI_Init (&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProc);
    // Comprobar argumentos de entrada
    if (argc != 2){
        printf ("Número incorrecto de argumentos\n");
        MPI_Abort (MPI_COMM_WORLD, 0);
    }
    // Comprobar número de procesos
    if (numProc < 3){
        printf ("Número incorrecto de procesos\n");
        MPI_Abort (MPI_COMM_WORLD, 0);
    }
    // Obtener tamaño de grano
    grain = atoi(argv[1]);
    if ((grain > 100) || (grain < 10)){
        printf ("Tamaño incorrecto de grano\n");
        MPI_Abort (MPI_COMM_WORLD, 0);
    }
    // Proceso master ¡
    if (rank == MASTER){

        executeMaster (grain, numProc);
    }
    // Procesos worker
    else{
        executeWorker (grain);
    }

return 0;
}