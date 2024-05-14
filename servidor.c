#include "servidor.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {

	signal(SIGINT, treat_signal);

	unsigned short tcp_port = htons(atoi(argv[1]));
    unsigned short udp_port = htons(atoi(argv[2]));
	filename = malloc(strlen(argv[3]) + 1);
    strcpy(filename, argv[3]);
	//
	create_shared();
	//
	init_shared_struct(share);

	ler_ficheiro();

	sem_unlink("utilizadores");
	sem_unlink("alunos");
	sem_utilizadores = sem_open("utilizadores", O_CREAT|O_EXCL, 0777,1);
	sem_alunos = sem_open("alunos", O_CREAT|O_EXCL, 0777,1);

	p_tcp = fork();
    if (p_tcp == 0) { // Cria um processo filho para o servidor TCP
		struct sockaddr_in tcp_addr;
    	socklen_t client_addr_size;

		if (tcp_fd < 0)
        	perror("Erro ao criar socket TCP");

		memset(&tcp_addr, 0, sizeof(tcp_addr));
		tcp_addr.sin_family = AF_INET;
		tcp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		tcp_addr.sin_port = tcp_port;
	    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
		

		if (bind(tcp_fd, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr)) < 0)
			erro("Erro no bind do socket TCP");

		if (listen(tcp_fd, 5) < 0)
			erro("Erro no listen");

		int client;
		client_addr_size = sizeof(tcp_addr);



		while (1) {
			client = accept(tcp_fd, (struct sockaddr *)&tcp_addr, &client_addr_size);
			if (client > 0) {
				if (fork() == 0) {
					close(tcp_fd);
					process_client(client, tcp_addr);
					close(client);
					exit(0);
				}
			}
    	}
		close(tcp_fd);
	}
	udp_server_function(udp_port);

    return 0;
}


//-------------------------DIVISÓRIA DAS FUNÇÔES PERTENCENTES A TCP COMEÇA AQUI----------------------------------


void process_client(int client_fd, struct sockaddr_in client_addr) {

	int multicast_socket; //multicast socket
	struct sockaddr_in multicast_addr;
	memset(&multicast_addr, 0, sizeof(multicast_addr));
	multicast_addr.sin_family = AF_INET;
	multicast_addr.sin_port = htons(MULTICAST_PORT);

	if((multicast_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		perror("Error creating socket for multicast group");

	int ttl_value = 2; //increase packet life time
	if(setsockopt(multicast_socket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl_value, sizeof(ttl_value)) < 0)
		perror("Error enabling multicast on socket");



    char buffer[BUF_SIZE];
	char nome[BUF_SIZE];
    char request[] = "LOGIN {username} {password}\n"; //mensagem de aviso para fazer o login
    write(client_fd, request, strlen(request));
    int client_logado = 0;
    while (1) { //ficamos sempre a ler as mensagens do utilizador e a lidar com elas, até a mensagem recebida ser "SAIR"
        bzero(buffer, BUF_SIZE);
        char comando[TAM];
        char arg1[TAM], arg2[TAM];
        if (read(client_fd, buffer, BUF_SIZE-1) <= 0) {
            break;
        }
        buffer[strcspn(buffer, "\n")] = 0; // Remove a mudança de linha "\n"

        sscanf(buffer, "%s %s %s", comando, arg1, arg2); // analisar cada um dos parametros enviados pelo admin 

        if (strcmp(comando, "LOGIN") == 0) {
            login_user(arg1, arg2, &client_logado);

            if(client_logado > 1) { //mensagem de ter conseguido logar
				strcpy(nome,arg1);
                if (write(client_fd, "OK!\n", strlen("OK!\n")) < 0) perror("Erro ao enviar resposta TCP");
            }
            else { //mensagem de não ter conseguido logar
                if (write(client_fd, "REJECTED!\n", strlen("REJECTED!\n")) < 0) perror("Erro ao enviar resposta TCP");
            }
        } 
        else if (client_logado > 1) { // O admin para ter acesso a esta parte vai ter de se logar primeiro
            if (strcmp(comando, "LIST_CLASSES") == 0) { 
                list_classes(client_fd);
            } 
            else if (strcmp(comando, "LIST_SUBSCRIBED") == 0) {
                list_subscribed(client_fd,nome);
            } 
            else if (strcmp(comando, "SUBSCRIBE_CLASS") == 0) {
				//SUBSCRIBE_CLASS <name> verificar argumentos
                subscribe_class(client_fd,nome,arg1); 
            } 
            else if (strcmp(comando, "DISCONNECT") == 0) { //não funciona
                escrever_ficheiro();
                close(client_fd);
                break;
            }
            else if (strcmp(comando, "CREATE_CLASS") == 0 && client_logado == 3) {
                create_class(client_fd, arg1, arg2, nome);
            }
            else if (strcmp(comando, "SEND") == 0 && client_logado == 3) { //não funciona
				//SEND name text
                send_cont(client_fd, arg1, arg2, multicast_socket, multicast_addr, nome);
            }
            else {
                char message[] = "Comando desconhecido.\n";
                write(client_fd, message, strlen(message));
            }
        }
        else {
            if (write(client_fd, "É necessário efetuar login primeiro!\n", strlen("É necessário efetuar login primeiro!\n")) < 0) perror("Erro ao enviar resposta TCP");
        }
    }
    exit(0);
}


void list_classes(int client_fd) {
    char buffer[BUF_SIZE];
	int existe = 0;
	sem_wait(sem_alunos);
    for (int i = 0; i < MAX_CLASSES; i++) {
        if ((share->aulas[i].name[0] != '\0') ){ // Apenas listamos classes que tenham um nome definido (não vazias)
            int num_students = 0;
            for (int j = 0; j < MAX_USERS_CLASS; j++) {
                if (share->aulas[i].alunos_turma[j].username[0] != '\0') {
                    num_students++;
                }
            }
			if(num_students==MAX_USERS_CLASS)	continue;

            snprintf(buffer, BUF_SIZE, "Classe: %s, Multicast: %s, Alunos: %d/%d\n",
                     share->aulas[i].name, share->aulas[i].multicast, num_students, share->aulas[i].max_alunos);
            send(client_fd, buffer, strlen(buffer), 0); // Envia a informação da classe para o cliente
			existe = 1;
        }
		
	}

	if (!existe){
		send(client_fd, "Não existem classes", strlen("Não existem classes"), 0);
	}
	sem_post(sem_alunos);
	
}

void list_subscribed(int client_fd, const char *nome) {
	sem_wait(sem_alunos);
	int cont=0;
	for (int i = 0; i < MAX_CLASSES; i++) {
		char buffer[BUF_SIZE];
		if(is_user_in_class(nome, share->aulas[i].name) == 1){
			cont++;
			snprintf(buffer, BUF_SIZE, "Classe: %s <Multicast: %s>\n",
                     share->aulas[i].name, share->aulas[i].multicast);
            send(client_fd, buffer, strlen(buffer), 0); // Envia a informação da classe para o cliente
		}
	}
	sem_post(sem_alunos);
	if(cont==0)	send(client_fd, "O utilizador não está inscrito em nenhuma aula.\n", strlen( "O utilizador não está inscrito em nennhuma aula.\n"), 0);

}

int subscribe_class(int client_fd,const char *username, const char *class_name){
	sem_wait(sem_alunos);
    for (int i = 0; i < MAX_CLASSES; i++) {
        if (strcmp(share->aulas[i].name, class_name) == 0) {  // Encontrar a turma pelo nome
            // Verificar se o utilizador já está matriculado
            for (int j = 0; j < MAX_USERS_CLASS; j++) {
                if (strcmp(share->aulas[i].alunos_turma[j].username, username) == 0) {
					sem_post(sem_alunos);
					send(client_fd, "O utilizador já está inscrito nessa turma.\n", strlen("O utilizador já está inscrito nessa turma.\n"), 0);
                    return 2;  // Utilizador já está matriculado na turma
                }
            }
            // Tentar adicionar o utilizador à turma
            for (int j = 0; j < MAX_USERS_CLASS; j++) {
                if (share->aulas[i].alunos_turma[j].username[0] == '\0') {  // Encontrar espaço vazio
                    strncpy(share->aulas[i].alunos_turma[j].username, username, TAM);
					sem_post(sem_alunos);
					char mensagem[BUF_SIZE];
					snprintf(mensagem,sizeof(mensagem),"ACCEPTED <%s>\n",share->aulas[i].multicast);
					send(client_fd,mensagem, strlen(mensagem), 0);
                    return 1;  // Sucesso ao adicionar o utilizador
                }
            }
			sem_post(sem_alunos);
			send(client_fd, "REJECTED\n", strlen("REJECTED\n"), 0);
            return 3;  // Turma cheia
        }
    }
	sem_post(sem_alunos);
	send(client_fd, "Turma não encontrada.\n", strlen("Turma não encontrada.\n"), 0);
    return 0;  // Turma não encontrada
}

int create_class(int  client_fd, const char *class_name, const char *max_alunos_str, const char *prof) {
	int id;
    int max_alunos = atoi(max_alunos_str); // Converte a string do tamanho máximo para um inteiro
    if (max_alunos < 1 || max_alunos > MAX_USERS_CLASS) {
		send(client_fd, "Número de alunos inválido.\n", strlen("Número de alunos inválido.\n"), 0);
        return 2; // Retorna 2 se o número de alunos não for válido
    }
	sem_wait(sem_alunos);
    for (int i = 0; i < MAX_CLASSES; i++) {
        if (share->aulas[i].name[0] == '\0') { // Procura por um espaço livre no array de classes
			id = i;
			char mensagem [BUF_SIZE];
            strncpy(share->aulas[i].name, class_name, TAM - 1); // Define o nome da classe
            share->aulas[i].name[TAM - 1] = '\0'; // Garante que a string seja terminada corretamente
            share->aulas[i].max_alunos = max_alunos; // Define o número máximo de alunos
            memset(share->aulas[i].alunos_turma, 0, sizeof(share->aulas[i].alunos_turma)); // Inicializa a lista de alunos
			strcpy(share->aulas[i].prof,prof);

			//set the multicast base address
			struct in_addr multicast_base = { htonl(0xEF000000) };
			//add the group id to the base address
			multicast_base.s_addr = htonl(ntohl(multicast_base.s_addr) + id);
			//convert the multicast address to a string
			char multicast_addr_str[INET_ADDRSTRLEN];
			if(inet_ntop(AF_INET, &multicast_base, multicast_addr_str, sizeof(multicast_addr_str)) == NULL)
				perror("Error converting multicast address to a string");

			//copying multicast address to respective multicast group struct addr
			strcpy(share->aulas[i].multicast, multicast_addr_str);
			sem_post(sem_alunos);
			snprintf(mensagem,sizeof(mensagem),"OK <%s>\n",share->aulas[i].multicast);
			send(client_fd,mensagem, strlen(mensagem), 0);

            return 1; // Sucesso na criação da classe
        } else if (strcmp(share->aulas[i].name, class_name) == 0) {
			sem_post(sem_alunos);
			send(client_fd, "Nome da classe já existe.\n", strlen("Nome da classe já existe.\n"), 0);
            return 3; // Retorna 3 se uma classe com o mesmo nome já existe
        }
    }
	sem_post(sem_alunos);
	send(client_fd, "Número máximo de classes excedido.\n", strlen("Número máximo de classes excedido.\n"), 0);
    return 0; // Retorna 0 se não houver espaço disponível para novas classes
}

void send_cont(int client_fd, const char *class_name,const char *message, int multicast_socket, struct sockaddr_in multicast_addr,const char *nome){
	for (int i = 0; i < MAX_CLASSES; i++) {
        if (strcmp(share->aulas[i].name, class_name) == 0) { 
			if(strcmp(nome, share->aulas->prof)==0){
				multicast_addr.sin_addr.s_addr = inet_addr(share->aulas[i].multicast);

				if(sendto(multicast_socket, message, strlen(message), 0, (struct sockaddr*)&multicast_addr, sizeof(multicast_addr)) < 0)
					perror("Erro a enviar mensagem para o grupo multicast");
				write(client_fd, "Mensagem enviada!\n", strlen("Mensagem enviada!\n"));
				return;
			}else{
				write(client_fd, "Não é o professor regente desta aula!\n", strlen("Não é o professor regente desta aula!\n"));
				return;
			}
		}
	}
	write(client_fd, "Turma selecioanda não existe!\n", strlen("Turma selecioanda não existe!\n"));
}

int is_user_in_class(const char *username, const char *class_name) {
    for (int i = 0; i < MAX_CLASSES; i++) {
        if (strcmp(share->aulas[i].name, class_name) == 0) {  // Encontrar a turma pelo nome
            for (int j = 0; j < MAX_USERS_CLASS; j++) {
                if (strcmp(share->aulas[i].alunos_turma[j].username, username) == 0) {
                    return 1;  // Utilizador encontrado na turma
                }
            }
            break;  // Para a busca uma vez que a turma foi encontrada
        }
    }
    return 0;  // Utilizador não encontrado na turma ou turma não encontrada
}

//-------------------------DIVISÓRIA DAS FUNÇÔES PERTENCENTES A UDP COMEÇA AQUI----------------------------------
void udp_server_function(unsigned short udp_port) {
    struct sockaddr_in udp_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);
    char buf[BUF_SIZE], comando[TAM],arg1[TAM], arg2[TAM], arg3[TAM];


     // Cria um socket UDP
    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0) {
        erro("Erro ao criar socket UDP");
        exit(EXIT_FAILURE);
    }

    // Configura o endereço do servidor
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = udp_port;
    udp_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Aceita conexões em qualquer interface de rede

    // Vincula (bind) o socket ao endereço/porta
    if (bind(udp_fd, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0) {
        erro("Erro no bind do socket UDP");
        close(udp_fd);
        exit(EXIT_FAILURE);
    }

	int admin_logado = 0;
    while (1) {
		memset(buf, '\0', sizeof(buf));
		memset(comando, '\0', sizeof(comando));
		memset(arg1, '\0', sizeof(arg1));
		memset(arg2, '\0', sizeof(arg2));
		memset(arg3, '\0', sizeof(arg3));

        int bytes_received = recvfrom(udp_fd, buf, BUF_SIZE, 0, (struct sockaddr*)&client_addr, &addrlen);
        if (bytes_received < 0) {
            perror("Erro ao receber dados UDP");
            continue;
		}


        sscanf(buf, "%s %s %s %s", comando, arg1, arg2, arg3);
		if (strcmp(buf, "X") != 0 && strlen(buf) != 1) {
			if (strcmp(comando, "LOGIN") == 0) {
				login_user(arg1, arg2, &admin_logado);

				if(admin_logado == 1){//mensagem de ter conseguido logar
					sendto(udp_fd, "Login efetuado com sucesso!\n", strlen("Login efetuado com sucesso!\n"), 0, (struct sockaddr*) &client_addr, addrlen);
				}

				else{//mensagem de não ter conseguido logar
					sendto(udp_fd, "Dados de acesso inválidos!\n", strlen("Dados de acesso inválidos!\n"), 0, (struct sockaddr*) &client_addr, addrlen);
				}
			} 
			else if (admin_logado == 1) {
				if (strcmp(comando, "ADD_USER") == 0) {

					if (add_user(arg1, arg2, arg3)==1) {
						sendto(udp_fd, "Utilizador adicionado com sucesso.\n", strlen("Utilizador adicionado com sucesso.\n"), 0, (struct sockaddr*)&client_addr, addrlen);
					} else if(add_user(arg1, arg2, arg3)==0){
						sendto(udp_fd, "Cargo nao existente.\n", strlen("Cargo nao existente.\n"), 0, (struct sockaddr*)&client_addr, addrlen);
					}
					else
						sendto(udp_fd, "Já existe esse username.\n", strlen("Já existe esse username.\n"), 0, (struct sockaddr*)&client_addr, addrlen);
				} 
				else if (strcmp(comando, "DEL") == 0) {
					if (remove_utilizador(arg1)) {
						sendto(udp_fd, "Utilizador eliminado com sucesso.\n", strlen("Utilizador eliminado com sucesso.\n"), 0, (struct sockaddr*)&client_addr, addrlen);
					} else {
						sendto(udp_fd, "Utilizador não encontrado.\n", strlen("Utilizador não encontrado.\n"), 0, (struct sockaddr*)&client_addr, addrlen);
					}
				} 
				else if (strcmp(comando, "LIST") == 0) {
					listar_users(udp_fd, client_addr, addrlen);

				} 
				else if (strcmp(comando, "QUIT_SERVER") == 0) {
					escrever_ficheiro();
					printf("Servidor UDP encerrando...\n");
					fflush(stdout);
					kill(p_tcp, SIGTERM);
                    waitpid(p_tcp, NULL, 0); // Espera o filho terminar
					sem_close(sem_alunos);
					sem_close(sem_utilizadores);
					sem_unlink("utilizadores");
					sem_unlink("alunos");
					if (shmdt(share) == -1) {
						perror("ERROR IN shmdt");
					}
					if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
						perror("ERROR IN shmctl");
					}
					printf("Servidor encerrado.\n");
					free(file);

                    break;

				} 
				else {
					sendto(udp_fd, "Comando desconhecido.\n", strlen("Comando desconhecido.\n"), 0, (struct sockaddr*)&client_addr, addrlen);
				}
			} 
			else {
				sendto(udp_fd, "Inicia sessão primeiramente!\n", strlen("Inicia sessão primeiramente!\n"), 0, (struct sockaddr*)&client_addr, addrlen);
			}
		}
		else if(strcmp(buf, "X") != 0 ){
			sendto(udp_fd, "Nenhum comando recebido!\n", strlen("Nenhum comando recebido!\n"), 0, (struct sockaddr*) &client_addr, addrlen);
    		continue;
		}
		else
			continue;
    }
	close(udp_fd);
}


void login_user(const char *username, const char *password, int *login) {
    *login = 0; 
    for (int i = 0; i < MAX_USERS; i++) {
        if (share->users[i].username[0] != '\0' &&  // Checa se o usuário está ativo
            strcmp(username, share->users[i].username) == 0 && 
            strcmp(password, share->users[i].password) == 0) {
            
            if (strcmp(share->users[i].role, "administrador") == 0) {
                *login = 1; // Sucesso no login como administrador
				sem_post(sem_utilizadores);
                return;
            } else if (strcmp(share->users[i].role, "aluno") == 0) {
                *login = 2; // Sucesso no login como aluno
				sem_post(sem_utilizadores);
                return;
            } else if (strcmp(share->users[i].role, "professor") == 0) {
                *login = 3; // Sucesso no login como professor
				sem_post(sem_utilizadores);
                return;
            }
        }
    }
	sem_post(sem_utilizadores);
    // Se chegar aqui, significa que não encontrou um usuário com as credenciais fornecidas
}

int add_user(const char *name, const char *pass, const char *ro) {  			// admin adiciona um usuário
	int result = 1;
	utilizador person;
    char aux[TAM];
    strcpy(aux, ro);

	if(verifica_nome(name)){
		if (verifica_func(ro)) { //Verifica se o terceiro parametro é leitor, admin ou jornalista
			strcpy(person.username, name); //Guarda as 3 strings nos seus respetivos lugares na struct
			strcpy(person.password, pass);
			strcpy(person.role, ro);
			insere_utilizador(person); //Adiciona-se essa struct à lista
		} else {
			result=0;
		}
	}
	else{
		result=2;
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

int verifica_nome(const char aux[TAM]){
	for (int j = 0; j < MAX_USERS; j++) {
        if (strcmp(share->users[j].username, aux) == 0) {
			return 0;  // Já existe com esse nome
		}
	}
	return 1;
}


void insere_utilizador(utilizador person) {
	sem_wait(sem_utilizadores);
    for (int i = 0; i < MAX_USERS; i++) {
        if (share->users[i].username[0] == '\0') {  // Presume-se que um nome de usuário vazio indica uma posição livre
            share->users[i] = person;
			sem_post(sem_utilizadores);
            return;
        }
    }
	sem_post(sem_utilizadores);
    printf("Erro: Limite máximo de utilizadores atingido.\n");
}

int remove_utilizador(char username[TAM]) {
	sem_wait(sem_utilizadores);
    for (int i = 0; i < MAX_USERS; i++) {
        if (strcmp(share->users[i].username, username) == 0) {
            memset(&share->users[i], 0, sizeof(utilizador));  // Limpa os dados do utilizador
			sem_post(sem_utilizadores);
            return 1;  // Sucesso na remoção
        }
    }
	sem_post(sem_utilizadores);
    return 0;  // Utilizador não encontrado
}

void listar_users(int udp_fd, struct sockaddr_in client_addr, socklen_t addrlen) {
    char buf[BUF_SIZE];
	sem_wait(sem_utilizadores);
    for (int i = 0; i < MAX_USERS; i++) {
        if (share->users[i].username[0] != '\0') {  // Checa se o nome de usuário não está vazio
            sprintf(buf, "%s -> %s\n", share->users[i].username, share->users[i].role);
            sendto(udp_fd, buf, strlen(buf), 0, (struct sockaddr*)&client_addr, addrlen);
        }
    }
	sem_post(sem_utilizadores);
}


//------------------------- FUNÇÕES RESPONSAVEIS PELO TRATAMENTO DO FICHEIRO DE TEXTO ----------------------------------

void ler_ficheiro() {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Erro ao abrir o ficheiro.");
        return;
    }
    utilizador person;
    while (fscanf(file, "%[^;];%[^;];%[^;\n]\n", person.username, person.password, person.role) == 3) {
        insere_utilizador(person);
    }   
    fclose(file);
}

void escrever_ficheiro() {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Erro ao abrir o ficheiro para escrita.");
        return;
    }
    for (int i = 0; i < MAX_USERS; i++) {
        if (share->users[i].username[0] != '\0') {  // Checa se o nome de usuário não está vazio
            fprintf(file, "%s;%s;%s\n", share->users[i].username, share->users[i].password, share->users[i].role);
        }
    }
    fclose(file);
}

void treat_signal(int sig){
	escrever_ficheiro();
    if (p_tcp != 0) {
        kill(p_tcp, SIGTERM);
        waitpid(p_tcp, NULL, 0);
    }
	
	sem_close(sem_alunos);
	sem_close(sem_utilizadores);
	sem_unlink("utilizadores");
	sem_unlink("alunos");
	if (shmdt(share) == -1) {
		perror("ERROR IN shmdt");
	}
	if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
		perror("ERROR IN shmctl");
	}
    printf("Servidor encerrado.\n");
	free(file);
    exit(0);
}

void erro(char *msg) {
    perror(msg);
    exit(1);
}



void create_shared() {
    int shm_size = sizeof(shared);  // Como 'shared' agora contém tudo, não precisamos adicionar mais nada.

    if ((shm_id = shmget(IPC_PRIVATE, shm_size, IPC_CREAT | IPC_EXCL | 0700)) < 0) {
        printf("ERROR IN SHMGET\n");
        exit(1);
    }

    share = (shared *)shmat(shm_id, NULL, 0);
    if (share == (void *)-1) {
        printf("ERROR IN SHMAT");
        exit(1);
    }

    // Inicialização de valores padrão (opcional)
    memset(share, 0, shm_size);  // Zera a memória alocada
}

void init_shared_struct(shared *share) {
    // Inicializar todos os utilizadores
    for (int i = 0; i < MAX_USERS; i++) {
        memset(share->users[i].username, 0, TAM);
        memset(share->users[i].password, 0, TAM);
        memset(share->users[i].role, 0, TAM);
    }

    // Inicializar todas as classes
    for (int i = 0; i < MAX_CLASSES; i++) {
        memset(share->aulas[i].name, 0, TAM);
        memset(share->aulas[i].multicast, 0, TAM);
        share->aulas[i].max_alunos = -1;

        // Inicializar os alunos em cada turma
        for (int j = 0; j < MAX_USERS_CLASS; j++) {
            memset(share->aulas[i].alunos_turma[j].username, 0, TAM);
            memset(share->aulas[i].alunos_turma[j].password, 0, TAM);
            memset(share->aulas[i].alunos_turma[j].role, 0, TAM);
        }
    }
}