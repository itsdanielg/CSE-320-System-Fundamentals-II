/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include "helpers.h"
#include <errno.h>

void *sf_malloc(size_t size) {
    if (size != 0) {
        // First call of sf_malloc
        if (sf_free_list_head.next == &sf_free_list_head) {
            sf_mem_grow();
            sf_prologue *heap_prologue = sf_mem_start();
            sf_epilogue *heap_epilogue = sf_mem_end() - 8;
            init_prologue(heap_prologue);
            init_epilogue(heap_epilogue);
            size_t available_size = PAGE_SZ - 48;
            sf_free_list_node* new_list = sf_add_free_list(available_size, &sf_free_list_head);
            init_first_block(new_list, available_size);
        }
        // Figure out total block size needed
        size_t total_block_size = size + 8;
        if (total_block_size % 16 != 0) total_block_size += (16 - total_block_size % 16);
        if (total_block_size < 32) total_block_size = 32;
        sf_free_list_node* current_list = &sf_free_list_head;
        while (current_list->next != &sf_free_list_head) {
            current_list = current_list->next;
            // If free list of size exists
            if (total_block_size == current_list->size) {
                // Point to the sentinel header of the free list with the best fit size
                sf_header *list_node = &(current_list->head);
                // If this free list is not empty
                if (list_node->links.next != list_node) {
                    // Point to the last free block (because of LIFO placement)
                    list_node = list_node->links.next;
                    // Remove block from free list, adjust pointers
                    list_node->links.prev->links.next = list_node->links.next;
                    list_node->links.next->links.prev = list_node->links.prev;
                    list_node->links.next = NULL;
                    list_node->links.prev = NULL;
                    // Use current block
                    list_node->info.allocated = 1;
                    if ((void*)list_node - 40 == sf_mem_start()) {
                        list_node->info.prev_allocated = 1;
                    }
                    list_node->info.two_zeroes = 0;
                    list_node->info.block_size = total_block_size >> 4;
                    list_node->info.requested_size = size;
                    list_node->payload = total_block_size - 8;
                    // Set prev_allocated of next block to 1
                    void* next_block_add = (void*)list_node + (list_node->info.block_size << 4);
                    if (next_block_add == sf_mem_end() - 8) {
                        sf_epilogue* epilogue = (sf_epilogue*)next_block_add;
                        epilogue->footer.info.prev_allocated = 1;
                    }
                    else {
                        sf_header* block_header = (sf_header*)next_block_add;
                        block_header->info.prev_allocated = 1;
                    }
                    // Return address of payload
                    return &list_node->payload;
                }
            }
            // Else if bigger free list sizes exist
            else if (total_block_size < current_list->size) {
                // Point to the sentinel header of the free list with the best fit size
                sf_header *list_node = &(current_list->head);
                // If this free list is not empty
                if (list_node->links.next != list_node) {
                    // Point to the last free block (because of LIFO placement)
                    list_node = list_node->links.next;
                    // If the block can be split
                    if (current_list->size - total_block_size > 32) {
                        // Remove block from free list, adjust pointers
                        list_node->links.prev->links.next = list_node->links.next;
                        list_node->links.next->links.prev = list_node->links.prev;
                        list_node->links.next = NULL;
                        list_node->links.prev = NULL;
                        // Allocate split block
                        int split_block_size = (list_node->info.block_size << 4)- total_block_size;
                        list_node->info.allocated = 1;
                        if ((void*)list_node - 40 == sf_mem_start()) {
                            list_node->info.prev_allocated = 1;
                        }
                        list_node->info.two_zeroes = 0;
                        list_node->info.block_size = (total_block_size >> 4);
                        list_node->info.requested_size = size;
                        list_node->payload = total_block_size - 8;
                        // Create new block header for split free block
                        sf_header *new_block_header = (void*)list_node + total_block_size;
                        new_block_header->info.allocated = 0;
                        new_block_header->info.prev_allocated = 1;
                        new_block_header->info.two_zeroes = 0;
                        new_block_header->info.block_size = split_block_size >> 4;
                        new_block_header->info.requested_size = 0;
                        // Edit block footer for split free block
                        sf_footer *block_footer =  (void*)new_block_header + split_block_size - 8;
                        block_footer->info.allocated = 0;
                        block_footer->info.prev_allocated = 1;
                        block_footer->info.two_zeroes = 0;
                        block_footer->info.block_size = split_block_size >> 4;
                        block_footer->info.requested_size = 0;
                        // Place new free block in new/existing free list
                        current_list = &sf_free_list_head;
                        while (current_list->next != &sf_free_list_head) {
                            current_list = current_list->next;
                            // If free list already exists for new size
                            if ((new_block_header->info.block_size << 4) == current_list->size) {
                                sf_header* list_header = &current_list->head;
                                // If this list is empty
                                if (list_header->links.prev == list_header) {
                                    list_header->links.prev = new_block_header;
                                    list_header->links.next = new_block_header;
                                    new_block_header->links.next = list_header;
                                    new_block_header->links.prev = list_header;
                                    break;
                                }
                                // Else add new free block to list with existing blocks
                                else {
                                    new_block_header->links.next = list_header->links.next;
                                    new_block_header->links.prev = list_header;
                                    list_header->links.next->links.prev = new_block_header;
                                    list_header->links.next = new_block_header;
                                    break;
                                }
                            }
                            else if ((new_block_header->info.block_size << 4) < current_list->size) {
                                // Create new free list
                                current_list = current_list->prev;
                                sf_free_list_node* new_list = sf_add_free_list((new_block_header->info.block_size << 4), current_list->next);
                                // Add new block to free list, adjust pointers
                                new_list->head.links.next = new_block_header;
                                new_list->head.links.prev = new_block_header;
                                new_block_header->links.next = &new_list->head;
                                new_block_header->links.prev = &new_list->head;
                                break;
                            }
                            //If every free list available has been checked
                            if (current_list->next == &sf_free_list_head) {
                                // Create new free list before head
                                sf_free_list_node* new_list = sf_add_free_list((new_block_header->info.block_size << 4), &sf_free_list_head);
                                // Add new block to free list, adjust pointers
                                new_block_header->links.next = &new_list->head;
                                new_block_header->links.prev = &new_list->head;
                                new_list->head.links.next = new_block_header;
                                new_list->head.links.prev = new_block_header;
                                break;
                            }
                        }
                        // Return address of payload
                        return &list_node->payload;
                    }
                    // Else use the entire block
                    else {
                        // Remove block from free list, adjust pointers
                        list_node->links.prev->links.next = list_node->links.next;
                        list_node->links.next->links.prev = list_node->links.prev;
                        list_node->links.next = NULL;
                        list_node->links.prev = NULL;
                        // Use current block
                        list_node->info.allocated = 1;
                        if ((void*)list_node - 40 == sf_mem_start()) {
                            list_node->info.prev_allocated = 1;
                        }
                        list_node->info.two_zeroes = 0;
                        list_node->info.block_size = current_list->size >> 4;
                        list_node->info.requested_size = size;
                        list_node->payload = current_list->size - 8;
                        // Set prev_allocated of next block to 1
                        void* next_block_add = (void*)list_node + (list_node->info.block_size << 4);
                        if (next_block_add == sf_mem_end() - 8) {
                            sf_epilogue* epilogue = (sf_epilogue*)next_block_add;
                            epilogue->footer.info.prev_allocated = 1;
                        }
                        else {
                            sf_header* block_header = (sf_header*)next_block_add;
                            block_header->info.prev_allocated = 1;
                        }
                        // Return address of payload
                        return &list_node->payload;
                    }
                }
            }
        }
        // If no free lists are found
        sf_epilogue* old_epilogue = sf_mem_end() - 8;
        char ep_prev_allocated = old_epilogue->footer.info.prev_allocated;
        // Create new page
        void* new_page_add = sf_mem_grow();
        // If memory cannot grow anymore
        if (new_page_add == NULL) {
            sf_errno = ENOMEM;
            return NULL;
        }
        // Make new epilogue
        sf_epilogue *heap_epilogue = sf_mem_end() - 8;
        init_epilogue(heap_epilogue);
        // Replace old epilogue with header of new block
        size_t available_size = PAGE_SZ;
        sf_header *new_page_block_header = new_page_add - 8;
        new_page_block_header->info.allocated = 0;
        new_page_block_header->info.prev_allocated = ep_prev_allocated;
        new_page_block_header->info.two_zeroes = 0;
        new_page_block_header->info.block_size = (available_size) >> 4;
        new_page_block_header->info.requested_size = 0;
        new_page_block_header->payload = 0;
        // If previous block is free, coalesce
        if (ep_prev_allocated == 0) {
            sf_footer* old_footer = (void*)new_page_block_header - 8;
            size_t old_block_size = old_footer->info.block_size << 4;
            sf_header* old_header = (void*)new_page_block_header - old_block_size;
            size_t new_block_size = sf_mem_end() -8 - (void*)old_header;
            old_header->info.block_size = new_block_size >> 4;
            sf_footer* new_footer = sf_mem_end() - 16;
            new_footer->info.allocated = 0;
            new_footer->info.prev_allocated = 1;
            new_footer->info.two_zeroes = 0;
            new_footer->info.block_size = new_block_size >> 4;
            new_footer->info.requested_size = 0;
            // Check if free list exists for new block size
            current_list = &sf_free_list_head;
            while (current_list->next != &sf_free_list_head) {
                sf_header* old_block_header;
                current_list = current_list->next;
                // Remove free block from old free list
                if (current_list->size == old_block_size) {
                    old_block_header = &current_list->head;
                }
                // If free list does exists
                if (current_list->size == new_block_size) {
                    // Remove free block from old free list
                    old_block_header = old_block_header->links.prev;
                    old_block_header->links.prev->links.next = old_block_header->links.next;
                    old_block_header->links.next->links.prev = old_block_header->links.prev;
                    old_block_header->links.next = NULL;
                    old_block_header->links.prev = NULL;
                    sf_header* list_node = &current_list->head;
                    // If this list is empty
                    if (list_node->links.prev == list_node) {
                        list_node->links.prev = old_header;
                        list_node->links.next = old_header;
                        old_header->links.next = list_node;
                        old_header->links.prev = list_node;
                        break;
                    }
                    // Else add new free block to list with existing blocks
                    else {
                        old_header->links.next = list_node->links.next;
                        old_header->links.prev = list_node;
                        list_node->links.next->links.prev = old_header;
                        list_node->links.next = old_header;
                        break;
                    }
                }
            }
            // If free list doesn't exist, add to new free list
            sf_free_list_node *new_list = sf_add_free_list(new_block_size, &sf_free_list_head);
            current_list = &sf_free_list_head;
            while (current_list->next != &sf_free_list_head) {
                current_list = current_list->next;
                if (current_list->size == old_block_size) {
                    sf_header* list_node = &current_list->head;
                    // Remove free block from old free list
                    list_node = list_node->links.prev;
                    list_node->links.prev->links.next = list_node->links.next;
                    list_node->links.next->links.prev = list_node->links.prev;
                    list_node->links.next = NULL;
                    list_node->links.prev = NULL;
                    // Adjust pointers; Add new header to head of new list
                    new_list->head.links.next = old_header;
                    new_list->head.links.prev = old_header;
                    old_header->links.next = &(new_list->head);
                    old_header->links.prev = &(new_list->head);
                    break;
                }
            }
        }
        // Else just add new block to free list
        else {
            // Create new footer
            sf_footer* block_footer = sf_mem_end() - 16;
            block_footer->info.allocated = 0;
            block_footer->info.prev_allocated = 1;
            block_footer->info.two_zeroes = 0;
            block_footer->info.block_size = available_size >> 4;
            block_footer->info.requested_size = 0;
            sf_free_list_node* current_list = &sf_free_list_head;
            // Traverse through list of free lists
            current_list = current_list->prev;
            do {
                current_list = current_list->next;
                // Look for a list with the same block size
                if (current_list->size == available_size) {
                    sf_header* list_head = &current_list->head;
                    // If this list is empty
                    if (list_head->links.prev == list_head) {
                        list_head->links.prev = new_page_block_header;
                        list_head->links.next = new_page_block_header;
                        new_page_block_header->links.next = list_head;
                        new_page_block_header->links.prev = list_head;
                        return sf_malloc(size);
                    }
                    // Else add new free block to list with existing blocks
                    else {
                        new_page_block_header->links.next = list_head->links.next;
                        new_page_block_header->links.prev = list_head;
                        list_head->links.next->links.prev = new_page_block_header;
                        list_head->links.next = new_page_block_header;
                        return sf_malloc(size);
                    }
                }
                // Else look for list that fits between two block sizes
                else if (current_list->size < available_size && current_list->next->size > available_size) {
                    sf_free_list_node* new_list = sf_add_free_list(available_size, current_list->next);
                    new_list->head.links.next = new_page_block_header;
                    new_list->head.links.prev = new_page_block_header;
                    new_page_block_header->links.next = &(new_list->head);
                    new_page_block_header->links.prev = &(new_list->head);
                    return sf_malloc(size);
                }
            } while (current_list->next != &sf_free_list_head);
            // Else insert block at new free list at end of list
            sf_free_list_node* new_list = sf_add_free_list(available_size, &sf_free_list_head);
            new_list->head.links.next = new_page_block_header;
            new_list->head.links.prev = new_page_block_header;
            new_page_block_header->links.next = &(new_list->head);
            new_page_block_header->links.prev = &(new_list->head);
            return sf_malloc(size);
        }
        // Recursively call sf_malloc() until payload is returned
        return sf_malloc(size);
    }
    return NULL;
}

void sf_free(void *pp) {
    // If pointer is not null
    if (pp != NULL) {
        // If pointer is aligned
        if (((unsigned long)pp & 15) == 0) {
            // Set header of block to pointer
            sf_header* block_header = (void*)pp - 8;
            void* end_of_prologue = sf_mem_start() + 40;
            void* beginning_of_epilogue = sf_mem_end() - 8;
            // If header is before the end of prologue, abort
            if ((void*)block_header < end_of_prologue) abort();
            // If header is after the beginning of epilogue, abort
            if ((void*)block_header > beginning_of_epilogue) abort();
            // If allocated bit in header is 0, abort
            if (block_header->info.allocated == 0) abort();
            // Set block size of block to a variable
            size_t block_size = block_header->info.block_size << 4;
            // If block size is not a muliple of 16 or less than 32, abort
            if (block_size % 16 != 0 || block_size < 32) abort();
            // Set requested size of block to a variable
            size_t requested_size = block_header->info.requested_size;
            // If requested size plus size of header is greater than block size, abort
            if (requested_size + 8 > block_size) abort();
            // Coalescing
            char prev_is_free = 1;
            char next_is_free = 1;
            if (block_header->info.prev_allocated == 0) {
                sf_footer* prev_block_footer = (void*)block_header - 8;
                size_t prev_block_size = prev_block_footer->info.block_size << 4;
                sf_header* prev_block_header = (void*)block_header - prev_block_size;
                if (prev_block_header->info.allocated == 0 && prev_block_footer->info.allocated == 0) {
                    // Do nothing
                }
                else abort();
            }
            else prev_is_free = 0;
            sf_header* next_block_header = (void*)block_header + block_size;
            if (next_block_header->info.allocated == 1) next_is_free = 0;
            // If both adjacent blocks are free
            if (prev_is_free && next_is_free) {
                // Remove both blocks from their free lists
                // Remove left free block from free list
                sf_footer* prev_block_footer = (void*)block_header - 8;
                sf_header* prev_block_header = (void*)block_header - (prev_block_footer->info.block_size << 4);
                sf_free_list_node* current_list = &sf_free_list_head;
                while (current_list->next != &sf_free_list_head) {
                    current_list = current_list->next;
                    if (current_list->size == prev_block_footer->info.block_size << 4) {
                        sf_header* current_list_header = &current_list->head;
                        while (current_list_header->links.next != &current_list->head) {
                            current_list_header = current_list_header->links.next;
                            if (current_list_header == prev_block_header) {
                                current_list_header->links.prev->links.next = prev_block_header->links.next;
                                current_list_header->links.next->links.prev = prev_block_header->links.prev;
                                prev_block_header->links.next = NULL;
                                prev_block_header->links.prev = NULL;
                                break;
                            }
                        }
                        break;
                    }
                }
                // Remove right free block from free list
                current_list = &sf_free_list_head;
                while (current_list->next != &sf_free_list_head) {
                    current_list = current_list->next;
                    if (current_list->size == next_block_header->info.block_size << 4) {
                        sf_header* current_list_header = &current_list->head;
                        while (current_list_header->links.next != &current_list->head) {
                            current_list_header = current_list_header->links.next;
                            if (current_list_header == next_block_header) {
                                current_list_header->links.prev->links.next = next_block_header->links.next;
                                current_list_header->links.next->links.prev = next_block_header->links.prev;
                                next_block_header->links.next = NULL;
                                next_block_header->links.prev = NULL;
                                break;
                            }
                        }
                        break;
                    }
                }
                // Coalesce all three blocks
                size_t new_block_size = (block_header->info.block_size << 4) + (prev_block_header->info.block_size << 4) + (next_block_header->info.block_size << 4);
                prev_block_header->info.allocated = 0;
                prev_block_header->info.block_size = new_block_size >> 4;
                prev_block_header->info.requested_size = 0;
                prev_block_header->payload = 0;
                // Create new footer
                sf_footer* block_footer = (void*)prev_block_header + new_block_size - 8;
                block_footer->info.allocated = 0;
                block_footer->info.prev_allocated = 1;
                block_footer->info.two_zeroes = 0;
                block_footer->info.block_size = new_block_size >> 4;
                block_footer->info.requested_size = 0;
                // Set prev allocated bit of next block to 0
                sf_header* next_alloc_block = (void*)block_footer + 8;
                next_alloc_block->info.prev_allocated = 0;
                // Add free block to free lists
                current_list = &sf_free_list_head;
                current_list = current_list->prev;
                do {
                    current_list = current_list->next;
                    // Look for a list with the same block size
                    if (current_list->size == new_block_size) {
                        sf_header* list_head = &current_list->head;
                        // If this list is empty
                        if (list_head->links.prev == list_head) {
                            list_head->links.prev = prev_block_header;
                            list_head->links.next = prev_block_header;
                            prev_block_header->links.next = list_head;
                            prev_block_header->links.prev = list_head;
                            return;
                        }
                        // Else add new free block to list with existing blocks
                        else {
                            prev_block_header->links.next = list_head->links.next;
                            prev_block_header->links.prev = list_head;
                            list_head->links.next->links.prev = prev_block_header;
                            list_head->links.next = prev_block_header;
                            return;
                        }
                    }
                    // Else look for list that fits between two block sizes
                    else if (current_list->size < new_block_size && current_list->next->size > new_block_size) {
                        sf_free_list_node* new_list = sf_add_free_list(new_block_size, current_list->next);
                        new_list->head.links.next = prev_block_header;
                        new_list->head.links.prev = prev_block_header;
                        prev_block_header->links.next = &(new_list->head);
                        prev_block_header->links.prev = &(new_list->head);
                        return;
                    }
                } while (current_list->next != &sf_free_list_head);
                // Else insert block at new free list at end of list
                sf_free_list_node* new_list = sf_add_free_list(new_block_size, &sf_free_list_head);
                new_list->head.links.next = prev_block_header;
                new_list->head.links.prev = prev_block_header;
                prev_block_header->links.next = &(new_list->head);
                prev_block_header->links.prev = &(new_list->head);
                return;
            }
            // If only left adjacent block is free
            else if (prev_is_free) {
                // Remove left free block from free list
                sf_footer* prev_block_footer = (void*)block_header - 8;
                sf_header* prev_block_header = (void*)block_header - (prev_block_footer->info.block_size << 4);
                sf_free_list_node* current_list = &sf_free_list_head;
                while (current_list->next != &sf_free_list_head) {
                    current_list = current_list->next;
                    if (current_list->size == prev_block_footer->info.block_size << 4) {
                        sf_header* current_list_header = &current_list->head;
                        while (current_list_header->links.next != &current_list->head) {
                            current_list_header = current_list_header->links.next;
                            if (current_list_header == prev_block_header) {
                                current_list_header->links.prev->links.next = prev_block_header->links.next;
                                current_list_header->links.next->links.prev = prev_block_header->links.prev;
                                prev_block_header->links.next = NULL;
                                prev_block_header->links.prev = NULL;
                                break;
                            }
                        }
                        break;
                    }
                }
                // Coalesce both blocks
                size_t new_block_size = (block_header->info.block_size << 4) + (prev_block_header->info.block_size << 4);
                prev_block_header->info.allocated = 0;
                prev_block_header->info.block_size = new_block_size >> 4;
                prev_block_header->info.requested_size = 0;
                prev_block_header->payload = 0;
                // Create new footer
                sf_footer* block_footer = (void*)prev_block_header + new_block_size - 8;
                block_footer->info.allocated = 0;
                block_footer->info.prev_allocated = 1;
                block_footer->info.two_zeroes = 0;
                block_footer->info.block_size = new_block_size >> 4;
                block_footer->info.requested_size = 0;
                // Set prev allocated bit of next block to 0
                sf_header* next_alloc_block = (void*)block_footer + 8;
                next_alloc_block->info.prev_allocated = 0;
                // Add free block to free lists
                current_list = &sf_free_list_head;
                current_list = current_list->prev;
                do {
                    current_list = current_list->next;
                    // Look for a list with the same block size
                    if (current_list->size == new_block_size) {
                        sf_header* list_head = &current_list->head;
                        // If this list is empty
                        if (list_head->links.prev == list_head) {
                            list_head->links.prev = prev_block_header;
                            list_head->links.next = prev_block_header;
                            prev_block_header->links.next = list_head;
                            prev_block_header->links.prev = list_head;
                            return;
                        }
                        // Else add new free block to list with existing blocks
                        else {
                            prev_block_header->links.next = list_head->links.next;
                            prev_block_header->links.prev = list_head;
                            list_head->links.next->links.prev = prev_block_header;
                            list_head->links.next = prev_block_header;
                            return;
                        }
                    }
                    // Else look for list that fits between two block sizes
                    else if (current_list->size < new_block_size && current_list->next->size > new_block_size) {
                        sf_free_list_node* new_list = sf_add_free_list(new_block_size, current_list->next);
                        new_list->head.links.next = prev_block_header;
                        new_list->head.links.prev = prev_block_header;
                        prev_block_header->links.next = &(new_list->head);
                        prev_block_header->links.prev = &(new_list->head);
                        return;
                    }
                } while (current_list->next != &sf_free_list_head);
                // Else insert block at new free list at end of list
                sf_free_list_node* new_list = sf_add_free_list(new_block_size, &sf_free_list_head);
                new_list->head.links.next = prev_block_header;
                new_list->head.links.prev = prev_block_header;
                prev_block_header->links.next = &(new_list->head);
                prev_block_header->links.prev = &(new_list->head);
                return;
            }
            // If only right adjacent block is free
            else if (next_is_free) {
                // Remove right free block from free list
                sf_free_list_node* current_list = &sf_free_list_head;
                while (current_list->next != &sf_free_list_head) {
                    current_list = current_list->next;
                    if (current_list->size == next_block_header->info.block_size << 4) {
                        sf_header* current_list_header = &current_list->head;
                        while (current_list_header->links.next != &current_list->head) {
                            current_list_header = current_list_header->links.next;
                            if (current_list_header == next_block_header) {
                                current_list_header->links.prev->links.next = next_block_header->links.next;
                                current_list_header->links.next->links.prev = next_block_header->links.prev;
                                next_block_header->links.next = NULL;
                                next_block_header->links.prev = NULL;
                                break;
                            }
                        }
                        break;
                    }
                }
                // Coalesce both blocks
                size_t new_block_size = (block_header->info.block_size << 4) + (next_block_header->info.block_size << 4);
                block_header->info.allocated = 0;
                block_header->info.block_size = new_block_size >> 4;
                block_header->info.requested_size = 0;
                block_header->payload = 0;
                // Create new footer
                sf_footer* block_footer = (void*)block_header + new_block_size - 8;
                block_footer->info.allocated = 0;
                block_footer->info.prev_allocated = 1;
                block_footer->info.two_zeroes = 0;
                block_footer->info.block_size = new_block_size >> 4;
                block_footer->info.requested_size = 0;
                // Set prev allocated bit of next block to 0
                sf_header* next_alloc_block = (void*)block_footer + 8;
                next_alloc_block->info.prev_allocated = 0;
                // Add free block to free lists
                current_list = &sf_free_list_head;
                current_list = current_list->prev;
                do {
                    current_list = current_list->next;
                    // Look for a list with the same block size
                    if (current_list->size == new_block_size) {
                        sf_header* list_head = &current_list->head;
                        // If this list is empty
                        if (list_head->links.prev == list_head) {
                            list_head->links.prev = block_header;
                            list_head->links.next = block_header;
                            block_header->links.next = list_head;
                            block_header->links.prev = list_head;
                            return;
                        }
                        // Else add new free block to list with existing blocks
                        else {
                            block_header->links.next = list_head->links.next;
                            block_header->links.prev = list_head;
                            list_head->links.next->links.prev = block_header;
                            list_head->links.next = block_header;
                            return;
                        }
                    }
                    // Else look for list that fits between two block sizes
                    else if (current_list->size < new_block_size && current_list->next->size > new_block_size) {
                        sf_free_list_node* new_list = sf_add_free_list(new_block_size, current_list->next);
                        new_list->head.links.next = block_header;
                        new_list->head.links.prev = block_header;
                        block_header->links.next = &(new_list->head);
                        block_header->links.prev = &(new_list->head);
                        return;
                    }
                } while (current_list->next != &sf_free_list_head);
                // Else insert block at new free list at end of list
                sf_free_list_node* new_list = sf_add_free_list(new_block_size, &sf_free_list_head);
                new_list->head.links.next = block_header;
                new_list->head.links.prev = block_header;
                block_header->links.next = &(new_list->head);
                block_header->links.prev = &(new_list->head);
                return;
            }
            // If both adjacent blocks are not free
            else {
                block_header->info.allocated = 0;
                block_header->info.requested_size = 0;
                block_header->payload = 0;
                // Create new footer
                sf_footer* block_footer = (void*)block_header + block_size - 8;
                block_footer->info.allocated = 0;
                block_footer->info.prev_allocated = 1;
                block_footer->info.two_zeroes = 0;
                block_footer->info.block_size = block_size >> 4;
                block_footer->info.requested_size = 0;
                // Set prev allocated bit of next block to 0
                sf_header* next_alloc_block = (void*)block_footer + 8;
                next_alloc_block->info.prev_allocated = 0;
                // Traverse through list of free lists
                sf_free_list_node* current_list = &sf_free_list_head;
                current_list = current_list->prev;
                do {
                    current_list = current_list->next;
                    // Look for a list with the same block size
                    if (current_list->size == block_size) {
                        sf_header* list_head = &current_list->head;
                        // If this list is empty
                        if (list_head->links.prev == list_head) {
                            list_head->links.prev = block_header;
                            list_head->links.next = block_header;
                            block_header->links.next = list_head;
                            block_header->links.prev = list_head;
                            return;
                        }
                        // Else add new free block to list with existing blocks
                        else {
                            block_header->links.next = list_head->links.next;
                            block_header->links.prev = list_head;
                            list_head->links.next->links.prev = block_header;
                            list_head->links.next = block_header;
                            return;
                        }
                    }
                    // Else look for list that fits between two block sizes
                    else if (current_list->size < block_size && current_list->next->size > block_size) {
                        sf_free_list_node* new_list = sf_add_free_list(block_size, current_list->next);
                        new_list->head.links.next = block_header;
                        new_list->head.links.prev = block_header;
                        block_header->links.next = &(new_list->head);
                        block_header->links.prev = &(new_list->head);
                        return;
                    }
                } while (current_list->next != &sf_free_list_head);
                // Else insert block at new free list at end of list
                sf_free_list_node* new_list = sf_add_free_list(block_size, &sf_free_list_head);
                new_list->head.links.next = block_header;
                new_list->head.links.prev = block_header;
                block_header->links.next = &(new_list->head);
                block_header->links.prev = &(new_list->head);
                return;
            }
        }
        abort();
    }
    abort();
}

void *sf_realloc(void *pp, size_t rsize) {
    if (pp != NULL) {
        // If pointer is aligned
        if (((unsigned long)pp & 15) == 0) {
            // Set header of block to pointer
            sf_header* block_header = (void*)pp - 8;
            void* end_of_prologue = sf_mem_start() + 40;
            void* beginning_of_epilogue = sf_mem_end() - 8;
            // If header is before the end of prologue, abort
            if ((void*)block_header < end_of_prologue) {
                sf_errno = EINVAL;
                return NULL;
            }
            // If header is after the beginning of epilogue, abort
            if ((void*)block_header > beginning_of_epilogue) {
                sf_errno = EINVAL;
                return NULL;
            }
            // If allocated bit in header is 0, abort
            if (block_header->info.allocated == 0) {
                sf_errno = EINVAL;
                return NULL;
            }
            // Set block size of block to a variable
            size_t block_size = block_header->info.block_size << 4;
            // If block size is not a muliple of 16 or less than 32, abort
            if (block_size % 16 != 0 || block_size < 32) {
                sf_errno = EINVAL;
                return NULL;
            }
            // Set requested size of block to a variable
            size_t requested_size = block_header->info.requested_size;
            // If requested size plus size of header is greater than block size, abort
            if (requested_size + 8 > block_size) {
                sf_errno = EINVAL;
                return NULL;
            }
            if (block_header->info.prev_allocated == 0) {
                sf_footer* prev_block_footer = (void*)block_header - 8;
                size_t prev_block_size = prev_block_footer->info.block_size << 4;
                sf_header* prev_block_header = (void*)block_header - prev_block_size;
                if (prev_block_header->info.allocated == 0 && prev_block_footer->info.allocated == 0) {
                    // Do nothing
                }
                else {
                    sf_errno = EINVAL;
                    return NULL;
                }
            }
            // If reallocated size is 0
            if (rsize == 0) {
                sf_free(pp);
                return NULL;
            }
            // Else if reallocated size is the same as the pointer block size
            else if (rsize == block_size) {
                return pp;
            }
            // Else if reallocated size is larger than pointer block size
            else if (rsize > block_size) {
                void* new_pp = sf_malloc(rsize);
                if (new_pp == NULL) {
                    return NULL;
                }
                memcpy(new_pp, pp, block_size);
                sf_free(pp);
                return new_pp;
            }
            // Else if reallocated size is smaller than pointer block size
            else {
                size_t total_resize = rsize + 8;
                if (total_resize % 16 != 0) total_resize += (16 - total_resize % 16);
                if (total_resize < 32) total_resize = 32;
                // If the block cannot be split
                if (block_size - total_resize < 32) {
                    block_header->info.requested_size = rsize;
                }
                // Else if the block can be split
                else {
                    block_header->info.block_size = total_resize >> 4;
                    block_header->info.requested_size = rsize;
                    block_header->payload = total_resize - 8;
                    sf_header* new_header = (void*)block_header + total_resize;
                    new_header->info.allocated = 1;
                    new_header->info.prev_allocated = 1;
                    new_header->info.two_zeroes = 0;
                    new_header->info.block_size = (block_size - total_resize) >> 4;
                    new_header->info.requested_size = 0;
                    new_header->payload = (block_size - total_resize) - 8;
                    sf_free((void*)new_header + 8);
                }
                return pp;
            }
        }
        else {
            sf_errno = EINVAL;
            return NULL;
        }
    }
    sf_errno = EINVAL;
    return NULL;
}

//############################################
//              HELPER METHODS              //
//############################################

void init_prologue(sf_prologue *heap_prologue) {
    heap_prologue->padding = 0;
    heap_prologue->header.info.allocated = 1;
    heap_prologue->header.info.prev_allocated = 0;
    heap_prologue->header.info.two_zeroes = 0;
    heap_prologue->header.info.block_size = 0;
    heap_prologue->header.info.requested_size = 0;
    heap_prologue->header.payload = 0;
    heap_prologue->footer.info.allocated = 1;
    heap_prologue->footer.info.prev_allocated = 0;
    heap_prologue->footer.info.two_zeroes = 0;
    heap_prologue->footer.info.block_size = 0;
    heap_prologue->footer.info.requested_size = 0;
}

void init_epilogue(sf_epilogue *heap_epilogue) {
    heap_epilogue->footer.info.allocated = 1;
    heap_epilogue->footer.info.prev_allocated = 0;
    heap_epilogue->footer.info.two_zeroes = 0;
    heap_epilogue->footer.info.block_size = 0;
    heap_epilogue->footer.info.requested_size = 0;
}

void init_first_block(sf_free_list_node *current_list, size_t available_size) {
    sf_header *new_block_header = sf_mem_start() + 40;
    new_block_header->info.allocated = 0;
    new_block_header->info.prev_allocated = 0;
    new_block_header->info.two_zeroes = 0;
    new_block_header->info.block_size = available_size >> 4;
    new_block_header->info.requested_size = 0;
    current_list->head.links.next = new_block_header;
    current_list->head.links.prev = new_block_header;
    new_block_header->links.next = &(current_list->head);
    new_block_header->links.prev = &(current_list->head);
    sf_footer *new_block_footer = sf_mem_end() - 16;
    new_block_footer->info.allocated = 0;
    new_block_footer->info.prev_allocated = 0;
    new_block_footer->info.two_zeroes = 0;
    new_block_footer->info.block_size = available_size >> 4;
    new_block_footer->info.requested_size = 0;
}