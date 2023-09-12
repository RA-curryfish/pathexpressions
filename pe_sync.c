/* Toy synchronizer: Sample template
 * Your synchronizer should implement the three functions listed below. 
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

void ENTER_OPERATION(const char *op_name)
{
}

void EXIT_OPERATION(const char *op_name)
{
}

void INIT_SYNCHRONIZER(const char *path_exp)
{
  printf("Initializing Synchronizer with path_exp %s\n", path_exp);
}
