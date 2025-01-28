#include "graph.h"
#include "mpi.h"

// Enables/Disables the log messages from the master process
#define DEBUG_MASTER 0

// Probability that a cataclysm may occur [0-100] :(
#define PROB_CATACLYSM 100

// Number of iterations between two possible cataclysms
#define ITER_CATACLYSM 5


void masterSubprogram(SDL_Window *window, SDL_Renderer *renderer, int nproc, int worldWidth, int worldHeight, int totalIterations, int autoMode, int distModeStatic, int grainSize, char* outputFile);
//LOS 2 tipos de procesos
void dynamicProcess(unsigned short *worldA, unsigned short *worldB, SDL_Renderer *renderer, int worldWidth, int worldHeight, int size, int grainSize);
void staticProcess(unsigned short *worldA, unsigned short *worldB, SDL_Renderer *renderer, int worldWidth, int worldHeight, int size);
//Para las filas de contexto
unsigned short *sigRow(unsigned short *world, int currRow, int worldWidth, int worldHeight);
unsigned short *antesRow(unsigned short *world, int currRow, int worldWidth, int worldHeight);
//Para los envios y recibidas
void sendRowsToWorker(int workerRank, unsigned short rows, unsigned short start, unsigned short *worldA, int worldWidth, int worldHeight);
void receiveAndDraw(SDL_Renderer *renderer,unsigned short *worldB, unsigned short *worldA, int worldWidth, int wRank, unsigned short rows, unsigned short start, int worldHeight);


