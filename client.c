#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <gtk/gtk.h>
#include "my_protocol.h"
#include <pthread.h>


#define SERVER_PORT 10007        /* arbitrary, but client and server must agree */
#define BUF_SIZE 4096            /* block transfer size */

//gcc -o client.o client.c -lpthread -Wall `pkg-config --cflags --libs gtk+-3.0` -export-dynamic
GtkWidget *input;
GtkWidget *text_output;
GtkWidget *user_list;
GtkWidget *room_list;
int s, auxargc;
char *my_name, *js;
char **auxargv;
pthread_t tid;

void send_msg(int type, ssize_t bytes, char *buf);

char *create_malloc_string(char str[]);

char *create_new_message(char *type, char *origin, char *destination, char *message);

void print_to_text_area(const gchar *text);

void print_to_list_user(const gchar *text);

void print_to_list_room(const gchar *text);

void *thread(void *vargp);

void on_send_button_clicked();

void on_input_activate();

void on_window_main_destroy();

struct sockaddr_in init_client_info(char *name) {
	struct hostent *h; /* info about server */
	h = gethostbyname(name);        /* look up host's IP address */
	if (!h) {
		perror("gethostbyname");
		exit(-1);
	}
	struct sockaddr_in channel;
	memset(&channel, 0, sizeof(channel));
	channel.sin_family = AF_INET;
	memcpy(&channel.sin_addr.s_addr, h->h_addr, h->h_length);
	channel.sin_port = htons(SERVER_PORT);
	return channel;
}

int main(int argc, char **argv) {

	/*
	 * GUI
	 */
	GtkBuilder *builder;
	GtkWidget *window;

	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "glade/window_main.glade", NULL);

	window = GTK_WIDGET(gtk_builder_get_object(builder, "window_main"));
	gtk_builder_connect_signals(builder, NULL);

	// get pointers to the two labels
	input = GTK_WIDGET(gtk_builder_get_object(builder, "input"));
	text_output = GTK_WIDGET(gtk_builder_get_object(builder, "text_output"));
	user_list = GTK_WIDGET(gtk_builder_get_object(builder, "user_list"));
	room_list = GTK_WIDGET(gtk_builder_get_object(builder, "room_list"));

	g_object_unref(builder);
	gtk_widget_show(window);


	/*
	 * Client
	 */
	auxargc = argc;
	auxargv = argv;
	pthread_create(&tid, NULL, thread, NULL);
	//pthread_join(tid, (void**)&i);
	while (1) {
		//gtk_main();
	}
}

void send_msg(int type, ssize_t bytes, char *buf) {
	char *user = NULL, str[256], *stype;
	if (type == 0) {
		stype = create_malloc_string("broadcast");
		user = create_malloc_string("null");
	} else if (type == 1) {
		stype = create_malloc_string("room");
		user = create_malloc_string("null");
	} else if (type == 2) {
		stype = create_malloc_string("private");
		puts("Utilizador?\n");
		scanf("%s", str);
		user = create_malloc_string(str);
	} else if (type == 3) {
		stype = create_malloc_string("whoisonline");
		user = create_malloc_string("null");
	} else if (type == 4) {
		stype = create_malloc_string("create");
		user = create_malloc_string("null");
	} else if (type == 5) {
		stype = create_malloc_string("join");
		user = create_malloc_string("null");
	} else if (type == 6) {
		stype = create_malloc_string("leave");
		user = create_malloc_string("null");
	} else if (type == 7) {
		stype = create_malloc_string("rooms");
		user = create_malloc_string("null");
	} else {
		puts("Error;\n");
		return;
	}
	if (buf != NULL)
		buf[bytes - 1] = '\0';
	else
		buf="null";
	js = create_new_message(stype, my_name, user, buf);
	write(s, js, strlen(js));
}

char *create_malloc_string(char str[]) {
	char *newstr = malloc(strlen(str));
	strcpy(newstr, str);
	return newstr;
}

char *create_new_message(char *type, char *origin, char *destination, char *message) {
	char *new_str;
	if ((new_str = malloc(strlen(type) + strlen(origin) + strlen(destination) + strlen(message) + 65)) != NULL) {
		new_str[0] = '\0';   // ensures the memory is an empty string
		strcat(new_str, "{\"type\":\"");
		strcat(new_str, type);
		strcat(new_str, "\",\"origin\":\"");
		strcat(new_str, origin);
		strcat(new_str, "\",\"destination\":\"");
		strcat(new_str, destination);
		strcat(new_str, "\",\"message\":\"");
		strcat(new_str, message);
		strcat(new_str, "\"}\0");
		return new_str;
	}
	exit(-1);
}

void print_to_text_area(const gchar *text) {

	GtkTextBuffer *buffer;
	GtkTextMark *mark;
	GtkTextIter iter;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (text_output));

	mark = gtk_text_buffer_get_insert(buffer);
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);

//	/* Insert newline (only if there's already text in the buffer). */
	if (gtk_text_buffer_get_char_count(buffer)) {
		gtk_text_buffer_insert(buffer, &iter, "\n", 1);
	}

	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
	gtk_text_buffer_insert(buffer, &iter, text, -1);
	gtk_entry_set_text(GTK_ENTRY(input), "");
}

void print_to_list_user(const gchar *text) {

//	GtkTextBuffer *buffer;
//	GtkTextMark *mark;
//	GtkTextIter iter;
//
//	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (user_list));
//
//	mark = gtk_text_buffer_get_insert(buffer);
//	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
//
//	/* Insert newline (only if there's already text in the buffer). */
//	if (gtk_text_buffer_get_char_count(buffer)) {
//		gtk_text_buffer_insert(buffer, &iter, "\n", 1);
//	}
//
//	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
//	gtk_text_buffer_insert(buffer, &iter, text, -1);
//	gtk_text_buffer_get_end_iter(buffer, &iter);
//	gtk_text_buffer_insert(buffer, 0, text, -1);
	GtkTextBuffer *buffer = gtk_text_buffer_new(NULL);
	//user_list = gtk_text_view_new_with_buffer(buffer);
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(user_list), buffer);

	//buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

	gtk_text_buffer_set_text (buffer, text, -1);
}

void print_to_list_room(const gchar *text) {

	GtkTextBuffer *buffer;
	GtkTextMark *mark;
	GtkTextIter iter;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (room_list));

	mark = gtk_text_buffer_get_insert(buffer);
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);

//	/* Insert newline (only if there's already text in the buffer). */
	if (gtk_text_buffer_get_char_count(buffer)) {
		gtk_text_buffer_insert(buffer, &iter, "\n", 1);
	}

	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
	gtk_text_buffer_insert(buffer, &iter, text, -1);
}

void *thread(void *vargp) {
	int c, /*s, bytes,*/ maxs;
	ssize_t bytes;
	char buf[BUF_SIZE];            /* buffer for incoming file */
	struct hostent *h;            /* info about server */
	struct sockaddr_in channel;        /* holds IP address */
	fd_set rset;
	if (auxargc != 3) {
		printf("Usage: is-client server-name nickname\n");
		exit(-1);
	}

	channel = init_client_info(auxargv[1]);

	s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s < 0) {
		perror("socket");
		exit(-1);
	}

	my_name = auxargv[2];

	c = connect(s, (struct sockaddr *) &channel, sizeof(channel));
	if (c < 0) {
		perror("connect");
		exit(-1);
	} else {
		js = create_new_message("registration", auxargv[2], "null", "null");
		write(s, js, strlen(js));  //enviar o nickname ao servidor

	}

	maxs = s;

	FD_ZERO(&rset);
	FD_SET(s, &rset);//watch for new connections
	FD_SET(0, &rset);//watch for something from the keyboard

	/* Socket is now set up and bound. Wait for connection and process it. */
	while (1) {


		FD_ZERO(&rset);
		FD_SET(s, &rset);//watch for new connections
		FD_SET(0, &rset);//watch for something from the keyboard

		int nready = select(maxs + 1, &rset, NULL, NULL,
		                    NULL);//blocks until something happen in the file descriptors watched
		if (FD_ISSET(s, &rset))//check if something happen in the listenfd (new connection)
		{
			bytes = read(s, buf, BUF_SIZE);
			if (bytes == 0) {
				printf("Server disconnected!\n");
				close(s);
				exit(0);
			}
			buf[bytes] = '\0';
			if (strstr(buf, "[USERS]") != NULL) {
				print_to_list_user(&buf[7]);
			} else if (strstr(buf, "[LROOM]") != NULL) {
				print_to_list_room(&buf[7]);
			} else
				print_to_text_area(buf);
			write(1, buf, (size_t) bytes);
			puts("\n");
		}
		if (FD_ISSET(0, &rset)) //check if something happen in the stdin (new input from the keyboard)
		{
			int type = 0;
			bytes = read(0, buf, BUF_SIZE);
			printf("\n"
			       " |0| Para enviar um broadcast\n"
			       " |1| Enviar para a sala\n"
			       " |2| Enviar para outro utilizador\n"
			       " |3| Listar utilizadores\n"
			       " |4| Criar uma sala\n"
			       " |5| Entrar sala\n"
			       " |6| Sair sala\n"
			       " |7| Listar salas\n"
			       "\n");
			scanf("%d", &type);
			send_msg(type, bytes, buf);
		}
	}
}

// called when button is clicked
void on_send_button_clicked() {
	GtkTextBuffer *buffer;
	GtkTextMark *mark;
	GtkTextIter iter;
	char *text;

//	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (text_output));
	text = (char *) gtk_entry_get_text(GTK_ENTRY (input));
//
//	mark = gtk_text_buffer_get_insert(buffer);
//	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
//
////	/* Insert newline (only if there's already text in the buffer). */
//	if (gtk_text_buffer_get_char_count(buffer)) {
//		gtk_text_buffer_insert(buffer, &iter, "\n", 1);
//	}
//
//	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
//	gtk_text_buffer_insert(buffer, &iter, text, -1);
//	gtk_entry_set_text(GTK_ENTRY(input), "");
	send_msg(0, (ssize_t) strlen(text)+1, text);
	gtk_entry_set_text(GTK_ENTRY(input), "");
}

void on_send_room_clicked() {
	puts("on_send_room_clicked");
	char *text = (char *) gtk_entry_get_text(GTK_ENTRY (input));
	send_msg(1, (ssize_t) strlen(text)+1, text);
	gtk_entry_set_text(GTK_ENTRY(input), "");
}

void on_list_user_clicked() {
	puts("on_list_user_clicked");
//	js = create_new_message("rooms", my_name, "null", "null");
	send_msg(3, 0, NULL);
	gtk_entry_set_text(GTK_ENTRY(input), "");
}

void on_list_room_clicked(){
	puts("on_list_room_clicked");
	send_msg(7, 0, NULL);
	gtk_entry_set_text(GTK_ENTRY(input), "");
}

void on_create_room_clicked(){
	puts("on_create_room_clicked");
	char *text = (char *) gtk_entry_get_text(GTK_ENTRY (input));
	send_msg(4, (ssize_t) strlen(text)+1, text);
	gtk_entry_set_text(GTK_ENTRY(input), "");
}

void on_enter_room_clicked(){
	puts("on_enter_room_clicked");
	char *text = (char *) gtk_entry_get_text(GTK_ENTRY (input));
	send_msg(5, (ssize_t) strlen(text)+1, text);
	gtk_entry_set_text(GTK_ENTRY(input), "");
}
void on_leave_room_clicked(){
	puts("on_leave_room_clicked");
	char *text = (char *) gtk_entry_get_text(GTK_ENTRY (input));
	send_msg(6, (ssize_t) strlen(text)+1, text);
	gtk_entry_set_text(GTK_ENTRY(input), "");
}

void on_input_activate() {
	on_send_button_clicked();
}

// called when window is closed
void on_window_main_destroy() {
	gtk_main_quit();
	pthread_kill(tid, 2);
	exit(0);
}