//
// Created by erwan on 15/11/2024.
//

#ifndef THEMIND_QUEUE_H
#define THEMIND_QUEUE_H

// Définition d'un élément de la file
typedef struct Node {
    int data;
    struct Node* next;
} Node;

// Définition de la file
typedef struct Queue {
    Node* front; // Tête de la file
    Node* rear;  // Fin de la file
} Queue;

Queue* create_queue();
void enqueue(Queue* queue, int value);
int dequeue(Queue* queue);
int peek(Queue* queue);
int isEmpty(Queue* queue);
void destroy_queue(Queue* queue);
void reset_queue(Queue* queue);
void sort_queue(Queue* queue);

#endif //THEMIND_QUEUE_H
