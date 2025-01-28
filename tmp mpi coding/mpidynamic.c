#include <mpi.h> 
#include <stdio.h>
#include <stdlib.h>

/** Matrix size*/
#define SIZE 1000
/** Maximum number value for generating each matrix */
#define MAX_INT_NUMBER 10
/** Matrix type */
typedef int* tMatrix;

tMatrix matrixA; /** First matrix */
tMatrix matrixB; /** Second matrix */
tMatrix matrixC; /** Resulting matrix */

/**
* Generate a matrix with random int values
*/
void generateMatrix(tMatrix matrix){
    int i;
    for (i=0; i<(SIZE*SIZE); i++)
        matrix[i] = (rand() % MAX_INT_NUMBER);
}



int main(int argc, char* argv[]){

    int myrank, numProc;
    // Init MPI
    MPI_Init (&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProc);

    MPI_Status status;
    matrixA = malloc(SIZE*SIZE*sizeof(int));
    matrixB = malloc(SIZE*SIZE*sizeof(int));
    matrixC = malloc(SIZE*SIZE*sizeof(int));
    
    if(myrank == 0){//el master
        

        generateMatrix(matrixA);

        generateMatrix(matrixB);

        MPI_Bcast(matrixB, SIZE * SIZE * sizeof(int), MPI_INT, 0, MPI_COMM_WORLD);

        int rows_send = 0, rows_receive = 0, nWorkers = numProc - 1;

        for(int i = 0; i < numProc - 1; i++){
            MPI_Send(&matrixA[rows_send * SIZE], SIZE, MPI_INT, i, 0, MPI_COMM_WORLD);//enviaos a acada worker una porcion
            rows_send++;
        }

        while(rows_receive < SIZE){//recepcion de lo datos
           
            MPI_Recv(&matrixC[rows_receive * SIZE], SIZE, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            int workerRank = status.MPI_SOURCE;
            rows_receive++;

            if(rows_send < SIZE){//no se envio todas las filas, asignar al worker qe acabo
                MPI_Send(&matrixA[rows_send * SIZE], SIZE, MPI_INT, workerRank, 0, MPI_COMM_WORLD);
                rows_send++;
            }else{
                int end_signal = -1;
                MPI_Send(&end_signal, 1, MPI_INT, workerRank, 1, MPI_COMM_WORLD);
            }
            
        }


        printf("Matriz C:\n");
        for(int i = 0; i < SIZE; i++){
            for(int j = 0; j < SIZE; j++){
                printf("%d", matrixC[i * SIZE +j]);
            }
            printf("\n");
        }

    }else{//workers

        MPI_Bcast(matrixB, SIZE * SIZE * sizeof(int),MPI_INT, 0, MPI_COMM_WORLD); //recibiemos la matriz b tambien

        while(1){
            int portion[SIZE];
            
            MPI_Recv(portion, SIZE, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status); //y la porcion
            
            if(status.MPI_TAG == 1){//el encio de finalizacion
                break;
            }

            for(int i = 0; i < SIZE; i++){
                for(int j = 0; j < SIZE; j++){
                    matrixC[i * SIZE + j] += portion[j] * matrixB[j * SIZE + i];
                }
            }

            MPI_Send(portion, SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }

    }

    MPI_Finalize();


    return 0;    
}