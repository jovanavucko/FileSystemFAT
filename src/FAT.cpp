#include "FAT.h"
#include "Drive.h"
#include <cmath>
#include "semapfores.h"


FAT::FAT(ClusterNo numCluster, Drive *d){
	myDrive = d;	
	brojFATClustera = (ClusterNo)((numCluster * 4 + 16) / ClusterSize) + ((numCluster * 4 + 16) % ClusterSize > 0 ? 1 : 0);
	entriesPerCluster = trunc(ClusterSize / 4);
	table = (ClusterNo*)(new char[brojFATClustera*ClusterSize]);
	fatAccess = CreateSemaphore(NULL, 1, 32, NULL);
}


FAT::~FAT(){
	delete []table;
}

void FAT::format(ClusterNo numOfClusters){

//	waitA(fatAccess);
	entriesPerCluster = trunc(ClusterSize / 4);
	table[0] = 5;
	table[1] = brojFATClustera;
	table[2] = 2;
	table[3] = 0;
	table[4] = 0;
	int nextFAT = 6;

	for (int i = 0; i < brojFATClustera; i++){
		int j = (i == 0 ? 5 : 0);
			for (; j < entriesPerCluster; j++){
				table[j + i*entriesPerCluster] = (nextFAT <= numOfClusters+ 4 - brojFATClustera ? nextFAT : 0);
				nextFAT++;
			}
	}  // kraj upisivanja

	flushFAT();

//	signalA(fatAccess);
}

void FAT::flushFAT(){

//	waitA(fatAccess);

	for (int i = 0; i < brojFATClustera-1; i++){
		myDrive->writeCl(i,(char*)(&(table[entriesPerCluster*i])));
	}
	/*
	unsigned long position = entriesPerCluster*(brojFATClustera - 1);
	unsigned long toWrite = (ukupno - position) * 4;
		myDrive->writePartCl(brojFATClustera - 1, (char*)&(table[position]), 0, toWrite);*/

//	signalA(fatAccess);
}

void FAT::loadFAT(){

	//	waitA(fatAccess);

	for (int i = 0; i < brojFATClustera; i++){
		myDrive->readCl(i, (char*)(&(table[entriesPerCluster*i])));
	}

	//	signalA(fatAccess);
}

ClusterNo FAT::freeClusterStartsAt(){
	return table[0];
}

ClusterNo FAT::numOfFATClusters(){
	return table[1];
}

ClusterNo FAT::rootClusterStartsAt(){
	return table[2];
}
ClusterNo FAT::rootEntrySize(){
	return table[3];
}


ClusterNo FAT::getFreeClusters(ClusterNo amount){

//	waitA(fatAccess);

	if (amount && checkIsFree(amount)){
		ClusterNo newFirstCluster = table[0], point = 0;
		do{
			point = table[point];
			amount--;
		}
		while (amount);
		table[0] = table[point]; // novi prazni pocetni klaster
		table[point] = 0;
//		signalA(fatAccess);
		return newFirstCluster - 4 + brojFATClustera;  // -4 za prva 4 ulaza tabele, vracamo BROJ KLASTERA
	}
//	signalA(fatAccess);
	return 0;
}

ClusterNo FAT::attachClusters(ClusterNo amount, ClusterNo start){

//	waitA(fatAccess);

	if (checkIsFree(amount)){
		start = start + 4 - brojFATClustera;  // realni broj klastera prebacujemo U ULAZ TABELE
		while (table[start]) start = table[start];
		while (amount){
			table[start] = table[0];
			table[0] = table[table[0]];
			start = table[start];
			table[start] = 0;
			amount--;
		}
//		signalA(fatAccess);
		return 1;
	}
//	signalA(fatAccess);
	return 0;
}

ClusterNo FAT::getNextOf(ClusterNo target){
	
	return table[target+4-brojFATClustera] -4 + brojFATClustera;
}

void FAT::freeNextOf(ClusterNo target){
	target = target + 4 - brojFATClustera;
	if (!(table[target])) return;
//	waitA(fatAccess);
	ClusterNo temp = table[target];
	while (table[temp]){
		temp = table[temp];
	}
	table[temp] = table[0];
	table[0] = table[target];
	table[target] = 0;
//	signalA(fatAccess);
}

void FAT::freeClusters(ClusterNo start){  //?? za sems
	start = start + 4 - brojFATClustera;
	if (table[start]) freeNextOf(start);
	table[start] = table[0];
	table[0] = start;
}

int FAT::checkIsFree(ClusterNo amount){  // samo lokalno se poziva
	ClusterNo temp = table[0];
	while (amount){
		if (table[temp]) {
			temp = table[temp];
			amount--;
		}
		else return 0;
	}
	return 1;
}


void FAT::incDirEntrys(){
	table[3]++;
}

void FAT::decDirEntries(){
	if (table[3] > 0) table[3]--;
}

ClusterNo FAT::positionOnLast(ClusterNo start){
	start = start + 4 - brojFATClustera;
	while (table[start] != 0) start = table[start];
	return start -4 + brojFATClustera;
}
