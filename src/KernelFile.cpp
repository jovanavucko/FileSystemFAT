#include "KernelFile.h"
#include <cstring>
#include "Drive.h"
#include "FileQueue.h"
#include <cmath>

KernelFile::KernelFile(Drive *ddrive, unsigned long eCluster, Entry* myE, char *nname){

	myDrive = ddrive;
	myEntry = myE;
	EntrysCluster = eCluster;
	fname = new char[strlen(nname)+1];
	strcpy(fname, nname);
	mode = 'w';
	curCluster = myE->firstCluster;
	curPosition = 0;
}

char KernelFile::write(BytesCnt bytes, char* buffer){
	if (mode == 'r') return 0;
	ClusterNo blocks;// = (ClusterNo)(trunc(bytes / ClusterSize) + (bytes%ClusterSize ? 1 : 0));

	if (myEntry->firstCluster == 0){
		ClusterNo temp = myDrive->getFreeClusters(1);
		if (temp){
			curCluster = temp;
			myDrive->updateEntry(myEntry,EntrysCluster,0,temp);
		}
		else return 0;
	}

		BytesCnt available = (trunc(myEntry->size/ClusterSize) + (myEntry->size%ClusterSize ? 1:0)
			+ (myEntry->size==0 ? 1 : 0))*ClusterSize - curPosition;
		if (available < bytes){  // dodajmo jos klastera, ako treba
			blocks = trunc((bytes - available) / ClusterSize) + ((bytes - available) % ClusterSize ? 1 : 0);
			if (!(myDrive->attachClusters(blocks, curCluster))) return 0;
		}
		if (myEntry->size < bytes + curPosition){  
			myDrive->updateEntry(myEntry,EntrysCluster, bytes + curPosition);
		}

		while (bytes){  // dok ima Bajtova za upis

			// pomeranje curCluster-a
			if ((curPosition >0) && (curPosition%ClusterSize == 0))	curCluster = myDrive->getNextOf(curCluster);

			BytesCnt temp = curPosition%ClusterSize;  // pozicija curPosition u odnosu na 1 klaster
			BytesCnt toAdd = ClusterSize - temp > bytes ? bytes : ClusterSize - temp;
			myDrive->writePartCl(curCluster, buffer, temp, toAdd);
			curPosition = curPosition + (ClusterSize - temp > bytes ? bytes : ClusterSize - temp);
			bytes -= toAdd;
			buffer += toAdd; // proveri sta se desava sa pokazivacem van f-ije
//			if (bytes)	curCluster = myDrive->getNextOf(curCluster);
		}

		return 1;

}

BytesCnt KernelFile::read(BytesCnt bytes, char* buffer){
	if (bytes == 0 || buffer == NULL) return 0;
	if (curPosition == myEntry->size) return 0;
	if (myEntry->name[0] == ' ') return 0; // ako je fajl zatvoren... proradi kako se otvara/zatvara 

	BytesCnt canRead = (myEntry->size-curPosition >bytes ? bytes: myEntry->size-curPosition);
	bytes = canRead;

	while (bytes){  // dok ima Bajtova za citanje

		// pomeranje curCluster-a
		if ((curPosition) >0 && (curPosition%ClusterSize == 0))	curCluster = myDrive->getNextOf(curCluster);

		BytesCnt temp = curPosition%ClusterSize;  // pozicija curPosition u odnosu na 1 klaster
		BytesCnt toRead = ClusterSize - temp > bytes ? bytes : ClusterSize - temp;
		myDrive->readPartCl(curCluster, buffer, temp, toRead);
		curPosition = curPosition + (ClusterSize - temp > bytes ? bytes : ClusterSize - curPosition%ClusterSize);
		bytes -= toRead;
		buffer += toRead; // proveri sta se desava sa pokazivacem van f-ije
		//if (bytes)	curCluster = myDrive->getNextOf(curCluster);
	}
	return canRead;
}

char KernelFile::seek(BytesCnt bytes){
	if (bytes> myEntry->size) return 0;
	if (bytes == curPosition) return 1;
	if (curPosition > bytes){
		curCluster = myEntry->firstCluster;
		curPosition = bytes;
		while (trunc(bytes / ClusterSize)){
			bytes -= ClusterSize;
			curCluster = myDrive->getNextOf(curCluster);
		}
		return 1;
	}
	else {
		BytesCnt toMove = bytes - curPosition;
		curPosition = bytes;
		while ((toMove>0) &&(trunc(toMove / ClusterSize))){
			toMove -= ClusterSize;
			curCluster = myDrive->getNextOf(curCluster);
		}
		return 1;
	}
}

BytesCnt KernelFile::filePos(){
	return curPosition;
}

char KernelFile::eof(){
	return (myEntry->size == curPosition ? 2 : 0);
}

BytesCnt KernelFile::getFileSize(){
	return myEntry->size;
}

char KernelFile::truncate(){
	if (eof()) return 0;

	if (curPosition == 0){
		myDrive->updateEntry(myEntry,EntrysCluster,0,0);
		myDrive->freeClusters(curCluster);
		curCluster = 0;
	}
	else {
		myDrive->updateEntry(myEntry,EntrysCluster,curPosition);
		myDrive->freeNextOf(curCluster);
	}
	return 1;
}

KernelFile::~KernelFile(){
	myDrive->closeFile(fname);

	delete myEntry;
	delete fname;
	myDrive = NULL;
}

void KernelFile::closeFile(){
//	myDrive->closeFile(fname);
}