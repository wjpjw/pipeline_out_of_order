#include <stdio.h>
#include <assert.h>

#include "rob.h"


extern int32_t NUM_ROB_ENTRIES;

/////////////////////////////////////////////////////////////
// Init function initializes the ROB
/////////////////////////////////////////////////////////////

ROB* ROB_init(void){
  int ii;
  ROB *t = (ROB *) calloc (1, sizeof (ROB));
  for(ii=0; ii<MAX_ROB_ENTRIES; ii++){
    t->ROB_Entries[ii].valid=false;
    t->ROB_Entries[ii].ready=false;
    t->ROB_Entries[ii].exec=false;
  }
  t->head_ptr=0;
  t->tail_ptr=0;
  return t;
}

/////////////////////////////////////////////////////////////
// Print State
/////////////////////////////////////////////////////////////
void ROB_print_state(ROB *t){
 int ii = 0;
  printf("Printing ROB \n");
  printf("Entry  Inst   Valid   ready\n");
  for(ii = 0; ii < 7; ii++) {
    printf("%5d ::  %d\t", ii, (int)t->ROB_Entries[ii].inst.inst_num);
    printf(" %5d\t", t->ROB_Entries[ii].valid);
    printf(" %5d\n", t->ROB_Entries[ii].ready);
    printf(" %5d\n", t->ROB_Entries[ii].exec);
  }
  printf("\n");
}

/////////////////////////////////////////////////////////////
// If there is space in ROB return true, else false
/////////////////////////////////////////////////////////////
int  ptr_next(int ptr){
  if(ptr<MAX_ROB_ENTRIES-1){
    return ptr++;    
  }
  else{
    return 0;
  }
}  
bool ROB_check_space(ROB *t){
  return ptr_next(t->tail_ptr)!=t->head_ptr;
}

/////////////////////////////////////////////////////////////
// insert entry at tail, increment tail (do check_space first)
/////////////////////////////////////////////////////////////
//return the prf_id assigned for the inst!
int ROB_insert(ROB *t, Inst_Info inst){
  if(!ROB_check_space(t)){
    return -1;
  }
  t->tail_ptr=ptr_next(t->tail_ptr);
  t->ROB_Entries[t->tail_ptr].valid=true;
  t->ROB_Entries[t->tail_ptr].exec =false;
  t->ROB_Entries[t->tail_ptr].ready=false;
  t->ROB_Entries[t->tail_ptr].inst=inst;
  inst.dr_tag=t->tail_ptr;
  return t->tail_ptr;
}

/////////////////////////////////////////////////////////////
// When an inst gets scheduled for execution, mark exec
/////////////////////////////////////////////////////////////

void ROB_mark_exec(ROB *t, Inst_Info inst){
  for(int i=0;i<t->tail_ptr;i++){
    if(t->ROB_Entries[i].inst.inst_num==inst.inst_num){
      t->ROB_Entries[i].exec=true;
    }  
  }  
}


/////////////////////////////////////////////////////////////
// Once an instruction finishes execution, mark rob entry as done
/////////////////////////////////////////////////////////////

void ROB_mark_ready(ROB *t, Inst_Info inst){
  for(int i=0;i<t->tail_ptr;i++){
    if(t->ROB_Entries[i].inst.inst_num==inst.inst_num){
      t->ROB_Entries[i].ready=true;
      t->ROB_Entries[i].inst=inst;  //update the inst!
      break;
    }  
  }    
}

/////////////////////////////////////////////////////////////
// Find whether the prf (rob entry) is ready
/////////////////////////////////////////////////////////////

bool ROB_check_ready(ROB *t, int tag){
  return t->ROB_Entries[tag].ready;
}


/////////////////////////////////////////////////////////////
// Check if the oldest ROB entry is ready for commit
/////////////////////////////////////////////////////////////

bool ROB_check_head(ROB *t){
  return t->ROB_Entries[t->head_ptr].valid && t->ROB_Entries[t->head_ptr].ready;
}

/////////////////////////////////////////////////////////////
// For writeback of freshly ready tags, wakeup waiting inst
/////////////////////////////////////////////////////////////

void  ROB_wakeup(ROB *t, int tag){
  int dr_tag=t->ROB_Entries[tag].inst.dr_tag;
  for(int i=0;i<t->tail_ptr;i++){
    if(t->ROB_Entries[i].inst.src1_tag==dr_tag){
      t->ROB_Entries[i].inst.src1_ready=true;
    }
    if(t->ROB_Entries[i].inst.src2_tag==dr_tag){
      t->ROB_Entries[i].inst.src2_ready=true;
    }
  }    
}

/////////////////////////////////////////////////////////////
// Remove oldest entry from ROB (after ROB_check_head)
/////////////////////////////////////////////////////////////

Inst_Info ROB_remove_head(ROB *t){   
   t->ROB_Entries[t->head_ptr].valid=false;
   Inst_Info r= t->ROB_Entries[t->head_ptr].inst;
   t->head_ptr=ptr_next(t->head_ptr);
   return r;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
