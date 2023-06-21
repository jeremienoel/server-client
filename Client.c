/* Sockets - Client
 * Avec recherche du serveur
 * Usage : client <n1> <n2>
 */

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <ifaddrs.h>

#define PORT_S_DIFF 8000
#define NB_CHAR 128
 
void find_ip(char *resultat) {
	struct ifaddrs *addr, *intf;
	char hostname[NI_MAXHOST];
	int family;
	if (getifaddrs(&intf) == -1) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}
	for (addr = intf; addr != NULL; addr = addr->ifa_next) {
		family = addr->ifa_addr->sa_family;
		//AF_INET est la famille d'adresses pour IPv4
		if (family == AF_INET) {
			//getnameinfo permet la résolution de noms.
			getnameinfo(addr->ifa_addr, sizeof(struct sockaddr_in), hostname, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			if (strcmp(addr->ifa_name, "lo") == 0) continue;
			else {
				strcpy(resultat, hostname);
				break;
			}
		}
	}
	free(intf);
}

void broad_IP(char *resultat) {
	struct ifaddrs *addr, *intf;
	char hostname[NI_MAXHOST];
	int family;
	if (getifaddrs(&intf) == -1) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}
	for (addr = intf; addr != NULL; addr = addr->ifa_next) {
		family = addr->ifa_addr->sa_family;
		//AF_INET est la famille d'adresses pour IPv4
		if (family == AF_INET) {
			//getnameinfo permet la résolution de noms.
			getnameinfo(addr->ifa_addr, sizeof(struct sockaddr_in), hostname, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			if (strcmp(addr->ifa_name, "lo") == 0) continue;
			else {
				// les champs ifa_addr et ifa_netmask contiennent ce qu'il faut !
				struct in_addr ip_addr;
				struct sockaddr_in *tmp = (struct sockaddr_in *) addr->ifa_addr;
				uint32_t address = tmp->sin_addr.s_addr;
				tmp = (struct sockaddr_in *) addr->ifa_netmask;
				uint32_t mask = tmp->sin_addr.s_addr;
				uint32_t broadcast  = address | (~mask);
				ip_addr.s_addr = broadcast;
				strcpy(resultat, inet_ntoa(ip_addr));
				break;
			}
		}
	}
	free(intf);
}

int main(int argc, char **argv) {	
	int sock_C_DIFF, sock_C;
	struct sockaddr_in sa_S, sa_S_DIFF;
	char message[NB_CHAR];
	int entiers[2], resultat;
	int port_TCP;
	unsigned int taille_sa = sizeof( struct sockaddr );
	const int oui = 1; /* validation d'option */
	
	if (argc != 3) {
		printf("Usage : ./%s entier1 entier2\n", argv[0]);
		exit(0);
	}
	
	entiers[0] = atoi(argv[1]);
	entiers[1] = atoi(argv[2]);
			
	/* PHASE 1 : Broadcast pour obtenir port_TCP (pour connexion phase 2)  ----------------*/
	char mon_IP[16];
	find_ip(mon_IP);
	printf("Mon adresse IP : %s\n", mon_IP);
	broad_IP(mon_IP);
	printf("Addresse Broadcast : %s\n", mon_IP);

	/* Socket Diffusion Client */
	sock_C_DIFF = socket(AF_INET, SOCK_DGRAM, 0);
	perror("socket 1: ");

	setsockopt(sock_C_DIFF, SOL_SOCKET, SO_BROADCAST, &oui, sizeof(const int));
	perror("setsockopt: ");
	
	/* @IP_DIFF et n° port Serveur */
	bzero((char*) &sa_S_DIFF, taille_sa);
	sa_S_DIFF.sin_family      = AF_INET;
	sa_S_DIFF.sin_addr.s_addr = inet_addr(mon_IP);
	sa_S_DIFF.sin_port        = htons( PORT_S_DIFF  );

	/* requete : diffusion */
	sprintf(message, "%s",  "Port pour la phase 2 ? \n");
	
	sendto(sock_C_DIFF, message, NB_CHAR*sizeof(char), 0, 
		(struct sockaddr*) &sa_S_DIFF, taille_sa);
	perror("sendto: ");
	
	/* Reponse du serveur */
	recvfrom(sock_C_DIFF, &port_TCP, sizeof(int), 0, (struct sockaddr*) &sa_S, &taille_sa);
	perror("recvfrom: ");
	
	printf("%d \n", port_TCP);
	
	/* fin PHASE 1 */
	close(sock_C_DIFF);
	perror("close 1: ");

	/* PHASE 2 : Requete TCP ----------------------------------------------------------------*/
	
	/* Socket Client */
	sock_C = socket(PF_INET, SOCK_STREAM, 0);
	perror("socket 2: ");
	 
	/* @IP et n° port Serveur TCP */
	sa_S.sin_port = htons( port_TCP );

	/* Connexion au serveur */
	connect(sock_C, (struct sockaddr *) &sa_S, taille_sa);
	perror("connect: ");
	
	/* service : lecture/ecriture */
	//entiers[0] = 45;
	//entiers[1] = 18;
	
	write(sock_C, entiers, 2*sizeof(int));
	read(sock_C, &resultat, sizeof(int));
	
	printf("resultat : %d + %d = %d \n", entiers[0], entiers[1], resultat);
	
	/* point d'arret (cf. pour faire un "netstat" et voir l'etat connecte) */
	printf("(Pour avoir un point d'arret une fois connecte) \n");
	printf("Saisir un caractere : \n"); // une instruction bloquante ...
	getchar();
	
	/* fin PHASE 2 et fin programme */
	close(sock_C);
	perror("close 2: ");
	
	exit(EXIT_SUCCESS);
	
}


