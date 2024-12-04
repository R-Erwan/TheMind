#!/usr/bin/env bash
PROJECT_NAME="TheMind"
BUILD_DIR="build"

echo "Installation de $PROJECT_NAME"

# Dépendances
echo "📦 Installation des dépendances..."
sudo apt-get update
sudo apt-get install -y cmake gcc gnuplot texlive texlive-latex-extra bash

# Arborescence
echo "📁 Création de l'arborescence des fichiers..."
mkdir -p {datas,pdf}
touch datas/rank.dat

# Compilation
if [ -d "$BUILD_DIR" ]; then
  echo "🧹 Nettoyage du répertoire $BUILD_DIR existant..."
  rm -rf "$BUILD_DIR"
fi
echo "📁 Création du répertoire $BUILD_DIR..."
mkdir "$BUILD_DIR"
cd "$BUILD_DIR"

echo "🔧 Configuration avec CMake..."
cmake ..

echo "⚙️ Compilation..."
cmake --build .

# Verification

# Étape 4 : Fin
echo "✅ Installation terminée. Exécutable disponible dans $BUILD_DIR/$PROJECT_NAME"
echo "run ./$PROJECT_NAME dans le dossier $BUILD_DIR pour lancer le server."

