// ProcessMemoryLog.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include <ctype.h>

#define BUF_SIZE 8000
#define REC_SIZE 1024
#define PROCNAME_SIZE 242

char *results = "MemoryResults.txt";
FILE *fp = NULL;

struct processrecord {
	char name[PROCNAME_SIZE];
	long code[REC_SIZE];
	long data[REC_SIZE];
	long rw[REC_SIZE];
	long stack[REC_SIZE];
	long reserved[REC_SIZE];
	int count;

};

struct dataandstack {
	long total[REC_SIZE];
	int count;
};

struct kernelmemory {
	long size[10][REC_SIZE];
	long used[10][REC_SIZE];
	long max[10][REC_SIZE];
	long extra[10][REC_SIZE];
	long entries[10][REC_SIZE];
	char name[10][PROCNAME_SIZE];
	int count;
};

struct kernelrecord {
	long totalpages[REC_SIZE];
	long freepages[REC_SIZE];
	long minfreepages[REC_SIZE];
	long currentbytes[REC_SIZE];
	long maxusedbytes[REC_SIZE];
	long pagesbytes[REC_SIZE];
    //
	struct kernelmemory KernelMemory;
	//
	long totalused[REC_SIZE];
	long totalextra[REC_SIZE];
	long waste[REC_SIZE];
	int count;
};

struct dataandstack DataAndStack;
struct kernelrecord KernelRecord;
//struct kernelmemory KernelMemory;
#define MAX_PROCESSES 32
struct processrecord Processes[MAX_PROCESSES];
int processcount = 0;

static int GetProcessRecord(char *processname)
{
	int j = 0, idx = -1;
	char tmpName[PROCNAME_SIZE];

	// copy name here
	processname++; // move off single quote

	while (*processname != '\'')
	{
		tmpName[j++] = *processname++;
		if (j == PROCNAME_SIZE)
			return -1;
	}
	tmpName[j] = 0;

	for (int i = 0; idx < MAX_PROCESSES; i++)
	{
		//
		// if processname equal struct name then we found it!
		if (strncmp(Processes[i].name, tmpName, strlen(tmpName)) == 0)
		{
			idx = i;
			break;
		}

		//
		// if at end grab it
		if (Processes[i].name[0] == 0)
		{
			// copy name here
			strcpy_s(Processes[i].name, 224, tmpName);
			idx = i;
			break;
		}

		//
		// if its a match
		if (strncmp(Processes[i].name, processname, strlen(Processes[i].name)) == 0)
		{
			idx = i;
			break;
		}
	}

	return idx;
}

static char * SkipToNextNumber(char *buf)
{
	char *num = buf;

	// move off current number
	while (isdigit(*num))
	{
		num++;
		if (*num == 0)
			return NULL;
	}

	// find beginning of next number
	while (!isdigit(*num))
	{
		num++;
		if (*num == 0)
			return NULL;
	}

	return num;
}

static char * SkipToChar(char *buf, char target)
{
	char *ptr = buf;

	while (*ptr != target)
	{
		ptr++;
		if (*ptr == 0)
			return NULL;
	}

	return ptr;
}

static char * SkipToNextProcessName(char *buf)
{
	char *name = buf;

	while (*name != '\'')
	{
		name++;
		if (*name == 0)
			return NULL;
		if (strncmp("Windows CE>", name - 11, 11) == 0)
			return NULL;

		// skip CMD.exe 
		if (strncmp(name, "\'CMD", 4) == 0 || strncmp(name, "\'cmd", 4) == 0)
		{
			name++;
			name = SkipToChar(name, '\'');
			name++;  // move off single quote
		}

		// skip my shells
		if (strncmp(name, "\'shell", 6) == 0)
		{
			name++;
			name = SkipToChar(name, '\'');
			name++;  // move off single quote
		}
	}

	return name;
}

static long Average(long *ary, long num)
{
	long ave = 0;
	double total = 0;

	for (int i = 0; i < num; i++)
	{
		total += (ary[i]);
	}

	ave = (long)(total / num);
	return ave;
}

static void PrintSection(char *section, long *ary, long count)
{
	printf("%s\t\t%d\t%d\t%d\t%d\t%d\n", section, count, ary[0], ary[count - 1], ary[count - 1] - ary[0], Average(ary, count));
}

static void PrintProcessInfo()
{
	int idx;

	for (idx = 0; idx < MAX_PROCESSES; idx++)
	{
		// Skip any empty slots
		if (Processes[idx].name[0] == 0)
			continue;

		printf("%s\n", Processes[idx].name);
		printf("\tSection\t\tCount\tBegin\tEnd\tDiff\tAverage\n");
		PrintSection("\tCode", Processes[idx].code, Processes[idx].count);
		PrintSection("\tData", Processes[idx].data, Processes[idx].count);
		PrintSection("\tR/W", Processes[idx].rw, Processes[idx].count);
		PrintSection("\tStack", Processes[idx].stack, Processes[idx].count);
		PrintSection("\tReserve", Processes[idx].reserved, Processes[idx].count);

		//printf("Code\t%d\t%d\t%d\t%d\n", Processes[idx].count, Processes[idx].code[0], Processes[idx].code[Processes[idx].count - 1], Average(Processes[idx].code, Processes[idx].count));
	}
}

void PrintDataAndStackInfo()
{
	printf("\n\nData + Stack\n");
	printf("\tCount\tBegin\tEnd\tDiff\tAverage\n");
	printf("\t%d\t%d\t%d\t%d\t%d\n\n", DataAndStack.count, DataAndStack.total[0], DataAndStack.total[DataAndStack.count - 1], DataAndStack.total[DataAndStack.count - 1] - DataAndStack.total[0], Average(DataAndStack.total, DataAndStack.count));

}

void PrintKernelGeneral()
{
	printf("\nKernel Memory\n\n");
	
	printf("\t\t\tcount\tBegin\tEnd\tDiff\tAverage\n");
//	printf("Total Pages\n");
//	printf("\t%d\t%d\t%d\t%d\n", KernelRecord.totalpages[0], KernelRecord.totalpages[KernelRecord.count - 1], KernelRecord.totalpages[KernelRecord.count - 1] - KernelRecord.totalpages[0], Average(KernelRecord.totalpages, KernelRecord.count));
	PrintSection("Total Pages", KernelRecord.totalpages, KernelRecord.count);
//	printf("Free\n");
//	printf("\t%d\t%d\t%d\t%d\n", KernelRecord.freepages[0], KernelRecord.freepages[KernelRecord.count - 1], KernelRecord.freepages[KernelRecord.count - 1] - KernelRecord.freepages[0], Average(KernelRecord.freepages, KernelRecord.count));
	PrintSection("Free Pages", KernelRecord.freepages, KernelRecord.count);
//	printf("Min Free\n");
//	printf("\t%d\t%d\t%d\t%d\n", KernelRecord.minfreepages[0], KernelRecord.minfreepages[KernelRecord.count - 1], KernelRecord.minfreepages[KernelRecord.count - 1] - KernelRecord.minfreepages[0], Average(KernelRecord.minfreepages, KernelRecord.count));
	PrintSection("Min Free", KernelRecord.minfreepages, KernelRecord.count);
//	printf("Current Bytes\n");
//	printf("\t%d\t%d\t%d\t%d\n", KernelRecord.currentbytes[0], KernelRecord.currentbytes[KernelRecord.count - 1], KernelRecord.currentbytes[KernelRecord.count - 1] - KernelRecord.currentbytes[0], Average(KernelRecord.currentbytes, KernelRecord.count));
	PrintSection("Min Free", KernelRecord.minfreepages, KernelRecord.count);
//	printf("Max Used Bytes\n");
//	printf("\t%d\t%d\t%d\t%d\n", KernelRecord.maxusedbytes[0], KernelRecord.maxusedbytes[KernelRecord.count - 1], KernelRecord.maxusedbytes[KernelRecord.count - 1] - KernelRecord.maxusedbytes[0], Average(KernelRecord.maxusedbytes, KernelRecord.count));
	PrintSection("Max Bytes", KernelRecord.maxusedbytes, KernelRecord.count);
//	printf("Total Used\n");
//	printf("\t%d\t%d\t%d\t%d\n", KernelRecord.totalused[0], KernelRecord.totalused[KernelRecord.count - 1], KernelRecord.totalused[KernelRecord.count - 1] - KernelRecord.totalused[0], Average(KernelRecord.totalused, KernelRecord.count));
	PrintSection("Total Used", KernelRecord.totalused, KernelRecord.count);
//	printf("Total Extra\n");
//	printf("\t%d\t%d\t%d\t%d\n", KernelRecord.totalextra[0], KernelRecord.totalextra[KernelRecord.count - 1], KernelRecord.totalextra[KernelRecord.count - 1] - KernelRecord.totalextra[0], Average(KernelRecord.totalextra, KernelRecord.count));
	PrintSection("Total Extra", KernelRecord.totalextra, KernelRecord.count);
//	printf("Waste\n");
//	printf("\t%d\t%d\t%d\t%d\n", KernelRecord.waste[0], KernelRecord.waste[KernelRecord.count - 1], KernelRecord.waste[KernelRecord.count - 1] - KernelRecord.waste[0], Average(KernelRecord.waste, KernelRecord.count));
	PrintSection("Waste   ", KernelRecord.waste, KernelRecord.count);
}

void PrintKernelSections()
{
	printf("\nKernel Sections\n");

	for (int i = 0; i < 10; i++)
	{
		if (KernelRecord.KernelMemory.name[i][0] != 0)
		{
			printf("\n%s\n", KernelRecord.KernelMemory.name[i]);
			printf("\tBegin\tEnd\tDiff\tAve\n");
			printf("Size\t%d\t%d\t%d\t%d\n", KernelRecord.KernelMemory.size[i][0],
				KernelRecord.KernelMemory.size[i][KernelRecord.KernelMemory.count - 1],
				KernelRecord.KernelMemory.size[i][KernelRecord.KernelMemory.count - 1] - KernelRecord.KernelMemory.size[i][0],
				Average(KernelRecord.KernelMemory.size[i], KernelRecord.KernelMemory.count));
			printf("Used\t%d\t%d\t%d\t%d\n", KernelRecord.KernelMemory.used[i][0],
				KernelRecord.KernelMemory.used[i][KernelRecord.KernelMemory.count - 1],
				KernelRecord.KernelMemory.used[i][KernelRecord.KernelMemory.count - 1] - KernelRecord.KernelMemory.used[i][0],
				Average(KernelRecord.KernelMemory.used[i], KernelRecord.KernelMemory.count));
			printf("Max\t%d\t%d\t%d\t%d\n", KernelRecord.KernelMemory.max[i][0],
				KernelRecord.KernelMemory.max[i][KernelRecord.KernelMemory.count - 1],
				KernelRecord.KernelMemory.max[i][KernelRecord.KernelMemory.count - 1] - KernelRecord.KernelMemory.max[i][0],
				Average(KernelRecord.KernelMemory.max[i], KernelRecord.KernelMemory.count));
			printf("Extra\t%d\t%d\t%d\t%d\n", KernelRecord.KernelMemory.extra[i][0],
				KernelRecord.KernelMemory.extra[i][KernelRecord.KernelMemory.count - 1],
				KernelRecord.KernelMemory.extra[i][KernelRecord.KernelMemory.count - 1] - KernelRecord.KernelMemory.extra[i][0],
				Average(KernelRecord.KernelMemory.extra[i], KernelRecord.KernelMemory.count));
			printf("Entries\t%d\t%d\t%d\t%d\n", KernelRecord.KernelMemory.entries[i][0],
				KernelRecord.KernelMemory.entries[i][KernelRecord.KernelMemory.count - 1],
				KernelRecord.KernelMemory.entries[i][KernelRecord.KernelMemory.count - 1] - KernelRecord.KernelMemory.entries[i][0],
				Average(KernelRecord.KernelMemory.extra[i], KernelRecord.KernelMemory.count));
		}
	}
}

void Report()
{

	PrintProcessInfo();
	PrintDataAndStackInfo();
	PrintKernelGeneral();
	PrintKernelSections();
}


int ProcessRecord(char *record, int len)
{
	int retval = 0;
	int size = 0, buflocation = 0, pidx;
	char *ptr1, *ptr2;

	//
	// first thing to find is the 'Page size' and read off page and byte info
	if ((ptr1 = strstr(record, "Page size")) == NULL)
	{
		return -1;
	}

	//
	// throw away page size and get next int which is total pages
	ptr1 = SkipToChar(ptr1, ',');

	// skip comma and space
	ptr1 = SkipToNextNumber(ptr1);
	KernelRecord.totalpages[KernelRecord.count] = atoi(ptr1);

	ptr1 = SkipToNextNumber(ptr1);
	KernelRecord.freepages[KernelRecord.count] = atoi(ptr1);

	ptr1 = SkipToNextNumber(ptr1);
	KernelRecord.minfreepages[KernelRecord.count] = atoi(ptr1);

	ptr1 = SkipToNextNumber(ptr1);
	KernelRecord.currentbytes[KernelRecord.count] = atoi(ptr1);

	ptr1 = SkipToNextNumber(ptr1);
	KernelRecord.maxusedbytes[KernelRecord.count] = atoi(ptr1);

	//
	// Get process memory info
	while ((ptr2 = SkipToNextProcessName(ptr1)) != NULL)
	{
		ptr1 = ptr2;
		pidx = GetProcessRecord(ptr1);

		ptr1 = SkipToChar(ptr1, '\n');  // skip process id
		ptr1 = SkipToNextNumber(ptr1);
		Processes[pidx].code[Processes[pidx].count] = atoi(ptr1);

		ptr1 = SkipToNextNumber(ptr1);  // skip min number
		ptr1 = SkipToNextNumber(ptr1);
		Processes[pidx].data[Processes[pidx].count] = atoi(ptr1);

		ptr1 = SkipToNextNumber(ptr1);
		Processes[pidx].rw[Processes[pidx].count] = atoi(ptr1);

		ptr1 = SkipToNextNumber(ptr1);
		Processes[pidx].stack[Processes[pidx].count] = atoi(ptr1);

		ptr1 = SkipToNextNumber(ptr1);
		Processes[pidx].reserved[Processes[pidx].count] = atoi(ptr1);
		
		// this process has one count
		Processes[pidx].count++;
	}

	//
	// get totoal R/W data + stack info
	if ((ptr1 = strstr(ptr1, "data + stack")) == NULL)
	{
		return -3;
	}

	ptr1 = SkipToNextNumber(ptr1);
	DataAndStack.total[DataAndStack.count++] = atoi(ptr1);

	//
	// get reset of kernel info
	if ((ptr1 = strstr(ptr1, "0:")) == NULL)
	{
		return -4;
	}

	for (int i = 0; i < 8; i++)
	{
		ptr1 = SkipToNextNumber(ptr1);
		KernelRecord.KernelMemory.size[i][KernelRecord.KernelMemory.count] = atoi(ptr1);
		ptr1 = SkipToNextNumber(ptr1);
		KernelRecord.KernelMemory.used[i][KernelRecord.KernelMemory.count] = atoi(ptr1);
		ptr1 = SkipToNextNumber(ptr1);
		KernelRecord.KernelMemory.max[i][KernelRecord.KernelMemory.count] = atoi(ptr1);
		ptr1 = SkipToNextNumber(ptr1);
		KernelRecord.KernelMemory.extra[i][KernelRecord.KernelMemory.count] = atoi(ptr1);
		ptr1 = SkipToNextNumber(ptr1);
		KernelRecord.KernelMemory.entries[i][KernelRecord.KernelMemory.count] = atoi(ptr1);

		// Skip to end of paren-ed number
		ptr1 = SkipToChar(ptr1, ')');

		//
		// Get the name the first time.
		if (KernelRecord.KernelMemory.count == 0)
		{
			int j = 0;
			ptr1 = SkipToChar(ptr1, ' ');
			ptr1++;

			while (*ptr1 != ' ')
				KernelRecord.KernelMemory.name[i][j++] = *ptr1++;
			KernelRecord.KernelMemory.name[i][j] = 0;
		}
		// skip the index
		ptr1 = SkipToNextNumber(ptr1);
	}

	// Increment the count
	KernelRecord.KernelMemory.count++;

	//
	// get last of the data total used, total extra, waste
	if (ptr1 != NULL)
	{
//		ptr1 = SkipToNextNumber(ptr1);
		// the last skip skipped to here!
		KernelRecord.totalused[KernelRecord.count] = atoi(ptr1);
		ptr1 = SkipToNextNumber(ptr1);
		KernelRecord.totalextra[KernelRecord.count] = atoi(ptr1);
		ptr1 = SkipToNextNumber(ptr1);
		KernelRecord.waste[KernelRecord.count] = atoi(ptr1);
	}

	// Done!
	// increment count
	KernelRecord.count++;

	return retval;
}

int ReadRecord(char *buf, int size)
{
	int len = 0;
	int c;

	//
	// clean the buffer we are writing to.
	memset((void *)buf, 0, size);

	//
	// skip until we get to 'Page size'.
	c = fgetc(fp);

	for (len = 0; (len < size) && (feof(fp) == 0); len++)
	{
		if ((char)c == 'P')
		{
			buf[0] = (char)c;
			buf[1] = fgetc(fp);
			buf[2] = fgetc(fp);
			buf[3] = fgetc(fp);
			buf[4] = fgetc(fp);
			buf[5] = fgetc(fp);
			buf[6] = fgetc(fp);
			buf[7] = fgetc(fp);
			buf[8] = fgetc(fp);
			if (strncmp(buf, "Page size", 9) == 0)
			{
				len = 9;
				break;
			}
		}
		c = fgetc(fp);
	}

	//
	// if size reached and 'Page size' not found!
	if (len == size  || len != 9)
		return 0;

	//
	// Read until "END" is found or eof
	while (feof(fp) == 0)
	{
		buf[len++] = fgetc(fp);
		if (strncmp(&buf[len - 8], "- END -", 7) == 0)
		{
			buf[len] = 0;
			break;
		}
	}

	return len;
}

void initializedata()
{
	memset((void *)&Processes, 0, sizeof(Processes));
	memset((void *)&DataAndStack, 0, sizeof(DataAndStack));
	memset((void *)&KernelRecord, 0, sizeof(KernelRecord));
	
}

void usage(char *prog)
{
	fprintf(stderr, "usage: %s <filename>", prog);
	exit(-1);
}

int main(int argc, char *argv[])
{
	char buf[BUF_SIZE+1];
	int len, count = 0;

	if (argc != 2) 
	{
		usage(argv[0]);
	}

	//
	// used to initialize global data structures
	initializedata();

	if ((fp = fopen(argv[1], "r")) == NULL)
	{
		fprintf(stderr, "Unable to open file %s\n", argv[1]);
		exit(-2);
	}

	while ((len = ReadRecord(buf, BUF_SIZE)) > 0)
	{
		ProcessRecord(buf, len);
//		printf("processed record %d\n", ++count);
	}
	fclose(fp);

	Report();

    return 0;
}

