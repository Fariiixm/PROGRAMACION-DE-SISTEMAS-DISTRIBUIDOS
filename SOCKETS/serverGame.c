#include "serverGame.h"
#include <stdlib.h>
#include <pthread.h>


#define MAX_MSG_LENGTH 256
#define MAX_LAST_GAMES 3
#define FILEREGISTER "last_games.txt"

typedef struct {
	char* winner;
	char* loser;
}tGameResult;

pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;
tGameResult lastGames[MAX_LAST_GAMES];
int gameCount = 0;

void sendMessageToPlayer (int socketClient, char* message){
	int size_message=strlen(message);
	send(socketClient, &size_message, sizeof(int),0);
	if(send(socketClient, message, size_message, 0) < 0)
			showError("ERROR while writing to the socket");

}

void receiveMessageFromPlayer (int socketClient, char* message){
	memset(message, 0, STRING_LENGTH);
	ssize_t bytesReceived = recv(socketClient, message, MAX_MSG_LENGTH - 1, 0);
    if (bytesReceived < 0)
        showError("ERROR while reading from the socket");
    message[bytesReceived] = '\0';  

}

void sendCodeToClient (int socketClient, unsigned int code){  
    if (send(socketClient, &code, sizeof(unsigned int), 0) < 0) {
        showError("ERROR while writing to the socket");
    }
	

}

void sendBoardToClient (int socketClient, tBoard board){
	if(send(socketClient, board, BOARD_WIDTH * BOARD_HEIGHT, 0) < 0)
		showError("ERROR while writing to the socket");
	

}

unsigned int receiveMoveFromPlayer (int socketClient){
	unsigned int column;
    if (recv(socketClient, &column, sizeof(unsigned int), 0) < 0)
        showError("ERROR while receiving the move");
    return column;
}

int getSocketPlayer (tPlayer player, int player1socket, int player2socket){

	int socket;

		if (player == player1)
			socket = player1socket;
		else
			socket = player2socket;

	return socket;
}

tPlayer switchPlayer (tPlayer currentPlayer){

	tPlayer nextPlayer;

		if (currentPlayer == player1)
			nextPlayer = player2;
		else
			nextPlayer = player1;

	return nextPlayer;
}


void saveResultsToFile() {
	//Guarda los resultados de las partidas en un fichero
	FILE *file = fopen(FILEREGISTER, "w");
	if(!file) {
		showError("ERROR while opening file to save results");
		return;
	}

	for(int i = 0; i < gameCount; ++i){

		fprintf(file, "Winner: %s, Loser: %s\n", lastGames[i].winner, lastGames[i].loser);
	}

	fclose(file);
}

void updateLastGames(char* winner, char* loser) {

	//Para que no se solapen las escrituras en el fichero usamos un cerrojo
    pthread_mutex_lock(&fileMutex);

    if (gameCount == MAX_LAST_GAMES) {
        //Desplazar juegos anteriores
        free(lastGames[0].winner);
        free(lastGames[0].loser);
        for (int i = 1; i < MAX_LAST_GAMES; ++i) {
            lastGames[i - 1] = lastGames[i];
        }
        gameCount--;
    }

    //Asignar memoria para el nuevo resultado
    lastGames[gameCount].winner = malloc(strlen(winner) + 1);
    lastGames[gameCount].loser = malloc(strlen(loser) + 1);

    //Comprobar asignacion
    if (lastGames[gameCount].winner == NULL || lastGames[gameCount].loser == NULL) {
        showError("ERROR allocating memory for game results");
        pthread_mutex_unlock(&fileMutex);
        return;
    }

	//Copiamos los nombres de los jugadores
    strcpy(lastGames[gameCount].winner, winner);
    strcpy(lastGames[gameCount].loser, loser);
    gameCount++;

    saveResultsToFile();
    pthread_mutex_unlock(&fileMutex);
}


void *gameLogic(void *threadArgs) {
	tBoard board;						/** Board of the game */
	tPlayer currentPlayer;				/** Current player */
	tMove moveResult;					/** Result of player's move */
	tString player1Name;				/** Name of player 1 */
	tString player2Name;				/** Name of player 2 */
	int endOfGame;						/** Flag to control the end of the game*/
	unsigned int column;				/** Selected column to insert the chip */
	tString message;					/** Message sent to the players */

	struct threadArgs *args = (struct threadArgs *) threadArgs;
	int socketPlayer1 = args->socketPlayer1;
	int socketPlayer2 = args->socketPlayer2;

	receiveMessageFromPlayer(socketPlayer1, player1Name);
	printf("Player 1 name received: %s\n", player1Name); 
	sendMessageToPlayer(socketPlayer2, player1Name);

	receiveMessageFromPlayer(socketPlayer2, player2Name);
	printf("Player 2 name received: %s\n", player2Name); 
	sendMessageToPlayer(socketPlayer1, player2Name);

	//Inicializar tablero
	initBoard(board);

	//Elegimos el jugador que va a empezar
	int inicialplayer = rand();
	
	if (inicialplayer % 2 == 0) {
		currentPlayer = player1;
		
	} else {
		currentPlayer = player2;
	}

	endOfGame = FALSE;
	int failedMove = FALSE;
	int firstPlay = TRUE;

	while(!endOfGame){
		//El jugador que le toca mover
		sendCodeToClient(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2), TURN_MOVE);

		if(failedMove == TRUE){//Si el movimiento anterior no ha sido valido, se lo indicamos
			sendMessageToPlayer(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2), "The column is full, try again with another one\n");
			failedMove = FALSE;
		}
		else {
			if(firstPlay == TRUE) {//Mostramos el mensaje de que fichas va a utilizar el jugador que empieza
				if(currentPlayer == player1) {
					sprintf(message, "Its your turn. You play with: %c\n", PLAYER_1_CHIP);
				}
				else {
					sprintf(message, "Its your turn. You play with: %c\n", PLAYER_2_CHIP);
				}

			}
			else sprintf(message, "Its your turn\n");
			sendMessageToPlayer(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2), message);
		}
		//Se le envia el tablero al jugador
		sendBoardToClient(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2), board);

		currentPlayer = switchPlayer(currentPlayer);

		//El jugador que le toca esperar
		sendCodeToClient(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2), TURN_WAIT);

		if(firstPlay == TRUE){
			if(currentPlayer == player1){//Mostramos el mensaje de que fichas va a utilizar el jugador que espera
				sprintf(message, "Your rival is playing, please wait... You play with: %c\n", PLAYER_1_CHIP);
			}
			else {
				sprintf(message, "Your rival is playing, please wait... You play with: %c\n", PLAYER_2_CHIP);
			}

		}
		else sprintf(message, "Your rival is playing, please wait...\n");
		sendMessageToPlayer(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2), message);
		sendBoardToClient(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2), board);
		/* **************************************************** */

		currentPlayer = switchPlayer(currentPlayer);
		//Recibimos el movimiento del jugador
		column = receiveMoveFromPlayer(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2));
		//Comprobamos que el movimiento es correcto
		moveResult = checkMove(board, column);

		if(moveResult ==  OK_move){//El movimiento es valido y entra al if
			insertChip(board, currentPlayer, column);//Insertamos la ficha en la posicion que ha escogido el jugador
			if(checkWinner(board, currentPlayer) == TRUE){//Se comprueba si ha sido un movimiento ganador
				
				//Guardamos el resultado de la partida en el fichero
				if(currentPlayer == player1){
					updateLastGames(player1Name, player2Name);
				}else{
					updateLastGames(player2Name, player1Name);
				}
				//Enviamos el codigo y mensaje al ganador
				sendCodeToClient(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2), GAMEOVER_WIN);
				sendMessageToPlayer(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2), "You win!\n");
				sendBoardToClient(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2), board);
				
				//Cambiamos el jugador
				currentPlayer = switchPlayer(currentPlayer);

				//Enviamos el codigo y mensaje al perdedor
				sendCodeToClient(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2), GAMEOVER_LOSE);
				sendMessageToPlayer(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2), "You lose\n");
				sendBoardToClient(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2), board);

				currentPlayer = switchPlayer(currentPlayer);

				endOfGame = TRUE;

			}else if(isBoardFull(board)){//Si el tablero esta lleno, la partida acaba en empate
				//Con este resultado de partida no se modifica el fichero que registra las partidas
				//Se envian los mensajes de empate a ambos jugadores
				sendCodeToClient(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2), GAMEOVER_DRAW);
				sendMessageToPlayer(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2), "It's a draw!\n");

				currentPlayer = switchPlayer(currentPlayer);

				sendCodeToClient(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2), GAMEOVER_DRAW);
				sendMessageToPlayer(getSocketPlayer(currentPlayer, socketPlayer1, socketPlayer2), "It's a draw!\n");
				
				currentPlayer = switchPlayer(currentPlayer);
				initBoard(board);

			}else {
				//El movimiento es aceptado, se cambia de jugador
				//Y la partida sigue
				currentPlayer = switchPlayer(currentPlayer);
			}
		}else{
			//El movimiento no se ha validado porque la columna estaba llena
			failedMove = TRUE;
		}
		//Usamos esta variable para mostrar en el primer movimiento
		//Que fichas van a utilizar a lo largo de la partida
		firstPlay = FALSE;
	}

	
	memset(message, 0, STRING_LENGTH);
	memset(player1Name, 0, STRING_LENGTH);
	memset(player2Name, 0, STRING_LENGTH);
	memset(board, 0, sizeof(tBoard));

	return NULL;
}

int main(int argc, char *argv[]){

	int socketfd;						/** Socket descriptor */
	struct sockaddr_in serverAddress;	/** Server address structure */
	unsigned int port;					/** Listening port */
	struct sockaddr_in player1Address;	/** Client address structure for player 1 */
	struct sockaddr_in player2Address;	/** Client address structure for player 2 */
	int socketPlayer1, socketPlayer2;	/** Socket descriptor for each player */
	unsigned int clientLength;			/** Length of client structure */

	pthread_t threadID; 	 
	struct threadArgs *args; //Puntero a la estructura de los hilos



	// Check arguments
	if (argc != 2) {
		fprintf(stderr,"ERROR wrong number of arguments\n");
		fprintf(stderr,"Usage:\n$>%s port\n", argv[0]);
		exit(1);
	}

	// Init seed
	srand(time(NULL));

	// Create the socket
	socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check
	if (socketfd < 0)
		showError("ERROR while opening socket");

	// Init server structure
	memset(&serverAddress, 0, sizeof(serverAddress));

	// Get listening port
	port = atoi(argv[1]);

	// Fill server structure
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(port);		

	// Bind
	if (bind(socketfd,(struct sockaddr *) &serverAddress,sizeof(serverAddress)) < 0)
		showError("ERROR while binding");


	// Listen
	listen(socketfd, 10);

	//Inicializamos el cerrojo
	if(pthread_mutex_init(&fileMutex, NULL) != 0) {
		showError("ERROR while initializing the mutex");
	}

	while(1) {
		//length del cliente 1
		clientLength = sizeof(player1Address);

		//Accept
		socketPlayer1 = accept(socketfd, (struct sockaddr *) &player1Address, &clientLength);

		if (socketPlayer1 < 0){
			showError("ERROR while accepting");
		}
		
		printf("Player 1 is connected!\n");

		/*********************************/
		//Informacion del cliente 2
		clientLength = sizeof(player2Address);

		socketPlayer2 = accept(socketfd, (struct sockaddr *) &player2Address, &clientLength);

		if(socketPlayer2 < 0){
			showError("ERROR while accepting");
		}	

		printf("Player 2 is connected!\n");
		//Creacion de un hilo para cada partida
		args = (struct threadArgs *) malloc(sizeof(struct threadArgs));

		if(args == NULL) {
			showError("ERROR while allocating memory for thread arguments");
		}

		//Asignamos los sockets a la estructura
		args->socketPlayer1 = socketPlayer1;
		args->socketPlayer2 = socketPlayer2;

		
		if(pthread_create(&threadID, NULL, (void *) &gameLogic, (void *) args) != 0){

			showError("ERROR while creating the thread for client");
		}

		if(pthread_detach(threadID) != 0) {
			
    		showError("ERROR while detaching the thread");
		}
	}

	pthread_mutex_destroy(&fileMutex);//Destruimos el cerrojo

	free(args);
	close(socketPlayer1);
	close(socketPlayer2);
	close(socketfd);

	return 0;
}
