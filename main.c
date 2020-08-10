#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "my_protocol.h"
#include "jsmn.h"
//gcc -o server.o main.c -pthread -include my_protocol.h my_protocol.c jsmn.h jsmn.c
#define SERVER_PORT 10007        /* arbitrary, but client and server must agree */
#define BUF_SIZE 4096            /* block transfer size */
#define QUEUE_SIZE 10

pthread_t *pcoon;

typedef struct client {

	char *nickname;
	int sockID;
	int on;
	struct client *next;

} CLIENT;

typedef struct clientlist {

	CLIENT *client;
	struct clientlist *next;

} CLIENTLIST;

typedef struct room {

	char *roomname;
	CLIENTLIST *clientlist;
	struct room *next;

} ROOM;

struct coon {
	CLIENT *client;
	int *connfd;
	int *max;
};

typedef struct coon COON;

// typedef struct conn {
// 	CLIENT *client;
// 	int connfd;
// 	int max;
// } COON;

struct sockaddr_in init_server_info() {
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERVER_PORT);
	return servaddr;
}


int find_sockID_by_name(CLIENT *auxmsg, char *dest);

CLIENT *find_client_by_sockID(CLIENT *temp, int sockID);

ROOM *find_room_by_name(ROOM *temp, char *name);

ROOM *find_room_by_user(ROOM *temp, char *user);

char *create_malloc_string(char str[]);

void *create_new_client(void *_coon);

void printClientList(CLIENT *head, int dest);

CLIENT *copy_client(CLIENT client);

void remove_client(CLIENTLIST *pcs, char *name);

//Globais resolver em breve
CLIENT *head = NULL;
ROOM *chatroom = NULL;

int main(int argc, char *argv[]) {
//	CLIENT *head = NULL;
//	ROOM *chatroom = NULL;
	char mensagem[BUF_SIZE] = {'\0'};
	int s, maxs, b, l;
	fd_set rset;
	pthread_t th1;
	char buf[BUF_SIZE];            /* buffer for outgoing file */
	struct sockaddr_in servaddr;    /* hold's IP address */

	/* Passive open. Wait for connection. */
	servaddr = init_server_info();

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); /* create socket */
	if (s < 0) {
		perror("socket error");
		exit(-1);
	}

	b = bind(s, (struct sockaddr *) &servaddr, sizeof(servaddr));
	if (b < 0) {
		perror("bind error");
		exit(-1);
	}

	l = listen(s, QUEUE_SIZE);        /* specify queue size */
	if (l < 0) {
		perror("listen error");
		exit(-1);
	}

	FD_ZERO(&rset);
	FD_SET(s, &rset);//watch for new connections
	maxs = s;

	/* Socket is now set up and bound. Wait for connection and process it. */
	while (1) {

		FD_ZERO(&rset);
		FD_SET(s, &rset);
		if (head != NULL) {
			CLIENT *auxiliar = head;
			while (auxiliar != NULL)  //definir todos os descritores com o FD_SET
			{
				if (auxiliar->on == 1) {
					FD_SET(auxiliar->sockID, &rset);

					if (maxs < auxiliar->sockID) {
						maxs = auxiliar->sockID;
					}
				}
				auxiliar = auxiliar->next;
			}
		}

		int nready = select(maxs + 1, &rset, NULL, NULL,
		                    NULL);  //blocks until something happen in the file descriptors watched
		if (FD_ISSET(s, &rset))//check if something happen in the listenfd (new connection)
		{
			socklen_t cliaddr_len = sizeof(servaddr);
			int connfd = accept(s, (struct sockaddr *) &servaddr, &cliaddr_len);

			if (find_client_by_sockID(head, connfd) == NULL) {
				COON *coon = (COON *) malloc(1 * sizeof(COON));
				coon->connfd = &connfd;
				coon->max = &maxs;
				//puts("I will try to create a thread...");
				if (pthread_create(&th1, NULL, &create_new_client, (void *) coon)) {
					fprintf(stderr, "Error creating thread\n");
					return 1;
				}
				pthread_join(th1, NULL);
				printf("New connection%d\n", connfd);
			}
		} else {    //else-> ciclo a percorrer todos os clientes-> fazer fd_isset a perguntar cada sockID de cada cliente se foi ele que mexeu
			CLIENT *auxtest = head;
			while (auxtest != NULL) {
				if (FD_ISSET(auxtest->sockID, &rset)) {
					int bytes = read(auxtest->sockID, buf, BUF_SIZE);
					printf("Bytes: %d\n", bytes);

					if (bytes == 0) {
						close(auxtest->sockID);
						printf("User \"%s\" disconnected\n", auxtest->nickname);
						auxtest->on = 0;

						CLIENT *auxmsg = (CLIENT *) malloc(1 * sizeof(CLIENT));
						auxmsg = head;
						sprintf(mensagem, "Client disconnected: %s\n", auxtest->nickname);
						while (auxmsg != NULL) {
							write(auxmsg->sockID, mensagem, strlen(mensagem));
							auxmsg = auxmsg->next;
						}

						break;
					}
					buf[bytes] = '\0';
					PROTOCOL *prot = parse_message(buf);

					if (strcmp(prot->type, "registration") ==
					    0) {                //erro cria um novo utilizador sempre que um utilizador sai e volta a entrar
						CLIENT *auxmsg = (CLIENT *) malloc(1 * sizeof(CLIENT));
						//if (find_sockID_by_name(auxmsg, prot->origin) != -1) {
						if (find_client_by_sockID(head, auxtest->sockID)) {
							puts("Turn online user\n");
							auxtest->on = 1;
						} else {
							puts("Create new user\n");
							auxtest->on = 1;
							auxtest->nickname = malloc(sizeof(char) * strlen(prot->origin) + 1);
							strcpy(auxtest->nickname, prot->origin);
							printf("New client: %s\n", auxtest->nickname);
						}
						auxmsg = head;
						sprintf(mensagem, "New client: %s\n", auxtest->nickname);
						while (auxmsg != NULL) {
							write(auxmsg->sockID, mensagem, strlen(mensagem));
							auxmsg = auxmsg->next;
						}
						puts("Done!!");
					} else {
						CLIENT *auxmsg = (CLIENT *) malloc(1 * sizeof(CLIENT));
						auxmsg = head;
						print_protocol(prot);
						if (strcmp(prot->type, "broadcast") == 0) {
							puts("Send broadcast\n");
							sprintf(mensagem, "[BROADCAST]  %s: %s", prot->origin, prot->message);
							while (auxmsg != NULL) {
								write(auxmsg->sockID, mensagem, strlen(mensagem));
								auxmsg = auxmsg->next;
							}
							//free(auxmsg);
						} else if (strcmp(prot->type, "room") == 0) {
							puts("Enviar para a sala\n");
							ROOM *roomtemp = find_room_by_user(chatroom, prot->origin);
							if (roomtemp == NULL) {
								sprintf(mensagem, "[ERRO] Voce nao esta registado numa sala!");
								write(find_sockID_by_name(head, prot->origin), mensagem, strlen(mensagem));
							} else {
								CLIENT * clienttemp;
								clienttemp=roomtemp->clientlist->client;
								while (clienttemp != NULL) {
									int dest = clienttemp->sockID;
									sprintf(mensagem, "[SALA - %s] %s: %s", roomtemp->roomname, prot->origin, prot->message);
									write(dest, mensagem, strlen(mensagem));
									clienttemp = clienttemp->next;
								}
							}
							//free(temp);
						} else if (strcmp(prot->type, "private") == 0) {
							puts("Enviar a user\n");
							CLIENT *temp = auxmsg;
							int dest = find_sockID_by_name(temp, prot->destination);
							if (dest == -1) {
								sprintf(mensagem, "[ERRO] Esse utilizador nao existe!");
								write(find_sockID_by_name(head, prot->origin), mensagem, strlen(mensagem));
							} else {
								sprintf(mensagem, "[PRIVATE] %s: %s", prot->origin, prot->message);
								printf("DEST: %d\n", dest);
								write(dest, mensagem, strlen(mensagem));
							}
							//free(temp);
						} else if (strcmp(prot->type, "whoisonline") == 0) {
							puts("Listar users\n");
							CLIENT *temp = auxmsg;
							int dest = find_sockID_by_name(temp, prot->origin);
							printClientList(head, dest);
							//free(temp);
						} else if (strcmp(prot->type, "create") == 0) {
							puts("Criar sala\n");
							if (strlen(prot->message) < 1 || strlen(prot->message) > 20) {
								sprintf(mensagem, "[ERRO] Nome invalido");
								write(find_sockID_by_name(head, prot->origin), mensagem, strlen(mensagem));
							} else {
								ROOM *new = (ROOM *) malloc(1 * sizeof(ROOM));
								CLIENTLIST *listtemp = (CLIENTLIST *) malloc(1 * sizeof(CLIENTLIST));
								listtemp->client = copy_client(
										*find_client_by_sockID(head, find_sockID_by_name(head, prot->origin)));
								new->roomname = prot->message;
								new->clientlist = listtemp;
								new->next = NULL;

								if (chatroom == NULL) {
									chatroom = new;
								} else {
									ROOM *roomtemp = chatroom;
									while (roomtemp->next != NULL) {
										roomtemp = roomtemp->next;
									}
									roomtemp->next = new;
								}
							}
						} else if (strcmp(prot->type, "join") == 0) {
							puts("Entrar sala\n");
							if (chatroom == NULL) {
								sprintf(mensagem, "Não existem salas!");
								write(find_sockID_by_name(head, prot->origin), mensagem, strlen(mensagem));
							} else {
								int flag = 0;
								ROOM *roomtemp = chatroom;
								while (roomtemp != NULL) {
									if (strcmp(roomtemp->roomname, prot->message) == 0) {
										CLIENTLIST *clienttemp = roomtemp->clientlist;
										while (clienttemp->client->next != NULL) {
											sprintf(mensagem, "[ALERT] %s entrou na sala %s", prot->origin, prot->message);
											write(clienttemp->client->sockID, mensagem, strlen(mensagem));
											clienttemp->client = clienttemp->client->next;
										}
										clienttemp->client->next = copy_client(
												*find_client_by_sockID(head, find_sockID_by_name(head, prot->origin)));
										flag = 1;
										//free(clienttemp);
									}
									roomtemp = roomtemp->next;
								}
								if (flag == 0)
									sprintf(mensagem, "Sala nao existe!");
								else
									sprintf(mensagem, "Entrou na sala %s", prot->message);
								write(find_sockID_by_name(head, prot->origin), mensagem, strlen(mensagem));

							}
						} else if (strcmp(prot->type, "leave") == 0) {
							puts("Sair sala\n");
							if (chatroom == NULL) {
								sprintf(mensagem, "Não existem salas!");
								write(find_sockID_by_name(head, prot->origin), mensagem, strlen(mensagem));
							} else {
								int flag = 0;
								ROOM *roomtemp = chatroom;
								while (roomtemp != NULL) {
									if (strcmp(roomtemp->roomname, prot->message) == 0) {
										CLIENTLIST *clienttemp = roomtemp->clientlist;
										remove_client(clienttemp, prot->origin);
										flag = 1;
										//free(clienttemp);
									}
									roomtemp = roomtemp->next;
								}
								if (flag == 0)
									sprintf(mensagem, "Sala nao existe!");
								else
									sprintf(mensagem, "Saiu na sala %s", prot->message);
								write(find_sockID_by_name(head, prot->origin), mensagem, strlen(mensagem));

							}
							//free(temp);
						} else if (strcmp(prot->type, "rooms") == 0) {
							puts("Listar sala\n");
							ROOM *roomtemp = chatroom;
							int dest = find_sockID_by_name(head, prot->origin);
							char str[1024], *msg;
							if (roomtemp == NULL) {
								sprintf(str, "[LROOM]Não existem salas!");
							} else {
								sprintf(str, "[LROOM]");
							}
							while (roomtemp != NULL) {
								CLIENTLIST clientlist = *roomtemp->clientlist;
								sprintf(str, "%sSala \"%s\"", str, roomtemp->roomname);
								if (clientlist.client == NULL) {
									sprintf(str, "%s\tNão existem users", str);
								}
								while (clientlist.client != NULL) {
									sprintf(str, "%s\n\tUser: \"%s\" SocketID: %d", str,
									        clientlist.client->nickname,
									        clientlist.client->sockID);
									clientlist.client = clientlist.client->next;
								}
								roomtemp = roomtemp->next;
							}
							msg = create_malloc_string(str);
							write(dest, msg, strlen(msg));
							//free(temp);
						} else {
							puts("Error, try again!");
							break;
						}
					}
				}
				auxtest = auxtest->next;
			}
		}
	}
	return 1;
}

int find_sockID_by_name(CLIENT *temp, char *dest) {
	//do {
	while (temp != NULL) {
		printf("Nick: %s - Dest: %s\n", temp->nickname, dest);
		if (temp->nickname == NULL) return -1;
		if (strcmp(temp->nickname, dest) == 0) {
			return temp->sockID;
		}
		temp = temp->next;
	}
	return -1;
}

CLIENT *find_client_by_sockID(CLIENT *temp, int sockID) {
	//do {
	CLIENT *aux = temp;
	while (aux != NULL) {
		printf("Nick: %s %d - Dest: %d\n", aux->nickname, aux->sockID, sockID);
		if (aux->nickname == NULL) return NULL;
		if (aux->sockID == sockID) {
			return aux;
		}
		aux = aux->next;
	}
	return NULL;
}

ROOM *find_room_by_name(ROOM *temp, char *name) {
	//do {
	ROOM *aux = temp;
	while (aux != NULL) {
		printf("Room: %s - Target: %s\n", aux->roomname, name);
		if (aux->roomname == NULL) return NULL;
		if (strcmp(aux->roomname, name) == 0) {
			return aux;
		}
		aux = aux->next;
	}
	return NULL;
}

ROOM *find_room_by_user(ROOM *temp, char *name) {
	ROOM *aux = temp;
	while (aux != NULL) {
		if (aux->roomname == NULL) return NULL;
		CLIENTLIST *clientlist = aux->clientlist;
		while (clientlist != NULL) {
			if (clientlist == NULL) break;
			if (strcmp(clientlist->client->nickname, name) == 0) {
				return aux;
			}
			clientlist->client = clientlist->client->next;
		}
		aux = aux->next;
	}
	return NULL;
}


char *create_malloc_string(char str[]) {
	char *newstr = malloc(strlen(str));
	strcpy(newstr, str);
	return newstr;
}

//thread new connection
void *create_new_client(void *_coon) {
	COON coon = *((COON *) _coon);
	CLIENT *new = (CLIENT *) malloc(1 * sizeof(CLIENT));
	new->nickname = NULL;
	new->sockID = *(coon.connfd);
	new->next = NULL;
	new->on = 1;
	if (head == NULL) {
		head = (CLIENT *) malloc(1 * sizeof(CLIENT));
		head = new;
	} else {
		CLIENT *auxhead = head;
		while (auxhead->next != NULL) {
			auxhead = auxhead->next;
		}
		auxhead->next = new;
	}
	//update maxs
	if (*(coon.max) < *(coon.connfd)) {
		*(coon.max) = *(coon.connfd);
	}
	return NULL;
}

void printClientList(CLIENT *head, int dest) {
	puts("User\n");
	char str[1024], *msg;
	sprintf(str, "[USERS]");
	do {
		sprintf(str, "%s\n%s, %d, %d", str,  head->nickname, head->sockID, head->on);
		head = head->next;
	} while (head != NULL);
	msg = create_malloc_string(str);
	write(dest, msg, strlen(msg));

}

CLIENT *copy_client(CLIENT client) {
	CLIENT *aux = (CLIENT *) malloc(1 * sizeof(CLIENT));
	aux->nickname = client.nickname;
	aux->sockID = client.sockID;
	aux->on = client.on;
	aux->next = NULL;
	return aux;

}

void remove_client(CLIENTLIST *pcs, char *name) {
	if (pcs->client == NULL) {
		return;
	}
	if (strcmp(pcs->client->nickname, name) == 0) {
		while (pcs->client != NULL) {
			if (pcs->next == NULL) {
				pcs->client = NULL;
			} else {
				pcs->client->nickname = pcs->next->client->nickname;
				pcs->client->sockID = pcs->next->client->sockID;
				pcs->client->on = pcs->next->client->on;
				pcs->client->next = pcs->next->client->next;
				pcs->client = pcs->client->next;
			}
		}
		return;
	}
	CLIENT *current = pcs->client, *next = current->next;
	while (next != NULL && strcmp(next->nickname, name) < 0) {
		current = next;
		next = next->next;
	}
	if (next != NULL && strcmp(next->nickname, name) == 0) {
		current->next = next->next;
		free(next);
	}
}