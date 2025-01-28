#include "soapH.h"
#include "wsFileServer.nsmap"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DEBUG 1

int calculateFileSize(char* fileName);

int main(int argc, char **argv) { 
    int primarySocket, secondarySocket;
    struct soap soap;

    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Initialize SOAP environment
    soap_init(&soap);

    // Bind to the specified port
    if ((primarySocket = soap_bind(&soap, NULL, atoi(argv[1]), 100)) < 0) {
        soap_print_fault(&soap, stderr);
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on port %s (socket: %d)\n", argv[1], primarySocket);

    // Accept connections in a loop
    while (1) {
        // Accept a new connection
        if ((secondarySocket = soap_accept(&soap)) < 0) {
            soap_print_fault(&soap, stderr);
            break;
        }

        printf("Connection from IP: %d.%d.%d.%d (socket: %d)\n",
               (soap.ip >> 24) & 0xFF, (soap.ip >> 16) & 0xFF,
               (soap.ip >> 8) & 0xFF, soap.ip & 0xFF, secondarySocket);

        // Serve the invoked operation
        if (soap_serve(&soap) != SOAP_OK) {
            soap_print_fault(&soap, stderr);
        }

        // Clean up after the request
        soap_end(&soap);
    }

    // Clean up the SOAP environment
    soap_done(&soap);

    return 0;
}

int calculateFileSize(char* fileName) {
    struct stat st;
    int result;

    if (stat(fileName, &st) == -1) {
        printf("[calculateFileSize] Error while executing stat(%s)\n", fileName);
        result = -1;
    } else {
        result = st.st_size;
    }

    return result;
}

int wsFilens__getFileSize(struct soap *soap, wsFilens__tFileName fileName, int *result) {
    *result = calculateFileSize(fileName.__ptr);

    if (DEBUG)
        printf("wsFilens__getFileSize (%s) = %d\n", fileName.__ptr, *result);

    return SOAP_OK;
}

int wsFilens__createFile(struct soap *soap, wsFilens__tFileName fileName, int *result) {
    int fd;

    *result = 0;

    // Create file
    fd = open(fileName.__ptr, O_WRONLY | O_TRUNC | O_CREAT, 0777);

    // Error?
    if (fd < 0) {
        printf("[wsFilens__createFile] Error while creating(%s)\n", fileName.__ptr);
        *result = -1;
    } else {
        close(fd);
    }

    if (DEBUG)
        printf("wsFilens__createFile (%s) = %d\n", fileName.__ptr, *result);

    return SOAP_OK;
}

int wsFilens__readFile(struct soap *soap, wsFilens__tFileName fileName, int offset, int length, wsFilens__tData *data) {
    int fd;
    int totalRead = 0, bytesRead = 0;
    char buffer[MAX_DATA_SIZE];

    // Inicializar la estructura de salida
    data->__size = 0;
    data->__ptr = NULL;

    // Abrir el archivo en modo lectura
    fd = open(fileName.__ptr, O_RDONLY);
    if (fd < 0) {
        printf("[wsFilens__readFile] Error al abrir el archivo (%s)\n", fileName.__ptr);
        return SOAP_OK;
    }

    // Posicionarse en el offset especificado
    if (lseek(fd, offset, SEEK_SET) < 0) {
        printf("[wsFilens__readFile] Error al posicionarse en el offset (%d)\n", offset);
        close(fd);
        return SOAP_OK;
    }

    // Reservar memoria para los datos
    data->__ptr = (unsigned char *)soap_malloc(soap, length);
    if (!data->__ptr) {
        printf("[wsFilens__readFile] Error al reservar memoria\n");
        close(fd);
        return SOAP_OK;
    }

    // Leer los datos en bloques
    while (totalRead < length) {
        int toRead = (length - totalRead) > MAX_DATA_SIZE ? MAX_DATA_SIZE : (length - totalRead);
        bytesRead = read(fd, buffer, toRead);

        if (bytesRead < 0) {
            printf("[wsFilens__readFile] Error al leer del archivo\n");
            close(fd);
            return SOAP_OK;
        }

        if (bytesRead == 0) break; // EOF alcanzado

        memcpy(data->__ptr + totalRead, buffer, bytesRead);
        totalRead += bytesRead;
    }

    // Actualizar el tamaño real leído
    data->__size = totalRead;

    // Cerrar el archivo
    close(fd);

    return SOAP_OK;
}

int wsFilens__writeFile(struct soap *soap, wsFilens__tFileName fileName, int offset, wsFilens__tData data, int *result) {
    int fd, totalWritten = 0, bytesWritten = 0;

    // Inicializar el resultado
    *result = 0;

    // Abrir el archivo para escritura (crear si no existe)
    fd = open(fileName.__ptr, O_WRONLY | O_CREAT, 0777);
    if (fd < 0) {
        printf("[wsFilens__writeFile] Error al abrir el archivo (%s)\n", fileName.__ptr);
        *result = -1;
        return SOAP_OK;
    }

    // Posicionarse en el offset especificado
    if (lseek(fd, offset, SEEK_SET) < 0) {
        printf("[wsFilens__writeFile] Error al posicionarse en el offset (%d)\n", offset);
        close(fd);
        *result = -1;
        return SOAP_OK;
    }

    // Escribir los datos en bloques
    while (totalWritten < data.__size) {
        int toWrite = (data.__size - totalWritten) > MAX_DATA_SIZE ? MAX_DATA_SIZE : (data.__size - totalWritten);
        bytesWritten = write(fd, data.__ptr + totalWritten, toWrite);

        if (bytesWritten < 0) {
            printf("[wsFilens__writeFile] Error al escribir en el archivo\n");
            close(fd);
            *result = -1;
            return SOAP_OK;
        }

        totalWritten += bytesWritten;
    }

    // Cerrar el archivo
    close(fd);

    return SOAP_OK;
}


