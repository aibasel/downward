#ifndef STATE_VAR_T_H
#define STATE_VAR_T_H

#if STATE_VAR_BYTES == 1
typedef unsigned char state_var_t;

#elif STATE_VAR_BYTES == 2
typedef unsigned short state_var_t;

#elif STATE_VAR_BYTES == 4
typedef int state_var_t;

#endif


#endif
