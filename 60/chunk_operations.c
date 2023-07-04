/*
 * chunk_operations.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include <kern/trap/fault_handler.h>
#include <kern/disk/pagefile_manager.h>
#include "kheap.h"
#include "memory_manager.h"


/******************************/
/*[1] RAM CHUNKS MANIPULATION */
/******************************/

//===============================
// 1) CUT-PASTE PAGES IN RAM:
//===============================
//This function should cut-paste the given number of pages from source_va to dest_va
//if the page table at any destination page in the range is not exist, it should create it
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
int cut_paste_pages(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 num_of_pages)
{
	uint32 *ptr_page_table = NULL;
	int start_source_va = ROUNDDOWN(source_va , PAGE_SIZE);
	int start_dest_va = ROUNDDOWN(dest_va , PAGE_SIZE);

	//if dest exist
	int temp_itr =  start_dest_va;
	for(int i = 0 ; i< num_of_pages ; i++){
		if(get_frame_info(page_directory , temp_itr , &ptr_page_table) != NULL)
			return -1;
		temp_itr += PAGE_SIZE;
	}

	// check page table existence
	temp_itr =  start_dest_va;
	for(int i = 0 ; i< num_of_pages ; i++){
		get_page_table(page_directory , temp_itr , &ptr_page_table);
		if(ptr_page_table == NULL) //if no page table then create it
			create_page_table(page_directory , temp_itr);
		temp_itr += PAGE_SIZE;
	}

	int source_itr = start_source_va;
	int dest_itr = start_dest_va;
	for(int i = 0 ; i< num_of_pages ; i++){
		int perms = pt_get_page_permissions(page_directory , source_itr);
		unmap_frame(page_directory , source_itr);
		struct FrameInfo *temp = NULL;
		allocate_frame(&temp);
		map_frame(page_directory , temp , dest_itr , perms);
		source_itr += PAGE_SIZE;
		dest_itr += PAGE_SIZE;
	}
	return 0;

}

//===============================
// 2) COPY-PASTE RANGE IN RAM:
//===============================
//This function should copy-paste the given size from source_va to dest_va
//if the page table at any destination page in the range is not exist, it should create it
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
int copy_paste_chunk(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 size)
{
	int source_end_addresse = source_va + size;

	int temp_source_itr = source_va;
	int temp_dest_itr = dest_va;
	while(temp_source_itr < source_end_addresse){

		uint32 *ptr_page_table = NULL;
		get_page_table(page_directory , temp_dest_itr , &ptr_page_table);
		if(ptr_page_table == NULL)
			create_page_table(page_directory , temp_dest_itr);

		struct FrameInfo *temp = get_frame_info(page_directory , temp_dest_itr , &ptr_page_table);

		if(temp != NULL){
			int perms = pt_get_page_permissions(page_directory , temp_dest_itr);
			int writable_bit = perms & PERM_WRITEABLE;
			if(writable_bit == 0)
				return -1;
			if(writable_bit == 1)
				continue;
		}

		if(temp == NULL){
			allocate_frame(&temp);
			int perms = pt_get_page_permissions(page_directory , temp_source_itr);
			map_frame(page_directory , temp , temp_dest_itr , perms|PERM_WRITEABLE);
		}
		temp_source_itr += PAGE_SIZE;
		temp_dest_itr += PAGE_SIZE;
	}

	while(source_va < source_end_addresse){

		unsigned char *ptr_source_content = (unsigned char *)source_va;
		unsigned char content = *ptr_source_content;
		unsigned char *ptr_dest_content = (unsigned char *)dest_va;
		*ptr_dest_content = content;

		ptr_source_content++;
		ptr_dest_content++;
		source_va++;
		dest_va++;
	}
	return 0;
}

//===============================
// 3) SHARE RANGE IN RAM:
//===============================
//This function should share the given size from dest_va with the source_va
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
int share_chunk(uint32* page_directory, uint32 source_va,uint32 dest_va, uint32 size, uint32 perms)
{
	uint32 *ptr_page_table = NULL;

	int start_source_va = ROUNDDOWN(source_va,PAGE_SIZE);
	int end_source_va = ROUNDUP(source_va + size,PAGE_SIZE);
	int start_dest_va = ROUNDDOWN(dest_va,PAGE_SIZE);
	int end_dest_va = ROUNDUP(dest_va + size,PAGE_SIZE);

	//if dest exist
	int temp_itr =  start_dest_va;
	while (temp_itr < end_dest_va){
		if(get_frame_info(page_directory , temp_itr , &ptr_page_table) != NULL)
			return -1;
		temp_itr += PAGE_SIZE;
	}

	// check page table existence
	temp_itr =  start_dest_va;
	while (temp_itr < end_dest_va){
		get_page_table(page_directory , temp_itr , &ptr_page_table);
		if(ptr_page_table == NULL) //if no page table then create it
			create_page_table(page_directory , temp_itr);
		temp_itr += PAGE_SIZE;
	}

	int source_itr = start_source_va;
	int dest_itr = start_dest_va;
	while (dest_itr < end_dest_va){
		struct FrameInfo *temp = get_frame_info(page_directory,source_itr,&ptr_page_table);
		map_frame(page_directory , temp , dest_itr , perms);
		source_itr += PAGE_SIZE;
		dest_itr += PAGE_SIZE;
		}
	return 0;
}

//===============================
// 4) ALLOCATE CHUNK IN RAM:
//===============================
//This function should allocate in RAM the given range [va, va+size)
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
int allocate_chunk(uint32* page_directory, uint32 va, uint32 size, uint32 perms)
{
	int start_Addresse = ROUNDDOWN(va , PAGE_SIZE);
	int End_Addresse = ROUNDUP((va + size) , PAGE_SIZE);
	int iterator = start_Addresse;
	int Available_space = free_frame_list.size * PAGE_SIZE;
	int Required_space = (va + size) - va;

	if(Available_space < Required_space)
		return -1;

	while(iterator < End_Addresse ){
		uint32 *ptr_page_table = NULL;
		get_page_table(page_directory , iterator , &ptr_page_table);
		if(ptr_page_table == NULL) //if no page table then create it
			create_page_table(page_directory , iterator);

		if(get_frame_info(page_directory , iterator , &ptr_page_table) != NULL) //if this frame is allocated then return -1
			return -1;
		else{//if this frame is not allocated then allocate it
			struct FrameInfo *temp = NULL;
			allocate_frame(&temp);
			temp->va = iterator;
			map_frame(page_directory , temp , iterator , perms);
		}
		iterator += PAGE_SIZE;
	}
	return 0;

	/*uint32 v1=ROUNDDOWN(va,PAGE_SIZE);
	uint32 v2=va+size;
	v2=ROUNDUP(v2,PAGE_SIZE);
	uint32* ptr;
	if(free_frame_list.size<(size/PAGE_SIZE))
		return -1;
	for(uint32 i=v1;i<v2;i+=PAGE_SIZE){

		struct FrameInfo *ptrframe=get_frame_info(page_directory,i,&ptr);
		if(ptr==NULL)
			ptr=create_page_table(page_directory,i);
		if(ptrframe!=NULL)
			return -1;
			int ret = allocate_frame(&ptrframe);
			if(ret==E_NO_MEM)
				return -1;
			map_frame(page_directory, ptrframe, i, perms);
			ptrframe->va=i;
	}
	return 0;*/
}

/*BONUS*/
//=====================================
// 5) CALCULATE ALLOCATED SPACE IN RAM:
//=====================================
void calculate_allocated_space(uint32* page_directory, uint32 sva, uint32 eva, uint32 *num_tables, uint32 *num_pages)
{
	uint32 *ptr_page_table;
	const int TABLE_SIZE = PAGE_SIZE * 1024;
	uint32 pages = 0;
	uint32 tables = 0;
	int pages_itr = ROUNDDOWN(sva , PAGE_SIZE);
	int tables_itr = ROUNDDOWN(sva , TABLE_SIZE);

	while(pages_itr < eva){
		struct FrameInfo *temp = get_frame_info(page_directory,pages_itr , &ptr_page_table);
		if (temp != NULL)
			(pages)++;
		pages_itr += PAGE_SIZE;
	}

	while(tables_itr < eva){
		get_page_table(page_directory,tables_itr , &ptr_page_table);
		if (ptr_page_table != NULL)
			(tables)++;
		tables_itr += TABLE_SIZE;
	}
	*num_pages = pages;
	*num_tables = tables;
}

/*BONUS*/
//=====================================
// 6) CALCULATE REQUIRED FRAMES IN RAM:
//=====================================
// calculate_required_frames:
// calculates the new allocation size required for given address+size,
// we are not interested in knowing if pages or tables actually exist in memory or the page file,
// we are interested in knowing whether they are allocated or not.
uint32 calculate_required_frames(uint32* page_directory, uint32 sva, uint32 size)
{
	uint32 *ptr_page_table;
	const int TABLE_SIZE = PAGE_SIZE * 1024;
	int pages = 0;
	int tables = 0;
	int pages_itr = ROUNDDOWN(sva , PAGE_SIZE);
	int tables_itr = ROUNDDOWN(sva , TABLE_SIZE);

	while(pages_itr < (sva + size)){
		struct FrameInfo *temp = get_frame_info(page_directory,pages_itr , &ptr_page_table);
		if (temp == NULL)
			pages++;
		pages_itr += PAGE_SIZE;
	}

	while(tables_itr < (sva + size)){
		get_page_table(page_directory,tables_itr , &ptr_page_table);
		if (ptr_page_table == NULL)
			tables++;
		tables_itr += TABLE_SIZE;
	}
	return (pages + tables);
}

//=================================================================================//
//===========================END RAM CHUNKS MANIPULATION ==========================//
//=================================================================================//

/*******************************/
/*[2] USER CHUNKS MANIPULATION */
/*******************************/

//======================================================
/// functions used for USER HEAP (malloc, free, ...)
//======================================================

//=====================================
// 1) ALLOCATE USER MEMORY:
//=====================================
void allocate_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{
	// Write your code here, remove the panic and write your code
	panic("allocate_user_mem() is not implemented yet...!!");
}

//=====================================
// 2) FREE USER MEMORY:
//=====================================
void free_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{
	//TODO: [PROJECT MS3] [USER HEAP - KERNEL SIDE] free_user_mem
	// Write your code here, remove the panic and write your code
//	panic("free_user_mem() is not implemented yet...!!");

	int sva = ROUNDDOWN( virtual_address , PAGE_SIZE);
			int eva = ROUNDUP((virtual_address + size ) , PAGE_SIZE );
			uint32 *page_table = NULL;

			int itr = sva;
			while(itr < eva)//ram and page file
			{
				pf_remove_env_page(e , itr);
				unmap_frame(e->env_page_directory  , itr);
				itr += PAGE_SIZE;
			}

			itr = sva;
			while(itr < eva)//ws
			{
				int i=0;
				for(; i<e->page_WS_max_size;i++)
				{
					if(e->ptr_pageWorkingSet[i].virtual_address == itr )
					{
						e->page_last_WS_index =i;
						env_page_ws_clear_entry(e , e->page_last_WS_index);
					}
					else
						continue;
				}
				itr += PAGE_SIZE;
			}

			itr = sva;
			while(itr < eva)//empty tables
			{
				get_page_table(e->env_page_directory , itr , &page_table);

				if(page_table != NULL)
				{
					int table_is_empty = 1;

					int i = 0;
					for(; i < 1024 ; i++)
					{
						if(page_table[i] != 0)
						{
							table_is_empty = 0;
							break;
						}

						else
							continue;
					}

					if(table_is_empty == 1)
					{
						kfree(page_table);
						pd_clear_page_dir_entry(e->env_page_directory , itr);
					}

				}
				itr += PAGE_SIZE;
			}



/*	//This function should:
	//1. Free ALL pages of the given range from the Page File
	uint32 n = size / PAGE_SIZE;
		uint32 va = virtual_address;
		for(int i=0; i<n;i++)
		{
//			cprintf("loop1");
			pf_remove_env_page(e, va);
			va +=PAGE_SIZE;
		}

		//2. Free ONLY pages that are resident in the working set from the memory
		struct FrameInfo* ptr_frame_info = NULL;
		uint32* ptr_page_table;
		va=virtual_address;
		for(int i=0; i<e->page_WS_max_size; i++)
		{
//			cprintf("loop2");
			uint32 va_of_ws=env_page_ws_get_virtual_address(e,i);
			if((va_of_ws>=va) && (va_of_ws<(va+ROUNDUP(size,PAGE_SIZE))))
			{
				unmap_frame(e->env_page_directory,va_of_ws);
				env_page_ws_clear_entry(e,i);
			}
		}

		//3. Removes ONLY the empty page tables (i.e. not used) (no pages are mapped in the table)
		va= virtual_address;
		for(int i=0; i<n; i++)
		{
//			cprintf("loop3");
			int flag=1;
			ptr_page_table=NULL;
			get_page_table(e->env_page_directory,va,&ptr_page_table);
			if(ptr_page_table !=NULL){
				for(int j=0; j<1024; j++)
				{
//					cprintf("loop5");
					if(ptr_page_table[j] != 0) {
						flag=0;
						break;
					}
				}
				if(flag){
					kfree((void*)ptr_page_table);
					pd_clear_page_dir_entry(e->env_page_directory,(uint32)va);
				}
			}
			va+=PAGE_SIZE;
		}
		//Refresh the cache memory
		tlbflush();*/

	/*uint32 n = size / PAGE_SIZE;
		uint32 va = virtual_address;
		for(int i=0; i<n;i++)
		{
//			cprintf("loop1");
			pf_remove_env_page(e, va);
			va +=PAGE_SIZE;
		}
	//2. Free ONLY pages that are resident in the working set from the memory
		struct Frame_Info* ptr_frame_info = NULL;
			uint32* ptr_page_table;
			va=virtual_address;
			for(int i=0; i<e->page_WS_max_size; i++)
			{
//				cprintf("loop2");
				uint32 va_of_ws=env_page_ws_get_virtual_address(e,i);
				if((va_of_ws>=va) && (va_of_ws<(va+ROUNDUP(size,PAGE_SIZE))))
				{
					unmap_frame(e->env_page_directory,va_of_ws);
					env_page_ws_clear_entry(e,i);
				}
			}
	//3. Removes ONLY the empty page tables (i.e. not used) (no pages are mapped in the table)
			va= virtual_address;
				for(int i=0; i<n; i++)
				{
//					cprintf("loop3");
					int flag=1;
					ptr_page_table=NULL;
					get_page_table(e->env_page_directory,va,&ptr_page_table);
					if(ptr_page_table !=NULL){
						for(int j=0; j<1024; j++)
						{
//							cprintf("loop5");
							if(ptr_page_table[j] != 0) {
								flag=0;
								break;
							}
						}
						if(flag){
							kfree((void*)ptr_page_table);
							pd_clear_page_dir_entry(e->env_page_directory,(uint32)va);
						}
					}
					va+=PAGE_SIZE;
				}
				tlbflush();*/
}

//=====================================
// 2) FREE USER MEMORY (BUFFERING):
//=====================================
void __free_user_mem_with_buffering(struct Env* e, uint32 virtual_address, uint32 size)
{
	// your code is here, remove the panic and write your code
	panic("__free_user_mem_with_buffering() is not implemented yet...!!");

	//This function should:
	//1. Free ALL pages of the given range from the Page File
	//2. Free ONLY pages that are resident in the working set from the memory
	//3. Free any BUFFERED pages in the given range
	//4. Removes ONLY the empty page tables (i.e. not used) (no pages are mapped in the table)
}

//=====================================
// 3) MOVE USER MEMORY:
//=====================================
void move_user_mem(struct Env* e, uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
{
	//TODO: [PROJECT MS3 - BONUS] [USER HEAP - KERNEL SIDE] move_user_mem
	//your code is here, remove the panic and write your code
	panic("move_user_mem() is not implemented yet...!!");

	// This function should move all pages from "src_virtual_address" to "dst_virtual_address"
	// with the given size
	// After finished, the src_virtual_address must no longer be accessed/exist in either page file
	// or main memory

	/**/
}

//=================================================================================//
//========================== END USER CHUNKS MANIPULATION =========================//
//=================================================================================//

