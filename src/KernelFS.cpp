#include "KernelFS.h"
#include <cstdlib>
#include "Drive.h"
#include "file.h"


KernelFS::~KernelFS() {
	for (int i = 0; i < 26; i++){
		if (myDrives) unmount(i);
	}
	delete[]myDrives;
	delete[]accessAlowed;
}  

KernelFS::KernelFS() {
	myDrives = new Drive*[26];
	accessAlowed = new short[26];
	for (int i = 0; i < 26; i++){
		accessAlowed[i] = 1;
		myDrives[i] = NULL;
	}
	mutexAccess = CreateSemaphore(NULL,1,32,NULL);
} 

char KernelFS::mount(Partition* partition) {
	char letter;
	int firstFree = -1;

	for (int j = 0; j < 26; j++){
		if (checkAccess(j)){
			if (myDrives[j]){
				if (myDrives[j]->myPartition == partition)	{
					letter = 'A' + j;
					return letter;
				}
			}
			else {
				if ((firstFree < 0) && (myDrives[j] == NULL)) {
					firstFree = j;
				}
			}
		}
	}

	if (firstFree >= 0){
			char newLetter = 'A' + firstFree;
			myDrives[firstFree] = new Drive(partition, this);
			return newLetter;
		}
	return 0;
} 

char KernelFS::unmount(char part) {
	int numPart = (int)(part - 'A');
	
	if (blockAccess(numPart)){
		if (myDrives[numPart]) {
			myDrives[numPart]->unmount();
//			myDrives[numPart] = NULL;
			Drive *temp = myDrives[numPart];
			myDrives[numPart] = NULL;
			delete temp;
			allowAccess(numPart);
			return 1;
		}
	}
	return 0;
}

char KernelFS::format(char part) {
	int numPart = (int)(part - 'A');
	
	if (blockAccess(numPart)){
		if (myDrives[numPart]) {
			myDrives[numPart]->format();
			allowAccess(numPart);
			return 1;
		}
	}
	return 0;
}

char KernelFS::doesExist(char* fname) {
	if (!fname) return 0;
	char point = fname[0];
	int numPart = (int)(point - 'A');
	char *address = fname;
	if ((address[1] != ':') || (address[2] != '\\')) {
		address = NULL;
		return 0;
	}
	address = address + 3;  // address = ...\...\xyz.txt

	waitA(mutexAccess);
//	if (accessAlowed[numPart] != 0){
		Drive *temp = myDrives[numPart];
		if (temp){
			char result = temp->checkExists(address);
			signalA(mutexAccess);
			return result;
		}
//	}
	signalA(mutexAccess);
	return 0;
}

File* KernelFS::open(char* fname, char mode) {
	if (!fname) return 0;
	char point = fname[0];
	int numPart = (int)(point - 'A');
	if ((fname[1] != ':') || (fname[2] != '\\')) {
		return 0;
	}
	fname += 3;  // fname = ...\...\xyz.txt
	
	if (checkAccess(numPart)){
		File *temp = new File();
		temp->myImpl = myDrives[numPart]->open(fname, mode);
		if (temp->myImpl == NULL){
//			delete temp;
			return NULL;
		}
		else return temp;
	};
	return NULL;
}

char KernelFS::deleteFile(char* fname) {
	if (!fname) return 0;
	char point = fname[0];
	int numPart = (int)(point - 'A');
	if ((fname[1] != ':') || (fname[2] != '\\')) {
		return 0;
	}
	fname += 3;  // fname = ...\...\xyz.txt

	// semafor?
	return myDrives[numPart]->deleteFile(fname);
}

char KernelFS::createDir(char* dirname){
	if (!dirname) return 0;
	char point = dirname[0];
	int numPart = (int)(point - 'A');
	if ((dirname[1] != ':') || (dirname[2] != '\\')) {
		return 0;
	}
	dirname += 3;  // fname = ...\...\xyz.txt

	if (checkAccess(numPart)){
		if (myDrives[numPart]) {
			return myDrives[numPart]->addDirEntry(dirname);
		};
	}
	return 0;
}

char KernelFS::deleteDir(char* dirname){
	if (!dirname) return 0;
	char point = dirname[0];
	int numPart = (int)(point - 'A');
	if ((dirname[1] != ':') || (dirname[2] != '\\')) {
		return 0;
	}
	dirname += 3;  // fname = ...\...\xyz.txt

	if (checkAccess(numPart)){
		if (myDrives[numPart]) {
			return myDrives[numPart]->deleteDir(dirname);
		};
	}
	return 0;
}

char KernelFS::readDir(char* dirname, EntryNum n, Entry &e){
	int numPart = (int)(dirname[0] - 'A');
	if (myDrives[numPart]) return myDrives[numPart]->readDir(strchr(dirname, '\\'), n, e);
	return 0;
}

int KernelFS::checkAccess(int slot){
	waitA(mutexAccess);
	int pom = accessAlowed[slot];
	signalA(mutexAccess);
	return pom;
}

int KernelFS::blockAccess(int slot){
	waitA(mutexAccess);
	int pom = accessAlowed[slot];
	accessAlowed[slot] = 0;
	signalA(mutexAccess);
	return pom;
}

int KernelFS::allowAccess(int slot){
	waitA(mutexAccess);
	int pom = accessAlowed[slot];
	accessAlowed[slot] = 1;
	signalA(mutexAccess);
	return (pom == 0 ? 1 : 0);
}
