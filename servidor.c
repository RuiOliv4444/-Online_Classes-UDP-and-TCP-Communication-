#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORTO_TURMAS 12345 // Porta TCP
#define PORTO_CONFIG 12346 // Porta UDP
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

int main() {
    int tcp_fd;
    struct sockaddr_in addr;
    socklen_t client_addr_size;

    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_fd < 0)
        erro("Erro ao criar socket TCP");

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
                process_client(client, client_addr);
                exit(0);
            }
            close(client);
        }
    }
}

void process_client(int client_fd, struct sockaddr_in client_addr) {
    // Função para processar cliente TCP
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

    if (bind(udp_fd, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0)
        erro("Erro no bind do socket UDP");

        while (1) {
			int *admin_logado = 0;
			char comando[TAM];
			char arg1[TAM], arg2[TAM], arg3[TAM];

			if (recvfrom(udp_fd, buf, BUF_SIZE, 0, (struct sockaddr*)&client_addr, &addrlen) < 0) erro("Erro ao receber dados UDP");

			sscanf(buf, "%s %s %s %s", comando, arg1, arg2, arg3); // analisar cada um dos parametros enviados pelo admin 

			if (strcmp(comando, "LOGIN") == 0) {
				login_admin(arg1,arg2,&admin_logado);
				char resposta0 = "Dados de acesso inválidos!\n";
				char resposta1 = "Login efetuado com sucesso!\n";
				if(*admin_logado == 1){
					if (sendto(udp_fd,resposta0, strlen(resposta0), 0, (struct sockaddr*)&client_addr, addrlen) < 0) erro("Erro ao enviar resposta UDP");
				}else{
					if (sendto(udp_fd,resposta1, strlen(resposta1), 0, (struct sockaddr*)&client_addr, addrlen) < 0) erro("Erro ao enviar resposta UDP");
				}

			} else if (admin_logado) {								// O admin para ter acesso a esta parte vai ter de se logar primeiro, portanto a primeira mensagem dele terá de ser o login e só depois
																	//realizar uma das operações abaixo
				if (strcmp(comando, "ADD_USER") == 0) {
					// Implemente a lógica para adicionar um usuário.
				} else if (strcmp(comando, "DEL") == 0) {
					// Implemente a lógica para remover um usuário.
				} else if (strcmp(comando, "LIST") == 0) {
					// Implemente a lógica para listar os usuários.
				} else if (strcmp(comando, "QUIT_SERVER") == 0) {
					// Encerra o servidor.
					break;
				}
			} else {
				// Responde que é necessário fazer login primeiro.
			}
			
			memset(buf, 0, BUF_SIZE); // Prepara para a próxima mensagem.
    }
}

void login_admin(const char *username, const char *password, int *admin_logado) {
    FILE *file = fopen(FILENAME, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo de utilizadores");
        return; // Retorna 0 se não conseguir abrir o arquivo
    }

    char file_username[TAM], file_password[TAM], file_role[TAM];
    while (fscanf(file, "%[^;];%[^;];%s\n", file_username, file_password, file_role) == 3) {
        // Verifica se o username e a password correspondem e se o role é de administrador
        if (strcmp(username, file_username) == 0 && strcmp(password, file_password) == 0 && strcmp(file_role, "administrador") == 0) {
            fclose(file);
            *admin_logado = 1;
        }
	}
    fclose(file);
}

//------------------------- FUNÇÕES RESPONSAVEIS PELO TRATAMENTO DOS DADOS ----------------------------------

void ler_ficheiro(lista lista_utilizadores) {
    FILE *file = fopen("FILENAME", "r");
    if (file == NULL) {
        erro("Erro ao abrir o ficheiro.");
    }

    struct utilizador person;

    while (fscanf(file, "%s %s %s\n", person.username, person.password, person.role) == 3) {
        insere_utilizador(lista_utilizadores, person);
    }

    fclose(file);
}


void escrever_ficheiro(lista lista_utilizadores) {
    FILE *file = fopen("FILENAME", "w");
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
