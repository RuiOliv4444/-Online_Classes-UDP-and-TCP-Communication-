
#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <signal.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <sys/time.h>
#include <ctype.h>

#define BUF_SIZE 512
#define MAX_USERS 999
#define MAX_CLASSES 100
#define MAX_USERS_CLASS 50
#define TAM 30 
#define MULTICAST_PORT 8888

FILE *file;
char *filename;
int tcp_fd, udp_fd;
pid_t p_tcp;
int shm_id;

typedef struct utilizador {
    char username[TAM];
    char password[TAM];
    char role[TAM];
}utilizador;

typedef struct classes {
    char name[TAM];
	char multicast[TAM];
	int max_alunos;
	utilizador alunos_turma[MAX_USERS_CLASS];
	char prof;
}classes;

sem_t * sem_utilizadores;
sem_t * sem_alunos;


typedef struct shared{
	utilizador users[MAX_USERS]; //lista dos utilizadores que se vao registar
	classes aulas[MAX_CLASSES];
}shared;

shared * share;


void create_shared();
void init_shared_struct(shared *share);
void erro(char *msg);
void process_client(int client_fd, struct sockaddr_in client_addr);
void login_user(const char *username, const char *password, int *login);
void udp_server_function(unsigned short udp_port);
void list_classes(int client_fd, const char *nome);
void list_subscribed(int client_fd, const char *nome);
int subscribe_class(int client_fd,const char *username, const char *class_name);
int create_class(int  client_fd, const char *class_name, const char *max_alunos_str);
void send_cont(int client_fd, const char *class_name,const char *message, int multicast_socket, struct sockaddr_in multicast_addr,const char *nome);
int is_user_in_class(const char *username, const char *class_name);
int add_user(const char *name, const char *pass, const char *ro);
void insere_utilizador(utilizador person);
int remove_utilizador(char username[TAM]);
void ler_ficheiro();
void escrever_ficheiro();
void listar_users(int udp_fd, struct sockaddr_in client_addr, socklen_t addrlen);
int verifica_func(const char aux[TAM]);
int verifica_nome(const char aux[TAM]);
void treat_signal(int sig);


#endif 