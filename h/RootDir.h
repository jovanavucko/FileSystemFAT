#pragma once

#include "part.h"
#include "fs.h"

class Drive;

class RootDir{
private:
	Drive *myDrive;
	Entry *curEntry;
	ClusterNo tag;
	ClusterNo maxEntries , tail;
	int Dirty;
public:
	RootDir(Drive *drive);
	~RootDir();
	void initialize(ClusterNo);
	void flushRoot();
	void loadRoot(ClusterNo tag, ClusterNo tail);

	char readEntry(EntryNum n, Entry &e);  // dodaj ili clusterNo ili char putanju

	Entry* findEntryOnDisk(char *ename, unsigned long &EntrysCluster);
	Entry* addEntry(char *fname, unsigned long &EntrysCluster);
	char addDirEntry(char*);
	char updateEntry(Entry*,unsigned long EntrysCluster, unsigned long newSize, unsigned long newFCluster);
	char updateEntry(Entry*,unsigned long EntrysCluster, unsigned long newSize);
	char deleteEntry(char*);
	char deleteDir(char*);


	Entry *findPath(char* fdName, ClusterNo &EntrysCluster);  // vraca NULL i 0 za neuspeh, NULL i rootCluster za koreni ulaz.

};

