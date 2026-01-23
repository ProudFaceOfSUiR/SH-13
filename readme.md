# Compilation du client

```bash
gcc -o sh13 sh13.c -lSDL2 -lSDL2_image -lSDL2_ttf -lpthread
```

# Lancement

```bash
./server <port>
# ex:   ./server 5187000
```

# Client

```bash
./client <IP_serveur> <port_serveur> <IP_client> <port_client> <nom_joueur>
# ex:   ./sh13 127.0.0.1 5187000 127.0.0.1 5001 joueur1
```

```bash
ou lancement de 4 clients en même temps :
# ex: ./launch.sh
```
