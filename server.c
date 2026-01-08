
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
    char ipAddress[40];     // Adresse IP du client 
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
int joueursPerdu[4];

int joueurPerdu(int id)
{
    return joueursPerdu[id];
}

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
                case 0: // Sebastian Moran 
                    tableCartes[i][7]++;    // Incrémente les points totaux (crâne)
                    tableCartes[i][2]++;    // Incrémente le symbole 2 (poing)
                    break;
                    
                case 1: // Irene Adler
                    tableCartes[i][7]++;    // Incrémente les points totaux (crâne)
                    tableCartes[i][1]++;    // Incrémente le symbole 1 (ampoule)
                    tableCartes[i][5]++;    // Incrémente le symbole 5 (collier)
                    break;
                    
                case 2: // Inspector Lestrade
                    tableCartes[i][3]++;    // Incrémente le symbole 3 (couronne)
                    tableCartes[i][6]++;    // Incrémente le symbole 6 (oeil)
                    tableCartes[i][4]++;    // Incrémente le symbole 4 (carnet)
                    break;
                    
                case 3: // Inspector Gregson 
                    tableCartes[i][3]++;    // Incrémente le symbole 3 (couronne)
                    tableCartes[i][2]++;    // Incrémente le symbole 2 (poing)
                    tableCartes[i][4]++;    // Incrémente le symbole 4 (carnet)
                    break;
                    
                case 4: // Inspector Baynes 
                    tableCartes[i][3]++;    // Incrémente le symbole 3 (couronne)
                    tableCartes[i][1]++;    // Incrémente le symbole 1 (ampoule)
                    break;
                    
                case 5: // Inspector Bradstreet 
                    tableCartes[i][3]++;    // Incrémente le symbole 3 (couronne)
                    tableCartes[i][2]++;    // Incrémente le symbole 2 (poing)
                    break;
                    
                case 6: // Inspector Hopkins 
                    tableCartes[i][3]++;    // Incrémente le symbole 3 (couronne)
                    tableCartes[i][0]++;    // Incrémente le symbole 0 (pipe)
                    tableCartes[i][6]++;    // Incrémente le symbole 6 (oeil)
                    break;
                    
                case 7: // Sherlock Holmes 
                    tableCartes[i][0]++;    // Incrémente le symbole 0 (pipe)
                    tableCartes[i][1]++;    // Incrémente le symbole 1 (ampoule)
                    tableCartes[i][2]++;    // Incrémente le symbole 2 (poing)
                    break;
                    
                case 8: // John Watson 
                    tableCartes[i][0]++;    // Incrémente le symbole 0 (pipe)
                    tableCartes[i][6]++;    // Incrémente le symbole 6 (oeil)
                    tableCartes[i][2]++;    // Incrémente le symbole 2 (poing)
                    break;
                    
                case 9: // Mycroft Holmes 
                    tableCartes[i][0]++;    // Incrémente le symbole 0 (pipe)
                    tableCartes[i][1]++;    // Incrémente le symbole 1 (ampoule)
                    tableCartes[i][4]++;    // Incrémente le symbole 4 (carnet)
                    break;
                    
                case 10: // Mrs. Hudson 
                    tableCartes[i][0]++;    // Incrémente le symbole 0 (pipe)
                    tableCartes[i][5]++;    // Incrémente le symbole 5 (collier)
                    break;
                    
                case 11: // Mary Morstan 
                    tableCartes[i][4]++;    // Incrémente le symbole 4 (carnet)
                    tableCartes[i][5]++;    // Incrémente le symbole 5 (collier)
                    break;
                    
                case 12: // James Moriarty 
                    tableCartes[i][7]++;    // Incrémente les points totaux (crâne)
                    tableCartes[i][1]++;    // Incrémente le symbole 1 (ampoule)
                    break;
            }
        }
    }
}

/*******************************************************************************
 * SECTION 6: FONCTIONS D'AFFICHAGE (POUR LE DEBUG)
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
               tcpClients[i].port,                   // Port 
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
        if (strcmp(tcpClients[i].name, name) == 0)  // Compare les noms 
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
    
    serv_addr.sin_port = htons(clientport);              // Convertit le port en format réseau 
    
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
    int joueur;                                  // Numéro du joueur cible (pour commande S)
    int objet;                                   // Numéro de l'objet/symbole demandé
    int coupable;                                // Numéro de la carte accusée (pour commande G)

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
    serv_addr.sin_port = htons(portno);                  // Convertit le port en format réseau 
    
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
    
    // Initialise le nombre de clients à 0
    nbClients = 0;
    
    // Initialise la machine à états à 0 (attente des joueurs)
    fsmServer = 0;

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
               ntohs(cli_addr.sin_port),                // Convertit le port en format hôte 
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

                    // ===== MESSAGE 'I' : ENVOI DE L'ID AU JOUEUR =====
                    // Format: "I <id>"
                    // Envoie un message personnel au joueur pour lui communiquer son ID unique
                    sprintf(reply, "I %d", id);
                    sendMessageToClient(tcpClients[id].ipAddress,
                                      tcpClients[id].port,
                                      reply);
                    printf("Envoi de l'ID %d au joueur %s\n", id, clientName);

                    // ===== MESSAGE 'L' : BROADCAST DE LA LISTE DES JOUEURS =====
                    // Format: "L <nom1> <nom2> <nom3> <nom4>"
                    // Envoie à tous les joueurs la liste complète des noms (même ceux pas encore connectés)
                    sprintf(reply, "L %s %s %s %s", 
                           tcpClients[0].name, 
                           tcpClients[1].name, 
                           tcpClients[2].name, 
                           tcpClients[3].name);
                    broadcastMessage(reply);
                    printf("Broadcast de la liste des joueurs: %s\n", reply);

                    // Si 4 joueurs sont connectés, lance la partie
                    if (nbClients == 4)
                    {
                        printf("\n=== DÉBUT DE LA PARTIE ===\n");
                        printf("4 joueurs connectés, distribution des cartes...\n\n");
                        
                        // ===== MESSAGE 'D' : DISTRIBUTION DES CARTES =====
                        // Format: "D <carte1> <carte2> <carte3>"
                        // Envoie à chaque joueur ses 3 cartes (indices du deck)
                        // SYNCHRONISATION CLIENT: Le client attend le format "D %d %d %d"
                        
                        // Distribution au joueur 0 (cartes 0, 1, 2 du deck mélangé)
                        sprintf(reply, "D %d %d %d", deck[0], deck[1], deck[2]);
                        sendMessageToClient(tcpClients[0].ipAddress, tcpClients[0].port, reply);
                        printf("Joueur 0 (%s) reçoit: %s => %s, %s, %s\n", 
                               tcpClients[0].name, reply,
                               nomcartes[deck[0]], nomcartes[deck[1]], nomcartes[deck[2]]);

                        // Distribution au joueur 1 (cartes 3, 4, 5 du deck mélangé)
                        sprintf(reply, "D %d %d %d", deck[3], deck[4], deck[5]);
                        sendMessageToClient(tcpClients[1].ipAddress, tcpClients[1].port, reply);
                        printf("Joueur 1 (%s) reçoit: %s => %s, %s, %s\n", 
                               tcpClients[1].name, reply,
                               nomcartes[deck[3]], nomcartes[deck[4]], nomcartes[deck[5]]);

                        // Distribution au joueur 2 (cartes 6, 7, 8 du deck mélangé)
                        sprintf(reply, "D %d %d %d", deck[6], deck[7], deck[8]);
                        sendMessageToClient(tcpClients[2].ipAddress, tcpClients[2].port, reply);
                        printf("Joueur 2 (%s) reçoit: %s => %s, %s, %s\n", 
                               tcpClients[2].name, reply,
                               nomcartes[deck[6]], nomcartes[deck[7]], nomcartes[deck[8]]);

                        // Distribution au joueur 3 (cartes 9, 10, 11 du deck mélangé)
                        sprintf(reply, "D %d %d %d", deck[9], deck[10], deck[11]);
                        sendMessageToClient(tcpClients[3].ipAddress, tcpClients[3].port, reply);
                        printf("Joueur 3 (%s) reçoit: %s => %s, %s, %s\n", 
                               tcpClients[3].name, reply,
                               nomcartes[deck[9]], nomcartes[deck[10]], nomcartes[deck[11]]);

                        // Affiche le personnage coupable (carte 12) pour le debug du serveur
                        printf("\n>>> PERSONNAGE COUPABLE: %s (indice %d) <<<\n\n", 
                               nomcartes[deck[12]], deck[12]);

                        // ===== MESSAGE 'M' : INDICATION DU JOUEUR COURANT =====
                        // Format: "M <idJoueur>"
                        // SYNCHRONISATION CLIENT: Le client attend 'M' (pas 'T' comme dans l'ancienne version)
                        // Ce message active le bouton "GO" pour le joueur dont c'est le tour
                        sprintf(reply, "M %d", joueurCourant);
                        broadcastMessage(reply);
                        printf("C'est au tour du joueur %d (%s)\n\n", 
                               joueurCourant, tcpClients[joueurCourant].name);

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
        /***************************************************************
         * COMMANDE 'G' : ACCUSATION DU COUPABLE
         * Format: "G <idJoueur> <numCarte>"
         ***************************************************************/
        case 'G':
            sscanf(buffer, "%c %d %d", &com, &idJoueur, &coupable);

            // Ignore si ce n'est pas le tour du joueur
            if (idJoueur != joueurCourant)
                break;

            printf(">>> ACCUSATION: Joueur %d (%s) accuse %s <<<\n",
                   idJoueur,
                   tcpClients[idJoueur].name,
                   nomcartes[coupable]);

            // Vérifie si l'accusation est correcte
            if (coupable == deck[12])
            {
                // Victoire
                sprintf(reply, "W %d %d", idJoueur, coupable);
                broadcastMessage(reply);
                printf(">>> VICTOIRE DU JOUEUR %d <<<\n", idJoueur);
                exit(0);
            }
            else
            {
                // Mauvaise accusation
                joueursPerdu[idJoueur] = 1;
                for (j = 0; j < 4; j++){
                    if (j != idJoueur){
                        sprintf(reply, "F %d %d", idJoueur, coupable);
                        broadcastMessage(reply);
                        printf("Mauvaise accusation du joueur %d\n", idJoueur);
                    }
                }
            }

            // Passe au joueur suivant
            joueurCourant = (joueurCourant + 1) % 4;
            sprintf(reply, "M %d", joueurCourant);
            broadcastMessage(reply);
            break;

        /***************************************************************
         * COMMANDE 'O' : QUESTION OUI / NON
         * Format: "O <idJoueur> <joueurCible> <objet>"
         ***************************************************************/
        case 'O':
            sscanf(buffer, "%c %d %d", &com, &idJoueur, &objet);

            if (idJoueur != joueurCourant)
                break;

            printf(">>> QUESTION O/N: Joueur %d demande symbole %d<<<\n",
                   idJoueur, objet);

            // Réponse = 1 si le joueur ciblé possède le symbole
            for (j = 0; j < 4; j++){
                if (tableCartes[j][objet] > 0)
                    sprintf(reply, "R %d %d %d", objet, j, 1);
                else
                    sprintf(reply, "R %d %d %d", objet, j, 0);
                broadcastMessage(reply);
            }

            broadcastMessage(reply);

            // Joueur suivant
            joueurCourant = (joueurCourant + 1) % 4;
            sprintf(reply, "M %d", joueurCourant);
            broadcastMessage(reply);
            break;

        /***************************************************************
         * COMMANDE 'S' : QUESTION STATISTIQUE
         * Format: "S <idJoueur> <joueur> <objet>"
         ***************************************************************/
        case 'S':
            sscanf(buffer, "%c %d %d %d", &com, &idJoueur, &joueur, &objet);

            if (idJoueur != joueurCourant)
                break;

            printf(">>> QUESTION STAT: Joueur %d demande statistique %d au %d <<<\n",
                   idJoueur, objet, joueur);

            // Réponse uniquement au joueur demandeur
            sprintf(reply, "S %d %d", objet, tableCartes[joueur][objet]);
            sendMessageToClient(tcpClients[idJoueur].ipAddress,
                                tcpClients[idJoueur].port,
                                reply);

        }
        // Joueur suivant
        joueurCourant = (joueurCourant + 1) % 4;
        if (joueurPerdu(joueurCourant))
            joueurCourant = (joueurCourant + 1) % 4;
        sprintf(reply, "M %d", joueurCourant);
        broadcastMessage(reply);
        /* break; */
}
}
}
