#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>

#include "servidor.h"

#define MAX_CLASSES_PER_USER 10

void erro(char *msg);
void join_multicast(const char *multicast, int port);
void listen_class(void* arg);
void sigint_handler();

int fd;
int classes=0;
int sockets[MAX_CLASSES_PER_USER];
struct ip_mreq mreqs[MAX_CLASSES_PER_USER];
char groups[MAX_CLASSES_PER_USER][16];


int main(int argc, char *argv[]) {
  char endServer[100];
  struct sockaddr_in addr;
  struct hostent *hostPtr;
  char buffer[1024];
  char mensagem[1024];

  if (argc != 3) {
    printf("cliente <host> <class port> \n");
    exit(-1);
  }

	struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

  strcpy(endServer, argv[1]); 

  if ((hostPtr = gethostbyname(endServer)) == 0)						
    erro("Não consegui obter endereço");

  bzero((void *) &addr, sizeof(addr));									
  addr.sin_family = AF_INET; 											
  addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr; 
  addr.sin_port = htons((short) atoi(argv[2]));							

  if ((fd = socket(AF_INET,SOCK_STREAM,0)) == -1)						
    erro("socket");
  if (connect(fd,(struct sockaddr *)&addr,sizeof (addr)) < 0)			
    erro("Connect");

	int i = 0;
  while (1) { //ciclo infinito para estar sempre a ler e a responder ás mensagens do servidor até ordem contrária
    bzero(buffer, 1024);
    int bytes_read = read(fd, buffer, 1023);// ler a mensagem eviado pelo server, que pode ser a de boas vindas, informação sobre o dominio, ou mensagem de despedida
    if (bytes_read <= 0) break;

	if(i==1){
		char *resultado = strstr(buffer, "Utilizador adicionado à turma com sucesso");
		if (resultado != NULL) {
			char endereco[50];  // Array para armazenar o endereço extraído

		// Usando sscanf para extrair o endereço dentro dos sinais de menor e maior
		if (sscanf(mensagem, "Utilizador adicionado à turma com sucesso.\nEndereço Multicast: <%49[^>]>%*s", endereco) == 1) {
			join_multicast(endereco, atoi(argv[2]));
		}
		} else {

		}
	}

    printf("%s\n", buffer);
	
    fgets(mensagem, 1024, stdin);  //guardar na variavel "mensagem" a mensagem do cliente

    mensagem[strcspn(mensagem, "\n")] = 0; 
    write(fd, mensagem, strlen(mensagem));
	if (strncmp(mensagem, "DISCONNECT\n", 11) == 0) {//verificar se o server enviou a mensagem de despedida, se sim, temos de encerrar o programa
        break;
    }
	char *resultado = strstr(mensagem, "SUBSCRIBE_CLASS");
    if (resultado != NULL) {
		i=1;
    } else {
		i=0;
    }
  }

  close(fd); //fechar o socket
  exit(0);
}

void join_multicast(const char *multicast, int port){
	struct sockaddr_in addr;
    struct ip_mreq mreq;

    //create socket
    if ((sockets[classes] = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    //reuse port
    int optval = 1;
    setsockopt(sockets[classes], SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    //bind to all local interfaces
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(MULTICAST_PORT);

    if (bind(sockets[classes], (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    //join multicast group
    mreq.imr_multiaddr.s_addr = inet_addr(multicast);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(sockets[classes], IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt");
    }

    //add to mreq struct array
    mreqs[classes-0] = mreq;

    //start listening thread
    pthread_t thread;
    pthread_create(&thread, NULL, listen_class, &sockets[classes]);

    //add to groups array
    strncpy(groups[classes], multicast, sizeof(groups[classes]));
    classes++;
}

void listen_class(void* arg){
    int socket = *(int*)arg;

    while (1) {
        char buf[BUF_SIZE];
        struct sockaddr_in addr;
        int addrlen = sizeof(addr);
        int n = recvfrom(socket, buf, sizeof(buf), 0, (struct sockaddr *)&addr, &addrlen);

        if (n < 0) {
            perror("recvfrom");
            exit(1);
        }

        buf[n] = '\0';
        printf("Received message from class: %s\n->", buf);
        fflush(stdout);
    }

    return NULL;
}


void erro(char *msg) {
  printf("Erro: %s\n", msg);
  exit(-1);
}

void sigint_handler(){
    write(fd, "CLOSING", strlen("CLOSING"));
    printf("SIGINT received. Closing multicast sockets...\n");
    close_connections();
    exit(0);
}

void close_connections(){
    for(int i=0; i<classes; i++){
        if (setsockopt(sockets[i], IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreqs[i], sizeof(mreqs[i])) < 0) {
            perror("setsockopt");
            exit(1);
        }
        close(sockets[i]);
    }
}