
#include "fs.h"
#include "disk.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define FS_MAGIC           0xf0f03410
#define INODES_PER_BLOCK   128
#define POINTERS_PER_INODE 5
#define POINTERS_PER_BLOCK 1024
#define BLOCK_SIZE 				 4096
int *bitmap;
int sizeBitmap;

struct fs_superblock {
	int magic;
	int nblocks;
	int ninodeblocks;
	int ninodes;
}; //4 * 4-byte ints = 16-btye superblock

struct fs_inode {
	int isvalid;
	int size;
	int direct[POINTERS_PER_INODE];
	int indirect;
}; //4 * 4-byte ints = 16-btye inodes


union fs_block { //4kb blocks
	struct fs_superblock super;
	struct fs_inode inode[INODES_PER_BLOCK]; 	//struct fs_inode inodes[128];
	int pointers[POINTERS_PER_BLOCK];					//int pointers[1024]; 4byte ptrs
	char data[DISK_BLOCK_SIZE];								//char data[4096];		1byte chars
};

int fs_format()
{
	union fs_block superblock;
	if (bitmap != NULL) {
		printf("simplefs: Error. disk already mounted\n");
		return 0;
	}

	superblock.super.magic = FS_MAGIC;
	superblock.super.nblocks = disk_size();

	if (superblock.super.nblocks % 10 == 0) {
		superblock.super.ninodeblocks = superblock.super.nblocks/10;
	}
	else {
		superblock.super.ninodeblocks = superblock.super.nblocks/10 + 1;
	}

	superblock.super.ninodes = INODES_PER_BLOCK * superblock.super.ninodeblocks;

	disk_write(0, superblock.data);
	union fs_block reset;
	memset(reset.data, 0, BLOCK_SIZE);
	int i;
	for (i = 1; i < superblock.super.ninodeblocks; i++) {
		disk_write(i, reset.data);
	}

	return 1;
}

void fs_debug()
{
	union fs_block block;
	disk_read(0, block.data);
	printf("superblock:\n");

	if (block.super.magic == FS_MAGIC) {
		printf("valid magic number\n");
	}
	else {
		printf("simplefs: Error. invalid magic number\n");
		return;
	}

	printf("\t%d blocks on disk\n", block.super.nblocks);
	printf("\t%d blocks for inodes\n", block.super.ninodeblocks);
	printf("\t%d inodes total\n", block.super.ninodes);
	union fs_block inodeblock;
	struct fs_inode inode;

	int i;
	for (i = 1; i < block.super.ninodeblocks; i++) {
		disk_read(i, inodeblock.data);
		int j;
		for (j = 0; j < INODES_PER_BLOCK; j++) {
			inode = inodeblock.inode[j];

			if (inode.isvalid) {
				printf("inode %d:\n", j);
				printf("\tsize: %d bytes\n", inode.size);
				printf("\tdirect blocks:");
				int k;
				for (k = 0; (k * BLOCK_SIZE < inode.size) && (k < 5); k++) {
					printf(" %d", inode.direct[k]);
				}
				printf("\n");

				if (inode.size > 5 * BLOCK_SIZE) {
					printf("\tindirect block: %d\n", inode.indirect);

					union fs_block blockforindirects;
					disk_read(inode.indirect, blockforindirects.data);

					int indirectblocks;
					if (inode.size % BLOCK_SIZE == 0) {
						indirectblocks = inode.size/BLOCK_SIZE - 5;
					}
					else {
						indirectblocks = inode.size/BLOCK_SIZE - 5 + 1;
					}

					printf("\tindirect data blocks:");

					int l;
					for (l = 0; l < indirectblocks; l++) {
						printf(" %d", blockforindirects.pointers[l]);
					}
					printf("\n");
				}
			}
		}
	}
}

int fs_mount()
{

	//mount instruction
	/* Creates a new filesystem on the disk, destroying any data already present.
	Sets aside ten percent of the blocks for inodes, clears the inode table, and
	writes the superblock. Note that formatting a filesystem does not cause it to
	be mounted.	Also, an attempt to format an already-mounted disk should do nothing
	and return failure. */

	union fs_block block;
	disk_read(0, block.data);
	bitmap = calloc(block.super.nblocks, sizeof(int));
	sizeBitmap = block.super.nblocks;
	union fs_block inode_block;
	struct fs_inode inode;
	int i;

	for (i = 1; i < block.super.ninodeblocks; i++) {
		disk_read(i, inode_block.data);
		int j;
		for (j = 0; j < INODES_PER_BLOCK; j++) {
			inode = inode_block.inode[j];
			if (inode.isvalid) {
				bitmap[i] = 1;
				int k;
				for (k = 0; k * BLOCK_SIZE < inode.size && k < 5; k++) {
					bitmap[inode.direct[k]] = 1;
				}
				if (inode.size > 5 * BLOCK_SIZE) {
					bitmap[inode.indirect] = 1;

					union fs_block temp;
					disk_read(inode.indirect, temp.data);
					int q;
					int indirectblocks;
					if (inode.size % BLOCK_SIZE == 0) {
						indirectblocks = inode.size / BLOCK_SIZE - 5;
					}
					else {
						indirectblocks = inode.size / BLOCK_SIZE - 5 + 1;
					}
					for (q = 0; q < indirectblocks; q++) {
						bitmap[temp.pointers[q]] = 1;
					}
				}
			}

		}
	}

	return 1;
}

int getInodeNumber(int blockindex, int inodeindex) {
	int temp = ((blockindex - 1) * INODES_PER_BLOCK) + inodeindex;
	return temp;
}

int fs_create()
{

	if (bitmap == NULL) {
		printf("simplefs: Error! No mounted disk.\n");
		return 0;
	}

	union fs_block block;
	disk_read(0, block.data);

	int i;

	for (i = 1; i < block.super.ninodeblocks + 1; i++) {

		disk_read(i, block.data);

		struct fs_inode inode;
		int j;

		for (j = 0; j < INODES_PER_BLOCK; j++) {
			if (j == 0 && i == 1) {
				j = 1;
			}

			inode = block.inode[j];


			if (!inode.isvalid) {
				inode.size = 0;
				memset(inode.direct, 0, sizeof(inode.direct));
				inode.indirect = 0;
				inode.isvalid = 1;

				bitmap[i] = 1;

				block.inode[j] = inode;
				disk_write(i, block.data);

				// if success return the inode number
				int inodeNumber = getInodeNumber(i, j);
				return inodeNumber;
			}
		}
	}

	//return 0 on failure
	printf("simplefs: Error. Unable to create a new inode\n");
	return 0;
}

int getblock_num(int inumber) {
	int temp = (inumber + INODES_PER_BLOCK - 1) / INODES_PER_BLOCK;
	return temp;
}

int fs_delete(int inumber)
{
	int block_num = getblock_num(inumber);

	union fs_block block;
	disk_read(0, block.data);

	if (block_num > block.super.ninodeblocks)
	{
		printf("simplefs: Error. Block number out of bounds.\n");
		return 0;
	}

	disk_read(block_num, block.data);

	struct fs_inode inode = block.inode[inumber % 128];
	if (inode.isvalid) {

		inode.size = 0;
		memset(inode.direct, 0, sizeof(inode.direct));
		inode.indirect = 0;
		inode.isvalid = 0;
		block.inode[inumber % 128] = inode;
		disk_write(block_num, block.data);
		return 1;
	}

	printf("simplefs: Error. Unable to delete inode.\n");
	return 0;
}

int fs_getsize(int inumber)
{

	int block_num = getblock_num(inumber);
	union fs_block block;
	disk_read(0, block.data);

	// return -1 on error if the block number is valid
	if (block_num > block.super.ninodeblocks)
	{
		printf("simplefs: Error! Block number is out of bounds.\n");
		return -1;
	}

	disk_read(block_num, block.data);

	struct fs_inode inode = block.inode[inumber % 128];

	if (inode.isvalid)
	{
		return inode.size;
	}

	printf("simplefs: Error! Invalid inode number.\n");
	return -1;
}

int fs_read( int inumber, char *data, int length, int offset )
{

	int block_num = getblock_num(inumber);
	union fs_block block;
	disk_read(0, block.data);
	if (block_num > block.super.ninodeblocks || block_num == 0)
	{
		printf("simplefs: Error! Block number is out of bounds.\n");
		return -1;
	}
	disk_read(block_num, block.data);

	struct fs_inode inode;
	inode = block.inode[inumber % 128];

	//check for error
	if (!inode.isvalid)
	{
		printf("simplefs: Error. Invalid inode\n");
		return 0;
	}
	else if (inode.size == 0)
	{
		printf("simplefs: Error. Inode's size is 0\n");
		return 0;
	}

	else if (inode.size < offset)
	{
		printf("simplefs: Error. Offset out of bounds\n");
		return 0;
	}


	int direct_index_num = offset / BLOCK_SIZE;
	if (direct_index_num > 5)
	{
		printf("simplefs: Error. Direct block index out of bounds\n");
		return 0;
	}
	if (inode.size < length + offset)
	{
		length = inode.size - offset;
	}
	union fs_block directblock;
	int totalbytesread = 0;
	memset(data, 0, length);
	int tempbytesread = BLOCK_SIZE;
	while (direct_index_num < 5 && totalbytesread < length)
	{
		disk_read(inode.direct[direct_index_num], directblock.data);
		if (tempbytesread + totalbytesread > length)
		{
			tempbytesread = length - totalbytesread;
		}
		strncat(data, directblock.data, tempbytesread);
		direct_index_num++;
		totalbytesread += tempbytesread;
	}

	// read from indirect block if we still have bytes left to be read
	if (totalbytesread < length)
	{

		// read in the indirect block
		union fs_block indirectblock;
		union fs_block tempblock;
		disk_read(inode.indirect, indirectblock.data);

		// iterate through the indirect data blocks
		int indirectblocks;
		if (inode.size % BLOCK_SIZE == 0)
		{
			indirectblocks = inode.size / BLOCK_SIZE - 5;
		}
		else
		{
			indirectblocks = inode.size / BLOCK_SIZE - 5 + 1;
		}
		int i;
		tempbytesread = BLOCK_SIZE;
		for (i = 0; (i < indirectblocks) && (totalbytesread < length); i++)
		{
			disk_read(indirectblock.pointers[i], tempblock.data);

			// adjust tempbytesread variable if we have reached the end of the inode
			if (tempbytesread + totalbytesread > length)
			{
				tempbytesread = length - totalbytesread;
			}

			// append read data to our data variable
			strncat(data, tempblock.data, tempbytesread);
			totalbytesread += tempbytesread;
		}
	}

	// return the total number of bytes read (could be smaller than the number requested)
	return totalbytesread;
}

int getNextBlock() {
	union fs_block block;
	disk_read(0, block.data); //read superblock

	int i;
	for (i = block.super.ninodeblocks + 1; i < sizeBitmap; i++) {
		if (bitmap[i] == 0) {
			memset(&bitmap[i], 0, sizeof(bitmap[0]));
			return i;
		}
	}

	printf("simplefs: Error. no more room for blocks\n");
	return -1;
}

int fs_write( int inumber, const char *data, int length, int offset )
{
	union fs_block block;
	disk_read(0, block.data);
	if (inumber == 0 || inumber > block.super.ninodes) {
		printf("simplefs: Error. invalid inumber\n");
		return 0;
	}

	int written_bytes = 0;
	int block_num = getblock_num(inumber);

	disk_read(block_num, block.data);

	struct fs_inode inode = block.inode[inumber % 128];
	if (!inode.isvalid) {
		printf("simplefs: Error. invalid inode\n");
	}
	else {
		union fs_block temp;
		int section_size = BLOCK_SIZE;

		int overallIndex = offset / BLOCK_SIZE;
		while (overallIndex < 5 && written_bytes < length) {
			if (inode.direct[overallIndex] == 0) {
				int index = getNextBlock();

				if (index == -1)
				{
					printf("simplefs: Error. No space left to write\n");
					return -1;
				}
				inode.direct[overallIndex] = index;
				bitmap[overallIndex] = 1;
			}

			if (section_size + written_bytes > length) {
				section_size = length - written_bytes;
			}

			strncpy(temp.data, data, section_size);
			data += section_size;

			disk_write(inode.direct[overallIndex], temp.data);
			inode.size += section_size;
			written_bytes += section_size;
			overallIndex++;
		}

		if (written_bytes < length) {
			union fs_block indirect_block;
			if (inode.indirect == 0) {
				int index = getNextBlock();
				if (index == -1) {
					printf("simplefs: Error. no space left to write\n");
					return -1;
				}

				inode.indirect = index;
				bitmap[index] = 1;
			}

			disk_read(inode.indirect, indirect_block.data);

			int blockIndex;
			for (blockIndex = 0; written_bytes < length; blockIndex++) {
				if (indirect_block.pointers[blockIndex] == 0) {
					int index = getNextBlock();
					if (index == -1) {
						printf("simplefs: Error. no space left to write\n");
						return -1;
					}
					inode.direct[overallIndex] = index;
				}

				if (section_size + written_bytes > length) {
					section_size = length - written_bytes;
				}
				strncpy(temp.data, data, section_size);
				data += section_size;
				disk_write(inode.direct[overallIndex], temp.data);
				inode.size += section_size;
				written_bytes += section_size;
			}
		}
		block.inode[inumber % 128] = inode;
		disk_write(block_num, block.data);
		return written_bytes;
	}

	return 0;
}
