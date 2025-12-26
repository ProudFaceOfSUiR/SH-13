/*******************************************************************************
 * FICHIER: Serveur de jeu Sherlock 13
 * DESCRIPTION: Serveur TCP pour un jeu de cartes multijoueur (4 joueurs)
 *              basé sur l'univers de Sherlock Holmes
 * AUTEUR: Projet Info Système
 * DATE: 2020/2021
 ******************************************************************************/

/*******************************************************************************
 * SECTION 1: INCLUSION DES BIBLIOTHÈQUES
 ******************************************************************************/
#include <stdio.h>          // Fonctions d'entrée/sortie standard (printf, scanf, etc.)
#include <stdlib.h>         // Fonctions utilitaires (malloc, rand, exit, etc.)
#include <string.h>         // Manipulation de chaînes de caractères (strcpy, strcmp, etc.)
#include <unistd.h>         // API POSIX (read, write, close, etc.)
#include <sys/types.h>      // Types de données pour les appels système
#include <sys/socket.h>     // Structures et fonctions pour les sockets
#include <netinet/in.h>     // Structures pour les adresses Internet
#include <netdb.h>          // Définitions pour les opérations de base de données réseau
#include <arpa/inet.h>      // Fonctions de manipulation d'adresses Internet

/*******************************************************************************
 * SECTION 2: STRUCTURES ET VARIABLES GLOBALES
 ******************************************************************************/

// Structure représentant un client connecté au serveur
struct _client
{
    char ipAddress[40];     // Adresse IP du client (ex: "192.168.1.1")
    int port;               // Port d'écoute du client
    char name[40];          // Nom du joueur
} tcpClients[4];            // Tableau de 4 clients (4 joueurs maximum)

int nbClients;              // Nombre de clients actuellement connectés

int fsmServer;              // Machine à états du serveur (0=attente joueurs, 1=partie en cours)

// Deck de 13 cartes (indices 0 à 12 correspondant aux personnages)
int deck[13]={0,1,2,3,4,5,6,7,8,9,10,11,12};

// Tableau des statistiques de chaque joueur
// tableCartes[i][j] = statistique j du joueur i
// 8 colonnes: [0-6]=différentes catégories de symboles, [7]=points totaux
int tableCartes[4][8];

// Noms des 13 cartes/personnages du jeu Sherlock 13
char *nomcartes[]=
{
    "Sebastian Moran",          // Carte 0
    "irene Adler",              // Carte 1
    "inspector Lestrade",       // Carte 2
    "inspector Gregson",        // Carte 3
    "inspector Baynes",         // Carte 4
    "inspector Bradstreet",     // Carte 5
    "inspector Hopkins",        // Carte 6
    "Sherlock Holmes",          // Carte 7
    "John Watson",              // Carte 8
    "Mycroft Holmes",           // Carte 9
    "Mrs. Hudson",              // Carte 10
    "Mary Morstan",             // Carte 11
    "James Moriarty"            // Carte 12
};

int joueurCourant;          // Indice du joueur dont c'est le tour (0 à 3)

/*******************************************************************************
 * SECTION 3: FONCTION DE GESTION D'ERREUR
 ******************************************************************************/

// Affiche un message d'erreur et termine le programme
void error(const char *msg)
{
    perror(msg);            // Affiche le message d'erreur avec détails système
    exit(1);                // Termine le programme avec code d'erreur 1
}

/*******************************************************************************
 * SECTION 4: FONCTION DE MÉLANGE DU DECK
 ******************************************************************************/

// Mélange aléatoirement le deck de cartes par la méthode de Fisher-Yates
void melangerDeck()
{
    int i;                  // Compteur de boucle
    int index1, index2;     // Indices des deux cartes à échanger
    int tmp;                // Variable temporaire pour l'échange

    // Effectue 1000 échanges aléatoires pour bien mélanger le deck
    for (i=0; i<1000; i++)
    {
        index1 = rand() % 13;       // Choix d'un premier indice aléatoire (0-12)
        index2 = rand() % 13;       // Choix d'un deuxième indice aléatoire (0-12)

        // Échange les deux cartes en utilisant une variable temporaire
        tmp = deck[index1];
        deck[index1] = deck[index2];
        deck[index2] = tmp;
    }
}

/*******************************************************************************
 * SECTION 5: FONCTION DE CRÉATION DU TABLEAU DE STATISTIQUES
 ******************************************************************************/

// Crée le tableau de statistiques basé sur les cartes distribuées
// Chaque carte possède des caractéristiques (symboles) qui s'additionnent
void createTable()
{
    // DISTRIBUTION DES CARTES:
    // Joueur 0: cartes d'indices 0, 1, 2 du deck mélangé
    // Joueur 1: cartes d'indices 3, 4, 5 du deck mélangé
    // Joueur 2: cartes d'indices 6, 7, 8 du deck mélangé
    // Joueur 3: cartes d'indices 9, 10, 11 du deck mélangé
    // Coupable: carte d'indice 12 (la carte à deviner)
    
    int i, j, c;            // Variables de boucle et carte courante

    // Initialise toutes les statistiques à 0
    for (i=0; i<4; i++)                 // Pour chaque joueur
        for (j=0; j<8; j++)             // Pour chaque catégorie de symbole
            tableCartes[i][j] = 0;      // Remise à zéro

    // Pour chaque joueur (0 à 3)
    for (i=0; i<4; i++)
    {
        // Pour chacune de ses 3 cartes
        for (j=0; j<3; j++)
        {
            c = deck[i*3+j];        // Récupère l'indice de la carte du joueur i
            
            // Selon la carte, incrémente les statistiques correspondantes
            // Chaque case représente un symbole ou caractéristique du personnage
            switch (c)
            {
                case 0: // Sebastian Moran (adversaire de Holmes)
                    tableCartes[i][7]++;    // Incrémente les points totaux
                    tableCartes[i][2]++;    // Incrémente le symbole 2
                    break;
                    
                case 1: // Irene Adler (l'Aventurière)
                    tableCartes[i][7]++;    // Incrémente les points totaux
                    tableCartes[i][1]++;    // Incrémente le symbole 1
                    tableCartes[i][5]++;    // Incrémente le symbole 5
                    break;
                    
                case 2: // Inspector Lestrade (Scotland Yard)
                    tableCartes[i][3]++;    // Incrémente le symbole 3
                    tableCartes[i][6]++;    // Incrémente le symbole 6
                    tableCartes[i][4]++;    // Incrémente le symbole 4
                    break;
                    
                case 3: // Inspector Gregson (Scotland Yard)
                    tableCartes[i][3]++;    // Incrémente le symbole 3
                    tableCartes[i][2]++;    // Incrémente le symbole 2
                    tableCartes[i][4]++;    // Incrémente le symbole 4
                    break;
                    
                case 4: // Inspector Baynes (Scotland Yard)
                    tableCartes[i][3]++;    // Incrémente le symbole 3
                    tableCartes[i][1]++;    // Incrémente le symbole 1
                    break;
                    
                case 5: // Inspector Bradstreet (Scotland Yard)
                    tableCartes[i][3]++;    // Incrémente le symbole 3
                    tableCartes[i][2]++;    // Incrémente le symbole 2
                    break;
                    
                case 6: // Inspector Hopkins (Scotland Yard)
                    tableCartes[i][3]++;    // Incrémente le symbole 3
                    tableCartes[i][0]++;    // Incrémente le symbole 0
                    tableCartes[i][6]++;    // Incrémente le symbole 6
                    break;
                    
                case 7: // Sherlock Holmes (le Détective)
                    tableCartes[i][0]++;    // Incrémente le symbole 0
                    tableCartes[i][1]++;    // Incrémente le symbole 1
                    tableCartes[i][2]++;    // Incrémente le symbole 2
                    break;
                    
                case 8: // John Watson (le Compagnon)
                    tableCartes[i][0]++;    // Incrémente le symbole 0
                    tableCartes[i][6]++;    // Incrémente le symbole 6
                    tableCartes[i][2]++;    // Incrémente le symbole 2
                    break;
                    
                case 9: // Mycroft Holmes (le Frère)
                    tableCartes[i][0]++;    // Incrémente le symbole 0
                    tableCartes[i][1]++;    // Incrémente le symbole 1
                    tableCartes[i][4]++;    // Incrémente le symbole 4
                    break;
                    
                case 10: // Mrs. Hudson (la Logeuse)
                    tableCartes[i][0]++;    // Incrémente le symbole 0
                    tableCartes[i][5]++;    // Incrémente le symbole 5
                    break;
                    
                case 11: // Mary Morstan (l'Épouse de Watson)
                    tableCartes[i][4]++;    // Incrémente le symbole 4
                    tableCartes[i][5]++;    // Incrémente le symbole 5
                    break;
                    
                case 12: // James Moriarty (l'Ennemi)
                    tableCartes[i][7]++;    // Incrémente les points totaux
                    tableCartes[i][1]++;    // Incrémente le symbole 1
                    break;
            }
        }
    }
}

/*******************************************************************************
 * SECTION 6: FONCTIONS D'AFFICHAGE (DEBUG)
 ******************************************************************************/

// Affiche le deck et le tableau de statistiques dans le terminal du serveur
// Utilisé pour le débogage et le suivi de la partie
void printDeck()
{
    int i, j;               // Compteurs de boucle

    // Affiche toutes les cartes du deck avec leurs noms
    printf("=== DECK DE CARTES ===\n");
    for (i=0; i<13; i++)
        printf("%d %s\n", deck[i], nomcartes[deck[i]]);

    // Affiche le tableau de statistiques de tous les joueurs
    printf("\n=== TABLEAU DES CARACTÉRISTIQUES ===\n");
    for (i=0; i<4; i++)
    {
        printf("Joueur %d: ", i);
        for (j=0; j<8; j++)
            printf("%2.2d ", tableCartes[i][j]);    // Format: 2 chiffres avec 0 initial si nécessaire
        puts("");                                    // Retour à la ligne
    }
    printf("\n");
}

// Affiche la liste des clients connectés (pour debug)
void printClients()
{
    int i;                  // Compteur de boucle

    printf("=== CLIENTS CONNECTÉS ===\n");
    // Pour chaque client connecté, affiche ses informations
    for (i=0; i<nbClients; i++)
        printf("%d: %s %5.5d %s\n", i,              // Numéro du client
               tcpClients[i].ipAddress,              // Adresse IP
               tcpClients[i].port,                   // Port (format 5 chiffres)
               tcpClients[i].name);                  // Nom du joueur
    printf("\n");
}

/*******************************************************************************
 * SECTION 7: FONCTION DE RECHERCHE DE CLIENT
 ******************************************************************************/

// Recherche un client par son nom dans le tableau tcpClients
// Retourne l'indice du client ou -1 si non trouvé
int findClientByName(char *name)
{
    int i;                  // Compteur de boucle

    // Parcourt tous les clients connectés
    for (i=0; i<nbClients; i++)
        if (strcmp(tcpClients[i].name, name) == 0)  // Compare les noms (sensible à la casse)
            return i;                                 // Client trouvé, retourne son indice
    
    return -1;                                       // Client non trouvé
}

/*******************************************************************************
 * SECTION 8: FONCTIONS D'ENVOI DE MESSAGES
 ******************************************************************************/

// Envoie un message à un client spécifique via TCP
// Crée une nouvelle connexion temporaire pour chaque message
void sendMessageToClient(char *clientip, int clientport, char *mess)
{
    int sockfd, portno, n;              // Descripteur de socket, numéro de port, résultat
    struct sockaddr_in serv_addr;       // Structure d'adresse du serveur
    struct hostent *server;              // Informations sur l'hôte
    char buffer[256];                    // Buffer pour le message à envoyer

    // Crée un nouveau socket TCP pour la connexion
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Résout le nom d'hôte (ou IP) en adresse IP
    server = gethostbyname(clientip);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    
    // Initialise la structure d'adresse
    bzero((char *) &serv_addr, sizeof(serv_addr));      // Remise à zéro de la structure
    serv_addr.sin_family = AF_INET;                      // Famille d'adresses IPv4
    
    // Copie l'adresse IP du serveur dans la structure
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    
    serv_addr.sin_port = htons(clientport);              // Convertit le port en format réseau (big-endian)
    
    // Établit la connexion avec le client
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("ERROR connecting\n");
        exit(1);
    }

    // Prépare le message avec un retour à la ligne
    sprintf(buffer, "%s\n", mess);
    
    // Envoie le message au client
    n = write(sockfd, buffer, strlen(buffer));

    // Ferme la connexion (le message est envoyé)
    close(sockfd);
}

// Envoie un message à tous les clients connectés (broadcast)
// Utilisé pour synchroniser l'état du jeu entre tous les joueurs
void broadcastMessage(char *mess)
{
    int i;                  // Compteur de boucle

    // Envoie le message à chaque client de la liste
    for (i=0; i<nbClients; i++)
        sendMessageToClient(tcpClients[i].ipAddress,
                          tcpClients[i].port,
                          mess);
}

/*******************************************************************************
 * SECTION 9: FONCTION PRINCIPALE
 ******************************************************************************/

int main(int argc, char *argv[])
{
    /***************************************************************************
     * SOUS-SECTION 9.1: DÉCLARATION DES VARIABLES
     ***************************************************************************/
    
    // Variables pour le socket serveur
    int sockfd, newsockfd, portno;              // Descripteurs de socket et numéro de port
    socklen_t clilen;                            // Taille de la structure d'adresse client
    char buffer[256];                            // Buffer pour la réception de messages
    struct sockaddr_in serv_addr, cli_addr;     // Adresses serveur et client
    int n;                                       // Résultat des opérations de lecture
    int i, j;                                    // Compteurs de boucle

    // Variables pour le traitement des messages de connexion
    char com;                                    // Commande reçue (première lettre)
    char clientIpAddress[256];                   // Adresse IP du client
    char clientName[256];                        // Nom du joueur
    int clientPort;                              // Port du client
    int id;                                      // ID du joueur
    char reply[256];                             // Message de réponse à envoyer
    
    // Variables pour la phase de jeu
    int idJoueur;                                // ID du joueur qui fait l'action
    int colonne;                                 // Colonne du tableau (symbole/caractéristique)
    int ligne;                                   // Ligne du tableau (personnage ou joueur)
    int coupable;                                // La carte coupable (indice 12 du deck)
    int nombre;                                  // Nombre de caractéristiques (pour commande S)

    /***************************************************************************
     * SOUS-SECTION 9.2: VÉRIFICATION DES ARGUMENTS
     ***************************************************************************/
    
    // Vérifie qu'un numéro de port a été fourni en argument de ligne de commande
    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    /***************************************************************************
     * SOUS-SECTION 9.3: CRÉATION ET CONFIGURATION DU SOCKET SERVEUR
     ***************************************************************************/
    
    // Crée un socket TCP (SOCK_STREAM)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    // Initialise la structure d'adresse du serveur
    bzero((char *) &serv_addr, sizeof(serv_addr));      // Remise à zéro de la structure
    portno = atoi(argv[1]);                              // Convertit l'argument en entier (numéro de port)
    serv_addr.sin_family = AF_INET;                      // Famille IPv4
    serv_addr.sin_addr.s_addr = INADDR_ANY;             // Accepte les connexions de n'importe quelle interface réseau
    serv_addr.sin_port = htons(portno);                  // Convertit le port en format réseau (big-endian)
    
    // Lie le socket à l'adresse et au port spécifiés
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR on binding");
    
    // Met le socket en mode écoute (max 5 connexions en attente dans la queue)
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);                           // Taille de la structure d'adresse client

    /***************************************************************************
     * SOUS-SECTION 9.4: INITIALISATION DU JEU
     ***************************************************************************/
    
    printf("=== INITIALISATION DU JEU SHERLOCK 13 ===\n\n");
    
    // Affiche le deck initial (ordre original)
    printDeck();
    
    // Mélange le deck de manière aléatoire
    melangerDeck();
    
    // Crée le tableau de statistiques basé sur les cartes distribuées
    createTable();
    
    // Affiche le deck mélangé et les statistiques calculées
    printDeck();
    
    // Initialise le joueur courant à 0 (le premier à se connecter commence)
    joueurCourant = 0;

    // Initialise le tableau des clients avec des valeurs par défaut
    for (i=0; i<4; i++)
    {
        strcpy(tcpClients[i].ipAddress, "localhost");   // IP par défaut
        tcpClients[i].port = -1;                         // Port invalide (-1 indique non connecté)
        strcpy(tcpClients[i].name, "-");                // Nom vide
    }
    
    printf("=== SERVEUR EN ATTENTE DE CONNEXIONS ===\n");
    printf("Port d'écoute: %d\n\n", portno);

    /***************************************************************************
     * SOUS-SECTION 9.5: BOUCLE PRINCIPALE DU SERVEUR
     ***************************************************************************/
    
    while (1)       // Boucle infinie - le serveur ne s'arrête jamais
    {    
        // Accepte une nouvelle connexion entrante
        // Bloque jusqu'à ce qu'un client se connecte
        newsockfd = accept(sockfd, 
                          (struct sockaddr *) &cli_addr, 
                          &clilen);
        if (newsockfd < 0) 
            error("ERROR on accept");

        // Lit le message du client
        bzero(buffer, 256);                             // Remise à zéro du buffer
        n = read(newsockfd, buffer, 255);               // Lecture (max 255 caractères + '\0')
        if (n < 0) 
            error("ERROR reading from socket");

        // Affiche les informations de la connexion pour le débogage
        printf("Received packet from %s:%d\nData: [%s]\n\n",
               inet_ntoa(cli_addr.sin_addr),            // Convertit l'IP en chaîne de caractères
               ntohs(cli_addr.sin_port),                // Convertit le port en format hôte (little-endian)
               buffer);

        /***********************************************************************
         * SOUS-SECTION 9.6: MACHINE À ÉTATS - PHASE D'ATTENTE DES JOUEURS
         * État fsmServer == 0: Le serveur attend que 4 joueurs se connectent
         ***********************************************************************/
        
        if (fsmServer == 0)     // État 0: attente des connexions
        {
            switch (buffer[0])  // Analyse la première lettre de la commande
            {
                case 'C':       // Commande de Connexion
                    printf(">>> TRAITEMENT CONNEXION <<<\n");
                    
                    // Parse le message: "C <IP> <port> <nom>"
                    sscanf(buffer, "%c %s %d %s", &com, clientIpAddress, &clientPort, clientName);
                    printf("COM=%c ipAddress=%s port=%d name=%s\n", com, clientIpAddress, clientPort, clientName);

                    // Enregistre le nouveau client dans le tableau tcpClients
                    strcpy(tcpClients[nbClients].ipAddress, clientIpAddress);
                    tcpClients[nbClients].port = clientPort;
                    strcpy(tcpClients[nbClients].name, clientName);
                    nbClients++;                        // Incrémente le compteur de clients

                    // Affiche la liste des clients connectés
                    printClients();

                    // Recherche l'ID du joueur qui vient de se connecter
                    id = findClientByName(clientName);
                    printf("id=%d\n", id);

                    // Envoie un message personnel "I" au joueur pour lui communiquer son ID
                    sprintf(reply, "I %d", id);
                    sendMessageToClient(tcpClients[id].ipAddress,
                                      tcpClients[id].port,
                                      reply);
                    printf("Envoi de l'ID %d au joueur %s\n", id, clientName);

                    // Envoie un broadcast "L" avec la liste de tous les joueurs connectés
                    sprintf(reply, "L %s %s %s %s", 
                           tcpClients[0].name, 
                           tcpClients[1].name, 
                           tcpClients[2].name, 
                           tcpClients[3].name);
                    broadcastMessage(reply);
                    printf("Broadcast de la liste des joueurs\n");

                    // Si 4 joueurs sont connectés, lance la partie
                    if (nbClients == 4)
                    {
                        printf("\n=== DÉBUT DE LA PARTIE ===\n");
                        printf("4 joueurs connectés, distribution des cartes...\n\n");
                        
                        // ===== ENVOI DES CARTES AU JOUEUR 0 =====
                        // Format: "D <carte1> <carte2> <carte3> <stat0> ... <stat7>"
                        sprintf(reply, "D %s %s %s %d %d %d %d %d %d %d %d",
                               nomcartes[deck[0]], nomcartes[deck[1]], nomcartes[deck[2]],
                               tableCartes[0][0], tableCartes[0][1], tableCartes[0][2],
                               tableCartes[0][3], tableCartes[0][4], tableCartes[0][5],
                               tableCartes[0][6], tableCartes[0][7]);
                        sendMessageToClient(tcpClients[0].ipAddress, tcpClients[0].port, reply);
                        printf("Cartes envoyées au joueur 0: %s, %s, %s\n", 
                               nomcartes[deck[0]], nomcartes[deck[1]], nomcartes[deck[2]]);

                        // ===== ENVOI DES CARTES AU JOUEUR 1 =====
                        sprintf(reply, "D %s %s %s %d %d %d %d %d %d %d %d",
                               nomcartes[deck[3]], nomcartes[deck[4]], nomcartes[deck[5]],
                               tableCartes[1][0], tableCartes[1][1], tableCartes[1][2],
                               tableCartes[1][3], tableCartes[1][4], tableCartes[1][5],
                               tableCartes[1][6], tableCartes[1][7]);
                        sendMessageToClient(tcpClients[1].ipAddress, tcpClients[1].port, reply);
                        printf("Cartes envoyées au joueur 1: %s, %s, %s\n", 
                               nomcartes[deck[3]], nomcartes[deck[4]], nomcartes[deck[5]]);

                        // ===== ENVOI DES CARTES AU JOUEUR 2 =====
                        sprintf(reply, "D %s %s %s %d %d %d %d %d %d %d %d",
                               nomcartes[deck[6]], nomcartes[deck[7]], nomcartes[deck[8]],
                               tableCartes[2][0], tableCartes[2][1], tableCartes[2][2],
                               tableCartes[2][3], tableCartes[2][4], tableCartes[2][5],
                               tableCartes[2][6], tableCartes[2][7]);
                        sendMessageToClient(tcpClients[2].ipAddress, tcpClients[2].port, reply);
                        printf("Cartes envoyées au joueur 2: %s, %s, %s\n", 
                               nomcartes[deck[6]], nomcartes[deck[7]], nomcartes[deck[8]]);

                        // ===== ENVOI DES CARTES AU JOUEUR 3 =====
                        sprintf(reply, "D %s %s %s %d %d %d %d %d %d %d %d",
                               nomcartes[deck[9]], nomcartes[deck[10]], nomcartes[deck[11]],
                               tableCartes[3][0], tableCartes[3][1], tableCartes[3][2],
                               tableCartes[3][3], tableCartes[3][4], tableCartes[3][5],
                               tableCartes[3][6], tableCartes[3][7]);
                        sendMessageToClient(tcpClients[3].ipAddress, tcpClients[3].port, reply);
                        printf("Cartes envoyées au joueur 3: %s, %s, %s\n", 
                               nomcartes[deck[9]], nomcartes[deck[10]], nomcartes[deck[11]]);

                        // Affiche le personnage coupable (pour le debug du serveur)
                        printf("\n>>> PERSONNAGE COUPABLE: %s <<<\n\n", nomcartes[deck[12]]);

                        // Envoie un broadcast "T" pour indiquer le joueur courant (qui commence)
                        sprintf(reply, "T %d", joueurCourant);
                        broadcastMessage(reply);
                        printf("C'est au tour du joueur %d (%s)\n\n", joueurCourant, tcpClients[joueurCourant].name);

                        // Passe à l'état 1 (partie en cours)
                        fsmServer = 1;
                    }
                    break;
            }
        }
        
        /***********************************************************************
         * SOUS-SECTION 9.7: MACHINE À ÉTATS - PHASE DE JEU
         * État fsmServer == 1: La partie est en cours, traitement des actions
         ***********************************************************************/
        
        else if (fsmServer == 1)
{
    switch (buffer[0])
    {
        case 'G':   // Proposition du coupable
            sscanf(buffer, "%c %d %d", &com, &idJoueur, &ligne);

            if (idJoueur != joueurCourant)
                break;

            if (ligne == deck[12])
            {
                sprintf(reply, "W %d %s", idJoueur, nomcartes[ligne]);
                broadcastMessage(reply);
                printf(">>> VICTOIRE DU JOUEUR %d <<<\n", idJoueur);
                exit(0);
            }
            else
            {
                sprintf(reply, "F %d %s", idJoueur, nomcartes[ligne]);
                broadcastMessage(reply);
                printf("Mauvaise accusation du joueur %d\n", idJoueur);
            }

            joueurCourant = (joueurCourant + 1) % 4;
            sprintf(reply, "T %d", joueurCourant);
            broadcastMessage(reply);
            break;

        case 'O':   // Question oui/non
            sscanf(buffer, "%c %d %d %d", &com, &idJoueur, &ligne, &colonne);

            if (idJoueur != joueurCourant)
                break;

            int coupable = deck[12];
            int reponse = 0;

            // On recrée les caractéristiques du coupable
            for (i = 0; i < 4; i++)
                if (tableCartes[i][colonne] > 0 &&
                    (deck[i*3] == coupable ||
                     deck[i*3+1] == coupable ||
                     deck[i*3+2] == coupable))
                    reponse = 1;

            sprintf(reply, "R %d %d", colonne, reponse);
            broadcastMessage(reply);

            joueurCourant = (joueurCourant + 1) % 4;
            sprintf(reply, "T %d", joueurCourant);
            broadcastMessage(reply);
            break;

        case 'S':   // Question statistique
            sscanf(buffer, "%c %d %d", &com, &idJoueur, &nombre);

            if (idJoueur != joueurCourant)
                break;

            int count = 0;
            for (i = 0; i < 13; i++)
            {
                // logique simplifiée
                if (deck[i] == nombre)
                    count++;
            }

            sprintf(reply, "S %d %d", nombre, count);
            sendMessageToClient(tcpClients[idJoueur].ipAddress,
                                tcpClients[idJoueur].port,
                                reply);

            joueurCourant = (joueurCourant + 1) % 4;
            sprintf(reply, "T %d", joueurCourant);
            broadcastMessage(reply);
            break;
    }
}
