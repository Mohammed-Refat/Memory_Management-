/*
 * dyn_block_management.c
 *
 *  Created on: Sep 21, 2022
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//===========================
// PRINT MEM BLOCK LISTS:
//===========================

void print_mem_block_lists()
{
	cprintf("\n=========================================\n");
	struct MemBlock* blk ;
	struct MemBlock* lastBlk = NULL ;
	cprintf("\nFreeMemBlocksList:\n");
	uint8 sorted = 1 ;
	LIST_FOREACH(blk, &FreeMemBlocksList)
	{
		if (lastBlk && blk->sva < lastBlk->sva + lastBlk->size)
			sorted = 0 ;
		cprintf("[%x, %x)-->", blk->sva, blk->sva + blk->size) ;
		lastBlk = blk;
	}
	if (!sorted)	cprintf("\nFreeMemBlocksList is NOT SORTED!!\n") ;

	lastBlk = NULL ;
	cprintf("\nAllocMemBlocksList:\n");
	sorted = 1 ;
	LIST_FOREACH(blk, &AllocMemBlocksList)
	{
		if (lastBlk && blk->sva < lastBlk->sva + lastBlk->size)
			sorted = 0 ;
		cprintf("[%x, %x)-->", blk->sva, blk->sva + blk->size) ;
		lastBlk = blk;
	}
	if (!sorted)	cprintf("\nAllocMemBlocksList is NOT SORTED!!\n") ;
	cprintf("\n=========================================\n");

}

//********************************************************************************//
//********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
//===============================
// [1] INITIALIZE AVAILABLE LIST:
//===============================
void initialize_MemBlocksList(uint32 numOfBlocks)
{
	struct MemBlock *obj;
		LIST_INIT(&AvailableMemBlocksList);
		for(int i = 0 ; i < numOfBlocks ; i++)
		{
				LIST_INSERT_HEAD(&AvailableMemBlocksList, &(MemBlockNodes[i]));
		}
}

//===============================
// [2] FIND BLOCK:
//===============================
struct MemBlock *find_block(struct MemBlock_List *blockList, uint32 va)
{
	struct MemBlock* obj ;
	LIST_FOREACH(obj, blockList){
		if (obj->sva == va) {
			return obj;
		}
	}
	return NULL;
}

//=========================================
// [3] INSERT BLOCK IN ALLOC LIST [SORTED]:
//=========================================
void insert_sorted_allocList(struct MemBlock *blockToInsert)
{
	if(LIST_SIZE(&(AllocMemBlocksList))==0){
		LIST_INSERT_HEAD(&(AllocMemBlocksList),blockToInsert);
	}
	else{
		struct MemBlock* obj ;
		bool isLast = 1;
		LIST_FOREACH(obj ,&(AllocMemBlocksList) ){
			if(blockToInsert->sva < obj->sva){
				LIST_INSERT_BEFORE(&(AllocMemBlocksList),obj,blockToInsert);
				isLast = 0;
				break;
			}
		}
		if(isLast){
			LIST_INSERT_TAIL(&(AllocMemBlocksList),blockToInsert);
		}
	}
}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
struct MemBlock *alloc_block_FF(uint32 size)
{
	struct MemBlock* obj ;
	LIST_FOREACH(obj,&(FreeMemBlocksList)){
		if(obj->size == size){
			LIST_REMOVE(&(FreeMemBlocksList),obj);
			return obj;
		}
		else if(obj->size > size){
			struct MemBlock* tempAlloc = find_block(&AvailableMemBlocksList,0);

			tempAlloc->size = size;
			tempAlloc->sva = obj->sva;

			// edit size & sva of free mem
			obj->size -= size;
			obj->sva += size;

			LIST_REMOVE(&AvailableMemBlocksList,tempAlloc);
			return tempAlloc;
		}
	}
	return NULL;
}

//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
struct MemBlock *alloc_block_BF(uint32 size)
{
	struct MemBlock* obj ; // itr of list foreach
	struct MemBlock* bestObj ; // best block in freeMem
	uint32 bestSize = UINT_MAX;
	LIST_FOREACH(obj,&(FreeMemBlocksList)){
		if(obj->size == size){ // if equal remove it and return
			LIST_REMOVE(&FreeMemBlocksList,obj);
			return obj;
		}
		else if(obj->size > size){
			if(obj->size <= bestSize){
				bestSize = obj->size;
				bestObj = obj;
			}
		}
	}

	if(bestSize == -1) return NULL; // no block in freeMem has size > size
	else{
		struct MemBlock* tempAlloc = find_block(&AvailableMemBlocksList,0);

		tempAlloc->size = size;
		tempAlloc->sva = bestObj->sva;

		// edit size & sva of free mem
		bestObj->size -= size;
		bestObj->sva += size;

		LIST_REMOVE(&AvailableMemBlocksList,tempAlloc);
		return tempAlloc;
	}
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
struct MemBlock* lastAllocated = NULL;
struct MemBlock *alloc_block_NF(uint32 size)
{
	struct MemBlock* obj ;
	int freeSize = LIST_SIZE(&FreeMemBlocksList);
	int steps = 0;
	if(lastAllocated == NULL) lastAllocated = LIST_FIRST(&FreeMemBlocksList);
	LIST_FOREACH(obj,&FreeMemBlocksList){
		if(obj->sva <= lastAllocated->sva)
			continue;
		steps++;
		if(obj->size == size){
			lastAllocated = obj;
			LIST_REMOVE(&FreeMemBlocksList,obj);
			return obj;
		}
		else if(obj->size > size){
			struct MemBlock* tempAlloc = LIST_FIRST(&AvailableMemBlocksList);

			tempAlloc->size = size;
			tempAlloc->sva = obj->sva;
			lastAllocated = tempAlloc;

			// edit size & sva of free mem
			obj->size -= size;
			obj->sva += size;

			LIST_REMOVE(&AvailableMemBlocksList,tempAlloc);
			return tempAlloc;
		}
	}
	int i = 0;
	LIST_FOREACH(obj,&FreeMemBlocksList){
		if(i >= freeSize - steps) break;
		if(obj->size == size){
			lastAllocated = obj;
			LIST_REMOVE(&FreeMemBlocksList,obj);
			return obj;
		}
		else if(obj->size > size){
			struct MemBlock* tempAlloc = LIST_FIRST(&AvailableMemBlocksList);
			tempAlloc->size = size;
			tempAlloc->sva = obj->sva;
			lastAllocated = tempAlloc;
			// edit size & sva of free mem
			obj->size -= size;
			obj->sva += size;
			LIST_REMOVE(&AvailableMemBlocksList,tempAlloc);
			return tempAlloc;
		}
		i++;
	}
	return NULL;
}

//===================================================
// [8] INSERT BLOCK (SORTED WITH MERGE) IN FREE LIST:
//===================================================
void insert_sorted_with_merge_freeList(struct MemBlock *blockToInsert)
{
	struct MemBlock* temp ;
	if(LIST_SIZE(&(FreeMemBlocksList))==0){ // EMPTY LIST
		LIST_INSERT_HEAD(&(FreeMemBlocksList),blockToInsert);
	}
	else{
		struct MemBlock* obj ;
		int blockEndSva = blockToInsert->sva + blockToInsert->size;
		LIST_FOREACH(obj ,&(FreeMemBlocksList) ){
			int objEndSva = obj->sva + obj->size;
			if(blockEndSva < obj->sva){
				// add before obj and NO merge
				LIST_INSERT_BEFORE(&(FreeMemBlocksList),obj,blockToInsert);
				break;
			}
			else if(blockEndSva == obj->sva){
				// add before obj and MERGE (MERGE NEXT)
				obj->size += blockToInsert->size;
				obj->sva = blockToInsert->sva;
				blockToInsert->size = 0;
				blockToInsert->sva = 0;
				LIST_INSERT_HEAD(&(AvailableMemBlocksList),blockToInsert);
				break;

			}
			else if(objEndSva == blockToInsert->sva){ // Add after obj and
				if(LIST_NEXT(obj) != NULL && blockEndSva == LIST_NEXT(obj)->sva){
					//MERGE (MERGE PREV AND NEXT)
					struct MemBlock *nex ;
					nex = LIST_NEXT(obj);
					obj->size += blockToInsert->size + nex->size ;
					LIST_REMOVE(&(FreeMemBlocksList),nex);
					blockToInsert->size = 0;
					blockToInsert->sva = 0;
					LIST_INSERT_HEAD(&(AvailableMemBlocksList),blockToInsert);
					nex->size = 0;
					nex->sva = 0;
					LIST_INSERT_HEAD(&(AvailableMemBlocksList),nex);
					break;

				}
				else{
					// MERGE (MERGE PREV)
					obj->size += blockToInsert->size;
					blockToInsert->size = 0;
					blockToInsert->sva = 0;
					LIST_INSERT_HEAD(&(AvailableMemBlocksList),blockToInsert);
					break;
				}
			}
			else if(objEndSva < blockToInsert->sva){ // Add after obj and
				if(LIST_NEXT(obj) != NULL && blockEndSva == LIST_NEXT(obj)->sva){
					//MERGE (MERGE NEXT)
					LIST_NEXT(obj)->size += blockToInsert->size;
					LIST_NEXT(obj)->sva = blockToInsert->sva;
					blockToInsert->size = 0;
					blockToInsert->sva = 0;
					LIST_INSERT_HEAD(&(AvailableMemBlocksList),blockToInsert);
					break;
				}
				else if(LIST_NEXT(obj) == NULL || blockEndSva < LIST_NEXT(obj)->sva){
					// NO MERGE
					LIST_INSERT_AFTER(&(FreeMemBlocksList),obj,blockToInsert);
					break;
				}
			}
		}
	}
}
