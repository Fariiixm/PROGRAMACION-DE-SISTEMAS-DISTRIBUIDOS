//gsoap gatoRemoto_ns service name: gatoRemoto
//gsoap gatoRemoto_ns service style: rpc
//gsoap gatoRemoto_ns service encoding: encoded
//gsoap gatoRemoto_ns service namespace: urn:remoteCat
/** Length of data buffer */
#define MAX_DATA_SIZE 1024
/** Length for tString */
#define NAME_LENGTH 128
/** Typedef for the file name */
typedef char *xsd__string;
/** Struct that contains a file name */
typedef struct tFileName{
int __size;
xsd__string __ptr;
}gatoRemoto_ns__tFileName;
/** Binary data */
typedef struct tData{
int __size;
unsigned char *__ptr;
}gatoRemoto_ns__tData;
int gatoRemoto_ns__execGatoRemoto (gatoRemoto_ns__tFileName fileName,
gatoRemoto_ns__tData *data);