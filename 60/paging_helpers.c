/*
 * paging_helpers.c
 *
 *  Created on: Sep 30, 2022
 *      Author: HP
 */
#include "memory_manager.h"

/*[2.1] PAGE TABLE ENTRIES MANIPULATION */
inline void pt_set_page_permissions(uint32* page_directory, uint32 virtual_address, uint32 permissions_to_set, uint32 permissions_to_clear)
{
	uint32 *ptr_page_table = NULL;
	get_page_table(page_directory,virtual_address,&ptr_page_table);
	if(ptr_page_table != NULL){
//		uint32 clr = 0x00000FFF & (~permissions_to_clear);
		ptr_page_table[PTX(virtual_address)] &= (~permissions_to_clear);
		ptr_page_table[PTX(virtual_address)] |= permissions_to_set;
		tlb_invalidate((void *)page_directory,(void *)virtual_address);
	}
	else{
		panic("Invalid virtual address");
	}
}

inline int pt_get_page_permissions(uint32* page_directory, uint32 virtual_address )
{
	uint32 *ptr_page_table = NULL;
	get_page_table(page_directory,virtual_address,&ptr_page_table);
	if(ptr_page_table != NULL){
		return ptr_page_table[PTX(virtual_address)] & 0x00000FFF;
	}
	else
		return -1;
}

inline void pt_clear_page_table_entry(uint32* page_directory, uint32 virtual_address)
{
	uint32 *ptr_page_table = NULL;
	get_page_table(page_directory,virtual_address,&ptr_page_table);
	if(ptr_page_table != NULL){
		ptr_page_table[PTX(virtual_address)]=0;
		tlb_invalidate((void *)page_directory,(void *)virtual_address);
	}
	else{
		panic("Invalid virtual address");
	}
}

/***********************************************************************************************/

/*[2.2] ADDRESS CONVERTION*/
inline int virtual_to_physical(uint32* page_directory, uint32 virtual_address)
{
//	cprintf("PAGE VIR 2 PHYS\n");
	uint32 *ptr_page_table = NULL;
	get_page_table(page_directory,virtual_address,&ptr_page_table);
	if(ptr_page_table != NULL){
		int frame_num = ptr_page_table[PTX(virtual_address)]>>12;
		return frame_num * PAGE_SIZE;
		//since phyAdd = frame# * size of the frame which equal to the size of the page
	}
	else
		return -1;
}

/***********************************************************************************************/

/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/

///============================================================================================
/// Dealing with page directory entry flags

inline uint32 pd_is_table_used(uint32* page_directory, uint32 virtual_address)
{
	return ( (page_directory[PDX(virtual_address)] & PERM_USED) == PERM_USED ? 1 : 0);
}

inline void pd_set_table_unused(uint32* page_directory, uint32 virtual_address)
{
	page_directory[PDX(virtual_address)] &= (~PERM_USED);
	tlb_invalidate((void *)NULL, (void *)virtual_address);
}

inline void pd_clear_page_dir_entry(uint32* page_directory, uint32 virtual_address)
{
	page_directory[PDX(virtual_address)] = 0 ;
	tlbflush();
}
