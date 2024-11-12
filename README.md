
# The mind

## Developper le serveur et l'acceuil des joueurs : (+- Fait)

- Mise en place des sockets pour permettre la connexion des joueurs au serveur. Le serveur peut accepter plusieurs connexions et stocker les informations de chaque joueur (e.g., client_fd, nom/pseudo).

- Validation des connexions : Vérifiez que chaque connexion est enregistrée et que le serveur peut identifier chaque joueur individuellement.

- Gérer les connexion si le serveur est plein

- Gérer la déconnexion d'un joueur dans le lobby

Le serveur ouvre un socket d'écoute sur le port 4242 et permet aux utilisateur de se connecter.

Pour chaque connection, le serveur créer un socket pour le joueur, et lance un thread qui s'occupe des intéractions avec le joueur. Le thread client attend que le joueur lui envoie un message avec son nom, puis ajoute un nouveau Joueur au tableau des joueurs. Le thread envoie alors au joueur combien il ya de joueur connecté. Le thread client attend indéfiniment un message du joueur et l'affiche en console serveur (en attendant de faire autre chose pour le moment et pour tester si le serveur fonctionne bien).

A chaque nouvel connection, le thread client envoie un broadcast global, informant qu'un nouveau joueur a rejoint le lobby.

Si un joueur se déconnecte, le thread du joueur envoie également un broadcast global a tout les joueurs pour avertir de la déconnexion, met a jour le tableau de joueur, et met fin a la connexion avec le dit-joueur.

Le thread principal accepte les connection en boucle, mais bloque lorsque le nombre de joueur maximum est atteint (MAX_CLIENT). Il avertit alors au joueur qui essaie de se connecté que le serveur est plein pour le moment.

Si on interrompt le programme serveur (CTRL C) on capture le signal pour fermer proprement le serveur et envoyé un message a tout les joueurs connecté.

## Mécanisme du jeu (Pas fait)

- Lancement de la partie : Comment lancer la partie ? Qui gère la partie ? un autre thread ? comme sa le thread principal peut continué de répondre aux demandes de connection et prévenir les joueurs qu'une partie est en cour ?

- Ditribution des cartes : Ajouter un attribut carte dans la structure joueur ? Tableau dynamique ? Qui envoie les cartes le thread gestion du jeu ou les threads client ?.

- Jouer des cartes : Comment un joueur peut joué une carte ? File d'attente de carte joué ? Quand un client joue une carte, le thread client recoit le message et met alors la carte joué dans une file d'attente avec le numéro de carte et l'heure.

- Controller l'ordre de jeu des cartes : Le thread gestionnaire serait notifié a chaque carte dans la file d'attente, et il analyserait alors la carte joué, si la carte est valide il l'ajoute au plateau de jeu, et broadcast un message a tout les joueurs que X a joué telle carte. Si la carte fait perdre la partie, le thread met fin a la manche, et réinitialise toutes les cartes.

- Envoi des mises à jour : S'assurer que le serveur envoie à tous les joueurs les cartes jouées et leur ordre, pour qu’ils puissent voir l’évolution de la manche en temps réel. Si une carte est accepter, il faut avertir le joueur qui vient de joué sa carte que c'est bon, et donc lui renvoyé un message avec les cartes qui lui reste.

## Programem client

## Statistiques & Classement

## Robot

