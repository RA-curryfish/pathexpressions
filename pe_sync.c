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
//semaphore_t s1, s2, s3;
semaphore_t s_array[];
int s_array_index = 0;

// A node in the expression tree
typedef struct exp_node {
    char *op_name; // name of the operation (like "read" or "write")
    char type; // '{', '+', ';', '}', 'o'
    struct exp_node *left;
    int isLeftChildPorPP;
    int isLeftPorPP;
    int left_semephore_index;
    struct exp_node *right;
    int isRightChildVorVV;
    int isRightVorVV;
    int right_semephore_index;
} exp_node_t;

// Global root of the expression tree
exp_node_t *root = NULL;

// Recursive descent parser functions
//exp_node_t* parse_pathexp(const char **str);
exp_node_t* parse_pathexp(const char **str, exp_node_t *parentNode, int split_index);
exp_node_t* parse_sequence(const char **str, exp_node_t *parentNode, int split_index);
exp_node_t* parse_selection(const char **str, exp_node_t *parentNode, int split_index);
exp_node_t* parse_simultaneous(const char **str, exp_node_t *parentNode, int split_index);
exp_node_t* parse_operation(const char **str, exp_node_t *parentNode, int split_index, int isRightChild);
//exp_node_t* parse_sequence_or_selection(const char **str, exp_node_t *parentNode);

int find_outter_most_operation(const char **str);

// Helper function to skip white spaces
void skip_spaces(const char **str) {
    while (**str == ' ' || **str == '\t' || **str == '\n') {
        (*str)++;
    }
}

char* strip_path_and_end(const char *str) {
    if (str == NULL) {
        return NULL;
    }

    const char *prefix = "path";
    const char *suffix = "end";

    size_t str_len = strlen(str);
    size_t prefix_len = strlen(prefix);
    size_t suffix_len = strlen(suffix);

    // Pointers to the start and end of the portion to keep.
    const char *start = str;
    const char *end = str + str_len;

    // Check and strip prefix "path"
    if (strncmp(start, prefix, prefix_len) == 0) {
        start += prefix_len;
    }

    // Check and strip suffix "end"
    if (str_len >= suffix_len && strcmp(end - suffix_len, suffix) == 0) {
        end -= suffix_len;
    }

    // Allocate new string
    size_t new_str_len = end - start;
    char *new_str = (char *)malloc(new_str_len + 1);  // +1 for '\0'

    if (new_str == NULL) {
        // Handle allocation failure if needed
        return NULL;
    }

    // Copy relevant portion and add null-terminator
    strncpy(new_str, start, new_str_len);
    new_str[new_str_len] = '\0';

    return new_str;
}

int find_outter_most_operation(const char **str) {
    int a = -1;
    int b = -1;

    for (int i = 0; i <= strlen(*str); i++)
    {
        if (*str[i] == ';' || *str[i] == '+')
        {
            if (a == -1) {
               return i;
            } else {
                if (b != -1) {
                    return i;
                }
            }
        } else if (*str[i] == '{') {
            a = i;
        } else if (*str[i] == '}') {
            b = i;
        }
    }
    return -1;
}

exp_node_t* parse_pathexp(const char **str, exp_node_t *parentNode, int split_index) {
    int inner_split_index = find_outter_most_operation(str);

    if (*str[split_index] == '+') {
        return parse_selection(str, parentNode, split_index);
    } else if (*str[split_index] == ';'){
        return parse_sequence(str, parentNode, split_index);
    } else if (*str[split_index] == '{'){
        return parse_simultaneous(str, parentNode, split_index);
    } else {
        return parse_operation(str, parentNode, split_index, );
    }
}

exp_node_t* parse_sequence(const char **str, exp_node_t *parentNode, int split_index) {
    exp_node_t *node = (exp_node_t *)malloc(sizeof(exp_node_t));
    init_semaphore(&s_array[s_array_index], 0);

    node->type = ';';
    node->op_name = NULL;

    node->isLeftPorPP = 1;
    node->isRightVorVV = 1;

    node->isLeftChildPorPP = 1;
    node->isRightChildVorVV = 1;

    node->left_semephore_index = s_array_index;
    node->right_semephore_index = s_array_index;
    s_array_index++;

    node->left = parse_operation(str, node, split_index);
    skip_spaces(str);
    node->right = parse_pathexp(str, node, split_index);

    return node;
}

exp_node_t* parse_selection(const char **str, exp_node_t *parentNode, int split_index) {
    exp_node_t *node = (exp_node_t *)malloc(sizeof(exp_node_t));
    node->type = '+';
    node->op_name = NULL;
    node->isLeftPorPP = 1;
    node->isRightVorVV = 1;
    node->isLeftChildPorPP = parentNode->isLeftPorPP;
    node->isRightChildVorVV = parentNode->isRightChildVorVV;

    node->left = parse_operation(str, parentNode, split_index);
    node->right = parse_pathexp(str, parentNode, split_index);

    skip_spaces(str);

    return node;
}

exp_node_t* parse_simultaneous(const char **str, exp_node_t *parentNode, int split_index) {
    exp_node_t *node = (exp_node_t *)malloc(sizeof(exp_node_t));
    (*str)++; // skip '{'
    node->type = '{';
    node->op_name = NULL;
    node->isLeftPorPP =
    node->isRightVorVV =
    node->left = parse_pathexp(str, parentNode);
    skip_spaces(str);

    node->right = NULL;
    if (**str == '}') {
        (*str)++;
    }
    return node;
}

exp_node_t* parse_operation(const char **str, exp_node_t *parentNode, int split_index, int isRightChild) {
    skip_spaces(str);
    const char *start = *str;
    while (**str && **str != ';' && **str != '{' && **str != '}' && **str != '+') {
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
    if (parentNode->type == '+'){
        node->isLeftPorPP = parentNode->isLeftPorPP;
        node->isRightVorVV = parentNode->isRightVorVV;
        node->left_semephore_index = parentNode->left_semephore_index;
        node->right_semephore_index = parentNode->right_semephore_index;
    } else if (parentNode->type == ';'){
        if (op_name == "read"){

        } else {

        }
        node->isLeftPorPP = parentNode->isLeftPorPP;
        node->isRightVorVV = 0;
        node->left_semephore_index = parentNode->left_semephore_index;
        node->right_semephore_index = parentNode->right_semephore_index;
    }

    node->type = 'o'; // for operation

    return node;
}

void INIT_SYNCHRONIZER(const char *path_exp) {
    // Initialize global semaphores based on needs. For simplicity, initializing all to 1.
    // TODO: create these semaphores
    init_semaphore(&s_array[0], 1);
    s_array_index++;

    root->type = 'r';
    root->op_name = "root";
    root->right = NULL;
    root->isLeftPorPP = 0;
    root->isRightVorVV = 0;
    root->isLeftChildPorPP = 0;
    root->isRightChildVorVV = 0;
    root->left_semephore_index = 0;
    root->right_semephore_index = 0;

    skip_spaces(&path_exp);
    const char *stripped_str = strip_path_and_end(path_exp);
    skip_spaces(&stripped_str);
    fprintf(stderr, "Parsing path expression: %s\n", stripped_str);

    root->left = parse_pathexp(&stripped_str, root);
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
        //PP(&counter, &s1, &s2);
        if (node->isLeftChildPorPP == 0)
        {
            P(node->left_semephore_index);
        } else
        {
            init_semaphore(&s_array[s_array_index],1);
            PP(&counter,&s_array[s_array_index],node->left_semephore_index);
            s_array_index++;
        }
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
        //VV(&counter, &s1, &s2);
        if(node->isRightChildVorVV == 0)
        {
            V(node->right_semephore_index);
        }
        else
        {
            //need to use the sema generated in PP, so dont init, but create another struct variable that holds the semaphore if it was a seleciton and PP was involved
            //if s2 was created for PP(c,S2,node->leftsema), then this same s2 must be used here. so this node must carry this info too in the form of an index to a pp_sema array reserved just for pp and vv.
            VV(&counter, pp_s_array[pp_s_array_index], node->right_semephore_index);
        }
        return;
    }

    // Recur on the left and right subtree
    exit_operation_rec(node->left, op_name);
    exit_operation_rec(node->right, op_name);
}

void EXIT_OPERATION(const char *op_name) {
    exit_operation_rec(root, op_name);
}
