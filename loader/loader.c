/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>

#include "exec_parser.h"

static so_exec_t *exec;
static int exec_descriptor;

so_seg_t *loadSegment(siginfo_t *info)
{
	// find segment that generated page fault
	for (int i = 0; i < exec->segments_no; i++) {

		uintptr_t upperBound = (uintptr_t)exec->segments[i].vaddr + (uintptr_t)exec->segments[i].mem_size;
		uintptr_t lowerBound = (uintptr_t)exec->segments[i].vaddr;

		if ((uintptr_t)info->si_addr >= lowerBound && (uintptr_t)info->si_addr < upperBound)
			return &(exec->segments[i]);
	}
	return NULL;
}

// find page that generated page fault
void checkPage(siginfo_t *info, so_seg_t *mySeg)
{
	int pageSize = getpagesize();
	int page_no = ((char *)info->si_addr - (char *)mySeg->vaddr) / pageSize;

	// initilize data containing pages mapped if not already
	if (mySeg->data == NULL) {
		int mem = (int)mySeg->mem_size;
		int pages = mem / pageSize;

		if (mem % pageSize)
			pages += 1;
		mySeg->data = calloc(pages, sizeof(int));
	} else if (((int *)mySeg->data)[page_no] == 1) {
		// if page was already mapped call default handler
		SIG_DFL(SIGSEGV);
	}

	// mark page as mapped
	((int *)mySeg->data)[page_no] = 1;
}

void copyData(so_seg_t *mySeg, size_t offset, void *pageAddr)
{
	size_t pageSize = getpagesize();

	// set cursor in file
	lseek(exec_descriptor, mySeg->offset + offset, SEEK_SET);
	char *buf = malloc(pageSize * sizeof(char));
	size_t startPage = (char *)mySeg->vaddr + offset;
	size_t endPage = startPage + pageSize;

	if (endPage <= (size_t)(mySeg->vaddr + mySeg->file_size)) {
		// page is fully in segment file size; copy data
		read(exec_descriptor, buf, pageSize);
		memcpy(pageAddr, buf, pageSize);
	} else if (startPage > (size_t)(mySeg->vaddr + mySeg->file_size)) {
		// page is fully beyond segment file size; set bytes to 0
		memset(pageAddr, 0, pageSize);
	} else if (startPage <= (size_t)(mySeg->vaddr + mySeg->file_size)) {
		// page is partially in segment file size; copy data in segment
		// and set to 0 bytes outside the segment
		size_t count = (size_t)(mySeg->vaddr + mySeg->file_size) - startPage;

		read(exec_descriptor, buf, count);
		memset(buf + count, 0, pageSize - count);
		memcpy(pageAddr, buf, pageSize);
	} else {
		free(buf);
		SIG_DFL(SIGSEGV);
	}
	free(buf);

	// protect mapped page
	mprotect(pageAddr, pageSize, mySeg->perm);
}

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	const unsigned int pageSize = getpagesize(info);

	// find segment with page fault
	so_seg_t *mySeg = loadSegment(info);

	// if no segment was found call default handler
	if (mySeg == NULL) {
		SIG_DFL(SIGSEGV);
		return;
	}

	// find and check page generating page fault
	checkPage(info, mySeg);

	// map page into memory
	size_t offset = (char *)info->si_addr - (char *)mySeg->vaddr;
	size_t pageOffset = offset % pageSize;

	offset -= pageOffset;
	void *pageAddr = mmap((void *)(mySeg->vaddr + offset), pageSize,
						PROT_EXEC | PROT_WRITE | PROT_READ, MAP_FIXED | MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	// copy data from file to mapped memory
	copyData(mySeg, offset, pageAddr);
}

int so_init_loader(void)
{
	int rc;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = segv_handler;
	sa.sa_flags = SA_SIGINFO;
	rc = sigaction(SIGSEGV, &sa, NULL);
	if (rc < 0) {
		perror("sigaction");
		return -1;
	}
	return 0;
}

int so_execute(char *path, char *argv[])
{
	// open file in order to copy from it data
	exec_descriptor = open(path, O_RDONLY);

	// parse file
	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	//close file
	close(exec_descriptor);

	return -1;
}
