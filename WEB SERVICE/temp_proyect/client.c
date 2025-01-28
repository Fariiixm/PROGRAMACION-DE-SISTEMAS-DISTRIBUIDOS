#include "soapH.h"
#include "wsFileServer.nsmap"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DEBUG 1

void showError (char* msg){

	printf ("[Client] %s\n", msg);
	exit (1);
}

/**
 * Copy a file from server side to client side
 */
void receiveFile(char *host, char* src, char* dst) {
    struct soap soap;
    int fd, result, totalRead = 0, fileSize;
    wsFilens__tFileName fileName;
    wsFilens__tData data;

    // Inicializar SOAP
    soap_init(&soap);

    // Reservar memoria para el nombre del archivo origen
    fileName.__ptr = src;
    fileName.__size = strlen(src);

    // Obtener el tamaño del archivo en el servidor
    if (soap_call_wsFilens__getFileSize(&soap, host, "", fileName, &fileSize) != SOAP_OK || fileSize < 0) {
        soap_print_fault(&soap, stderr);
        showError("Error al obtener el tamaño del archivo en el servidor.");
    }

    // Crear el archivo destino en el cliente
    fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd < 0) {
        showError("Error al crear el archivo destino.");
    }

    // Leer y escribir datos en bloques
    while (totalRead < fileSize) {
        int length = (fileSize - totalRead) > MAX_DATA_SIZE ? MAX_DATA_SIZE : (fileSize - totalRead);
        if (soap_call_wsFilens__readFile(&soap, host, "", fileName, totalRead, length, &data) != SOAP_OK || data.__size <= 0) {
            close(fd);
            soap_print_fault(&soap, stderr);
            showError("Error al leer datos del servidor.");
        }

        // Escribir datos en el archivo destino
        if (write(fd, data.__ptr, data.__size) != data.__size) {
            close(fd);
            showError("Error al escribir datos en el archivo destino.");
        }
        totalRead += data.__size;
    }

    // Cerrar el archivo destino
    close(fd);

    // Limpiar el entorno SOAP
    soap_end(&soap);
}



/**
 * Copy a file from client side to server side
 */
void sendFile(char *host, char* src, char* dst) {
    struct soap soap;
    int fd, result, bytesRead, totalRead = 0;
    char buffer[MAX_DATA_SIZE];
    wsFilens__tFileName fileName;
    wsFilens__tData data;

    // Inicializar SOAP
    soap_init(&soap);

    // Reservar memoria para el nombre del archivo destino
    fileName.__ptr = dst;
    fileName.__size = strlen(dst);

    // Crear el archivo destino en el servidor
    if (soap_call_wsFilens__createFile(&soap, host, "", fileName, &result) != SOAP_OK || result != 0) {
        soap_print_fault(&soap, stderr);
        showError("Error al crear el archivo en el servidor.");
    }

    // Abrir el archivo origen en el cliente
    fd = open(src, O_RDONLY);
    if (fd < 0) {
        showError("Error al abrir el archivo de origen.");
    }

    // Leer y enviar datos en bloques
    while ((bytesRead = read(fd, buffer, MAX_DATA_SIZE)) > 0) {
        data.__ptr = (unsigned char *)buffer;
        data.__size = bytesRead;
        if (soap_call_wsFilens__writeFile(&soap, host, "", fileName, totalRead, data, &result) != SOAP_OK || result != 0) {
            close(fd);
            soap_print_fault(&soap, stderr);
            showError("Error al escribir datos en el servidor.");
        }
        totalRead += bytesRead;
    }

    // Cerrar el archivo origen
    close(fd);

    // Limpiar el entorno SOAP
    soap_end(&soap);
}



int main(int argc, char **argv){ 
    
  	// Check arguments
	if(argc != 5) {
		printf("Usage: %s http://server:port [sendFile|receiveFile] sourceFile destinationFile\n",argv[0]);
		exit(1);
	}

	// Check mode
	else{

		// Send file to server
		if (strcmp (argv[2], "sendFile") == 0){
			sendFile (argv[1], argv[3], argv[4]);
		}

		// Receive file from server
		else if (strcmp (argv[2], "receiveFile") == 0){
			receiveFile (argv[1], argv[3], argv[4]);
		}

		// Wrong mode!
		else{
			printf("Wrong mode!\nusage: %s serverIP [sendFile|receiveFile] sourceFile destinationFile\n", argv[0]);
			exit(1);
		}
	}	

  return 0;
}

