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

void printMatrix(tMatrix matrix, int size, const char* name){ 
    printf("Matrix %s:\n", name); 
    for (int i = 0; i < size; i++){ 
        for (int j = 0; j < size; j++){ 
            printf("%d ", matrix[i*size + j]); 
        } 
        printf("\n"); 
    }
}
/**
* Generate a matrix with random int values
*/
void generateMatrix(tMatrix matrix){
    int i;

    for (i=0; i<(SIZE*SIZE); i++)
        matrix[i] = (rand() % MAX_INT_NUMBER);
}

int main(int argc, char* argv[]){

    int rank, numProc;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numProc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    if(SIZE % numProc != 0){
        printf("Error\n");
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }
    //estatica scatter(envismos determinadas lineas o mensajes a otros workers) y gather(enciamos todos los datos al root, master)
    matrixB = malloc(SIZE*SIZE*sizeof(int)); 
    matrixA = malloc(SIZE*SIZE*sizeof(int));
    matrixC = malloc(SIZE*SIZE*sizeof(int));

    tMatrix A_aux;
    A_aux = malloc(((SIZE*SIZE)/numProc)*sizeof(int));

    tMatrix C_aux;
    C_aux = malloc(((SIZE*SIZE)/numProc)*sizeof(int));
    
    if(rank == 0){
        generateMatrix(matrixB); //genero una matrizB
        generateMatrix(matrixA);        
    }

    MPI_Bcast(matrixB, SIZE*SIZE, MPI_INT, 0, MPI_COMM_WORLD);// recibimos Bcast para root envia, y para los demas recibe
    MPI_Scatter(matrixA, (SIZE*SIZE)/numProc, MPI_INT, A_aux, (SIZE*SIZE)/numProc,MPI_INT, 0, MPI_COMM_WORLD);// master envia, workers recibe
    for(int i = 0; i < SIZE/numProc; i++){ // Recorre las filas de A_aux
        for(int j = 0; j < SIZE; j++){ // Recorre las columnas de matrixB
            C_aux[i * SIZE + j] = 0; // Inicializa el valor de C_aux en 0
            for(int k = 0; k < SIZE; k++){ // Recorre las filas de matrixB y columnas de A_aux
                C_aux[i * SIZE + j] += A_aux[i * SIZE + k] * matrixB[k * SIZE + j]; // Realiza la multiplicación de matrices
            }
        }
    }

    MPI_Gather(C_aux, (SIZE * SIZE) / numProc, MPI_INT, matrixC, (SIZE * SIZE) / numProc, MPI_INT, 0, MPI_COMM_WORLD);

    

    if(rank == 0){ 
        printMatrix(matrixA, SIZE, "A"); 
        printMatrix(matrixB, SIZE, "B"); 
        printMatrix(matrixC, SIZE, "C"); 
    } 
    free(A_aux); 
    free(C_aux); 
    free(matrixB); 
    free(matrixA); 
    free(matrixC); 
    MPI_Finalize(); 
    return 0;

}