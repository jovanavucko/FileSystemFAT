#pragma once

#include "semapfores.h"
#include "fs.h"

class KernelFile;
class Drive;

class FileQueue{
	struct S{
		HANDLE line = CreateSemaphore(NULL, 0, 32, NULL);
		S *next;
	};
	struct node{
		node* next;
		char* name;
		HANDLE waiting;
		short open;
		S *first, *last;
	};

	node *tail, *head;
public:
	FileQueue();
	~FileQueue();

	int getFile(char* fname); // 0 ako ne postoji, 1 kada prodje semafor
	int addFile(char* fname);  // postavi mode na w
	short findFile(char*, short&);
	char deleteFile(char *fname);

	void signalInQueue(char*);

	friend class KernelFile;
};

