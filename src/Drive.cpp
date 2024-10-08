#include "Drive.h"
#include "part.h"
#include "KernelFS.h"
#include "Cache.h"
#include "RootDir.h"
#include "FAT.h"
#include "FileQueue.h"
#include "KernelFile.h"
#include "file.h"


Drive::Drive(Partition *partition, KernelFS *myKFS){

	myPartition = partition;
	myKernelFS = myKFS;
	myCache = new Cache(partition, 4, ClusterSize);
	myRoot = new RootDir(this);
	fat = new FAT(partition->getNumOfClusters(), this);
	noOpenFiles = CreateSemaphore(NULL, 1, 32, NULL);
	partitionAccess = CreateSemaphore(NULL, 1, 32, NULL);
	incDecFiles = CreateSemaphore(NULL, 1, 32, NULL);
	fatAccess = CreateSemaphore(NULL, 1, 32, NULL);
	openFiles = 0;
	qfiles = new FileQueue();
	fat->loadFAT();
	myRoot->loadRoot(fat->rootClusterStartsAt(), fat->rootEntrySize());
}


Drive::~Drive(){
	delete myRoot;
	delete fat;
	delete qfiles;
	delete myCache;

}

void Drive::format(){
	waitA(noOpenFiles);
	fat->format(myPartition->getNumOfClusters());	
	myRoot->initialize(fat->rootClusterStartsAt());
	signalA(noOpenFiles);
}

void Drive::unmount(){
	waitA(noOpenFiles);
	myRoot->flushRoot();
	fat->flushFAT();
	myCache->flushCache();
}

char Drive::checkExists(char *address){
	short i = 0;
	unsigned long fpom = 0;
	short result = qfiles->findFile(address, i);
	return (result == 1 ? 1 : (myRoot->findEntryOnDisk(address,fpom) != NULL ? 1:0));
}

void Drive::readCl(ClusterNo num, char* buffer){
	waitA(partitionAccess);
	myCache->readCl(num, buffer);
	signalA(partitionAccess);
}

void Drive::readPartCl(ClusterNo num, char* buffer, BytesCnt start, BytesCnt amount){
	waitA(partitionAccess);
	myCache->readPartCl(num, buffer, start, amount);
	signalA(partitionAccess);
}

void Drive::writeCl(ClusterNo num, char* buffer){
	waitA(partitionAccess);
	myCache->writeCl(num, buffer);
	signalA(partitionAccess);
}

void Drive::writePartCl(ClusterNo num, char* buffer, BytesCnt start, BytesCnt amount){
	waitA(partitionAccess);
	myCache->writePartCl(num, buffer, start, amount);
	signalA(partitionAccess);
}

ClusterNo Drive::freeClusterStartsAt(){
	waitA(fatAccess);
	ClusterNo temp = fat->freeClusterStartsAt();
	signalA(fatAccess);
	return temp;
}

ClusterNo Drive::numOfFATClusters(){
	waitA(fatAccess);
	ClusterNo temp = fat->numOfFATClusters();
	signalA(fatAccess);
	return temp;
}

ClusterNo Drive::rootClusterStartsAt(){
	waitA(fatAccess);
	ClusterNo temp = fat->rootClusterStartsAt();
	signalA(fatAccess);
	return temp;
}

ClusterNo Drive::rootEntrySize(){
	waitA(fatAccess);
	ClusterNo temp = fat->rootEntrySize();
	signalA(fatAccess);
	return temp;
}

char Drive::readDir(char *dirName, EntryNum n, Entry &e){
	return myRoot->readEntry(n, e);
}

Entry* Drive::addEntry(char *name, unsigned long &eCluster){
	return myRoot->addEntry(name, eCluster);
}

char Drive::addDirEntry(char* dirname){
	return myRoot->addDirEntry(dirname);
}


char Drive::updateEntry(Entry* entry,unsigned long eCluster, unsigned long newSize, unsigned long newFCluster){
	return myRoot->updateEntry(entry, eCluster, newSize, newFCluster);
}

char Drive::updateEntry(Entry* entry,unsigned long eCluster, unsigned long newSize){
	return myRoot->updateEntry(entry,eCluster, newSize);
}

ClusterNo Drive::getFreeClusters(ClusterNo amount){
	waitA(fatAccess);
	ClusterNo temp = fat->getFreeClusters(amount);
	signalA(fatAccess);
	return temp;
}

ClusterNo Drive::attachClusters(ClusterNo amount, ClusterNo start){
	waitA(fatAccess);
	ClusterNo temp = fat->attachClusters(amount, start);
	signalA(fatAccess);
	return temp;
}

ClusterNo Drive::getNextOf(ClusterNo target){
	waitA(fatAccess);
	ClusterNo temp = fat->getNextOf(target);
	signalA(fatAccess);
	return temp;
}

void Drive::freeNextOf(ClusterNo target){
	waitA(fatAccess);
	fat->freeNextOf(target);
	signalA(fatAccess);
}

void Drive::freeClusters(ClusterNo start){
	waitA(fatAccess);
	fat->freeClusters(start);
	signalA(fatAccess);
}

KernelFile* Drive::open(char* fname, char mode){
	incOpenFiles();

	if (mode == 'r'){
		if (qfiles->getFile(fname)){  // fajl postoji, i dobili smo pravo na njega
			unsigned long eCluster = 0;
			Entry *pom = myRoot->findEntryOnDisk(fname,eCluster);
			KernelFile *temp = new KernelFile(this,eCluster, pom, fname);
			temp->seek(0);
			temp->mode = 'r';
			return temp;
		}
		else { // fajl ne postoji, da li je na disku? (recent mount)
			unsigned long eCluster = 0;
			Entry *entry = myRoot->findEntryOnDisk(fname, eCluster);
			if (entry != NULL){
				qfiles->addFile(fname);
				KernelFile *temp = new KernelFile(this,eCluster, entry, fname);
				temp->seek(0);
				temp->mode = 'r';
				return temp;
			}
		}
		decOpenFiles();  // ako je usao u 'r' i dodje ovde -> fajl ne postoji
		return NULL;
	};

	// KRAJ ZA 'R'

	if (mode == 'w'){
		short state = 0;
//		int result = qfiles->findFile(fname, state);
		int result = qfiles->getFile(fname);
		if (!result){  // nema fajla u file queue-u
			unsigned long eCluster = 0;
			Entry *e = myRoot->findEntryOnDisk(fname,eCluster);
			if (e == NULL) {
				e = myRoot->addEntry(fname, eCluster);
			}
			if (e == NULL) {  // ako je i ovde NULL -> nije uspeo da ga doda na disk
				decOpenFiles();
				return NULL;
			}
			else{
				qfiles->addFile(fname);  // implicitno je mode w
				return new KernelFile(this,eCluster, e, fname);
			}
		}
		else {  // fajl postoji
			if (state) {  // ako je otvoren, vraca NULL
				decOpenFiles();
				return NULL;
			}
			else {  // nije otvoren, brisemo ga
//				qfiles->getFile(fname);
				unsigned long eCluster = 0;
				Entry *pom = myRoot->findEntryOnDisk(fname, eCluster);
				KernelFile *temp = new KernelFile(this,eCluster, pom , fname);
				if (temp){
					temp->seek(0); // new
					temp->truncate();
					temp->mode = 'w';
					return temp;
				}
				decOpenFiles();
				return temp;
			}
		}
	}

	// KRAJ ZA 'W'

	else {  // ako je 'a'

		if (qfiles->getFile(fname)){  // fajl postoji, i dobili smo pravo na njega
			unsigned long eCluster = 0;
			Entry *pom = myRoot->findEntryOnDisk(fname, eCluster);
			KernelFile *temp = new KernelFile(this,eCluster, pom, fname);
			if (temp){
				temp->seek(temp->getFileSize());
				temp->mode = 'a';
				return temp;
			}
			else {
				decOpenFiles();
				return NULL;
			}
		}
		else {  // da li je na disku?
			unsigned long eCluster = 0;
			Entry *entry = myRoot->findEntryOnDisk(fname, eCluster);
			if (entry != NULL){
				qfiles->addFile(fname);
				KernelFile *temp = new KernelFile(this,eCluster, entry, fname);
				temp->seek(temp->getFileSize());
				temp->mode = 'a';
				return temp;
			}
		}
	}
}

char Drive::deleteFile(char* fname){  // delete, ako nije otvoren
	qfiles->deleteFile(fname);
	myRoot->deleteEntry(fname);
	return 1;
}

char Drive::deleteDir(char *dirname){
	return myRoot->deleteDir(dirname);
}


void Drive::incOpenFiles(){
	waitA(incDecFiles);
	if (!openFiles) waitA(noOpenFiles);
	openFiles++;
	signalA(incDecFiles);
}

void Drive::decOpenFiles(){
	waitA(incDecFiles);
	openFiles--;
	if (!openFiles) signalA(noOpenFiles);
	signalA(incDecFiles);

}

void Drive::incDirEntrys(){
	fat->incDirEntrys();
}

void Drive::decDirEntries(){
	fat->decDirEntries();
}

void Drive::closeFile(char* fname){
	qfiles->signalInQueue(fname);
	decOpenFiles();
}

ClusterNo Drive::positionOnLast(ClusterNo start){
	waitA(fatAccess);
	ClusterNo temp = fat->positionOnLast(start);
	signalA(fatAccess);
	return temp;
}
