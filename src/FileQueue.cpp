#include "FileQueue.h"
#include "KernelFile.h"
#include <cstdlib>
#include <cstring>


FileQueue::FileQueue(){
	head = tail = NULL;
}


FileQueue::~FileQueue(){
	while (head){
		tail = head;
		head = head->next;
		delete tail;
	}
	head = tail = NULL;
}

int FileQueue::getFile(char* fname){
	node *temp = head;
	while (temp){
		if (strcmp(temp->name, fname) == 0){

			waitA(temp->waiting);
			if (temp->last == NULL){
				temp->first = new S();
				temp->last = temp->first;
				signalA(temp->waiting);
			}
			else {
				temp->last->next = new S();
				temp->last = temp->last->next;
				signalA(temp->waiting);
				waitA(temp->last->line);
			}
			temp->open++;
			return 1;
		}
		temp = temp->next;
	}
	return 0;
}

int FileQueue::addFile(char* fname){  // postavi mode na w
	if (fname == NULL) return 0;
	if (head == NULL){
		head = new node();

		head->first = new S(); // nit koja pravi FileQueue dobija i prolaz
		head->last = head->first;  

		head->name = new char[strlen(fname) + 1];
		strcpy(head->name, fname);
		head->next = NULL;
		head->open = 1;
		head->waiting = CreateSemaphore(NULL, 1, 32, NULL);
		tail = head;
	}
	else {
		tail->next = new node();
		tail = tail->next;

		tail->first = new S(); // nit koja pravi FileQueue dobija i prolaz
		tail->last = tail->first;

		tail->name = new char[strlen(fname) + 1];
		strcpy(tail->name, fname);
		tail->next = NULL;
		tail->open = 1;
		tail->waiting = CreateSemaphore(NULL, 1, 32, NULL);
	}
	return 1;
}

short FileQueue::findFile(char *fname, short& state){
	node *temp = head;
	while (temp){
		if (strcmp(temp->name, fname) == 0) {
			state = temp->open;
			return 1;
		}
		temp = temp->next;
	}
	return 0;
}

char FileQueue::deleteFile(char *fname){
	node *temp = head, *prev = NULL;
	while (temp){
		if (strcmp(temp->name, fname) == 0) {
			if (temp->open) return 0;
			if (prev){
				prev->next = temp->next;
				if (tail == temp) tail = prev;
				delete[]temp->name;
				delete temp;
			}
			else {
				head = temp->next;
				if (tail == temp) tail = NULL;  // tail = head!!!!
				delete[]temp->name;
				delete temp;
			}
			return 1;
		}
		prev = temp;
		temp = temp->next;
	}
	return 0;
}

void FileQueue::signalInQueue(char *fname){
	node *temp = head;
	while (temp){
		if (strcmp(temp->name, fname) == 0) {
			temp->open--;

			waitA(temp->waiting);

			if (temp->first == temp->last){
				delete temp->first;
				temp->first = temp->last = NULL;
				signalA(temp->waiting);
			}
			else {
				S *pom = temp->first;
				temp->first = temp->first->next;
				delete pom;
//				signalA(temp->waiting);

				signalA(temp->first->line);
				signalA(temp->waiting);

			}
//			signalA(temp->waiting);

			return;
		}
		temp = temp->next;
	}
}
