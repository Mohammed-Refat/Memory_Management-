/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault
void Placement (struct Env * curenv, uint32 fault_va){

struct FrameInfo *temp = NULL;

	allocate_frame(&temp);
	map_frame(curenv->env_page_directory , temp , fault_va , PERM_USER|PERM_WRITEABLE);

	int result = pf_read_env_page(curenv , (void *)fault_va);

	int isHeap = fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX;
	int isStack = fault_va >= USTACKBOTTOM && fault_va < USTACKTOP;

	if(result == E_PAGE_NOT_EXIST_IN_PF )
		if((!isStack) && (!isHeap))panic("ILLEGAL MEMORY ACCESS");

	uint32 Rounded_fault_va = ROUNDDOWN(fault_va , PAGE_SIZE);

	env_page_ws_set_entry(curenv , curenv->page_last_WS_index , Rounded_fault_va);
	curenv->page_last_WS_index ++;

	if(curenv->page_last_WS_index == curenv->page_WS_max_size )
		curenv->page_last_WS_index = 0;
}

void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
	int current_size = env_page_ws_get_size(curenv);

	if(current_size < curenv->page_WS_max_size){
//		cprintf("PLACMENT\n");
		Placement(curenv , fault_va);
	}

	else{//ws is full
//		cprintf("REPLACMENT\n");

		while(1==1){

			if(curenv->page_last_WS_index == curenv->page_WS_max_size )
				curenv->page_last_WS_index = 0;

			uint32 va = curenv->ptr_pageWorkingSet[curenv->page_last_WS_index].virtual_address;
			int perms = pt_get_page_permissions(curenv->env_page_directory , va);

			if((perms & PERM_USED)){

				pt_set_page_permissions(curenv->env_page_directory , va , 0 , PERM_USED);
				curenv->page_last_WS_index ++;
			}

			else
				break;
		}
		//victim

		struct FrameInfo *temp = NULL;
		int perms = pt_get_page_permissions(curenv->env_page_directory , curenv->ptr_pageWorkingSet[curenv->page_last_WS_index].virtual_address);

		if((perms & PERM_MODIFIED)){

			uint32 pa = virtual_to_physical(curenv->env_page_directory,curenv->ptr_pageWorkingSet[curenv->page_last_WS_index].virtual_address);
			struct FrameInfo * frame = to_frame_info(pa);

			pf_update_env_page(curenv,curenv->ptr_pageWorkingSet[curenv->page_last_WS_index].virtual_address,frame);
		}

		unmap_frame(curenv->env_page_directory , curenv->ptr_pageWorkingSet[curenv->page_last_WS_index].virtual_address);
		env_page_ws_clear_entry(curenv , curenv->page_last_WS_index);
		Placement(curenv , fault_va);
	}
}/*
int index;
void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//TODO: [PROJECT MS3] [FAULT HANDLER] page_fault_handler
	// Write your code here, remove the panic and write your code

	int current_size = env_page_ws_get_size(curenv);
	curenv->page_last_WS_index = current_size;
	uint32 * ptrTable = NULL ;
	int cnt =0;
	if(current_size >= curenv->page_WS_max_size){
//		cprintf("=====================REPLACEMENT====================\n");
//		env_page_ws_print(curenv);
		while(1==1){
			cnt++;
			if(index == current_size){index = 0;}
			int flag = 0;
			for( ;index <current_size; index++){
				uint32 address = curenv->ptr_pageWorkingSet[index].virtual_address;
				uint32 perms = pt_get_page_permissions(curenv->env_page_directory,address);
				if((perms & PERM_USED)){
					pt_set_page_permissions(curenv->env_page_directory,address,0,PERM_USED);
				}
				else {
					flag = 1;
					if((perms & PERM_MODIFIED)){
						struct FrameInfo * temp = get_frame_info(curenv->env_page_directory,address,&ptrTable);
						pf_update_env_page(curenv,address,temp);
					}
					unmap_frame(curenv->env_page_directory,address);
					env_page_ws_invalidate(curenv,address);
					curenv->page_last_WS_index = index;
					break;
				}
			}
			if(flag) break;
		}
//		env_page_ws_print(curenv);
//		cprintf("==============================================\n\n");

	}

	// [1] Allocate Space in Memory
//	cprintf("=====================PLACEMENT====================\n");
	struct FrameInfo *temp = NULL;
	allocate_frame(&temp);
	map_frame(curenv->env_page_directory , temp , fault_va , PERM_USED|PERM_USER|PERM_WRITEABLE|PERM_MODIFIED);

	// [2] Read page from pagefile
	int result = pf_read_env_page(curenv , (void *)fault_va);

	// [3] check if page not exist
	bool isHeap = fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX;
//	(VA >= USER_HEAP_START && VA < USER_HEAP_MAX)
	bool isStack = fault_va >= USTACKBOTTOM && fault_va < USTACKTOP;
//	cprintf("%d %d\n",isHeap,isStack);
//	(VA >= USTACKBOTTOM && VA < USTACKTOP)

	if(result == E_PAGE_NOT_EXIST_IN_PF ){
		if((fault_va >= USTACKBOTTOM && fault_va < USTACKTOP) || (fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX))
		{
			pf_add_empty_env_page(curenv,fault_va,0);
//			pf_add_env_page(curenv,fault_va,ptr_zero_page);
		}
		else panic("ERROR: ILLEGAL MEMORY ACCESS");
	}

	// [4] Update WS
	uint32 Rounded_fault_va = ROUNDDOWN(fault_va , PAGE_SIZE);
	env_page_ws_set_entry(curenv , curenv->page_last_WS_index , Rounded_fault_va);
	curenv->page_last_WS_index ++;
	index = curenv->page_last_WS_index;
//	env_page_ws_print(curenv);
//	cprintf("==============================================\n\n");

}*/

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	// Write your code here, remove the panic and write your code
	panic("__page_fault_handler_with_buffering() is not implemented yet...!!");
}
