#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define TCP_PORT 8000
#define NOSEMAFOROS 4 

int cliente[4];
int cliente_semaforo;

void enVerde(int i);
void handler(int);

int main(int argc, const char * argv[]){
    sigset_t conjuntoSig;
    sigemptyset(&conjuntoSig);
    sigaddset(&conjuntoSig, SIGINT);
    sigaddset(&conjuntoSig, SIGTSTP);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT, SIG_IGN);

    struct sockaddr_in direccion;
    
    int pidReceived;
    int servidor;
    
    ssize_t leidos;
    socklen_t escritos;
    int continuar = 1;
    pid_t pid;
    
    if (argc != 2) {
        printf("Use: %s IP_Servidor \n", argv[0]);
        exit(-1);
    }
    // Crear el socket
    servidor = socket(PF_INET, SOCK_STREAM, 0);
    // Enlace con el socket
    inet_aton(argv[1], &direccion.sin_addr);
    direccion.sin_port = htons(TCP_PORT);
    direccion.sin_family = AF_INET;
    
    bind(servidor, (struct sockaddr *) &direccion, sizeof(direccion));
    
    // Escuhar
    listen(servidor, NOSEMAFOROS);
    
    escritos = sizeof(direccion);
 
    int * semaforos;
    semaforos=(int*)malloc(NOSEMAFOROS*sizeof(int));
    int * pidsSemaf;
    pidsSemaf=(int*)malloc(NOSEMAFOROS*sizeof(int));
    int * s=semaforos;
    int * i;
    int cont=0;

    for (i = s; i<s+4; i++, cont++) {
        cliente[cont] = accept(servidor, (struct sockaddr *) &direccion, &escritos);
        
        printf("Aceptando conexiones en %s:%d \n",
               inet_ntoa(direccion.sin_addr),
               ntohs(direccion.sin_port));
        
        pid = fork();
        
        if (pid == 0) {
            cliente_semaforo=cliente[cont];
            // asigna a los hijos el handler de señales
            if (signal(SIGTSTP, handler) == SIG_ERR){
            printf("ERROR: No se pudo llamar al manejador\n");
            }
            else if (signal(SIGINT, handler) == SIG_ERR){
                printf("ERROR: No se pudo llamar al manejador\n");
            }
            close(servidor);

            if (cliente[cont] >= 0) {
                // Recibe el semaforo y lo marca como verde
                int formatoRecib;
                while(leidos = read(cliente_semaforo, &formatoRecib, sizeof(formatoRecib))) {
                    enVerde(cont);
                }
            }
            close(cliente_semaforo);
        }
        else {
            //el proceso padre almacena los pids de los semaforos
            pidsSemaf[cont] = read(cliente[cont], i, sizeof(i));
        }
    }

    if (pid > 0) {
        //se le manda a cada semaforo el destino de sus señales
        for (int j = 0; j < NOSEMAFOROS; j++) {
            int nextSemaforo = (j + 1) % NOSEMAFOROS;
            write(cliente[j], &semaforos[nextSemaforo], sizeof(pidsSemaf[nextSemaforo]));
        }
        //Se manda la a los semaforos numero de inicio de ciclo
        int iniciar = 4; 
        int formatoInicia = htonl(iniciar);
        write(cliente[0], &formatoInicia, sizeof(formatoInicia));

        while (wait(NULL) != -1);
        close(servidor);
        
    }
    free(pidsSemaf);
    free(semaforos);
    return 0;
}
//identifica qual es el semaforo que esta prendido
void enVerde(int semaf) {
    printf("Semaforos\t Estado\n");
    for (int i=0; i < NOSEMAFOROS; i++) {
        if (i == semaf){
            printf("%d\t Verde\n", i);
        }
        else{
            printf("%d\t Rojo\n", i);
        }
    }
    printf("\n");
}

void handler(int s){
	if (s == SIGTSTP){
        int estado = htonl(3);
        printf("Ctrl + z: semaforos en rojo\n");
        printf("Semaforo en rojo forzado\n");
        write(cliente_semaforo, &estado, sizeof(estado));
	}
	else if(s == SIGINT){
        int estado = htonl(2);
		printf("Ctrl + c: semaforos en intermitente\n"); 
        printf("Semaforo en intermitente\n");
        write(cliente_semaforo, &estado, sizeof(estado));
        
	}
}
