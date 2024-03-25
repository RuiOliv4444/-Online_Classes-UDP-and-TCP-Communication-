#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

#define SERVER_PORT     9000
#define BUF_SIZE	1024

int client_count = 0;

void process_client(int fd, struct sockaddr_in client_addr);
void erro(char *msg);

int main() {
  int fd, client;
  struct sockaddr_in addr, client_addr;  					// client_addr foi inicializado aqui pois quando houver uma conexão, a informaçao da conexão vai
															//ser passado para esta variavel por referencia, por isso que na linha 39 estamos a passar o endereço da variavel 
  socklen_t client_addr_size;

  bzero((void *) &addr, sizeof(addr));
  addr.sin_family = AF_INET;								// AF_INET porque estamos a usar endereço IPV4, se fosse IPV6 usariamos AF_INET6
  addr.sin_addr.s_addr = htonl(INADDR_ANY); 				// escutar em todas as interfaces de rede disponíveis.
  addr.sin_port = htons(SERVER_PORT);						// escutar conexões apenas na porta configurada, no caso "9000"

  if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)			// criação de um socket TCP, o "welcoming socket"
    erro("na funcao socket");
  if ( bind(fd,(struct sockaddr*)&addr,sizeof(addr)) < 0)   // associa o socket ao endereço configurado
    erro("na funcao bind");
  if( listen(fd, 5) < 0)									// começa á escuta de conexões, neste caso permite ter até 5 conexões pendentes
    erro("na funcao listen");
  
  client_addr_size = sizeof(client_addr);
  while (1) {												// entra num loop infinito para estar sempre à escuta de novas ligações
    client = accept(fd,(struct sockaddr *)&client_addr,&client_addr_size);  // esta função dá return a um file descriptor(fd) para o socket, representando a conecção com o cliente,
																			//podendo ler, enviar mensagens(...), através deste fd (unico para cada conexão)
    if (client > 0) {
      client_count++;//incrementar o numero de clientes para fazer o que o exercício pede
      if (fork() == 0) {									// cria um processo filho para lidar com a conexão,desta maneira o servidor continua a conseguir 
															//aceitar mais conexões
        close(fd);											// fecha o socket de escuta pois o processo filho agora está ocupado a processar a conexão atual
															//no entanto o socket de escuta continua funcional no processo pai!
        process_client(client, client_addr);
        exit(0);//encerrar o processo filho
      }
      close(client);										// processo pai fecha o socket conectado ao cliente pois o processo filho já está a tratar dele
    }														//e desta maneira ele já está apto para receber mais conexões
  }
  return 0;
}

void process_client(int client_fd, struct sockaddr_in client_addr) {
  int nread = 0;											// numero de bytes lidos através da conexão
  char buffer[BUF_SIZE];									// armazenar os dados recebidos pelo cliente
  char client_ip[INET_ADDRSTRLEN];
  //### INICIO FOLHA 5###
  inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN); //converte o IP do cliente numa string e armazena em client_ip 
  int client_port = ntohs(client_addr.sin_port);

  printf("** New message received **\n");
  printf("Client %d connecting from (IP:port) %s:%d\n", client_count, client_ip, client_port);

  nread = read(client_fd, buffer, BUF_SIZE-1);
  buffer[nread] = '\0';
  printf("says \"%s\"\n", buffer);
  
  char msg_to_client[BUF_SIZE];
  sprintf(msg_to_client, "Server received connection from (IP:port) %s:%d; already received %d connections!", client_ip, client_port, client_count);
  write(client_fd, msg_to_client, strlen(msg_to_client) + 1);

  close(client_fd);											//agora é o processo filho que fecha o socket conectado ao cliente pois já fez as operações pretendidas
}

void erro(char *msg){
  printf("Erro: %s\n", msg);
  exit(-1);
}
