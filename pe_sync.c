#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int counter = 0;

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int value;
} semaphore_t;

void init_semaphore(semaphore_t *s, int value) {
    pthread_mutex_init(&s->lock, NULL);
    pthread_cond_init(&s->cond, NULL);
    s->value = value;
}

void P(semaphore_t *s) {
    pthread_mutex_lock(&s->lock);
    while (s->value <= 0)
        pthread_cond_wait(&s->cond, &s->lock);
    s->value--;
    pthread_mutex_unlock(&s->lock);
}

void V(semaphore_t *s) {
    pthread_mutex_lock(&s->lock);
    s->value++;
    pthread_cond_signal(&s->cond);
    pthread_mutex_unlock(&s->lock);
}

// These are the implementations for the PP and VV functions mentioned in the paper.
void PP(int *counter, semaphore_t *s1, semaphore_t *s2) {
    P(s1);
    (*counter)++;
    if (*counter == 1) {
        P(s2);
    }
    V(s1);
}

void VV(int *counter, semaphore_t *s1, semaphore_t *s2) {
    P(s1);
    (*counter)--;
    if (*counter == 0) {
        V(s2);
    }
    V(s1);
}

// Global semaphores
semaphore_t s1, s2, s3;

// A node in the expression tree
typedef struct exp_node {
    char *op_name; // name of the operation (like "read" or "write")
    struct exp_node *left;
    struct exp_node *right;
    char type; // '{', '+', ',', or '}'
} exp_node_t;

// Global root of the expression tree
exp_node_t *root = NULL;

// Recursive descent parser functions
exp_node_t* parse_pathexp(const char **str);
exp_node_t* parse_sequence(const char **str);
exp_node_t* parse_selection(const char **str);
exp_node_t* parse_simultaneous(const char **str);
exp_node_t* parse_operation(const char **str);

// Helper function to skip white spaces
void skip_spaces(const char **str) {
    while (**str == ' ' || **str == '\t' || **str == '\n') {
        (*str)++;
    }
}

exp_node_t* parse_pathexp(const char **str) {
    skip_spaces(str);
    if (**str == '{') {
        return parse_simultaneous(str);
    } else {
        return parse_sequence(str);
    }
}

exp_node_t* parse_sequence(const char **str) {
    exp_node_t *node = (exp_node_t *)malloc(sizeof(exp_node_t));
    node->left = parse_operation(str);
    skip_spaces(str);
    if (**str == ';') {
        (*str)++;
        node->right = parse_sequence(str);
        node->type = ';';
    } else {
        node->right = NULL;
    }
    return node;
}

exp_node_t* parse_selection(const char **str) {
    exp_node_t *node = (exp_node_t *)malloc(sizeof(exp_node_t));
    node->left = parse_operation(str);
    skip_spaces(str);
    if (**str == ',') {
        (*str)++;
        node->right = parse_selection(str);
        node->type = ',';
    } else {
        node->right = NULL;
    }
    return node;
}

exp_node_t* parse_simultaneous(const char **str) {
    exp_node_t *node = (exp_node_t *)malloc(sizeof(exp_node_t));
    (*str)++; // skip '{'
    node->left = parse_sequence(str);
    skip_spaces(str);
    if (**str == '+') {
        (*str)++;
        node->right = parse_sequence(str);
        node->type = '+';
    } else {
        node->right = NULL;
    }
    if (**str == '}') {
        (*str)++;
    }
    return node;
}

exp_node_t* parse_operation(const char **str) {
    skip_spaces(str);
    const char *start = *str;
    while (**str && **str != ';' && **str != ',' && **str != '}' && **str != '+') {
        (*str)++;
    }
    size_t len = *str - start;
    char *op_name = (char *)malloc(len + 1);
    strncpy(op_name, start, len);
    op_name[len] = '\0';

    exp_node_t *node = (exp_node_t *)malloc(sizeof(exp_node_t));
    node->op_name = op_name;
    node->left = NULL;
    node->right = NULL;
    node->type = 'o'; // for operation

    return node;
}

void INIT_SYNCHRONIZER(const char *path_exp) {
    // Initialize global semaphores based on needs. For simplicity, initializing all to 1.
    init_semaphore(&s1, 1);
    init_semaphore(&s2, 1);
    init_semaphore(&s3, 1);

    root = parse_pathexp(&path_exp);
}

// Recursively find and execute synchronization actions on entering the operation
void enter_operation_rec(exp_node_t *node, const char *op_name) {
    if (node == NULL) {
        return;
    }

    if (node->type == 'o' && strcmp(node->op_name, op_name) == 0) {
        // Found the operation, apply appropriate synchronization mechanisms here.
        // The actual action might depend on the precise rules of the path expressions.
        // For simplification, using PP(&counter, &s1, &s2) as an example.
        PP(&counter, &s1, &s2);
        return;
    }

    // Recur on the left and right subtree
    enter_operation_rec(node->left, op_name);
    enter_operation_rec(node->right, op_name);
}

void ENTER_OPERATION(const char *op_name) {
    enter_operation_rec(root, op_name);
}

// Recursively find and execute synchronization actions on exiting the operation
void exit_operation_rec(exp_node_t *node, const char *op_name) {
    if (node == NULL) {
        return;
    }

    if (node->type == 'o' && strcmp(node->op_name, op_name) == 0) {
        // Found the operation, apply appropriate synchronization mechanisms here.
        // The actual action might depend on the precise rules of the path expressions.
        // For simplification, using VV(&counter, &s1, &s2) as an example.
        VV(&counter, &s1, &s2);
        return;
    }

    // Recur on the left and right subtree
    exit_operation_rec(node->left, op_name);
    exit_operation_rec(node->right, op_name);
}

void EXIT_OPERATION(const char *op_name) {
    exit_operation_rec(root, op_name);
}
