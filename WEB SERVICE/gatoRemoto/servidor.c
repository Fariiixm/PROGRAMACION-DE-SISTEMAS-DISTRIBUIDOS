


int gatoRemoto_ns__execGatoRemoto (gatoRemoto_ns__tFileName fileName,gatoRemoto_ns__tData *data){

    int fd = open(fileName.__ptr, O_RDONLY);
    if(fd<0){
        perror("Error al abrir eñ archivo");
        data->__size= 0;
        data->__ptr = NULL;
         close(fd);
        return SOAP_FAULT;
    }
    

    int size = calculateFileSize(fileName.__ptr);
    if(size<0){
        perror("Error al abrir eñ archivo");
        data->__size= 0;
        data->__ptr = NULL;
         close(fd);
        return SOAP_FAULT;
    }

    lseek(fd, 0, SEEK_SET);//por si acaso volevmo a apauntar al principio
    data->__ptr = (xsd__string)malloc(size);//independiente mem qe la del cliente
    read(fd, data->__ptr, size);
    data.__size = size;


    free(data->__ptr);
    close(fd);
    return SOAP_OK;

}

int main(int argc, char* argv[]){

    if(argc != 2){
        printf("Usages.....\n");
        exit(EXIT_FAILURE);
    }

    struct soap soap;
    int m1, m2;
    soap_init(&soap);//inicializamos le entorno soap


    if((m1= soap_bind(&soap, NULL, argv[1], 100))<0){ //asiganmos el purto de escucha
        soap_print_fault(&soap, stderr);
        exit(-1);
    }



    while(1){//esto para cada hilo!!!!
    
        if((m2= soap_accept(&soap))<0){
            soap_print_fault(&soap, stderr);
            exit(-1);
        }

        //funcion a realizar
        if(soap_server(&soap)!=SOAP_OK){
            soap_print_fault(&soap, stderr);
            exit(-1);
        }

        soap_end(&soap);
    }

    soap_done(soap);
    return 0;
}


/*
1.Generar los archivos necesarios con gSOAP
    ->Escribir el archivo de descripción del servicio (.h):
 
    ->Generar los archivos fuente y el WSDL:(Usa las herramientas de gSOAP (soapcpp2 y wsdl2h) para generar los archivos necesarios)
        +genera:
            -Archivos fuente (soapC.c, soapClient.c, soapServer.c) para cliente y servidor.
            -Archivos auxiliares (soapH.h, gatoRemoto.nsmap).
            -Un WSDL que describe el servicio
*/