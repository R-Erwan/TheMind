//
// Created by erwan on 15/11/2024.
//

#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

// Crée une nouvelle file vide
Queue* create_queue() {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    if (!queue) {
        perror("Erreur d'allocation mémoire");
        exit(EXIT_FAILURE);
    }
    queue->front = queue->rear = NULL;
    return queue;
}

// Ajoute un élément à la file
void enqueue(Queue* queue, int value) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (!newNode) {
        perror("Erreur d'allocation mémoire");
        exit(EXIT_FAILURE);
    }
    newNode->data = value;
    newNode->next = NULL;

    if (queue->rear == NULL) {
        // La file est vide
        queue->front = queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
}

// Retire un élément de la file
int dequeue(Queue* queue) {
    if (queue->front == NULL) {
//        fprintf(stderr, "La file est vide\n");
        return -1; // Erreur
    }

    Node* temp = queue->front;
    int value = temp->data;
    queue->front = queue->front->next;

    if (queue->front == NULL) {
        queue->rear = NULL; // La file est maintenant vide
    }

    free(temp);
    return value;
}

// Regarde l'élément en tête sans le retirer
int peek(Queue* queue) {
    if (queue->front == NULL) {
        return -1;
    }
    return queue->front->data;
}

void reset_queue(Queue* queue) {
    if (!queue) {
        fprintf(stderr, "Erreur : file inexistante\n");
        return;
    }

    // Libérer tous les noeuds de la file
    while (queue->front != NULL) {
        Node* temp = queue->front;
        queue->front = queue->front->next;
        free(temp);
    }

    // Réinitialiser les pointeurs
    queue->front = NULL;
    queue->rear = NULL;
}

void sort_queue(Queue* queue) {
    if (!queue || isEmpty(queue)) {
        fprintf(stderr, "Erreur : File inexistante ou vide\n");
        return;
    }

    // Compter les éléments et les stocker dans un tableau dynamique
    int count = 0;
    Node* current = queue->front;
    while (current) {
        count++;
        current = current->next;
    }

    int* elements = (int*)malloc(count * sizeof(int));
    if (!elements) {
        perror("Erreur d'allocation mémoire pour le tri");
        exit(EXIT_FAILURE);
    }

    // Transférer les éléments de la file dans le tableau
    current = queue->front;
    for (int i = 0; i < count; i++) {
        elements[i] = current->data;
        current = current->next;
    }

    // Trier le tableau
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (elements[i] > elements[j]) {
                // Échanger les valeurs
                int temp = elements[i];
                elements[i] = elements[j];
                elements[j] = temp;
            }
        }
    }

    // Réinitialiser la file et insérer les éléments triés
    reset_queue(queue);
    for (int i = 0; i < count; i++) {
        enqueue(queue, elements[i]);
    }

    // Libérer la mémoire du tableau
    free(elements);
}

// Vérifie si la file est vide
int isEmpty(Queue* queue) {
    return queue->front == NULL;
}

// Libère toute la mémoire utilisée par la file
void destroy_queue(Queue* queue) {
    while (!isEmpty(queue)) {
        dequeue(queue);
    }
    free(queue);
}

