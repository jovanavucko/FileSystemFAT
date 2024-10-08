#pragma once
#include "part.h"
#include "semapfores.h"

class Drive;
class CacheImp;
class KernelFS;

class FAT{
private:
	Drive *myDrive;
	ClusterNo *table;
	ClusterNo brojFATClustera;
	ClusterNo entriesPerCluster;
	HANDLE fatAccess;

public:
	FAT(ClusterNo numCluster, Drive *d);
	~FAT();
	void format(ClusterNo numOfClusters);
	void flushFAT();
	void loadFAT();

	ClusterNo freeClusterStartsAt(); // pocetak liste slobodnih klastera
	ClusterNo numOfFATClusters(); // broj klastera koji zauzima FAT tabela
	ClusterNo rootClusterStartsAt(); // broj ulaza koji predstavlja pocetak sadrzaja korenog direktorijuma
	ClusterNo rootEntrySize(); // velicina korenog direktorijuma

	ClusterNo getFreeClusters(ClusterNo amount);
	ClusterNo attachClusters(ClusterNo amount, ClusterNo start);
	ClusterNo getNextOf(ClusterNo target);
	void freeNextOf(ClusterNo target);
	void freeClusters(ClusterNo start);
	int checkIsFree(ClusterNo amount);

	void incDirEntrys();
	void decDirEntries();

	ClusterNo positionOnLast(ClusterNo start);
};

