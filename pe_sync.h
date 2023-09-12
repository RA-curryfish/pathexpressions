#ifndef _PE_SYNC_H_
#define _PE_SYNC_H_

void ENTER_OPERATION(const char *op_name);

void EXIT_OPERATION(const char *op_name);

void INIT_SYNCHRONIZER(const char *path_exp);

#endif
