#ifndef _ROB_H_
#define _ROB_H_
#include <inttypes.h>
#include <assert.h>
#include "trace.h"
#include <cstdlib>

#define MAX_ROB_ENTRIES 256   // original:256 

typedef struct ROB_Entry_Struct {
  bool     valid;
  bool     exec;
  bool     ready;
  Inst_Info inst; // tags and ready bits, see trace.h
}ROB_Entry;

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

typedef struct ROB {
  ROB_Entry  ROB_Entries[MAX_ROB_ENTRIES];
  int head_ptr;
  int tail_ptr;
}ROB;

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
int       ptr_next(int ptr);  //added function
int       ROB_size(ROB*t);
ROB*      ROB_init(void);
void      ROB_print_state(ROB *t);

bool      ROB_check_space(ROB *t);
int       ROB_insert(ROB *t, Inst_Info inst);
void      ROB_mark_exec(ROB *t, Inst_Info inst);
void      ROB_mark_ready(ROB *t, Inst_Info inst);
bool      ROB_check_ready(ROB *t, int tag);
bool      ROB_check_head(ROB *t);

void      ROB_wakeup(ROB *t, int tag);
Inst_Info ROB_remove_head(ROB *t);

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

#endif

