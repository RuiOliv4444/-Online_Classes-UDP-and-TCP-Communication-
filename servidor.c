#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORTO_TURMAS 12345 // Porta TCP
#define PORTO_CONFIG 12347 // Porta UDP
#define BUF_SIZE 512
#define FILENAME "utilizadores.txt"
#define TAM 30 



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
void login_admin(const char *username, const char *password, int *admin_logado, lista lista_utilizadores);
void udp_server_function();
void tcp_server_function (int tcp_fd);
int add_user(const char *name, const char *pass, const char *ro, lista lista_utilizadores);
void insere_utilizador(lista lista_utilizadores, struct utilizador person);
void remove_utilizador(lista *lista_utilizadores, char username[TAM]);
void ler_ficheiro(lista lista_utilizadores);
void escrever_ficheiro(lista lista_utilizadores);
void listar_users(lista lista_utilizadores,int udp_fd,struct sockaddr_in client_addr, socklen_t addrlen);
int verifica_func(const char aux[TAM]);


int main(int argc, char *argv[]) {
	if (argc != 4) {
    	printf("news_server {PORTO_CLIENT} {PORTO_ADMIN} {ficheiro texto}\n"); //FEJFNEQJFNEQJDNQJQNF
    	exit(-1);
  	}


    int tcp_fd;
    struct sockaddr_in addr;
    socklen_t client_addr_size;

    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_fd < 0)
        perror("Erro ao criar socket TCP");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORTO_TURMAS);

    if (bind(tcp_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        erro("Erro no bind do socket TCP");

    if (listen(tcp_fd, 5) < 0)
        erro("Erro no listen");

    if (fork() == 0) { // Cria um processo filho para o servidor UDP
        udp_server_function();
        exit(0);
    }

	tcp_server_function(tcp_fd);
    return 0;
}


//-------------------------DIVISÓRIA DAS FUNÇÔES PERTENCENTES A TCP COMEÇA AQUI----------------------------------


void tcp_server_function (int tcp_fd){
	int client;
	struct sockaddr_in client_addr;
	socklen_t client_addr_size;
	client_addr_size = sizeof(client_addr);
    while (1) {
        client = accept(tcp_fd, (struct sockaddr *)&client_addr, &client_addr_size);
        if (client > 0) {
            if (fork() == 0) {
                close(tcp_fd);
                //função para lidar com todos os casos tem de ser inserida aqui
                exit(0);
            }
            close(client);
        }
    }
}



//-------------------------DIVISÓRIA DAS FUNÇÔES PERTENCENTES A UDP COMEÇA AQUI----------------------------------




void udp_server_function() {
    int udp_fd;
    struct sockaddr_in udp_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);
    char buf[BUF_SIZE];

    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0)
        erro("Erro ao criar socket UDP");

    memset(&udp_addr, 0, sizeof(udp_addr));

    udp_addr.sin_port = htons(PORTO_CONFIG);

	lista lista_utilizadores = cria();
	ler_ficheiro(lista_utilizadores);

    if (bind(udp_fd, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0)
        erro("Erro no bind do socket UDP");
		int admin_logado = 0;
		memset(buf, 0, BUF_SIZE); 
        while (1) {
			char comando[TAM];
			char arg1[TAM], arg2[TAM], arg3[TAM];

			char mensagem_boa[BUF_SIZE];
			char mensagem_ma[BUF_SIZE];
			if (recvfrom(udp_fd, buf, BUF_SIZE, 0, (struct sockaddr*)&client_addr, &addrlen) < 0){
				erro("Erro ao receber dados UDP");
				continue;											// ter a certeza 
			}
			if (strlen(buf) == 0) {									// Nenhum dado foi recebido, passar à frente, evitamos que envia a mensagem de pedir para iniciar sessão
				continue;
			}
			sscanf(buf, "%s %s %s %s", comando, arg1, arg2, arg3); // analisar cada um dos parametros enviados pelo admin 

			if (strcmp(comando, "LOGIN") == 0) {

				login_admin(arg1,arg2,&admin_logado, lista_utilizadores);

				if(admin_logado == 1){//mensagem de ter conseguido logar
					strcpy(mensagem_boa, "Login efetuado com sucesso!\n");
					if (sendto(udp_fd,mensagem_boa, strlen(mensagem_boa), 0, (struct sockaddr*)&client_addr, addrlen) < 0) erro("Erro ao enviar resposta UDP");
					memset(mensagem_boa, 0, sizeof(mensagem_boa));
				}
				else{//mensagem de não ter conseguido logar
					strcpy(mensagem_ma, "Dados de acesso inválidos!\n");
					if (sendto(udp_fd,mensagem_ma, strlen(mensagem_ma), 0, (struct sockaddr*)&client_addr, addrlen) < 0) erro("Erro ao enviar resposta UDP");
					memset(mensagem_ma, 0, sizeof(mensagem_ma));
				}
			} else if (admin_logado) {								// O admin para ter acesso a esta parte vai ter de se logar primeiro, portanto a primeira mensagem dele terá de ser o login e só depois
																	//realizar uma das operações abaixo
				if (strcmp(comando, "ADD_USER") == 0) {
					if(add_user(arg1, arg2, arg3,lista_utilizadores)){
						strcpy(mensagem_boa, "Utilizador adicionado com sucesso.\n");
						sendto(udp_fd, mensagem_boa, strlen(mensagem_boa), 0, (struct sockaddr*)&client_addr, addrlen);
						memset(mensagem_boa, 0, sizeof(mensagem_boa));
					} else {
						strcpy(mensagem_ma, "Cargo nao existente.\n");
						sendto(udp_fd, mensagem_ma, strlen(mensagem_ma), 0,(struct sockaddr*)&client_addr, addrlen);
						memset(mensagem_ma, 0, sizeof(mensagem_ma));
					}
					// Implemente a lógica para adicionar um usuário.
				} else if (strcmp(comando, "DEL") == 0) {
					// Implemente a lógica para remover um usuário.
					remove_utilizador(&lista_utilizadores, arg1);
				} else if (strcmp(comando, "LIST") == 0) {
					// Implemente a lógica para listar os usuários.
					listar_users(lista_utilizadores, udp_fd, client_addr, addrlen);
				} else if (strcmp(comando, "QUIT_SERVER") == 0) {
					// Encerra o servidor.
					escrever_ficheiro(lista_utilizadores);
					close(udp_fd);
					exit(0);
				}
			} else {
				sendto(udp_fd, "Inicia sessão primeiramente!\n", strlen("Inicia sessão primeiramente!\n"), 0,(struct sockaddr*)&client_addr, addrlen);// Responde que é necessário fazer login primeiro.
			}
			
			memset(buf, 0, BUF_SIZE); // Prepara para a próxima mensagem.
    }
}

void login_admin(const char *username, const char *password, int *admin_logado, lista lista_utilizadores) {//verifica se a pessoa que está a dar login está a por os dados corretos e
																											//e se é mesmo um administrador
    lista atual = lista_utilizadores->next; // Supondo que o primeiro nó seja um cabeçalho/dummy
    while (atual != NULL) {
        if (strcmp(username, atual->user.username) == 0 && strcmp(password, atual->user.password) == 0) {
            if (strcmp(atual->user.role, "administrador") == 0) {
                *admin_logado = 1;
                return; // Encontrou um administrador com as credenciais fornecidas
            }
        }
        atual = atual->next; // Move para o próximo nó
    }

    *admin_logado = 0; // Não encontrou um administrador com as credenciais fornecidas
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
//------------------------- FUNÇÕES RESPONSAVEIS PELO TRATAMENTO DOS DADOS ----------------------------------


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

void insere_utilizador(lista lista_utilizadores, struct utilizador person) {					//inserir um novo utilizador na lista(aluno,prof ou admin)
    lista no = (lista) malloc(sizeof(us_list)); // aloca espaço para um novo nó
    no->user = person; // armazena a struct utilizador no campo 'u' do novo nó
    no->next = lista_utilizadores->next; // define o próximo ponteiro do novo nó para o primeiro nó da lista 
    lista_utilizadores->next = no; // define o próximo ponteiro do nó anterior para o novo nó, adicionando-o na lista
}

void remove_utilizador(lista *lista_utilizadores, char username[TAM]) {							//elimina um utilizador, caso esse realmente exista na lista
    lista aux = *lista_utilizadores;
   
    lista ant = aux;
    aux = aux->next;
    int count = 0;

    while (aux != NULL) {
        if (strcmp(aux->user.username, username) == 0) {
            ant->next = aux->next;
            free(aux);
            count++;
            return;
        }
        ant = aux;
        aux = aux->next;
    }
    if (count == 0){
        printf("Elemento não encontrado\n");  //EFNAKFNEFJNEWFJEWNFJEWF
    }
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





void ler_ficheiro(lista lista_utilizadores) {
    FILE *file = fopen(FILENAME, "r");
    if (file == NULL) {
        erro("Erro ao abrir o ficheiro.");
    }
    struct utilizador person;

    while (fscanf(file, "%[^;];%[^;];%[^;\n]\n", person.username, person.password, person.role) == 3) {
    	printf("%s %s %s\n", person.username, person.password, person.role);
    	insere_utilizador(lista_utilizadores, person);
	}	
    fclose(file);
}


void escrever_ficheiro(lista lista_utilizadores) {
    FILE *file = fopen(FILENAME, "w");
    if (file == NULL) {
        erro("Erro ao abrir o ficheiro para escrita.");
    }

    lista atual = lista_utilizadores->next; // Pula o cabeçalho da lista
    while (atual != NULL) {
        fprintf(file, "%s %s %s\n", atual->user.username, atual->user.password, atual->user.role);
        atual = atual->next;
    }

    fclose(file);
}


void erro(char *msg) {
    perror(msg);
    exit(1);
}
