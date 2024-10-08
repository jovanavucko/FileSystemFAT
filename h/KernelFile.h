#pragma once

#include "fs.h"
#include "part.h"
#include "FileQueue.h"

class Drive;


class KernelFile{
private:
	Drive *myDrive;
	Entry *myEntry;
	unsigned long EntrysCluster;
	char* fname;
	BytesCnt curPosition;
	ClusterNo curCluster;
	char mode;
public:
	char write(BytesCnt, char* buffer);
	BytesCnt read(BytesCnt, char* buffer);
	char seek(BytesCnt);
	BytesCnt filePos();
	char eof();
	BytesCnt getFileSize();
	char truncate();
	~KernelFile();

	void closeFile();

private:
	friend class FileQueue;

	friend class FS;
	friend class KernelFS;
	friend class File;
	friend class Drive;
protected:
//	KernelFile();
	KernelFile(Drive*,unsigned long eCluster, Entry*, char*);
};