#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#define TCP_PORT 8000

//variables globales para usar en señales
int cliente; 
int estado;  
int pidDestino; 

void cambioVerde(int);
void enviaCambio(int);

int main(int argc, const char * argv[]){
    int pidGet;
    int pidSemaforo=htonl(getpid());
    int estadoAnterior;
 
    struct sockaddr_in direccion;

    ssize_t leidos;
    socklen_t escritos;
    
    sigset_t conjSenial;
    sigemptyset(&conjSenial);
    sigaddset(&conjSenial, SIGUSR1);
    sigaddset(&conjSenial,SIGALRM);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    
    if (argc != 2){
        printf("Use: %s IP_Servidor \n", argv[0]);
        exit(-1);
    }
    
    int intRecibido;
    // Crear el socket
    cliente = socket(PF_INET, SOCK_STREAM, 0);
    // Establecer conexión
    inet_aton(argv[1], &direccion.sin_addr);
    direccion.sin_port = htons(TCP_PORT);
    direccion.sin_family = AF_INET;
    
    escritos = connect(cliente, (struct sockaddr *) &direccion, sizeof(direccion));
    
    if (escritos == 0) {
        printf("Conectado a %s:%d \n",
               inet_ntoa(direccion.sin_addr),
               ntohs(direccion.sin_port));

        // Convertir el PID a un char para mandarlo al siguiente semaforo
        write(cliente, &pidSemaforo, sizeof(pidSemaforo));
        leidos = read(cliente, &pidGet, sizeof(pidGet));
        pidDestino = ntohl(pidGet);

        // Establecer señales a ser recibidas por el semáforo al obtener todos los parametros necesarios
        signal(SIGUSR1, cambioVerde);
        signal(SIGALRM, enviaCambio);

        // Escuchar de la consola remota para recibir interrupciones y manejarlas
                
        //estados verde=0 rojo=1 amarillo=2 todorojo=3 iniciar=4
        while ((leidos = read(cliente, &intRecibido, sizeof(intRecibido)))){
            if (ntohl(intRecibido) == 4) {
                raise(SIGUSR1);
            } 
            else if (ntohl(intRecibido) == 2 && estado != 2) {
                estado = 2;
                sigprocmask(SIG_BLOCK, &conjSenial, NULL);
            } 
            else if (ntohl(intRecibido) == 3 && estado != 3) {
                estado = 3;
                sigprocmask(SIG_BLOCK, &conjSenial, NULL);
            }
            else if (ntohl(intRecibido) == 2 && estado == 2) {
                sigprocmask(SIG_UNBLOCK, &conjSenial, NULL);
            } 
            else if (ntohl(intRecibido) == 3 && estado == 3) {
                sigprocmask(SIG_UNBLOCK, &conjSenial, NULL);
            }
        }
    }
    close(cliente);
    
    return 0;
}

void enviaCambio(int signal) {
    estado = 0;
    kill(pidDestino, SIGUSR1);
}

void cambioVerde(int signal) {
    estado = 1;
    int state=htonl(estado);
    write(cliente, &state, sizeof(state));
    printf("En verde\n");
    alarm(1);
}
