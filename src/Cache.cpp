#include "Cache.h"
#include <cstdlib>
#include <cstring>
#include "fs.h"


Cache::Cache(Partition *myP, unsigned long aamount, unsigned long bblockS){
	myPartition = myP;
	amount = aamount;
	blockSize = bblockS;
	nextToLoad = 0;
	Dirty = new short[aamount];
	Valid = new short[aamount];
	Tag = new int[aamount];
	dataC = new char[aamount*bblockS];
	for (int i = 0; i < amount; Valid[i++] = 0);
}


Cache::~Cache(){
	delete[]Dirty;
	delete[]Valid;
	delete[]Tag;
	delete[]dataC;
}


void Cache::readCl(ClusterNo num, char* buffer){
//	waitA(partitionAccess);
	for (int i = 0; i < amount; i++){
		if ((Valid[i]) && (num == Tag[i])){
			memcpy(buffer, &(dataC[i*blockSize]), ClusterSize);
			return;
		}
	}
	// ovde dolazimo ako blok nije u kesu -> ucitava se sa particije
	if ((Valid[nextToLoad]) && (Dirty[nextToLoad])) {  // ako je u njega upisivano
		myPartition->writeCluster(Tag[nextToLoad], &(dataC[nextToLoad*blockSize]));
	}

	int result = myPartition->readCluster(num, &(dataC[nextToLoad*blockSize]));
	memcpy(buffer, &(dataC[nextToLoad*blockSize]), ClusterSize);
	Valid[nextToLoad] = 1;
	Dirty[nextToLoad] = 0;
	Tag[nextToLoad] = num;
	nextToLoad = (nextToLoad + 1) % amount;
//	signalA(partitionAccess);
}

void Cache::readPartCl(ClusterNo num, char* buffer, BytesCnt start, BytesCnt aamount){
//	waitA(partitionAccess);

	for (int i = 0; i < amount; i++){
		if ((Valid[i]) && (num == Tag[i])){
			memcpy(buffer, &(dataC[i*blockSize]) + start, aamount);
			return;
		}
	}
	// ovde dolazimo ako blok nije u kesu -> ucitava se sa particije
	if ((Valid[nextToLoad]) && (Dirty[nextToLoad])) {  // ako je u njega upisivano
		myPartition->writeCluster(Tag[nextToLoad], &(dataC[nextToLoad*blockSize]));
	}

	/*
	int result = myPartition->readCluster(num, cluster);
	memcpy(buffer, cluster + start, amount);*/


	int result = myPartition->readCluster(num, &(dataC[nextToLoad*blockSize]));
	memcpy(buffer, &(dataC[nextToLoad*blockSize]) + start, aamount);
	Valid[nextToLoad] = 1;
	Dirty[nextToLoad] = 0;
	Tag[nextToLoad] = num;
	nextToLoad = (nextToLoad + 1) % amount;

//	signalA(partitionAccess);
}

void Cache::writeCl(ClusterNo num, char* buffer){
//	waitA(partitionAccess);


	for (int i = 0; i < amount; i++){
		if ((Valid[i]) && (num == Tag[i])){
			memcpy(&(dataC[i*blockSize]), buffer, ClusterSize);
			Dirty[i] = 1;
			return;
		}
	}
	// ovde dolazimo ako blok nije u kesu -> ucitava se sa particije
	if ((Valid[nextToLoad]) && (Dirty[nextToLoad])) {  // ako je u njega upisivano
		myPartition->writeCluster(Tag[nextToLoad], &(dataC[nextToLoad*blockSize]));
	}

	memcpy(&(dataC[nextToLoad*ClusterSize]), buffer, ClusterSize);
	Valid[nextToLoad] = 1;
	Dirty[nextToLoad] = 1;
	Tag[nextToLoad] = num;
	nextToLoad = (nextToLoad + 1) % amount;
//	signalA(partitionAccess);
}

void Cache::writePartCl(ClusterNo num, char* buffer, BytesCnt start, BytesCnt aamount){
//	waitA(partitionAccess);

	for (int i = 0; i < amount; i++){
		if ((Valid[i]) && (num == Tag[i])){
			memcpy(&(dataC[i*blockSize]) + start, buffer, aamount);
			Dirty[i] = 1;
			return;
		}
	}
	// ovde dolazimo ako blok nije u kesu -> ucitava se sa particije
	if ((Valid[nextToLoad]) && (Dirty[nextToLoad])) {  // ako je u njega upisivano
		myPartition->writeCluster(Tag[nextToLoad], &(dataC[nextToLoad*blockSize]));
	}

	/*
	int result = myPartition->readCluster(num, cluster);
	memcpy(cluster + start, buffer, amount);
	result = myPartition->writeCluster(num, cluster);*/


	int result = myPartition->readCluster(num, &(dataC[nextToLoad*blockSize]));
	memcpy(&(dataC[nextToLoad*blockSize]) + start, buffer, aamount);
	Valid[nextToLoad] = 1;
	Dirty[nextToLoad] = 1;
	Tag[nextToLoad] = num;
	nextToLoad = (nextToLoad + 1) % amount;
//	signalA(partitionAccess);
}

void Cache::flushCache(){
	for (int i = 0; i < amount; i++){
		if (Valid[i]){
			Dirty[i] = 0;
			myPartition->writeCluster(Tag[i], &(dataC[i*blockSize]));
		}
	}
}