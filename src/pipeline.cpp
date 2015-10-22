/***********************************************************************
 * File         : pipeline.cpp
 * Author       : Moinuddin K. Qureshi
 * Date         : 19th February 2014
 * Description  : Out of Order Pipeline for Lab3 ECE 6100

 * Update       : Shravan Ramani, Tushar Krishna, 27th Sept, 2015
 **********************************************************************/

#include "pipeline.h"
#include <cstdlib>
#include <cstring>


extern int32_t PIPE_WIDTH;
extern int32_t SCHED_POLICY;
extern int32_t LOAD_EXE_CYCLES;

/**********************************************************************
 * Support Function: Read 1 Trace Record From File and populate Fetch Inst
 **********************************************************************/

void pipe_fetch_inst(Pipeline *p, Pipe_Latch* fe_latch){
    static int halt_fetch = 0;
    uint8_t bytes_read = 0;
    Trace_Rec trace;
    if(halt_fetch != 1) {
      bytes_read = fread(&trace, 1, sizeof(Trace_Rec), p->tr_file);
      Inst_Info *fetch_inst = &(fe_latch->inst);
    // check for end of trace
    // Send out a dummy terminate op
      if( bytes_read < sizeof(Trace_Rec)) {
        p->halt_inst_num=p->inst_num_tracker;
        halt_fetch = 1;
        fe_latch->valid=true;
        fe_latch->inst.dest_reg = -1;
        fe_latch->inst.src1_reg = -1;
        fe_latch->inst.src1_reg = -1;
        fe_latch->inst.inst_num=-1;
        fe_latch->inst.op_type=4;
        return;
      }

    // got an instruction ... hooray!
      fe_latch->valid=true;
      fe_latch->stall=false;
      p->inst_num_tracker++;
      fetch_inst->inst_num=p->inst_num_tracker;
      fetch_inst->op_type=trace.op_type;

      fetch_inst->dest_reg=trace.dest_needed? trace.dest:-1;
      fetch_inst->src1_reg=trace.src1_needed? trace.src1_reg:-1;
      fetch_inst->src2_reg=trace.src2_needed? trace.src2_reg:-1;

      fetch_inst->dr_tag=-1;
      fetch_inst->src1_tag=-1;
      fetch_inst->src2_tag=-1;
      fetch_inst->src1_ready=false;
      fetch_inst->src2_ready=false;
      fetch_inst->exe_wait_cycles=0;
    } else {
      fe_latch->valid = false;
    }
    return;
}


/**********************************************************************
 * Pipeline Class Member Functions
 **********************************************************************/

Pipeline * pipe_init(FILE *tr_file_in){
    printf("\n** PIPELINE IS %d WIDE **\n\n", PIPE_WIDTH);

    // Initialize Pipeline Internals
    Pipeline *p = (Pipeline *) calloc (1, sizeof (Pipeline));

    p->pipe_RAT=RAT_init();
    p->pipe_ROB=ROB_init();
    p->pipe_EXEQ=EXEQ_init();
    p->tr_file = tr_file_in;
    p->halt_inst_num = ((uint64_t)-1) - 3;
    int ii =0;
    for(ii = 0; ii < PIPE_WIDTH; ii++) {  // Loop over No of Pipes
      p->FE_latch[ii].valid = false;
      p->ID_latch[ii].valid = false;
      p->EX_latch[ii].valid = false;
      p->SC_latch[ii].valid = false;
    }
    return p;
}


/**********************************************************************
 * Print the pipeline state (useful for debugging)
 **********************************************************************/

void pipe_print_state(Pipeline *p){
    std::cout << "--------------------------------------------" << std::endl;
    std::cout <<"cycle count : " << p->stat_num_cycle << " retired_instruction : " << p->stat_retired_inst << std::endl;
    uint8_t latch_type_i = 0;
    uint8_t width_i      = 0;
   for(latch_type_i = 0; latch_type_i < 4; latch_type_i++) {
        switch(latch_type_i) {
        case 0:
            printf(" FE: ");
            break;
        case 1:
            printf(" ID: ");
            break;
        case 2:
            printf(" SCH: ");
            break;
        case 3:
            printf(" EX: ");
            break;
        default:
            printf(" -- ");
          }
    }
   printf("\n");
   for(width_i = 0; width_i < PIPE_WIDTH; width_i++) {
       if(p->FE_latch[width_i].valid == true) {
         printf("  %d  ", (int)p->FE_latch[width_i].inst.inst_num);
       } else {
         printf(" --  ");
       }
       if(p->ID_latch[width_i].valid == true) {
         printf("  %d  ", (int)p->ID_latch[width_i].inst.inst_num);
       } else {
         printf(" --  ");
       }
       if(p->SC_latch[width_i].valid == true) {
         printf("  %d  ", (int)p->SC_latch[width_i].inst.inst_num);
       } else {
         printf(" --  ");
       }
       if(p->EX_latch[width_i].valid == true) {
         for(int ii = 0; ii < MAX_WRITEBACKS; ii++) {
            if(p->EX_latch[ii].valid)
	      printf("  %d  ", (int)p->EX_latch[ii].inst.inst_num);
         }
       } else {
         printf(" --  ");
       }
        printf("\n");
     }
     printf("\n");

     RAT_print_state(p->pipe_RAT);
     EXEQ_print_state(p->pipe_EXEQ);
     ROB_print_state(p->pipe_ROB);
}


/**********************************************************************
 * Pipeline Main Function: Every cycle, cycle the stage
 **********************************************************************/
#define WJP 47500
ROB_Entry* oldest_entry(ROB*rob);
void pipe_cycle(Pipeline *p)
{
    p->stat_num_cycle++;
    //static int i=0;
    //i++;
    //p->stat_num_cycle=i;
    
    pipe_cycle_commit(p);
    pipe_cycle_writeback(p);
    pipe_cycle_exe(p);
    pipe_cycle_schedule(p);
    pipe_cycle_issue(p);
    pipe_cycle_decode(p);
    pipe_cycle_fetch(p);
    /*/------------------test-----------------------
    if(i>WJP-2){
      printf("Cycle:%d\n",i);
      if(ROB_check_head(p->pipe_ROB)){
	printf("%d committed! halt:%d\n",p->pipe_ROB->head_ptr, p->halt_inst_num);
      }

      printf("inst->dr_tag:%d",oldest_entry(p->pipe_ROB)->inst.dr_tag);

      printf("tail:%d, head: %d\n",p->pipe_ROB->tail_ptr, p->pipe_ROB->head_ptr);
      ROB_print_state(p->pipe_ROB);
      //RAT_print_state(p->pipe_RAT);
      //EXEQ_print_state(p->pipe_EXEQ);
    }
    if(i==WJP){
      exit(0);
    }
    //-------------------test-----------------------*/
}

//--------------------------------------------------------------------//

void pipe_cycle_fetch(Pipeline *p){
  int ii = 0;
  Pipe_Latch fetch_latch;

  for(ii=0; ii<PIPE_WIDTH; ii++) {
    if((p->FE_latch[ii].stall) || (p->FE_latch[ii].valid)) {   // Stall
      continue;
    } else {  // No Stall and Latch Empty
        pipe_fetch_inst(p, &fetch_latch);
        // copy the op in FE LATCH
        p->FE_latch[ii]=fetch_latch;	
    }
  }
}

//--------------------------------------------------------------------//
void pipe_cycle_decode(Pipeline *p){
   int ii = 0;

   int jj = 0;

   static uint64_t start_inst_id = 1;

   // Loop Over ID Latch
   for(ii=0; ii<PIPE_WIDTH; ii++){
     if((p->ID_latch[ii].stall == 1) || (p->ID_latch[ii].valid)) { // Stall
       continue;
     } else {  // No Stall & there is Space in Latch
       for(jj = 0; jj < PIPE_WIDTH; jj++) { // Loop Over FE Latch
         if(p->FE_latch[jj].valid) {
           if(p->FE_latch[jj].inst.inst_num == start_inst_id) { // In Order Inst Found
             p->ID_latch[ii]        = p->FE_latch[jj];
             p->ID_latch[ii].valid  = true;
             p->FE_latch[jj].valid  = false;
             start_inst_id++;
             break;
           }
         }
       }
     }
   }
}


//--------------------------------------------------------------------//

void pipe_cycle_exe(Pipeline *p){

  int ii;
  //If all operations are single cycle, simply copy SC latches to EX latches
  if(LOAD_EXE_CYCLES == 1) {
    for(ii=0; ii<PIPE_WIDTH; ii++){
      if(p->SC_latch[ii].valid) {
        p->EX_latch[ii]=p->SC_latch[ii];
        p->EX_latch[ii].valid = true;
        p->SC_latch[ii].valid = false;
      }
      return;
    }
  }

  //---------Handling exe for multicycle operations is complex, and uses EXEQ

  // All valid entries from SC get into exeq

  for(ii = 0; ii < PIPE_WIDTH; ii++) {
    if(p->SC_latch[ii].valid) {
      EXEQ_insert(p->pipe_EXEQ, p->SC_latch[ii].inst);
      p->SC_latch[ii].valid = false;
    }
  }

  // Cycle the exeq, to reduce wait time for each inst by 1 cycle
  EXEQ_cycle(p->pipe_EXEQ);

  // Transfer all finished entries from EXEQ to EX_latch
  int index = 0;

  while(1) {
    if(EXEQ_check_done(p->pipe_EXEQ)) {
      p->EX_latch[index].valid = true;
      p->EX_latch[index].stall = false;
      p->EX_latch[index].inst  = EXEQ_remove(p->pipe_EXEQ);
      index++;
    } else { // No More Entry in EXEQ
      break;
    }
  }
}



/**********************************************************************
 * -----------  DO NOT MODIFY THE CODE ABOVE THIS LINE ----------------
 **********************************************************************/
void rename(RAT*rat, Inst_Info*inst){
  inst->src1_tag=RAT_get_remap(rat, inst->src1_reg);
  inst->src2_tag=RAT_get_remap(rat, inst->src2_reg);
  if(inst->dest_reg!=-1){ //if dest_reg==-1, there is no need to remap!
    RAT_set_remap(rat, inst->dest_reg, inst->dr_tag);
  }
}  
void pipe_cycle_issue(Pipeline *p) {
  int jj = 0;
  static uint64_t start_inst_id2 = 1;
  // Loop Over ID Latch
  for(jj = 0; jj < PIPE_WIDTH; jj++) {
    if(!ROB_check_space(p->pipe_ROB)){
      return;
    }
    if(p->ID_latch[jj].valid) {
      if(p->ID_latch[jj].inst.inst_num == start_inst_id2) {               // In Order Inst Found
        int index=ROB_insert(p->pipe_ROB,  p->ID_latch[jj].inst);         // insert inst into rob (valid = 1, exec = 0, ready = 0)
	rename(p->pipe_RAT, &p->pipe_ROB->ROB_Entries[index].inst);       // rename inst's src tags and write inst's dr_tag into rat!  
	p->ID_latch[jj].valid  = false;                                   // false means empty!
	start_inst_id2++;
      }
    }
  }
}

//--------------------------------------------------------------------//
bool inst_ready(Inst_Info* inst, ROB* rob){
  inst->src1_ready=true;
  inst->src2_ready=true;
  for(int i=rob->head_ptr;i!=inst->dr_tag;i=ptr_next(i)){
    if(!rob->ROB_Entries[i].valid)continue;
    int tmp=rob->ROB_Entries[i].inst.dr_tag;
    if(tmp==inst->src1_tag||tmp==inst->src2_tag){
      if(inst->src1_tag==tmp){
	inst->src1_ready=false;
      }
      if(inst->src2_tag==tmp){
	inst->src2_ready=false;
      }
      return false;  // 
    }
  }
  return true;
}
ROB_Entry* oldest_entry(ROB*rob){
  for(int i=rob->head_ptr;i!=rob->tail_ptr;i=ptr_next(i)){
    ROB_Entry*t=&rob->ROB_Entries[i];
    if(!t->exec&&!t->ready&&t->valid){
      return t;
    }  
  }
  return 0;
}
ROB_Entry* ooo_oldest_entry(ROB*rob){ 
  for(int i=rob->head_ptr;i!=rob->tail_ptr;i=ptr_next(i)){
    ROB_Entry*t=&rob->ROB_Entries[i];
    if(inst_ready(&t->inst,rob) && !t->exec && t->valid && !t->ready){
      rob->ROB_Entries[i].exec=true;
      return &rob->ROB_Entries[i];
    }  
  }  
  return 0;
}  
void pipe_cycle_schedule(Pipeline *p) {
  if(SCHED_POLICY==0){
    int jj = 0;
    for(jj = 0; jj < PIPE_WIDTH; jj++) {
      if(p->SC_latch[jj].valid || p->SC_latch[jj].stall) {  //will be always empty because ex stage does not stall!
	continue;
      }
      ROB_Entry* e=oldest_entry(p->pipe_ROB);
      if(e==0)return;//In ROB there is no executable inst!
      if(inst_ready(&e->inst, p->pipe_ROB)){
	p->SC_latch[jj].valid=true;
	p->SC_latch[jj].stall=false;
	p->SC_latch[jj].inst=e->inst;
	e->exec=true;
      }
    }
  }
  if(SCHED_POLICY==1){
    int jj = 0;
    for(jj = 0; jj < PIPE_WIDTH; jj++) {
      if(p->SC_latch[jj].valid || p->SC_latch[jj].stall){
	continue;
      }
      ROB_Entry* e=ooo_oldest_entry(p->pipe_ROB);
      if(e!=0){
	p->SC_latch[jj].valid=true;
	p->SC_latch[jj].stall=false;
	p->SC_latch[jj].inst=e->inst;
      }
    }
  }
}


//--------------------------------------------------------------------//

void pipe_cycle_writeback(Pipeline *p){
  // TODO: Go through all instructions out of EXE latch
  // TODO: Writeback to ROB (using wakeup function)
  // TODO: Update the ROB, mark ready, and update Inst Info in ROB
  int jj = 0;
  for(jj = 0; jj < MAX_WRITEBACKS; jj++) {      
    if(p->EX_latch[jj].valid){
      ROB_wakeup(p->pipe_ROB, p->EX_latch[jj].inst.dr_tag);
      ROB_mark_ready(p->pipe_ROB, p->EX_latch[jj].inst);      //mark ready!
      p->EX_latch[jj].valid=false;                            //empty!
    }
  }
}


//--------------------------------------------------------------------//


void pipe_cycle_commit(Pipeline *p) {
  // TODO: check the head of the ROB. If ready commit (update stats)
  // TODO: Deallocate entry from ROB
  // TODO: Update RAT after checking if the mapping is still relevant
  if(!ROB_check_head(p->pipe_ROB)){ 
    return; //do nothing!
  }

  if(p->halt_inst_num==p->pipe_ROB->ROB_Entries[p->pipe_ROB->head_ptr].inst.inst_num){
    p->halt=true;
  }
  
  Inst_Info committed=ROB_remove_head(p->pipe_ROB);
  RAT_reset_entry(p->pipe_RAT, committed.dest_reg);
  p->stat_retired_inst++;
  
}

//--------------------------------------------------------------------//




