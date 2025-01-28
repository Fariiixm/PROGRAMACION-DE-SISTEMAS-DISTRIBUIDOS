#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>

/** Amount of numbers to be processed */
#define MAX_NUMBERS 15
/** Array size to allocate the results */
#define RESULTS_SIZE 9
/** Workload to be processed by each worker */
#define GRAIN 4
/** Master process */
#define MASTER 0
/** End-of-processing flag */
#define END_OF_PROCESSING -1
// Funtion prototypes
void createNumbers(int* vector, int maxNum) {
    // Inicializar la semilla para números aleatorios
    srand(time(NULL));
    
    for (int i = 0; i < maxNum; i++) {
        vector[i] = rand() % 100 + 1; // Generar números aleatorios entre 1 y 100
    }
}
void executeMaster (int *array, int numProc){
    
    int nRecv = 0, nSend = 0;
    for(int i = 1; i < numProc - 1; i++){
        MPI_Send(&array[nSend * GRAIN], GRAIN, MPI_INT, i, 0, MPI_COMM_WORLD);
        nSend++;
    }
    int resul[RESULTS_SIZE] = { 0 };
    int aux[RESULTS_SIZE] = { 0 };
    MPI_Status status;
    while(nRecv < MAX_NUMBERS){
        MPI_Recv(&array[nRecv * GRAIN], GRAIN, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        MPI_Recv(&aux, RESULTS_SIZE, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        
        // Acumular los resultados recibidos
        for (int i = 0; i < RESULTS_SIZE; i++) {
            resul[i] += aux[i];
        }
        nRecv++;
        int workerRank = status.MPI_SOURCE;
        if(nSend < MAX_NUMBERS){//falta por enviar
            MPI_Send(&array[nSend * GRAIN], GRAIN, MPI_INT, workerRank, 0, MPI_COMM_WORLD);
            nSend++;
        }else{
            int endFlag = END_OF_PROCESSING;
            MPI_Send(&endFlag, 1, MPI_INT, workerRank, 1, MPI_COMM_WORLD);
        }
    }

    for(int i= 0; i < MAX_NUMBERS;i++){
        printf(" %d ", array[i]);
    }
    printf("\n");

    for(int i= 0; i < RESULTS_SIZE;i++){
        printf(" %d ", resul[i]);
    }
    printf("\n");
}
void executeWorker(){

    int porcion[GRAIN];
    int resul[RESULTS_SIZE];
    MPI_Status status;
    while(1){
        MPI_Recv(&porcion, GRAIN, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        if(status.MPI_TAG == 1 || porcion[0] == END_OF_PROCESSING) 
            break;
        memset(resul, 0, RESULTS_SIZE * sizeof(int));
        for(int i= 0; i < GRAIN; i++){
            for(int k = 1; k <= RESULTS_SIZE; k++){
                if(porcion[i]%k == 0){
                    resul[k - 1]++;
                }
            }
        }

        MPI_Send(&porcion, GRAIN, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&resul, RESULTS_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

}
int main(int argc, char *argv[]){
    int numProc, rank;
    int totalNum;
    int *array;
    // Init MPI
    MPI_Init (&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProc);
    // Check
    if (numProc < 3){
        printf ("Wrong number of parameters\n");
        MPI_Abort (MPI_COMM_WORLD, 0);
    }
    // Check the number of arguments
    if (argc != 1){
        printf ("Wrong number of parameters\n");
        MPI_Abort (MPI_COMM_WORLD, 0);
    }
    if (GRAIN*(numProc-1) > MAX_NUMBERS){
        printf ("Wrong number\n");
        MPI_Abort (MPI_COMM_WORLD, 0);
    }
    // Master process
    if (rank == MASTER){
        // Allocate memory and create random numbers
        array = (int*) malloc (MAX_NUMBERS * sizeof(int));
        createNumbers (array, MAX_NUMBERS);
        // Execute Master!
        executeMaster (array, numProc);
    }
    // Worker process
    else{
        executeWorker ();
    }
    // End MPI nvironment
    MPI_Finalize();
    return 0;
}