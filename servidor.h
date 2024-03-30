
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <signal.h>

#define BUF_SIZE 512
#define TAM 30 

FILE *file;
char *filename;
int tcp_fd, udp_fd;
pid_t p_tcp;

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
void process_client(int client_fd,struct sockaddr_in client_addr, lista lista_utilizadores);
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
void treat_signal(int sig);


#endif 
