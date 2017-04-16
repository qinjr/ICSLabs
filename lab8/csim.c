
#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define GET_VALID(blk_ptr)					(*((int *)(blk_ptr)))
#define SET_VALID(blk_ptr, valid)			(*((int *)(blk_ptr)) = valid)

#define GET_TIME(blk_ptr)					(*((int *)(blk_ptr + 4)))
#define SET_TIME(blk_ptr, time)				(*((int *)(blk_ptr + 4)) = time)

#define GET_TAG_CACHE(blk_ptr)				(*((long *)(blk_ptr + 8)))
#define SET_TAG_CACHE(blk_ptr, tag)			(*((long *)(blk_ptr + 8)) = tag)


#define GET_TAG_ADDR(addr, s, b)			(((long)addr >> (s + b)) & (long)((1 << (64 - s - b)) - 1))
#define GET_SET_ID(addr, s, b)				(((long)addr >> b) & (long)((1 << s) - 1))

#define ELE_SIZE                            16


/*
 * Name: Qin Jiarui
 * ID: 515030910475
 * loginID: ics515030910475
 */

char *trace_file = NULL;
int set_bit_amt = 0;
int ele_amt = 0;
int block_bit_amt = 0;
int v_flag = 0;

/*
 * get_args -- get command line arguments
 */
void get_args (int argc, char * const argv[], int *set_bit_amt, int *ele_amt, int *block_bit_amt, int *v_flag, char **trace_file) {
    int c;
    while ((c = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch(c) {
            case 'h':
                return;
            case 'v':
                *v_flag = 1;
                break;
            case 's':
                *set_bit_amt = atoi(optarg);
                break;
            case 'E':
                *ele_amt = atoi(optarg);
                break;
            case 'b':
                *block_bit_amt = atoi(optarg);
                break;
            case 't':
                *trace_file = optarg;
                break;
        }
    }
}

/*
 * handle_line -- deal with a single line, no matter it is L or S
 */
void handle_line (char *cache, char *addr, int *h_count, int *m_count, int *e_count, int *h_flag, int *m_flag, int *e_flag, int time) {
    long tag = GET_TAG_ADDR(addr, set_bit_amt, block_bit_amt);
    long set_id = GET_SET_ID(addr, set_bit_amt, block_bit_amt);

    char *set_ptr = cache + set_id * ele_amt * ELE_SIZE;
    char *blk_ptr;
    
    int eleid;
    //find a fit
    for (eleid = 0; eleid < ele_amt; ++ eleid) {
        blk_ptr = set_ptr + eleid * ELE_SIZE;
        if ((GET_VALID(blk_ptr)) && (GET_TAG_CACHE(blk_ptr) == tag)) {
            *h_flag = 1;
            (*h_count) ++;
            SET_TIME(blk_ptr, time);
            return;
        }
    }
    
    //no fit found, but there is empty space in the set
    for (eleid = 0; eleid < ele_amt; ++ eleid) {
        blk_ptr = set_ptr + eleid * ELE_SIZE;
        if (!GET_VALID(blk_ptr)) {
            *m_flag = 1;
            (*m_count) ++;
            SET_VALID(blk_ptr, 1);
            SET_TAG_CACHE(blk_ptr, tag);
            SET_TIME(blk_ptr, time);
            return;
        }
    }
    
    //have to evicte a block
    blk_ptr = set_ptr;
    for (eleid = 0; eleid < ele_amt; ++eleid) {
        if (GET_TIME(set_ptr + eleid * ELE_SIZE) < GET_TIME(blk_ptr))
            blk_ptr = set_ptr + eleid * ELE_SIZE;
    }
    SET_TIME(blk_ptr, time);
    SET_TAG_CACHE(blk_ptr, tag);
    *m_flag = 1;
    (*m_count) ++;
    *e_flag = 1;
    (*e_count) ++;
}

/*
 * print_status -- verbose information
 */
void print_status (char *addr, int h_flag, int m_flag, int e_flag) {
	printf("%p ", addr);
    if (m_flag)
        printf("miss ");
    if (e_flag)
        printf("eviction ");	
    if (h_flag)
        printf("hit ");
    printf("\n");
    return;
}


/*
 * data structure to store information:
 * a block:valid(4 bytes) + time(4 bytes, used in LRU) + tag(8bytes)
 * the variable 'cache' is a chunk of memory malloced to store the blocks defined above
 * we can use set_id and ele_id to retrieve a block from 'cache'
 */
int main (int argc, char * const argv[]) {
    get_args(argc, argv, &set_bit_amt, &ele_amt, &block_bit_amt, &v_flag, &trace_file);
    char *cache = (char *)malloc((1 << set_bit_amt) * ele_amt * ELE_SIZE);
    
    FILE *fp = fopen(trace_file, "rt");
    char remain[16];
    char buffer[128];
    
    char first;
    char second;
    char* addr;
    int time = 0;
 
	int h_count = 0;
	int m_count = 0;
	int e_count = 0;

    while (fgets(buffer, 128, fp)) {
        int h_flag = 0;
        int m_flag = 0;
        int e_flag = 0;
        time ++;

        sscanf(buffer, "%c%c %s", &first, &second, remain);
        addr = (char *)(strtol(remain, NULL, 16));
        if (first == 'I')
            continue;
        if (second == 'L')
            handle_line(cache, addr, &h_count, &m_count, &e_count, &h_flag, &m_flag, &e_flag, time);
        if (second == 'S')
            handle_line(cache, addr, &h_count, &m_count, &e_count, &h_flag, &m_flag, &e_flag, time);
        if (second == 'M') {
            handle_line(cache, addr, &h_count, &m_count, &e_count, &h_flag, &m_flag, &e_flag, time);
            if (v_flag)
            	print_status(addr, h_flag, m_flag, e_flag);

            int h_flag = 0;
	        int m_flag = 0;
	        int e_flag = 0;

            handle_line(cache, addr, &h_count, &m_count, &e_count, &h_flag, &m_flag, &e_flag, time);
            if (v_flag)
            	print_status(addr, h_flag, m_flag, e_flag);
            continue;
        }
        
        if (v_flag)
            print_status(addr, h_flag, m_flag, e_flag);
    }

    free(cache);
    printSummary(h_count, m_count, e_count);
    return 0;
}



