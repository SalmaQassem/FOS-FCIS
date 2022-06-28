
#include <inc/lib.h>
#define user_heap_size (USER_HEAP_MAX-USER_HEAP_START)/PAGE_SIZE
// malloc()
//	This function use NEXT FIT strategy to allocate space in heap
//  with the given size and return void pointer to the start of the allocated space

//	To do this, we need to switch to the kernel, allocate the required space
//	in Page File then switch back to the user again.
//
//	We can use sys_allocateMem(uint32 virtual_address, uint32 size); which
//		switches to the kernel mode, calls allocateMem(struct Env* e, uint32 virtual_address, uint32 size) in
//		"memory_manager.c", then switch back to the user mode here
//	the allocateMem function is empty, make sure to implement it.


//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
struct User_Heap{
	uint32 start_add;
	int start_index;
	int allocation_size;
};
//int user_heap_size = (USER_HEAP_MAX-USER_HEAP_START)/PAGE_SIZE;
int Arr[user_heap_size]={0};
struct User_Heap User_Heap_Allocations[user_heap_size];
//struct User_Heap_Allocations arr[max_number_of_frames];
int indx = 0;
int next_start = 0;
uint32 next_start_addr = USER_HEAP_START;
void* malloc(uint32 size)
{
	//TODO: [PROJECT 2022 - [9] User Heap malloc()] [User Side]
	// Write your code here, remove the panic and write your code
	//panic("malloc() is not implemented yet...!!");

	// Steps:
	//	1) Implement NEXT FIT strategy to search the heap for suitable space
	//		to the required allocation size (space should be on 4 KB BOUNDARY)
	//	2) if no suitable space found, return NULL
	//	 Else,
	//	3) Call sys_allocateMem to invoke the Kernel for allocation
	// 	4) Return pointer containing the virtual address of allocated space,
	//
	uint32 start;
	int start_indx;
	int Requested_frames = (ROUNDUP(size, 4096))/4096;
	if(sys_isUHeapPlacementStrategyNEXTFIT()){
		int count = 0;
		int found = 0;
		for(int i = next_start; i< user_heap_size; i++){
			if(Arr[i] == 0){
				count += 1;
				for(int j = 1; j< Requested_frames; j++){
					if((i+j >= user_heap_size) && count != Requested_frames){
						found = 0;
						break;
					}
					if(Arr[i+j] == 0){
						count += 1;
					}
				}
				if(count == Requested_frames){
					start = USER_HEAP_START + (i*PAGE_SIZE);
					start_indx = i;
					next_start = i + Requested_frames;
					if(next_start >= user_heap_size){
						next_start = 0;
					}
					found = 1;
					break;
				}
			}
			count = 0;
		}
		if(found == 0){
			count = 0;
			for(int i = 0; i< next_start; i++){
				if(Arr[i] == 0){
					count += 1;
					for(int j = 1; j< Requested_frames; j++){
						if((i+j >= user_heap_size) && count != Requested_frames){
							found = 0;
							break;
						}
						if(Arr[i+j] == 0){
							count += 1;
						}
					}
					if(count == Requested_frames){
						start = USER_HEAP_START + (i*PAGE_SIZE);
						start_indx = i;
						next_start = i + Requested_frames;
						if(next_start >= user_heap_size){
							next_start = 0;
						}
						found = 1;
						break;
					}
				}
				count = 0;
			}
		}
		if(found == 0){
			return NULL;
		}
		else{
			User_Heap_Allocations[indx].start_add = start;
			User_Heap_Allocations[indx].start_index = start_indx;
			User_Heap_Allocations[indx].allocation_size = Requested_frames;
			indx += 1;

			sys_allocateMem(start, Requested_frames);
			for(int i=0; i<Requested_frames; i++) {
				Arr[start_indx] = 1;
				start_indx += 1;
			}
		}
		return (void*)start;
	}
	if(sys_isUHeapPlacementStrategyBESTFIT()){
		int best_size = 10000000;
		uint32 best_addr;
		int best_index;
		int count =0;
		int found = 0;
		uint32 start;
		for(int i = 0; i< user_heap_size; i++){
			if(Arr[i] == 0){
				count += 1;
				if(count == 1){
					start = USER_HEAP_START + i*PAGE_SIZE;
					start_indx = i;
				}
			}
			else{
				if(count == Requested_frames){
					best_size = count;
					best_addr = start;
					best_index = start_indx;
					count = 0;
					found = 1;
					break;
				}
			    else if(count > Requested_frames && count < best_size){
					best_size = count;
					best_addr = start;
					best_index = start_indx;
					found = 1;
				}
				count = 0;
				start = -1;
			}
		}
		if(count == 0){
			if(found != 1){
				return NULL;
			}
		}
		if(count > 0){
			if(found != 1 && count < Requested_frames)
				return NULL;
		}
		if(count > 0 && count >= Requested_frames && count < best_size){
			best_size = count;
			best_addr = start;
			best_index = start_indx;
		}
		User_Heap_Allocations[indx].allocation_size = Requested_frames;
		User_Heap_Allocations[indx].start_add = best_addr;
		User_Heap_Allocations[indx].start_index = best_index;
		indx += 1;

		for(int i=0; i<Requested_frames; i++) {
			Arr[best_index] = 1;
			best_index += 1;
		}
		sys_allocateMem(best_addr, Requested_frames);
		return (void*)best_addr;
	}
	return NULL;
	//This function should find the space of the required range
	// ******** ON 4KB BOUNDARY ******************* //

	//Use sys_isUHeapPlacementStrategyNEXTFIT() and
	//sys_isUHeapPlacementStrategyBESTFIT() for the bonus
	//to check the current strategy
}


void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	panic("smalloc() is not required ..!!");
	return NULL;
}

void* sget(int32 ownerEnvID, char *sharedVarName)
{
	panic("sget() is not required ..!!");
	return 0;
}

// free():
//	This function frees the allocation of the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from page file and main memory then switch back to the user again.
//
//	We can use sys_freeMem(uint32 virtual_address, uint32 size); which
//		switches to the kernel mode, calls freeMem(struct Env* e, uint32 virtual_address, uint32 size) in
//		"memory_manager.c", then switch back to the user mode here
//	the freeMem function is empty, make sure to implement it.

void free(void* virtual_address)
{
	//TODO: [PROJECT 2022 - [11] User Heap free()] [User Side]
	// Write your code here, remove the panic and write your code
	//panic("free() is not implemented yet...!!");
	int size;
	int index;
	uint32 ret;
	//uint32 va = ROUNDDOWN((uint32)virtual_address, 4096);
	for(int i=0; i<user_heap_size; i++){
		uint32 end = User_Heap_Allocations[i].start_add + ((User_Heap_Allocations[i].allocation_size)*PAGE_SIZE);
		if(((uint32)virtual_address >= User_Heap_Allocations[i].start_add)&&((uint32)virtual_address<end)){
			size = User_Heap_Allocations[i].allocation_size;
			index = User_Heap_Allocations[i].start_index;
			ret = User_Heap_Allocations[i].start_add;
			User_Heap_Allocations[i].allocation_size = 0;
			User_Heap_Allocations[i].start_index = -1;
			User_Heap_Allocations[i].start_add = -1;
			break;
		}
	}
	for(int i=0; i<size; i++){
		Arr[index] = 0;
		index += 1;
	}
	sys_freeMem(ret,size);
	//you shold get the size of the given allocation using its address
	//you need to call sys_freeMem()
	//refer to the project presentation and documentation for details

}


void sfree(void* virtual_address)
{
	panic("sfree() is not requried ..!!");
}


//===============
// [2] realloc():
//===============

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_moveMem(uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
//		which switches to the kernel mode, calls moveMem(struct Env* e, uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
//		in "memory_manager.c", then switch back to the user mode here
//	the moveMem function is empty, make sure to implement it.

void *realloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT 2022 - BONUS3] User Heap Realloc [User Side]
	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");

	return NULL;
}
