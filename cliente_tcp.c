#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

void erro(char *msg);

int main(int argc, char *argv[]) {
  char endServer[100];
  int fd;
  struct sockaddr_in addr;
  struct hostent *hostPtr;
  char buffer[1024];
  char mensagem[1024];

  if (argc != 3) {
    printf("cliente <host> <class port> \n");
    exit(-1);
  }

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


  while (1) { //ciclo infinito para estar sempre a ler e a responder ás mensagens do servidor até ordem contrária
    bzero(buffer, 1024);
    int bytes_read = read(fd, buffer, 1023);// ler a mensagem eviado pelo server, que pode ser a de boas vindas, informação sobre o dominio, ou mensagem de despedida
    if (bytes_read <= 0) break;
    printf("%s\n", buffer);
	
    fgets(mensagem, 1024, stdin);  //guardar na variavel "mensagem" a mensagem do cliente

    mensagem[strcspn(mensagem, "\n")] = 0; 
    write(fd, mensagem, strlen(mensagem));
	if (strncmp(mensagem, "DISCONNECT\n", 11) == 0) {//verificar se o server enviou a mensagem de despedida, se sim, temos de encerrar o programa
        break;
    }
  }

  close(fd); //fechar o socket
  exit(0);
}

void erro(char *msg) {
  printf("Erro: %s\n", msg);
  exit(-1);
}