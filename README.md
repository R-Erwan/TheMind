# The Mind

Serveur de jeu multijoueur pour jouer a The Mind game.

## Installation

Utilisé le script d'installation `install.sh` pour installer les dépendances, et les dossiers nécéssaire.

```bash
./install.sh 
```

## Usage
### Lancer un serveur
Lancer le serveur avec `.\TheMnind <port> <backlog>` depuis le répertoire _build-server_ en précisant le port principal (pour les connexions clients), et un backlog.
> Le port de requête de téléchargement est définie automatiquement en fonction du port principal
```python
# Lancer le serveur
./TheMind 4242 10
Server listening on port 4242 # Port d'écoute connection joueur
Server listening on port 4243 # Port d'écoute requête de téléchargment de fichier
[DL] Server ready to handle new downloading request
```

### Connexion client :
Se connecter au serveur avec une connection **netcat** `nc <ipaddr> <port>` :
```python nc localhost 4242
Bienvenue sur TheMind !
Envoyé votre nom
Toto

Toto a rejoint !

------ LOBBY ------
Nb Joueurs : 1
Joueurs : Toto
Nb prêt : [1/1]
-------------------
```

Ou en utilisant le bin client dasn **TheMindClient**.
```python
./TheMindClient 127.0.0.1 4242
Check installation ...
Installation ok
                                                THE MIND CLIENT
===============================================================================================================
...
```

## Commandes possibles :
- `ready` et `unready` pour changer son état.
- `start` Pour lancer la partie ou le round.
- `stop` Pour mettre fin a une partie.
- `add robot`Pour ajouter un robot dans la partie.
- `[1-99]`Pour jouer une carte.  

## Télécharger les statistiques :
A la fin d'une partie le serveur enverra le nom du fichier de statistiques créer qu'il est possible de télécharger, ainsi que le top10 des parties en fonction du nombre de joueurs.
```python
Le fichier de statistiques est disponible.
Nom du fichier : 2024-12-11-23_30_41.pdf
Pour le récupérer, utiliser la commande : getfile 2024-12-11-23_30_41.pdf sur le port 4243

----------------------------- Classement -----------------------------
Rang nbJoueurs MancheMax Joueurs Date
1     1          2          Toto                 2024-12-10
2     1          2          Toto                 2024-12-10
3     1          2          Toto                 2024-12-11
4     1          1          Toto                 2024-12-10
5     1          1          Toto                 2024-12-10
6     1          1          Toto                 2024-12-10
7     1          1          Toto                 2024-12-10
8     1          0          Toto                 2024-12-10
---------------------------------------------------------------------
```
### Depuis un shell : 
```bash
echo "getfile 2024-12-11-23_30_41.pdf" | nc localhost 4243 > stats.pdf
```
### Depuis l'éxécutable client : 
Avec le programme client, le fichier est automatiquement télécharger et copier dans un répertoire **pdf** a la racine du dossier du programme.
