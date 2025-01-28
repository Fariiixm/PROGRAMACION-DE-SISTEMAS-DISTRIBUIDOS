#include "master.h"

void masterSubprogram(SDL_Window *window, SDL_Renderer *renderer, int size, int worldWidth, int worldHeight, int totalIterations, int autoMode, int distModeStatic, int grainSize, char* outputFile) {
    
    
    int isquit = 0;
    SDL_Event event;

    //WorldA representa el siguiente estado del mundo
    //WorldB representa el estado actual del mundo
    unsigned short *worldA = malloc(worldWidth * worldHeight * sizeof(unsigned short));
    unsigned short *worldB = malloc(worldWidth * worldHeight * sizeof(unsigned short));


    
    clearWorld(worldA, worldWidth, worldHeight);
    clearWorld(worldB, worldWidth, worldHeight);

    
    initRandomWorld(worldA, worldWidth, worldHeight);
    printf("World size: %dx%d\n", worldWidth, worldHeight);

    
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer);
    drawWorld(worldB, worldA, renderer, 0, worldHeight - 1, worldWidth, worldHeight);
    SDL_RenderPresent(renderer);
    SDL_UpdateWindowSurface(window);
   
    for (int iteration = 1; iteration <= totalIterations && !isquit; iteration++) {
        
        if (!autoMode) {

            printf("PRESS ENTER to continue: ITER[%d/%d]\n", iteration, totalIterations);
            fflush(stdout);
            getchar(); 
        }
        else {

            printf("ITER[%d/%d]\n", iteration, totalIterations);
            fflush(stdout);
        }

    
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer);

        if ((iteration % ITER_CATACLYSM == 0)){

            int aleatorio = rand() % 100;
            if(aleatorio < PROB_CATACLYSM){
                 
                printf("Oh dios un cataclismo...\n");
                int filaMedio = (worldHeight - 1) / 2;
                int colMedio = (worldWidth - 1) / 2;

                for(int col = 0; col < worldWidth; col++){

                    worldA[filaMedio * worldWidth + col] = CELL_CATACLYSM;
                }

                for(int fila = 0; fila < worldHeight; fila++){

                    worldA[fila * worldWidth + colMedio] = CELL_CATACLYSM;
                }

            }
        }

        //Aqui comprobamos si es en modo estatico o modo dinamico
        if (distModeStatic){
            staticProcess(worldA, worldB, renderer, worldWidth, worldHeight, size);
        }  
        else{
            dynamicProcess(worldA, worldB, renderer, worldWidth, worldHeight, size, grainSize);
        }

        //Hasta que no ejecutamos esto no se dibuja en la pantalla
        SDL_RenderPresent(renderer);
        SDL_UpdateWindowSurface(window);
         
        unsigned short *aux = worldA;
        worldA = worldB;
        worldB = aux;

        clearWorld(worldB, worldWidth, worldHeight);   
    }

    //Se le envia a los workers 0 filas para que sepan
    //que su labor ha terminaod
    unsigned short end = (unsigned short)END_PROCESSING;
    for (int workerRank = 1; workerRank < size; workerRank++){
        MPI_Send(&end, 1, MPI_UNSIGNED_SHORT, workerRank, MASTER, MPI_COMM_WORLD); //El valor de la etiqueta master es 0
    }

    //Aqui guardamos la imagen
    if(outputFile != NULL){
        saveImage(renderer, outputFile, worldWidth * CELL_SIZE, worldHeight * CELL_SIZE);
    }

    //MPI_Finalize();
    
    free(worldA);
    free(worldB);

    printf("EL JUEGO HA ACABADO, pulse ENTER...\n");
    fflush(stdout);
    getchar();
}


void sendRowsToWorker(int workerRank, unsigned short rows, unsigned short start, unsigned short *worldA, int worldWidth, int worldHeight) {
    //Enviamos las filuquis
    MPI_Send(&rows, 1, MPI_UNSIGNED_SHORT, workerRank, MASTER, MPI_COMM_WORLD);
    //Se envia donde se empieza
    MPI_Send(&start, 1, MPI_UNSIGNED_SHORT, workerRank, MASTER, MPI_COMM_WORLD);
    //Enviamos la fila de contexto anterior
    MPI_Send(antesRow(worldA, start, worldWidth, worldHeight), worldWidth, MPI_UNSIGNED_SHORT, workerRank, MASTER, MPI_COMM_WORLD);
    //Las filas a tratar
    MPI_Send(worldA + (start * worldWidth), rows * worldWidth, MPI_UNSIGNED_SHORT, workerRank, MASTER, MPI_COMM_WORLD);
    //Y la fila de contexto posterior
    MPI_Send(sigRow(worldA, start + rows, worldWidth, worldHeight), worldWidth, MPI_UNSIGNED_SHORT, workerRank, MASTER, MPI_COMM_WORLD);
}

void receiveAndDraw(SDL_Renderer *renderer,unsigned short *worldB, unsigned short *worldA, int worldWidth, int wRank, unsigned short rows, unsigned short start, int worldHeight) {
    MPI_Status status;
    //Recibir el número de filas a procesar
    MPI_Recv(&rows, 1, MPI_UNSIGNED_SHORT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
    wRank = status.MPI_SOURCE;

    //Recibir el índice de inicio
    MPI_Recv(&start, 1, MPI_UNSIGNED_SHORT, wRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //Recibir las filas procesadas
    MPI_Recv(worldB + (start * worldWidth), rows * worldWidth, MPI_UNSIGNED_SHORT, wRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    drawWorld(worldA, worldB, renderer, start, start + rows, worldWidth, worldHeight);
}

unsigned short *antesRow(unsigned short *world, int actRow, int worldWidth, int worldHeight) {
    if (actRow == 0){
        return world + (worldHeight - 1) * worldWidth;
    }
    else{
        return world + (actRow - 1) * worldWidth;
    }   
}

unsigned short *sigRow(unsigned short *world, int actRow, int worldWidth, int worldHeight) {
    if (actRow == worldHeight - 1){
        return world;
    }
    else{
        return world + (actRow + 1) * worldWidth;
    }
}

void staticProcess(unsigned short *worldA, unsigned short *worldB, SDL_Renderer *renderer, int worldWidth, int worldHeight, int size) {
    unsigned short rows = worldHeight / (size - 1);
    unsigned short start = 0;
    MPI_Status status;
    

    // Enviar datos iniciales a todos los trabajadores
    for (int workerRank = 1; workerRank < size; workerRank++) {
        
        // Si es el último trabajador, ajustar las filas para incluir el resto
        if (workerRank == (size - 1)) {
            rows += worldHeight % (size - 1);
        }

        // Enviar los datos al trabajador
        sendRowsToWorker(workerRank, rows, start, worldA, worldWidth, worldHeight);

        start += rows;
    }

    int wRank;
    // Recibir datos de todos los trabajadores
    for (int workerRank = 1; workerRank < size; workerRank++) {
        
        receiveAndDraw(renderer, worldB, worldA, worldWidth, wRank, rows, start, worldHeight);
    }
}


void dynamicProcess(unsigned short *worldA, unsigned short *worldB, SDL_Renderer *renderer, 
                    int worldWidth, int worldHeight, int size, int grainSize) {
    unsigned short rows = grainSize;
    unsigned short rowsSent = 0, rowsReceived = 0;
    unsigned short start = 0;

    // Enviar tareas iniciales a los trabajadores
    for (int workerRank = 1; workerRank < size; workerRank++) {
        sendRowsToWorker(workerRank, rows, start, worldA, worldWidth, worldHeight);
        start += rows;
        rowsSent += rows;
    }

    MPI_Status status;
    int wRank;

    
    while (rowsReceived < worldHeight){
        //Recibimos la informacion necesaria
        //filas, inicio y el mundo ya actualizado
        MPI_Recv(&rows, 1, MPI_UNSIGNED_SHORT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        wRank = status.MPI_SOURCE;

        
        MPI_Recv(&start, 1, MPI_UNSIGNED_SHORT, wRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(worldB + (start * worldWidth), rows * worldWidth, MPI_UNSIGNED_SHORT, wRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        //Pintamos el mundo
        drawWorld(worldA, worldB, renderer, start, start + rows, worldWidth, worldHeight);
              

        rowsReceived += rows;

        if (rowsSent < worldHeight) {
            if ((rowsSent + grainSize) > worldHeight){
                rows = worldHeight - rowsSent;
            }else{
                rows = grainSize;
            }

            sendRowsToWorker(wRank, rows, rowsSent, worldA, worldWidth, worldHeight);
            rowsSent += rows;
        }

    }
}


