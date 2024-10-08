#pragma once

#include "semapfores.h"
#include "part.h"
#include "fs.h" 

class KernelFS;
class Partition;
class FAT;
class Cache;
class RootDir;
class FileQueue;
class KernelFile;

class Drive{
private:
	Partition *myPartition;
	KernelFS *myKernelFS;
	RootDir *myRoot;
	FAT *fat;
	HANDLE noOpenFiles;
	HANDLE partitionAccess;
	HANDLE incDecFiles;
	HANDLE fatAccess;
	int openFiles;

	FileQueue *qfiles;
	Cache *myCache;
public:
	Drive(Partition*, KernelFS*);
	~Drive();
	void format();
	void unmount();
	char checkExists(char *address);
	void readCl(ClusterNo, char*);
	void readPartCl(ClusterNo, char*, BytesCnt start, BytesCnt amount);
	void writeCl(ClusterNo, char*);
	void writePartCl(ClusterNo, char*, BytesCnt start, BytesCnt amount);

	friend class KernelFS;

	ClusterNo freeClusterStartsAt(); // pocetak liste slobodnih klastera
	ClusterNo numOfFATClusters(); // broj klastera koji zauzima FAT tabela
	ClusterNo rootClusterStartsAt(); // broj ulaza koji predstavlja pocetak sadrzaja korenog direktorijuma
	ClusterNo rootEntrySize(); // velicina korenog direktorijuma

	char readDir(char* dirname, EntryNum n, Entry &e);

	Entry* addEntry(char*, unsigned long &EntrysCluster);
	char addDirEntry(char*);
	char updateEntry(Entry*,unsigned long EntrysCluster, unsigned long newSize, unsigned long newFCluster);
	char updateEntry(Entry*,unsigned long EntrysCluster, unsigned long newSize);


	ClusterNo getFreeClusters(ClusterNo amount);
	ClusterNo attachClusters(ClusterNo amount, ClusterNo start);
	ClusterNo getNextOf(ClusterNo target);
	void freeNextOf(ClusterNo target);
	void freeClusters(ClusterNo start);
//	int checkIsFree(ClusterNo amount);

	KernelFile* open(char*, char);
	char deleteFile(char*);
	char deleteDir(char*);
//	char deleteEntry(Entry*);

	void incOpenFiles();
	void decOpenFiles();
	
	void incDirEntrys();
	void decDirEntries();

	void closeFile(char*);
	ClusterNo positionOnLast(ClusterNo start);
};

