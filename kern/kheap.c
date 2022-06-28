#include <inc/memlayout.h>
#include <kern/kheap.h>
#include <kern/memory_manager.h>
#define kernel_heap_size (KERNEL_HEAP_MAX-KERNEL_HEAP_START)/PAGE_SIZE
//2022: NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)
struct Kernel_Heap{
	uint32 start_add;
	int start_index;
	int allocation_size;
};
int Arr[kernel_heap_size]={0};
struct Kernel_Heap Kernel_Heap_Allocations[kernel_heap_size];

int indx = 0;
int next_start = 0;
uint32 next_start_addr = KERNEL_HEAP_START;
void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT 2022 - [1] Kernel Heap] kmalloc()
	// Write your code here, remove the panic and write your code
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");
	uint32 start;
	int start_indx;
	int empty = 0;
	int Requested_frames = (ROUNDUP(size, 4096))/4096;
	for(int i = 0; i<kernel_heap_size; i++){
		if(Arr[i] == 0){
			empty += 1;
		}
	}
	if(Requested_frames > empty)
		return NULL;

	if(isKHeapPlacementStrategyNEXTFIT()){
		int count = 0;
		int found = 0;
		for(int i = next_start; i< kernel_heap_size; i++){
			if(Arr[i] == 0){
				count += 1;
				for(int j = 1; j< Requested_frames; j++){
					if((i+j >= kernel_heap_size) && count != Requested_frames){
						found = 0;
						break;
					}
					if(Arr[i+j] == 0){
						count += 1;
					}
				}
				if(count == Requested_frames){
					start = KERNEL_HEAP_START + (i*PAGE_SIZE);
					start_indx = i;
					next_start = i + Requested_frames;
					if(next_start >= kernel_heap_size){
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
						if((i+j >= next_start) && count !=Requested_frames){
							found = 0;
							break;
						}
						if(Arr[i+j] == 0){
							count += 1;
						}
					}
					if(count == Requested_frames){
						start = KERNEL_HEAP_START + (i*PAGE_SIZE);
						start_indx = i;
						next_start = i + Requested_frames;
						if(next_start >= kernel_heap_size){
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
			Kernel_Heap_Allocations[indx].start_add = start;
			Kernel_Heap_Allocations[indx].start_index = start_indx;
			Kernel_Heap_Allocations[indx].allocation_size = Requested_frames;
			indx += 1;

			for(int i = 0; i<Requested_frames; i++) {
				Arr[start_indx] = 1;
				start_indx += 1;
			}

			uint32* ptr = NULL;
			struct Frame_Info* ptr_frame_info;
			int s = Requested_frames;
			uint32 Addr = start;
			for(int i =0; i<s ;i++){
				ptr_frame_info = get_frame_info(ptr_page_directory, (void*)Addr, &ptr);
				allocate_frame(&ptr_frame_info);
				map_frame(ptr_page_directory, ptr_frame_info, (void*)Addr, PERM_USED|PERM_WRITEABLE);
				Addr += PAGE_SIZE;
			}
			return (void*)start;
		}
	}
	if(isKHeapPlacementStrategyBESTFIT()){
		int best_size = 10000000;
		uint32 best_addr;
		int best_index;
		int count =0;
		int found = 0;
		uint32 start;
		for(int i = 0; i< kernel_heap_size; i++){
			if(Arr[i] == 0){
				count += 1;
				if(count == 1){
					start = KERNEL_HEAP_START + i*PAGE_SIZE;
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
		Kernel_Heap_Allocations[indx].allocation_size = Requested_frames;
		Kernel_Heap_Allocations[indx].start_add = best_addr;
		Kernel_Heap_Allocations[indx].start_index = best_index;
		indx += 1;

		for(int i=0; i<Requested_frames; i++) {
			Arr[best_index] = 1;
			best_index += 1;
		}
		uint32 *ptr1 = NULL;
		struct Frame_Info* ptr_frame_info;
		uint32 Addr = best_addr;
		for(int i =0; i<Requested_frames; i++){
			ptr_frame_info = get_frame_info(ptr_page_directory, (void*)Addr, &ptr1);
			allocate_frame(&ptr_frame_info);
			map_frame(ptr_page_directory, ptr_frame_info, (void*)Addr, PERM_USED|PERM_WRITEABLE);
			Addr += PAGE_SIZE;
		}
		return (void*)best_addr;
	}
	return NULL;
	//NOTE: Allocation using NEXTFIT strategy
	//NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)
	//refer to the project presentation and documentation for details


	//TODO: [PROJECT 2022 - BONUS1] Implement a Kernel allocation strategy
	// Instead of the Next allocation/deallocation, implement
	// BEST FIT strategy
	// use "isKHeapPlacementStrategyBESTFIT() ..."
	// and "isKHeapPlacementStrategyNEXTFIT() ..."
	//functions to check the current strategy
	//change this "return" according to your answer
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT 2022 - [2] Kernel Heap] kfree()
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");

	//you need to get the size of the given allocation using its address
	//refer to the project presentation and documentation for details
	int size;
	int index;
	//uint32 va= ROUNDDOWN((uint32)virtual_address, 4096);
	uint32 ret;
	for(int i=0; i<kernel_heap_size; i++){
		uint32 end = Kernel_Heap_Allocations[i].start_add + ((Kernel_Heap_Allocations[i].allocation_size)*PAGE_SIZE);
		if(((uint32)virtual_address >= Kernel_Heap_Allocations[i].start_add)&&((uint32)virtual_address<end)){
			size = Kernel_Heap_Allocations[i].allocation_size;
			index = Kernel_Heap_Allocations[i].start_index;
			ret = Kernel_Heap_Allocations[i].start_add;
			Kernel_Heap_Allocations[i].allocation_size = 0;
			Kernel_Heap_Allocations[i].start_index = -1;
			Kernel_Heap_Allocations[i].start_add = -1;
			break;
		}
	}
	for(int i = 0; i<size; i++){
		Arr[index] = 0;
		index += 1;
	}
	uint32* ptr = NULL;
	struct Frame_Info* ptr_frame_info;
	for(int i = 0; i<size; i++){
		ptr_frame_info = get_frame_info(ptr_page_directory, (void*)ret, &ptr);
		if(ptr_frame_info != NULL){
		    free_frame(ptr_frame_info);
		    unmap_frame(ptr_page_directory,(void*)ret);
		}
		ret += PAGE_SIZE;
	}
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT 2022 - [3] Kernel Heap] kheap_virtual_address()
	// Write your code here, remove the panic and write your code
	//panic("kheap_virtual_address() is not implemented yet...!!");

	//return the virtual address corresponding to given physical_address
	//refer to the project presentation and documentation for details
	unsigned int pa = ROUNDDOWN(physical_address, PAGE_SIZE);
	int frame_number = pa/PAGE_SIZE;
	unsigned int va;
	int found = 0;
	uint32* ptr = NULL;
	for(unsigned int i = KERNEL_HEAP_START; i<KERNEL_HEAP_MAX; i+= PAGE_SIZE){
		if(get_page_table(ptr_page_directory, (void*)i,&ptr) == 0){
			uint32 entry = ptr[PTX(i)];
			int Page_frame_number = entry >> 12;
			if(Page_frame_number == frame_number){
				va = i;
				found = 1;
				break;
			}
		}
	}
	if(found == 1)
	    return va;
	else
		return 0;

	//change this "return" according to your answer
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT 2022 - [4] Kernel Heap] kheap_physical_address()
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");
	unsigned int va = ROUNDDOWN(virtual_address, PAGE_SIZE);
	if (va < KERNEL_HEAP_START || va > KERNEL_HEAP_MAX)
		return 0;
	uint32* ptr = NULL;
	uint32 entry;
	if(get_page_table(ptr_page_directory, (void*)va,&ptr) == 0){
		entry = ptr[PTX(va)];
	}
	int pa = entry & 0xFFFFF000;
	return pa;
	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details
	//change this "return" according to your answer
}

