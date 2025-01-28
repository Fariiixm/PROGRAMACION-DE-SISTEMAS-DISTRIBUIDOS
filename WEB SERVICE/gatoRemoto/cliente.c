
#include "soapH.h"
#include "gatoRemoto.nsmap"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//./client http://server:port sourceFile
int main(int argc, char* argv[]){

    struct soap soap;
    

    if(argc != 4){
        printf("Usages....\n");
        exit(EXIT_FAILURE);
    }
    soap_init(&soap);

    int serverPort = atoi(argv[2]);
    char* serverURL = argv[1];
    
    gatoRemoto_ns__tFileName fileName;

    //fileName.__ptr =(xsd__string) malloc(NAME_LENGTH);

    fileName.__ptr = argv[3];
    fileName.__size = strlen(fileName.__ptr);


    gatoRemoto_ns__tData dato;
    dato.__ptr = (xsd__string)malloc(MAX_DATA_SIZE);
    dato.__size = 0;

    if(soap_call_gatoRemoto_ns__execGatoRemoto(&soap, serverURL,fileName, data)==SOAP_OK){
        printf("Contenido del archivo %s: ", fileName.__ptr);

        fwrite(data.__ptr, sizeof(char), data.__size, stdout);
        printf("\n");

        
        
    }else{
        soap_print_fault(&soap, stderr);
    }
   
    free(data.__ptr);
    soap_end(&soap);
    return 0;
}