/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/


#include "page_table.h"
#include "disk.h"
#include "program.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>

#define STREQ(a,b) (strcmp(a,b) == 0)

const char *replacement_alg;
struct disk *disk;
int DISK_READ_COUNT;
int DISK_WRITE_COUNT;
int nframes;
int npages;
unsigned char *virtmem;
unsigned char *physmem;
int PAGE_FAULT_COUNT;
int PHYS_FREE[1024];
int FIFO[4096];
int CUSTOM[4096];
int POP_INDEX=0;
int PUSH_INDEX=0;
int CUSTOM_POP=0;
int CUSTOM_PUSH=0;
int BUFFSIZE;

void page_fault_handler( struct page_table *pt, int page)
{
	int frame, bits;
	page_table_get_entry(pt, page, &frame, &bits);
	int frame_num;

	if (!(bits&PROT_READ)) {
		PAGE_FAULT_COUNT++;
		bool free_bool = false;
		int i;
		for (i=0; i<nframes; i++) {
			if (PHYS_FREE[i] == 0) {
				free_bool = true;
				frame_num = i;
				page_table_set_entry(pt, page, frame_num, PROT_READ);

				if (strcmp(replacement_alg, "fifo") == 0) {
					FIFO[PUSH_INDEX] = page;
					PUSH_INDEX += 1;
				}

				if (strcmp(replacement_alg, "custom") == 0) {
					CUSTOM[CUSTOM_PUSH] = page;
					CUSTOM_PUSH++;
					if (CUSTOM_PUSH-CUSTOM_POP == nframes/BUFFSIZE)
						CUSTOM_POP++;
				}

				disk_read(disk, page, page_table_get_physmem(pt) + frame_num*PAGE_SIZE);
				PHYS_FREE[i] = 1;
				DISK_READ_COUNT++;
				break;
			}
		}

		if (!free_bool)	{

			if (strcmp(replacement_alg, "rand") == 0) {
				// Checking what was previously in a random spot in physcial memory
				int prev_bits = 0;
				int page_num;
				while (!(prev_bits&PROT_READ)) {
					page_num = rand() % npages;
					page_table_get_entry(pt, page_num, &frame_num, &prev_bits);
				}
				if (prev_bits&PROT_WRITE) {
					disk_write(disk, page_num, page_table_get_physmem(pt) + frame_num*PAGE_SIZE);
					DISK_WRITE_COUNT++;
				}
				page_table_set_entry(pt, page_num, 0, 0);
				page_table_set_entry(pt, page, frame_num, PROT_READ);
				disk_read(disk, page, page_table_get_physmem(pt) + frame_num*PAGE_SIZE);
				DISK_READ_COUNT++;

			} else if (strcmp(replacement_alg, "fifo") == 0) {
				int page_num;
				int prev_bits = 0;
				page_num = FIFO[POP_INDEX];
				POP_INDEX += 1;
				page_table_get_entry(pt, page_num, &frame_num, &prev_bits);

				if (prev_bits&PROT_WRITE) {
					disk_write(disk, page_num, page_table_get_physmem(pt) + frame_num*PAGE_SIZE);
					DISK_WRITE_COUNT ++;
				}
				page_table_set_entry(pt, page_num, 0, 0);
				page_table_set_entry(pt, page, frame_num, PROT_READ);

				FIFO[PUSH_INDEX] = page;
				PUSH_INDEX += 1;

				disk_read(disk, page, page_table_get_physmem(pt) + frame_num*PAGE_SIZE);
				DISK_READ_COUNT++;
			} else if (strcmp(replacement_alg, "custom") == 0) {

				// NRU
				// Checking what was previously in a random spot in physcial memory
				int prev_bits = 0;
				int page_num;
				while (!(prev_bits&PROT_READ)) {
					page_num = rand() % npages;
					page_table_get_entry(pt, page_num, &frame_num, &prev_bits);
					int i;
					for (i = CUSTOM_POP; i <= CUSTOM_PUSH; i++) {
						if (page_num == CUSTOM[i]) {
							prev_bits = 0;

							break;
						}
					}
				}

				if (prev_bits&PROT_WRITE) {
					disk_write(disk, page_num, page_table_get_physmem(pt) + frame_num*PAGE_SIZE);
					DISK_WRITE_COUNT++;
				}
				page_table_set_entry(pt, page_num, 0, 0);
				page_table_set_entry(pt, page, frame_num, PROT_READ);

				CUSTOM[CUSTOM_PUSH] = page;
				CUSTOM_PUSH++;
				if (CUSTOM_PUSH-CUSTOM_POP == nframes/BUFFSIZE)
					CUSTOM_POP++;

				disk_read(disk, page, page_table_get_physmem(pt) + frame_num*PAGE_SIZE);
				DISK_READ_COUNT++;

			} else {
				fprintf(stderr,"unknown approach\n");
				exit(1);
			}
		}

		return;

	} else if (!(bits&PROT_WRITE)) {
		PAGE_FAULT_COUNT++;
		page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
		return;
	}

}

int main( int argc, char *argv[] )
{
	srand(time(0));

	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <alpha|beta|gamma|delta>\n");
		return 1;
	}

	npages = atoi(argv[1]);
	nframes = atoi(argv[2]);
	if (nframes < 10) BUFFSIZE = 3;
	else BUFFSIZE = 10;
	if (npages < 3 || nframes < 3) {
		fprintf(stderr,"Error: Not enouge pages or frames\n");
		return 1;
	}
	if (npages < nframes){
		fprintf(stderr,"Error: Less pages than Frames.\n");
		return 1;
	}

	replacement_alg = argv[3];
	const char *program = argv[4];
	PAGE_FAULT_COUNT = 0;
	DISK_READ_COUNT = 0;
	DISK_WRITE_COUNT = 0;
	int i;
	for (i=0; i<nframes; i++) {
		PHYS_FREE[i] = 0;
	}


	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"Error: couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}

	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"Error: couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	virtmem = page_table_get_virtmem(pt);
	physmem = page_table_get_physmem(pt);

	if(!strcmp(program,"alpha")) {
		alpha_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"beta")) {
		beta_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"gamma")) {
		gamma_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"delta")) {
		delta_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"Error: unknown program: %s\n",argv[4]);
		return 1;
	}

	page_table_delete(pt);
	disk_close(disk);

	printf("# Page Faults: %d\n", PAGE_FAULT_COUNT);
	printf("# Disk Reads:  %d\n", DISK_READ_COUNT);
	printf("# Disk Writes: %d\n", DISK_WRITE_COUNT);

	return 0;
}
