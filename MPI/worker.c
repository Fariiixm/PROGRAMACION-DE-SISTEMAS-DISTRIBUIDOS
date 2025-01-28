#include "worker.h"

void workerSubprogram(int worldWidth){
    int worldHeight;
    unsigned short rows, start, actual = 0, end;
    unsigned short* worldA;
    unsigned short* worldB;

   
    //Enviamos la cantidad de filas a tratar
    //Se envia fuera del bucle para que en el dinamico concuerde luego en el bucle
    MPI_Recv(&rows, 1, MPI_UNSIGNED_SHORT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    //Es rows + 2 ya que se le envian las dos filas de contexto
    worldHeight = rows + 2; 

    //Reservamos memoria para los mundos
    worldA = malloc(worldHeight * worldWidth * sizeof(unsigned short));
    worldB = malloc(worldHeight * worldWidth * sizeof(unsigned short));
    clearWorld(worldA, worldWidth, worldHeight);
    clearWorld(worldB, worldWidth, worldHeight);

    while (rows > 0){
        
        MPI_Recv(&start, 1, MPI_UNSIGNED_SHORT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(worldA, worldWidth, MPI_UNSIGNED_SHORT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        actual++;//Aumenta porque justo se envia la fila previa
        MPI_Recv(worldA + (actual * worldWidth), rows * worldWidth, MPI_UNSIGNED_SHORT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        actual += rows;//Y sumamos toda la cantida de filas recibidas
        MPI_Recv(worldA + (actual * worldWidth), worldWidth, MPI_UNSIGNED_SHORT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        actual = 0;

        //Actualizamos el mundo segun la info que tenemos
        updateWorld(worldA, worldB, worldWidth, worldHeight);

        //Enviamos la informacion por paquetes: filas, start, el mundo
        MPI_Send(&rows, 1, MPI_UNSIGNED_SHORT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&start, 1, MPI_UNSIGNED_SHORT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(worldB + worldWidth, rows * worldWidth, MPI_UNSIGNED_SHORT, 0, 0, MPI_COMM_WORLD);

        //Y este ultimo Recv es para finalizar
        MPI_Recv(&end, 1, MPI_UNSIGNED_SHORT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if ((short)end == END_PROCESSING) {
           // MPI_Finalize();
            break;
            //Y se sale del bucle y cierra la fabrica worker
        }
    }

    free(worldA);
    free(worldB);
}

