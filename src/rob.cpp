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
  printf(" Entry   Inst      Valid   ready   exec  src1_tag src2_tag\n");
  for(ii = 0; ii < MAX_ROB_ENTRIES; ii++) {  //original:7 
    printf("%5d ::  %d\t", ii, (int)t->ROB_Entries[ii].inst.inst_num);
    printf(" %5d\t", t->ROB_Entries[ii].valid);
    printf(" %5d\t", t->ROB_Entries[ii].ready);
    printf(" %5d\t", t->ROB_Entries[ii].exec);
    printf(" %5d\t", t->ROB_Entries[ii].inst.src1_tag);
    printf(" %5d\n", t->ROB_Entries[ii].inst.src2_tag);
  }
  printf("\n");
}

/////////////////////////////////////////////////////////////
// If there is space in ROB return true, else false
/////////////////////////////////////////////////////////////
int  ptr_next(int ptr){
  if(ptr<MAX_ROB_ENTRIES-1){
    return ptr+1;    
  }
  else{
    return 0;
  }
}  
bool ROB_check_space(ROB *t){
  if(!t->ROB_Entries[t->head_ptr].valid){ //even head doesn't point to inst!
    return true;
  }  
  return t->tail_ptr!=t->head_ptr;
}

/////////////////////////////////////////////////////////////
// insert entry at tail, increment tail (do check_space first)
/////////////////////////////////////////////////////////////
//return the prf_id assigned for the inst!
int ROB_insert(ROB *t, Inst_Info inst){
  if(!ROB_check_space(t)){
    //printf("rob.cpp: ROB no space!");
    return -1;
  }
  int tmp=t->tail_ptr;
  t->ROB_Entries[tmp].valid=true;
  t->ROB_Entries[tmp].exec =false;
  t->ROB_Entries[tmp].ready=false;
  t->ROB_Entries[tmp].inst=inst;
  t->ROB_Entries[tmp].inst.dr_tag=tmp;  // use its index in rob to represent dest tag!
  t->tail_ptr=ptr_next(t->tail_ptr);                    // increment tail
  return tmp;
}


//ROB size!
int ROB_size(ROB*t){
  if(t->ROB_Entries[t->head_ptr].valid==false){
    return 0;
  }  
  if(t->head_ptr<t->tail_ptr){
    return t->tail_ptr-t->head_ptr;
  }
  else{
    return MAX_ROB_ENTRIES-(t->head_ptr-t->tail_ptr);
  }
  
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
  for(int i=0;i<MAX_ROB_ENTRIES;i++){
    if(t->ROB_Entries[i].inst.inst_num==inst.inst_num){
      t->ROB_Entries[i].ready=true;
      t->ROB_Entries[i].inst=inst;  //update the inst!
      //break;
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
