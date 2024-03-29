#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 512
#define TAM 30 

FILE *file;
char *filename;

struct utilizador {
    char username[TAM];
    char password[TAM];
    char role[TAM];
};

typedef struct us_list { //Struct da lista ligada
    struct utilizador user;
    struct us_list * next;
} us_list;

typedef us_list * lista;

lista cria(void);
void erro(char *msg);
void process_client(int client_fd, struct sockaddr_in client_addr, lista lista_utilizadores);
void login_user(const char *username, const char *password, int *login, lista lista_utilizadores);
void udp_server_function(unsigned short udp_port,lista lista_utilizadores);
void tcp_server_function (int tcp_fd, lista lista_utilizadores);
void list_classes(int client_fd);
void list_subscribed(int client_fd);
void subscribe_class(int client_fd);
void create_class(int client_fd);
void send_cont(int client_fd);
int add_user(const char *name, const char *pass, const char *ro, lista lista_utilizadores);
void insere_utilizador(lista lista_utilizadores, struct utilizador person);
int remove_utilizador(lista *lista_utilizadores, char username[TAM],struct sockaddr_in client_addr, socklen_t addrlen);
void ler_ficheiro(lista lista_utilizadores);
void escrever_ficheiro(lista lista_utilizadores);
void listar_users(lista lista_utilizadores,int udp_fd,struct sockaddr_in client_addr, socklen_t addrlen);
int verifica_func(const char aux[TAM]);


int main(int argc, char *argv[]) {
	if (argc != 4) {
    	printf("news_server {PORTO_CLIENT} {PORTO_ADMIN} {ficheiro texto}\n"); 
    	exit(-1);
  	}

	unsigned short tcp_port = htons(atoi(argv[1]));
    unsigned short udp_port = htons(atoi(argv[2]));
	filename = malloc(strlen(argv[3]) + 1);
    strcpy(filename, argv[3]);

    int tcp_fd;
    struct sockaddr_in addr;
    socklen_t client_addr_size;

    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_fd < 0)
        perror("Erro ao criar socket TCP");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = tcp_port;

    if (bind(tcp_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        erro("Erro no bind do socket TCP");

    if (listen(tcp_fd, 5) < 0)
        erro("Erro no listen");

	lista lista_utilizadores = cria(); 
	ler_ficheiro(lista_utilizadores);

    if (fork() == 0) { // Cria um processo filho para o servidor UDP
		udp_server_function(udp_port,lista_utilizadores); 
		exit(0);
	}
	else {
        waitpid(-1, NULL, 0); 
    }

	tcp_server_function(tcp_fd,lista_utilizadores);
    return 0;
}


//-------------------------DIVISÓRIA DAS FUNÇÔES PERTENCENTES A TCP COMEÇA AQUI----------------------------------


void tcp_server_function (int tcp_fd, lista lista_utilizadores){
	int client;
	struct sockaddr_in client_addr;
	socklen_t client_addr_size;
	client_addr_size = sizeof(client_addr);
    while (1) {
        client = accept(tcp_fd, (struct sockaddr *)&client_addr, &client_addr_size);
        if (client > 0) {
            if (fork() == 0) {
                close(tcp_fd);
                process_client(client, client_addr,lista_utilizadores);
                exit(0);
            }
            close(client);
        }
    }
}

void process_client(int client_fd, struct sockaddr_in client_addr, lista lista_utilizadores) {
    char buffer[BUF_SIZE];

    char request[] = "Para aceder à sua conta, faça o login\n"; //mensagem de aviso para fazer o login
    write(client_fd, request, strlen(request));
	int client_logado = 0;
    while (1) { //ficamos sempre a ler as mensagens do utilizador e a lidar com elas, até a mensagem recebida ser "SAIR"
        bzero(buffer, BUF_SIZE);
		char comando[TAM];
		char arg1[TAM], arg2[TAM];
		
        if (read(client_fd, buffer, BUF_SIZE-1) <= 0){
			erro("Erro ao receber dados TCP");
			continue;	
		}
        buffer[strcspn(buffer, "\n")] = 0; // Remove a mudança de linha "\n"

		sscanf(buffer, "%s %s %s", comando, arg1, arg2); // analisar cada um dos parametros enviados pelo admin 

		if (strcmp(comando, "LOGIN") == 0) {
			login_user(arg1,arg2,&client_logado, lista_utilizadores);

			if(client_logado > 1){//mensagem de ter conseguido logar
				write(client_fd, "OK!\n", strlen("OK!\n"));
			}
			else{//mensagem de não ter conseguido logar
				write(client_fd, "REJECTED!\n", strlen("REJECTED!\n"));
			}
		} else if (client_logado > 1) {								// O admin para ter acesso a esta parte vai ter de se logar primeiro, portanto a primeira mensagem dele terá de ser o login e só depois
																	//realizar uma das operações abaixo
			if (strcmp(comando, "LIST_CLASSES") == 0) {
				list_classes(client_fd);

			} else if (strcmp(comando, "LIST_SUBSCRIBED") == 0) {
				list_subscribed(client_fd);
				
			} else if (strcmp(comando, "SUSBCRIBE_CLASS") == 0) {
				subscribe_class(client_fd);

			} else if (strcmp(comando, "DISCONNECT") == 0) {
				escrever_ficheiro(lista_utilizadores);
				close(client_fd);
				exit(0);
			}
			else{
				write(client_fd, "Comando desconhecido!\n", strlen("Comando desconhecido!\n"));
			}
		}else if(client_logado == 3){
			if (strcmp(comando, "CREATE_CLASS") == 0) {
				create_class(client_fd);

			} else if (strcmp(comando, "SEND") == 0) {
				send_cont(client_fd);
			}		
		}else {
			if (write(client_fd, "É necessário efetuar login primeiro!\n", strlen("É necessário efetuar login primeiro!\n")) < 0) erro("Erro ao enviar resposta TCP");// Responde que é necessário fazer login primeiro.
		}
		memset(buffer, 0, BUF_SIZE); // Prepara para a próxima mensagem.
	}
    close(client_fd);// no fim fechamos sempre o socket e o ficheiro
}

void list_classes(int client_fd) {

    char message[] = "Esta é uma função protótipo para listar as classes.\n";
    write(client_fd, message, strlen(message));
}

void list_subscribed(int client_fd) {

    char message[] = "Esta é uma função protótipo para listar as classes em que participas.\n";
    write(client_fd, message, strlen(message));
}

void subscribe_class(int client_fd){

	char message[] = "Esta é uma função protótipo para subscrever uma classe.\n";
    write(client_fd, message, strlen(message));
}

void create_class(int client_fd) {

    char message[] = "Esta é uma função protótipo para criar uma aula.\n";
    write(client_fd, message, strlen(message));
}

void send_cont(int client_fd) {
    char message[] = "Esta é uma função protótipo para enviar conteúdo para uma aula.\n";
    write(client_fd, message, strlen(message));
}


//-------------------------DIVISÓRIA DAS FUNÇÔES PERTENCENTES A UDP COMEÇA AQUI----------------------------------
void udp_server_function(unsigned short udp_port,lista lista_utilizadores) {
    int udp_fd;
    struct sockaddr_in udp_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);
    int admin_logado = 0;
	char buf[BUF_SIZE], comando[TAM],arg1[TAM], arg2[TAM], arg3[TAM];

    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0)
        erro("Erro ao criar socket UDP");

    memset(&udp_addr, 0, sizeof(udp_addr));

    udp_addr.sin_port = udp_port;

    if (bind(udp_fd, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0)
        erro("Erro no bind do socket UDP");

    while (1) {
		memset(buf, '\0', sizeof(buf));
		memset(comando, '\0', sizeof(comando));
		memset(arg1, '\0', sizeof(arg1));
		memset(arg2, '\0', sizeof(arg2));
		memset(arg3, '\0', sizeof(arg3));

		int bytes_received = recvfrom(udp_fd, buf, BUF_SIZE, 0, (struct sockaddr*)&client_addr, &addrlen);

		if (bytes_received < 0) {
	    	erro("Erro ao receber dados UDP");
	    	continue;
		}

		sscanf(buf, "%s %s %s %s", comando, arg1, arg2, arg3);
		if (strcmp(buf, "X") != 0 && strlen(buf) != 1) {
			if (strcmp(comando, "LOGIN") == 0) {

				login_user(arg1,arg2,&admin_logado, lista_utilizadores);

				if(admin_logado == 1){//mensagem de ter conseguido logar
					sendto(udp_fd, "Login efetuado com sucesso!\n", strlen("Login efetuado com sucesso!\n"), 0, (struct sockaddr*) &client_addr, addrlen);
				}
				else{//mensagem de não ter conseguido logar
					sendto(udp_fd, "Dados de acesso inválidos!\n", strlen("Dados de acesso inválidos!\n"), 0, (struct sockaddr*) &client_addr, addrlen);
				}
			} else if (admin_logado == 1) {								// O admin para ter acesso a esta parte vai ter de se logar primeiro, portanto a primeira mensagem dele terá de ser o login e só depois
																	//realizar uma das operações abaixo
				if (strcmp(comando, "ADD_USER") == 0) {
					if(add_user(arg1, arg2, arg3,lista_utilizadores)){
						sendto(udp_fd, "Utilizador adicionado com sucesso.\n", strlen("Utilizador adicionado com sucesso.\n"), 0, (struct sockaddr*)&client_addr, addrlen);
					} else {
						sendto(udp_fd, "Cargo nao existente.\n", strlen("Cargo nao existente.\n"), 0,(struct sockaddr*)&client_addr, addrlen);
					}
					
				} else if (strcmp(comando, "DEL") == 0) {
					if(remove_utilizador(&lista_utilizadores, arg1,client_addr, addrlen)){
						sendto(udp_fd, "Utilizador eliminado com sucesso.\n", strlen("Utilizador adicionado com sucesso.\n"), 0, (struct sockaddr*)&client_addr, addrlen);
					} else {
						sendto(udp_fd, "Utilizador não encontrado.\n", strlen("Utilizador não encontrado.\n"), 0,(struct sockaddr*)&client_addr, addrlen);
					}
				} else if (strcmp(comando, "LIST") == 0) {
					listar_users(lista_utilizadores, udp_fd, client_addr, addrlen);

				} else if (strcmp(comando, "QUIT_SERVER") == 0) {
					escrever_ficheiro(lista_utilizadores);
					close(udp_fd);
					printf("Servidor UDP encerrando...\n");
					free(filename);
					fflush(stdout);
					break;
				}
				else{
					sendto(udp_fd, "Comando desconhecido.\n", strlen("Comando desconhecido.\n"), 0, (struct sockaddr*)&client_addr, addrlen);
				}
			} else {
					sendto(udp_fd, "Inicia sessão primeiramente!\n", strlen("Inicia sessão primeiramente!\n"), 0,(struct sockaddr*)&client_addr, addrlen);// Responde que é necessário fazer login primeiro.
			}
		}else if(strcmp(buf, "X") != 0 ){
			sendto(udp_fd, "Nenhum comando recebido!\n", strlen("Nenhum comando recebido!\n"), 0, (struct sockaddr*) &client_addr, addrlen);
    		continue;
		}else
			continue;
    }
	exit(0);
}

void login_user(const char *username, const char *password, int *login, lista lista_utilizadores) {//verifica se a pessoa que está a dar login está a por os dados corretos e
    *login = 0;
	lista atual = lista_utilizadores->next;
    while (atual != NULL) {
        if (strcmp(username, atual->user.username) == 0 && strcmp(password, atual->user.password) == 0) {
            if (strcmp(atual->user.role, "administrador") == 0) {
                *login = 1;
				return;
            }else if(strcmp(atual->user.role, "aluno") == 0){
				*login = 2; 
				return;
			}else if(strcmp(atual->user.role, "professor") == 0)
				*login = 3; 
				return;
        }
        atual = atual->next; // Move para o próximo nó
    }
}
int add_user(const char *name, const char *pass, const char *ro, lista lista_utilizadores) {  			// admin adiciona um usuário
	int result = 1;
    struct utilizador person;
    char aux[TAM];
    strcpy(aux, ro);

    if (verifica_func(ro)) { //Verifica se o terceiro parametro é leitor, admin ou jornalista
        strcpy(person.username, name); //Guarda as 3 strings nos seus respetivos lugares na struct
        strcpy(person.password, pass);
        strcpy(person.role, ro);
        insere_utilizador(lista_utilizadores, person); //Adiciona-se essa struct à lista
    } else {
		result=0;
    }
	return result;
}

int verifica_func(const char aux[TAM]){																// verifica se estamos a atribuir uma função existente
    int count = 0;

    if (strcmp(aux, "aluno") == 0 || strcmp(aux, "professor") == 0 || strcmp(aux, "administrador") == 0){
        count = 1;
    }

    return count;
}
//------------------------- FUNÇÕES RESPONSAVEIS PELO TRATAMENTO DA LISTA DE UTILIZADORES ----------------------------------

lista cria (){  //Funcao que cria uma lista com 1 elemento e retorna essa mesma lista
    lista aux;
    struct utilizador p = {"", "", ""};
    aux = (lista)malloc(sizeof (us_list));
    if (aux != NULL) {
        aux->user = p;
        aux->next = NULL;
    }
    return aux;
}

void insere_utilizador(lista lista_utilizadores, struct utilizador person) {//inserir um novo utilizador na lista(aluno,prof ou admin)
    lista no = (lista) malloc(sizeof(us_list)); // aloca espaço para um novo nó
    no->user = person; // armazena a struct utilizador no campo 'u' do novo nó
    no->next = lista_utilizadores->next; // define o próximo ponteiro do novo nó para o primeiro nó da lista 
    lista_utilizadores->next = no; // define o próximo ponteiro do nó anterior para o novo nó, adicionando-o na lista
}

int remove_utilizador(lista *lista_utilizadores, char username[TAM],struct sockaddr_in client_addr, socklen_t addrlen) {							//elimina um utilizador, caso esse realmente exista na lista
		lista aux = *lista_utilizadores;
   
    lista ant = aux;
    aux = aux->next;

    while (aux != NULL) {
        if (strcmp(aux->user.username, username) == 0) {
            ant->next = aux->next;
            free(aux);
            return 1;
        }
        ant = aux;
        aux = aux->next;
    }
    return 0;
}

void listar_users(lista lista_utilizadores,int udp_fd,struct sockaddr_in client_addr, socklen_t addrlen){ //mostrar os utilizadores todos
    lista aux = lista_utilizadores -> next;
	int cont=0;
    while(aux){
		cont=1;
        char buf[BUF_SIZE];
        strcpy(buf, aux->user.username); 
		strcat(buf, " -> ");             
		strcat(buf, aux->user.role);	
        strcat(buf, "\n");
        sendto(udp_fd, buf, strlen(buf), 0, (struct sockaddr*)&client_addr, addrlen);
        aux = aux -> next;
    }
}


//------------------------- FUNÇÕES RESPONSAVEIS PELO TRATAMENTO DO FICHEIRO DE TEXTO ----------------------------------

void ler_ficheiro(lista lista_utilizadores) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        erro("Erro ao abrir o ficheiro.");
    }
    struct utilizador person;
    while (fscanf(file, "%[^;];%[^;];%[^;\n]\n", person.username, person.password, person.role) == 3) {
    	insere_utilizador(lista_utilizadores, person);
	}	
    fclose(file);
}


void escrever_ficheiro(lista lista_utilizadores) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        erro("Erro ao abrir o ficheiro para escrita.");
    }

    lista atual = lista_utilizadores->next; // Pula o cabeçalho da lista
    while (atual != NULL) {
        fprintf(file, "%s;%s;%s\n", atual->user.username, atual->user.password, atual->user.role);
        atual = atual->next;
    }

    fclose(file);
}


void erro(char *msg) {
    perror(msg);
    exit(1);
}
