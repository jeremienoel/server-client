/* Sockets - Serveur
 * Avec recherche du serveur
 * Usage : serveur
 */

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

#define PORT_S_DIFF 8000
#define PORT_S_TCP  8001

#define NB_CHAR 128

int sock_S_DIFF;
int sock_S;

void fin(int n) { // gestionnaire pour SIGCHLD et SIGINT
	switch(n) {
		case SIGCHLD:
			printf("Terminaison du fils !\n");
			wait(NULL);
			break;
		case SIGINT:
			printf("Terminaison du serveur !\n");
			if (close(sock_S_DIFF) == -1) perror("close 3: ");
			if (close(sock_S) == -1) perror("close 4: ");
			exit(EXIT_SUCCESS);
			break;
		default:
			printf("Signal %d non pris en compte...\n", n);
	}
}

int main(int argc, char **argv) {
	int new_sock_S;
	int port_S_TCP = PORT_S_TCP;
	char message[NB_CHAR];
	int entiers[2], resultat;
	struct sockaddr_in sa_S_DIFF, sa_S, sa_C;
	unsigned int taille_sa = sizeof( struct sockaddr );
	int fils_pid;
	struct sigaction sa;
	
	/* installation du gestionnaire du signal SIGCHLD */
	
	sa.sa_handler = fin;
	sa.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT | SA_RESTART;
	sigaction(SIGCHLD, &sa, NULL);
	
	/* installation du gestionnaire pour SIGINT */
	sa.sa_flags = SA_RESTART;
	sigaction(SIGINT, &sa, NULL);
	
	/* PHASE 1 : Reponse au Broadcast ----------------------------------------------------*/

	/* socket Serveur Diffusion */
	sock_S_DIFF = socket(AF_INET, SOCK_DGRAM, 0);
	perror("socket 1:");
	
	/* structure adresse diffusion */
	bzero((char*) &sa_S_DIFF, taille_sa);
	sa_S_DIFF.sin_family      = AF_INET;
	sa_S_DIFF.sin_addr.s_addr = htonl(INADDR_ANY);
	sa_S_DIFF.sin_port        = htons(PORT_S_DIFF);

	/* attachement */
	bind(sock_S_DIFF, (struct sockaddr *) &sa_S_DIFF, taille_sa);
	perror("bind 1: ");
	
	/* initialisation PHASE 2 -------------------------------------------------------*/
	
	/* creation socket Serveur */
	sock_S = socket(AF_INET, SOCK_STREAM, 0);
	perror("socket 2: ");
	
	/* @IP et n° port Serveur */
	bzero((char*) &sa_S, taille_sa);
	sa_S.sin_family      = AF_INET;
	sa_S.sin_addr.s_addr = htonl(INADDR_ANY);
	sa_S.sin_port        = htons(port_S_TCP);
	
	/* attachement sock_S -> sa_S */
	bind(sock_S, (struct sockaddr *) &sa_S, taille_sa);
	perror("bind 2: ");

	/* definition longueur file d'attente */
	listen(sock_S, 2);
	perror("listen: ");
	
	/* FIN initialisation PHASE 2 ---------------------------------------------------*/
	
	while(1) {
		/* reception */
		if (recvfrom(sock_S_DIFF, message, NB_CHAR*sizeof(char), 0, (struct sockaddr*) &sa_C, &taille_sa) == -1) {
			perror("recvfrom: ");
		}
		printf("%s demande : %s", inet_ntoa(sa_C.sin_addr), message );
		/* reponse */
		if (sendto(sock_S_DIFF, &port_S_TCP , sizeof(int), 0, (struct sockaddr*) &sa_C, taille_sa) == -1) {
			perror("sendto: ");
		}
		/* PHASE 2 : Requete TCP ------------------------------------------------------*/
		/* acceptation connexion */
		new_sock_S = accept(sock_S, (struct sockaddr *) &sa_C, &taille_sa);
		if (new_sock_S == -1) {
			perror("accept: ");
		}
		/* creation processus fils */		
		switch(fils_pid = fork()) {
			case -1:
				perror("fork: "); break;
			case 0: /* c'est le fils */
				if (close(sock_S_DIFF) == -1) perror("close 1: "); 
				/* FIN PHASE 1 */
	
				if (close(sock_S) == -1) perror("close 2: ");
				
				/* service : lecture/ecriture */
				read(new_sock_S, entiers, 2*sizeof(int));
				printf("Entiers recus: %d %d\n", entiers[0],entiers[1]);
				resultat = entiers[0] + entiers[1];				
				write(new_sock_S, &resultat, sizeof(int));	
				
				/* fin service */
				if (close(new_sock_S) == -1) perror("close 3: ");
				/* FIN PHASE 2 */
				/* fin processus fils */
				exit(EXIT_SUCCESS);
			default: // le père ne fait rien
				break;
		}
	}
	
	return 0;



}


