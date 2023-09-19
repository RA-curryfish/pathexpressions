/*INIT_SYNCHRONIZE Toy synchronizer: Sample template
 * Your synchronizer should implement the three functions listed below. 
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

typedef struct {
    int read_count; // For the readers-writers problem
    int write_count;
    pthread_mutex_t mutex;
    pthread_cond_t read_cond;
    pthread_cond_t write_cond;
    // Add other state fields based on the path expressions
} sync_state;

sync_state state;

void INIT_SYNCHRONIZER(const char *path_exp) {
    printf("Initializing Synchronizer with path_exp %s\n", path_exp);
    state.read_count = 0;
    state.write_count = 0;
    pthread_mutex_init(&state.mutex, NULL);
    pthread_cond_init(&state.read_cond, NULL);
    pthread_cond_init(&state.write_cond, NULL);
    // Parse the path expression to set up other state
}

void ENTER_OPERATION(const char *op_name) {
    pthread_mutex_lock(&state.mutex);
    if (strcmp(op_name, "read") == 0) {
        while (state.write_count > 0) {
            pthread_cond_wait(&state.read_cond, &state.mutex);
        }
        state.read_count++;
    } else if (strcmp(op_name, "write") == 0) {
        while (state.read_count > 0 || state.write_count > 0) {
            pthread_cond_wait(&state.write_cond, &state.mutex);
        }
        state.write_count++;
    }
    // Handle other operations based on the parsed path expression
    pthread_mutex_unlock(&state.mutex);
}

void EXIT_OPERATION(const char *op_name) {
    pthread_mutex_lock(&state.mutex);
    if (strcmp(op_name, "read") == 0) {
        state.read_count--;
        if (state.read_count == 0) {
            pthread_cond_signal(&state.write_cond);
        }
    } else if (strcmp(op_name, "write") == 0) {
        state.write_count--;
        pthread_cond_signal(&state.read_cond);
        pthread_cond_signal(&state.write_cond);
    }
    // Handle other operations based on the parsed path expression
    pthread_mutex_unlock(&state.mutex);
}
