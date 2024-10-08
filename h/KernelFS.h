#pragma once

#include "fs.h"
#include "semapfores.h"

class Partition;
class File;
class Drive;

class KernelFS{
private:
	Drive** myDrives;
	short* accessAlowed;
	HANDLE mutexAccess;
	
public:
	~KernelFS();

	char mount(Partition* partition);

	char unmount(char part);	

	char format(char part);

	char doesExist(char* fname);


	File* open(char* fname, char mode);	

	char deleteFile(char* fname);	

	char createDir(char* dirname);		//argument je naziv direktorijuma zadat apsolutnom putanjom

	char deleteDir(char* dirname);		//argument je naziv direktorijuma zadat apsolutnom putanjom

	char readDir(char* dirname, EntryNum n, Entry &e);

	int checkAccess(int);
	int blockAccess(int);
	int allowAccess(int);

	friend class FS;
protected:
	KernelFS();
};