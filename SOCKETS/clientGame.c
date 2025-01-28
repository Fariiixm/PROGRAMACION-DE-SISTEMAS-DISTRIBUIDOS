#include "clientGame.h"

#define MAX_MSG_LENGTH 256

void sendMessageToServer (int socketServer, char* message){
	if(send(socketServer, message, strlen(message), 0) < 0)
			showError("ERROR while writing to the socket");

}

void receiveMessageFromServer(int socketServer, char* message) {
	memset(message, 0, STRING_LENGTH);
	int size_mensje;
	recv(socketServer, &size_mensje, sizeof(int),0);
    ssize_t bytesReceived = recv(socketServer, message, size_mensje, 0);
    if (bytesReceived < 0) {
        showError("ERROR while reading from the socket");
    }
    message[bytesReceived] = '\0'; // Asegurar de que el string esta terminado
}


void receiveBoard (int socketServer, tBoard board){
	ssize_t bytesReceived = recv(socketServer, board, BOARD_WIDTH * BOARD_HEIGHT, 0);
	if (bytesReceived < 0) {
    	showError("ERROR while receiving the board");
	}	
}

unsigned int receiveCode (int socketServer){

	unsigned int code;
    ssize_t bytesReceived = recv(socketServer, &code, sizeof(unsigned int), 0);
    if (bytesReceived < 0) {
        showError("ERROR while receiving the code");
    } 
    return code;
}

unsigned int readMove (){

	tString enteredMove;
	unsigned int move;
	unsigned int isRightMove;


		// Init...
		isRightMove = FALSE;
		move = STRING_LENGTH;

		while (!isRightMove){

			printf ("Enter a move [0-6]:");

			// Read move
			fgets (enteredMove, STRING_LENGTH-1, stdin);

			// Remove new-line char
			enteredMove[strlen(enteredMove)-1] = 0;

			// Length of entered move is not correct
			if (strlen(enteredMove) != 1){
				printf ("Entered move is not correct. It must be a number between [0-6]\n");
			}

			// Check if entered move is a number
			else if (isdigit(enteredMove[0])){

				// Convert move to an int
				move =  enteredMove[0] - '0';

				if (move > 6)
					printf ("Entered move is not correct. It must be a number between [0-6]\n");
				else
					isRightMove = TRUE;
			}

			// Entered move is not a number
			else
				printf ("Entered move is not correct. It must be a number between [0-6]\n");
		}

	return move;
}

void sendMoveToServer (int socketServer, unsigned int move){

	send(socketServer, &move, sizeof(unsigned int), 0);
}



int main(int argc, char *argv[]){

	int socketfd;						/** Socket descriptor */
	unsigned int port;					/** Port number (server) */
	struct sockaddr_in server_address;	/** Server address structure */
	char* serverIP;						/** Server IP */

	tBoard board;						/** Board to be displayed */
	tString playerName;					/** Name of the player */
	tString rivalName;					/** Name of the rival */
	tString message;					/** Message received from server */
	unsigned int column;				/** Selected column */
	unsigned int code;					/** Code sent/receive to/from server */
	unsigned int endOfGame;				/** Flag to control the end of the game */



		// Check arguments!
		if (argc != 3){
			fprintf(stderr,"ERROR wrong number of arguments\n");
			fprintf(stderr,"Usage:\n$>%s serverIP port\n", argv[0]);
			exit(0);
		}

		// Get the server address
		serverIP = argv[1];

		// Get the port
		port = atoi(argv[2]);

		// Create socket
		socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		// Check if the socket has been successfully created
		if (socketfd < 0)
			showError("ERROR while creating the socket");

		// Fill server address structure

		memset(&server_address, 0, sizeof(server_address));
		server_address.sin_family = AF_INET;
		server_address.sin_addr.s_addr = inet_addr(serverIP);
		server_address.sin_port = htons(port);

		// Connect with server
		if (connect(socketfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
			showError("ERROR while establishing connection");

		printf ("Connection established with server!\n");

		// Init player's name
		do{
			memset(playerName, 0, STRING_LENGTH);
			printf ("Enter player name:");
			fgets(playerName, STRING_LENGTH-1, stdin);

			// Remove '\n'
			playerName[strlen(playerName)-1] = 0;

		}while (strlen(playerName) <= 2);


		// Send player's name to the server
		sendMessageToServer(socketfd, playerName);

		// Receive rival's name
		receiveMessageFromServer(socketfd, rivalName);

		printf ("You are playing against %s\n", rivalName);

		// Init
		endOfGame = FALSE;

		// Game starts
		printf ("Game starts!\n\n");
		// While game continues...
		while(!endOfGame){

			code = receiveCode(socketfd);

			if (code == GAMEOVER_WIN || code == GAMEOVER_LOSE || code == GAMEOVER_DRAW) {//Termina el juego
				endOfGame = TRUE;	
            	
			}

			
			receiveMessageFromServer(socketfd, message);
			receiveBoard(socketfd, board);
			printBoard(board, message);
			if(code == TURN_MOVE){//Si es nuestro turno enviamos el movimiento
				column = readMove();
				sendMoveToServer(socketfd,column);
			}
		}


		memset(message, 0, STRING_LENGTH);
		memset(playerName, 0, STRING_LENGTH);
		memset(rivalName, 0, STRING_LENGTH);
		memset(board, 0, sizeof(tBoard));




	// Close socket
	close(socketfd);


	

}
