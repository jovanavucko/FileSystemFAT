#pragma once

#include "fs.h"
class KernelFile;

class File{
public:
	char write(BytesCnt, char* buffer);
	BytesCnt read(BytesCnt, char* buffer);
	char seek(BytesCnt);
	BytesCnt filePos();
	char eof();
	BytesCnt getFileSize();
	char truncate();
	~File();

private:
	friend class FS;
	friend class KernelFS;
	File();  // objekat koji moze da se kreira samo otvaranjem
	KernelFile *myImpl;
};