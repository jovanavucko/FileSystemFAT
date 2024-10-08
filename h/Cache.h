#pragma once
#include "part.h"
#include "fs.h"

class Cache{
private:
	char *dataC;
	short *Dirty, *Valid;
	int amount, blockSize, *Tag, nextToLoad;
	Partition *myPartition;
public:
	Cache(Partition *myP, unsigned long aamount, unsigned long bblock);
	~Cache();


	void readCl(ClusterNo, char*);
	void readPartCl(ClusterNo, char*, BytesCnt start, BytesCnt amount);
	void writeCl(ClusterNo, char*);
	void writePartCl(ClusterNo, char*, BytesCnt start, BytesCnt amount);

	void flushCache();
};

