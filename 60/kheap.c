#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

//==================================================================//
//==================================================================//
//NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)//
//==================================================================//
//==================================================================//

void initialize_dyn_block_system()
{
	const int KERNEL_HEAP_SIZE = KERNEL_HEAP_MAX - KERNEL_HEAP_START;

	//[1] Initialize two lists (AllocMemBlocksList & FreeMemBlocksList)
	LIST_INIT(&FreeMemBlocksList);
	LIST_INIT(&AllocMemBlocksList);
#if STATIC_MEMBLOCK_ALLOC

#else
	// [2] : set MAX_MEM_BLOCK_CNT with the chosen size of the array
	MAX_MEM_BLOCK_CNT = KERNEL_HEAP_SIZE / PAGE_SIZE;

	// [3] : assign starting address of MemBlockNodes array
	MemBlockNodes= (struct MemBlock*) KERNEL_HEAP_START;

	// [4] : calculate the total size of memory required for the MemBlockNodes array
	uint32 memSize = ROUNDUP((MAX_MEM_BLOCK_CNT * sizeof(struct MemBlock)), PAGE_SIZE);

	// [5] : allocate_chunk for this total memory size, with correct startAddress
	allocate_chunk(ptr_page_directory, KERNEL_HEAP_START, memSize, PERM_WRITEABLE);

	// [6] : initialize_MemBlocksList with the total number of MemBlockNodes
	initialize_MemBlocksList(MAX_MEM_BLOCK_CNT);

	// [7] : Take a block from the AvailableMemBlocksList and fill its size with all of the heap size (without size allocated for the array)
	struct MemBlock *KHEAP = LIST_FIRST(&AvailableMemBlocksList);
	LIST_REMOVE(&AvailableMemBlocksList, KHEAP);
	KHEAP->size = KERNEL_HEAP_SIZE - memSize;
	KHEAP->sva = KERNEL_HEAP_START + memSize;

	//[8] : Finally insert this block to the FreeMemBlocksList instead of AvailableMemBlocksList
	LIST_INSERT_HEAD(&FreeMemBlocksList,KHEAP);
#endif
}

void* kmalloc(unsigned int size)
{
	//NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)
	//refer to the project presentation and documentation for details
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy
	//change this "return" according to your answer

	size=ROUNDUP(size,PAGE_SIZE);
	struct MemBlock *allocated_block;
	if(isKHeapPlacementStrategyFIRSTFIT())
		allocated_block = alloc_block_FF(size);

	else if(isKHeapPlacementStrategyBESTFIT())
		allocated_block = alloc_block_BF(size);

	else if(isKHeapPlacementStrategyNEXTFIT())
		allocated_block = alloc_block_NF(size);

	if(allocated_block !=NULL)
	{
		insert_sorted_allocList(allocated_block);
		allocate_chunk(ptr_page_directory, allocated_block->sva, size, PERM_WRITEABLE);
		return (void*)allocated_block->sva;
	}
	return NULL;
}

void kfree(void* virtual_address)
{
	struct MemBlock * get_block = find_block(&AllocMemBlocksList , (uint32)virtual_address);

		if(get_block != NULL)
			LIST_REMOVE(&AllocMemBlocksList , get_block);

		int temp_itr = get_block->sva;
		int end_addresse = get_block->sva + get_block->size;

		while(temp_itr < end_addresse){
			unmap_frame(ptr_page_directory , temp_itr);
			temp_itr += PAGE_SIZE;
		}
		insert_sorted_with_merge_freeList(get_block);
	/*uint32 sva = (uint32)virtual_address;
	struct MemBlock * block = find_block(&AllocMemBlocksList,sva);
	if(block != NULL){
		LIST_REMOVE(&AllocMemBlocksList,block);
		int num_of_pages = block->size / PAGE_SIZE;
		for(int i = 0 ; i< num_of_pages;i++){
			unmap_frame(ptr_page_directory,sva);
			sva+= PAGE_SIZE;
		}
		insert_sorted_with_merge_freeList(block);
	}*/
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//return the virtual address corresponding to given physical_address
	//refer to the project presentation and documentation for details
	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	struct FrameInfo * temp = to_frame_info(physical_address);
	return temp->va;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details

	return virtual_to_physical(ptr_page_directory,virtual_address);
}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT MS2 - BONUS] [KERNEL HEAP] krealloc
	// Write your code here, remove the panic and write your code
//	panic("krealloc() is not implemented yet...!!");
	uint32 freeSize = FreeMemBlocksList.size * PAGE_SIZE;
	if(new_size == 0) { kfree(virtual_address); return NULL;}
	if(virtual_address == NULL) return kmalloc(new_size);
//	if(new_size > freeSize) return NULL;
//	else{
		struct MemBlock * alloc_block = find_block(&AllocMemBlocksList,(uint32)virtual_address);
//		cprintf("\n%d %x %d %d\n",alloc_block->size,alloc_block->sva,new_size,freeSize);
		if(new_size <= alloc_block->size){
			struct MemBlock * free_block  = LIST_FIRST(&AvailableMemBlocksList);
			free_block->size =  alloc_block->size - new_size;
			alloc_block->size = new_size;
			free_block->sva = alloc_block->sva + new_size;
			insert_sorted_with_merge_freeList(free_block);
			return (void *)alloc_block->sva;
		}
		else{
			struct MemBlock * nextalloc_block = LIST_NEXT(alloc_block);
			uint32 space = nextalloc_block->sva - (alloc_block->sva + alloc_block->size);
//			cprintf("\n%d %x %d %d\n",alloc_block->size,alloc_block->sva,new_size,freeSize);
//			cprintf("\n%d %x %d %d\n",nextalloc_block->size,nextalloc_block->sva,new_size,freeSize);
			if(space >= new_size){
				struct MemBlock *free_block = find_block(&FreeMemBlocksList,alloc_block->sva + alloc_block->size);
//				cprintf("\n%d %d\n",space,free_block->size);
				free_block->size =  nextalloc_block->sva - alloc_block->sva - new_size;
				alloc_block->size = new_size;
				free_block->sva = alloc_block->sva + new_size;
				return (void *)alloc_block->sva;
			}
//			cprintf("OUT\n");

		}
		return NULL;
//	}


}
