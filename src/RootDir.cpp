#include "RootDir.h"
#include "Drive.h"
#include <cmath>

RootDir::RootDir(Drive *d){
	myDrive = d;
	maxEntries = trunc(ClusterSize / sizeof(Entry));
	tail = 0;
	curEntry = new Entry();
}


RootDir::~RootDir(){
	delete curEntry;
}


void RootDir::initialize(ClusterNo ttag){
	tag = ttag;
	tail = 0;
	curEntry->name[0] = ' ';
	myDrive->writePartCl(ttag, (char*)(curEntry), 0, sizeof(Entry));
}

void RootDir::flushRoot(){
}

void RootDir::loadRoot(ClusterNo ttag, ClusterNo ttail){
	tag = ttag;
	tail = ttail;
}


char RootDir::readEntry(EntryNum n, Entry &e){
	if (n < 0 || n > tail) return 0;
	if (n == tail) return 2;
	myDrive->readPartCl(tag, (char*)(curEntry), n*sizeof(Entry), sizeof(Entry));
	e = *curEntry;
	return 1;
}


Entry* RootDir::findEntryOnDisk(char *ffname, ClusterNo &startCluster){
	// pozicionira se na root i iz njega redom trazi dir po dir
	Entry *temp = findPath(ffname, startCluster);

	if (temp == NULL && startCluster != 0){  // znaci da je u root-u, nema pomeranja fname-a
		tag = startCluster;
		tail = myDrive->rootEntrySize();
	}
	else {
		if ((temp == NULL) && (startCluster == 0)) {
			return NULL;
		}
		else {  // ima pomeranja fname-a
			ffname = strrchr(ffname, '\\');
			ffname++;
			tag = temp->firstCluster;
			tail = temp->size;
		}
	}

	if (temp) delete temp;


	// pozicioniran je na cluster gde moze da se nadje Entry
	int j = 0, ok = 1;

	if (strrchr(ffname, '.') != NULL){  // TRAZIMO FAJL

		while (j < tail){

			char *fname = ffname;
			if ((j != 0) && ((j%maxEntries) == 0)) {
				tag = myDrive->getNextOf(tag);
			}
			myDrive->readPartCl(tag, (char*)curEntry, (j%maxEntries)*sizeof(Entry), sizeof(Entry));
			ok = 1;


			if (curEntry->atributes != 1) {
				j++;
				continue;
			}

			for (int i = 0; i < 8; i++){
				if (fname[0] != '.') {
					if (curEntry->name[i] != fname[0]) {
						ok = 0;
						break;
					}
					fname++;
				}
				else{
					if (curEntry->name[i] != ' ') {
						ok = 0;
						break;
					}
				}
			}

			if (ok == 0) { j++;  continue; }

			fname++; // da prodje onu tacku

			for (int i = 0; i < 3; i++){
				if (fname){
					if (curEntry->ext[i] != fname[0]) {
						ok = 0;
						break;
					}
					fname++;
				}
				else {
					if (curEntry->ext[i] != ' ') {
						ok = 0;
						break;
					}
				}
			}

			if (ok == 0) { j++; continue; }
			// ako je stigao ovde -> poklapaju se ime i ext
			Entry* temp = new Entry();
			memcpy(temp, (char*)curEntry, sizeof(Entry));
			startCluster = tag;
			return temp;
		}
		startCluster = tag;
		return NULL;
	}
	else {  // TRAZIMO DIREKTORIJUM
		while (j < tail){

			char *fname = ffname;
			if ((j != 0) && ((j%maxEntries) == 0)) {
				tag = myDrive->getNextOf(tag);
			}
			myDrive->readPartCl(tag, (char*)curEntry, (j%maxEntries)*sizeof(Entry), sizeof(Entry));
			ok = 1;

			if (curEntry->atributes != 2) {
				j++;				
				continue;
			}

			for (int i = 0; i < 8; i++){
				if (fname) {
					if (curEntry->name[i] != fname[0]) {
						ok = 0;
						break;
					}
					fname++;
				}
				else{
					if (curEntry->name[i] != ' ') {
						ok = 0;
						break;
					}
				}
			}

			if (ok == 0) { j++;  continue; }

			// ako je stigao ovde -> poklapaju se ime i ext
			Entry* temp = new Entry();
			memcpy(temp, (char*)curEntry, sizeof(Entry));
			startCluster = tag;
			return temp;
		}
		startCluster = tag;
		return NULL;
	}
}


Entry* RootDir::addEntry(char *fname, ClusterNo &startCluster){
	// pozicionira se na root i iz njega redom trazi dir po dir
	int isRoot = 0;
	Entry *temp = findPath(fname, startCluster);
	if (temp == NULL && startCluster != 0){  // znaci da je u root-u, nema pomeranja fname-a
		tag = startCluster;
		tail = myDrive->rootEntrySize();
		isRoot = 1;
	}
	else {
		if ((temp == NULL) && (startCluster == 0)) {
			return NULL;
		}
		else {  // ima pomeranja fname-a
			fname = strrchr(fname, '\\');
			fname++;
			tag = temp->firstCluster;
			tail = temp->size;
		}
	}

	// ovde je tag, tail iz dir-a u koji upisujemo, i isRoot ako je koreni -> extra update u fat-u
	if (isRoot){
		if ((tail) && (tail%maxEntries == 0)){
			myDrive->attachClusters(1, tag);
		}
		myDrive->incDirEntrys();
	}
	else {
		if (tag == 0){
			unsigned long pom = myDrive->getFreeClusters(1);
			if (pom == 0) return NULL; // nema vise slobodnih klastera
			updateEntry(temp, startCluster, 1, pom);
			startCluster = pom;
			tag = pom;
		}
		else {
			if ((temp->size != 0) && (temp->size%maxEntries == 0)){  // treba dodati novi cluster
				startCluster = temp->firstCluster;
				tag = myDrive->attachClusters(1, tag);
				if (tag == 0) return NULL; // nema vise slobodnih klastera
				updateEntry(temp, startCluster, temp->size + 1);
			}

		}
	}
	tag = myDrive->positionOnLast(tag);
	if (temp) delete temp;

	// ovde imamo tag u koji upisujemo, tail na koji upisujemo

	temp = new Entry();
	temp->firstCluster = 0;
	temp->size = 0;

		temp->atributes = 1; // file
		for (int i = 0; i < 8; i++){
			if (fname[0] != '.') {
				temp->name[i] = fname[0];
				fname++;
			}
			else{
				temp->name[i] = ' ';
			}
		}
		fname++; // da prodje onu tacku
		for (int i = 0; i < 3; i++){
			if (fname){
				temp->ext[i] = fname[0];
				fname++;
			}
			else {
				temp->ext[i] = ' ';
			}
		}
		myDrive->writePartCl(tag, (char*)temp, (tail%maxEntries)*sizeof(Entry), sizeof(Entry));
		tail = tail + 1;
		if (tail%maxEntries != 0){
			memcpy((char*)curEntry, (char*)temp, sizeof(Entry));
			curEntry->name[0] = ' ';
			myDrive->writePartCl(tag, (char*)curEntry, tail*sizeof(Entry), sizeof(Entry));
		}
		return temp;
}

char RootDir::addDirEntry(char* dirname){
	int isRoot = 0;
	unsigned long startCluster = 0;
	Entry *temp = findPath(dirname, startCluster);
	if (temp == NULL && startCluster != 0){  // znaci da je u root-u, nema pomeranja fname-a
		tag = startCluster;
		tail = myDrive->rootEntrySize();
		isRoot = 1;
	}
	else {
		if ((temp == NULL) && (startCluster == 0)) {
			return NULL;
		}
		else {  // ima pomeranja fname-a
			dirname = strrchr(dirname, '\\');
			dirname++;
			tag = temp->firstCluster;
			tail = temp->size;
		}
	}

	// ovde je tag, tail iz dir-a u koji upisujemo, i isRoot ako je koreni -> extra update u fat-u
	if (isRoot){
		if ((tail) && (tail%maxEntries == 0)){
			myDrive->attachClusters(1, tag);
		}
		myDrive->incDirEntrys();
	}
	else {
		if (tag == 0){
			unsigned long pom = myDrive->getFreeClusters(1);
			if (pom == 0) return 0; // nema vise slobodnih klastera
			updateEntry(temp, startCluster, 1, pom);
			startCluster = pom;
			tag = pom;
		}
		else {
			if ((temp->size != 0) && (temp->size%maxEntries == 0)){  // treba dodati novi cluster
				startCluster = temp->firstCluster;
				tag = myDrive->attachClusters(1, tag);
				if (tag == 0) return 0; // nema vise slobodnih klastera
				updateEntry(temp, startCluster, temp->size + 1);
			}

		}
	}
	tag = myDrive->positionOnLast(tag);
	if (temp) delete temp;

	// ovde imamo tag u koji upisujemo, tail na koji upisujemo

	temp = new Entry();
	temp->firstCluster = 0;
	temp->size = 0;

		temp->atributes = 2; // dir
		for (int i = 0; i < 8; i++){
			if (dirname[0] != '\0') {
				temp->name[i] = dirname[0];
				dirname++;
			}
			else{
				temp->name[i] = ' ';
			}
		}
		myDrive->writePartCl(tag, (char*)temp, (tail%maxEntries)*sizeof(Entry), sizeof(Entry));
		tail = tail + 1;
		if (tail%maxEntries != 0){
			memcpy((char*)curEntry, (char*)temp, sizeof(Entry));
			curEntry->name[0] = ' ';
			myDrive->writePartCl(tag, (char*)curEntry, tail*sizeof(Entry), sizeof(Entry));
		}
		return 1;
}


char RootDir::updateEntry(Entry *entry, ClusterNo start, unsigned long newSize, unsigned long newFCluster){
	tag = start;
	int i = 0;
	myDrive->readPartCl(tag, (char*)curEntry, i*sizeof(Entry), sizeof(Entry));

	while (curEntry->name[0] != ' '){
		if (memcmp((char*)entry, (char*)curEntry, sizeof(Entry)) == 0){
			entry->size = newSize;
			entry->firstCluster = newFCluster;
			myDrive->writePartCl(tag, (char*)entry, i*sizeof(Entry), sizeof(Entry));
			return 1;
		}
		else {
			i++;
			if (i%maxEntries == 0) tag = myDrive->getNextOf(tag);
			if (tag == 0) {
				return 0;
			}
			else myDrive->readPartCl(tag, (char*)curEntry, i*sizeof(Entry), sizeof(Entry));
		}
	}
	return 0;
}

char RootDir::updateEntry(Entry *entry, ClusterNo start, unsigned long newSize){
	tag = start;
	int i = 0;
	myDrive->readPartCl(tag, (char*)curEntry, i*sizeof(Entry), sizeof(Entry));

	while (curEntry->name[0] != ' '){
		if (memcmp((char*)entry, (char*)curEntry, sizeof(Entry)) == 0){
			entry->size = newSize;
			myDrive->writePartCl(tag, (char*)entry, i*sizeof(Entry), sizeof(Entry));
			return 1;
		}
		else {
			i++;
			if (i%maxEntries == 0) tag = myDrive->getNextOf(tag);
			if (tag == 0) {
				return 0;
			}
			else myDrive->readPartCl(tag, (char*)curEntry, i*sizeof(Entry), sizeof(Entry));
		}
	}
	return 0;
}


char RootDir::deleteEntry(char *ename){
	// u temp se nalazi Entry ciji se size/first cluster modifikuje
	ClusterNo myTag = 0, tempTag = 0, myTail = 0;
	Entry *temp = findPath(ename,tempTag);


	if (temp == NULL && tempTag != 0){  // znaci da je u root-u, nema pomeranja fname-a
		myTag = myDrive->rootClusterStartsAt();
		myTail = myDrive->rootEntrySize();
	}
	else {
		if ((temp == NULL) && (tempTag == 0)) {
			return NULL;
		}
		else {  // ima pomeranja fname-a
			ename = strrchr(ename, '\\');
			ename++;
			myTag = temp->firstCluster;
			myTail = temp->size;
		}
	}

	Entry *curEntry = new Entry();

	int j = 0, ok = 1;

	while (j < myTail){
		char *fname = ename;
		myDrive->readPartCl(myTag, (char*)curEntry, j*sizeof(Entry), sizeof(Entry));
		ok = 1;
		if (curEntry->atributes != 1) {
			j++;
			continue;
		}
		for (int i = 0; i < 8; i++){
			if (fname[0] != '.') {
				if (curEntry->name[i] != fname[0]) {
					ok = 0;
					break;
				}
				fname++;
			}
			else{
				if (curEntry->name[i] != ' ') {
					ok = 0;
					break;
				}
			}
		}

		if (ok == 0) { j++;  continue; }

		fname++; // da prodje onu tacku
		for (int i = 0; i < 3; i++){
			if (fname[0] != '\0'){
				if (curEntry->ext[i] != fname[0]) {
					ok = 0;
					break;
				}
				fname++;
			}
			else {
				if (curEntry->ext[i] != ' ') {
					ok = 0;
					break;
				}
			}
		}
		if (ok == 0) { j++; continue; }
		// ako smo ovde-> nasli smo fajl entrie
		myTail--;

		if (myTail == 0){
			if (temp){  // ako brisemo poslednji ulaz iz ne-root cluster-a
				myDrive->freeClusters(temp->firstCluster);
				myDrive->updateEntry(temp, tempTag, 0,0);
			}
			else {  // brisemo poslednji ulaz iz root-a
				curEntry->name[0] = ' ';
				myDrive->writePartCl(myTag, (char*)curEntry, 0, sizeof(Entry));
				myDrive->decDirEntries();
			}
		}
		else {  // ima vise stvari u dir-u, ima pomeranja
			ClusterNo last = myDrive->positionOnLast(tempTag);
			myDrive->readPartCl(last, (char*)curEntry, (myTail%maxEntries)*sizeof(Entry), sizeof(Entry));
			myDrive->writePartCl(myTag, (char*)curEntry, (j%maxEntries)*sizeof(Entry), sizeof(Entry));
			curEntry->name[0] = ' ';
			myDrive->writePartCl(last, (char*)curEntry, (myTail%maxEntries)*sizeof(Entry), sizeof(Entry));

			if (temp) {
				myDrive->updateEntry(temp, tempTag, myTail);
			}
			else {
				myDrive->decDirEntries();
			}
		j = myTail;
		continue;
		}
	}
	if (curEntry) delete curEntry;
	if (temp) delete temp;
	return ok;
}

char RootDir::deleteDir(char* dirname){
	// u temp se nalazi Entry ciji se size/first cluster modifikuje
	ClusterNo myTag = 0, tempTag = 0, myTail = 0;
	Entry *temp = findPath(dirname, tempTag);


	if (temp == NULL && tempTag != 0){  // znaci da je u root-u, nema pomeranja fname-a
		myTag = myDrive->rootClusterStartsAt();
		myTail = myDrive->rootEntrySize();
	}
	else {
		if ((temp == NULL) && (tempTag == 0)) {
			return NULL;
		}
		else {  // ima pomeranja fname-a
			dirname = strrchr(dirname, '\\');
			dirname++;
			myTag = temp->firstCluster;
			myTail = temp->size;
		}
	}

	Entry *curEntry = new Entry();

	int j = 0, ok = 1;

	while (j < myTail){
		char *fname = dirname;
		myDrive->readPartCl(myTag, (char*)curEntry, j*sizeof(Entry), sizeof(Entry));
		ok = 1;
		if (curEntry->atributes != 2) {
			j++;
			continue;
		}

		if (curEntry->size) {
			j++;
			continue;
		}
		for (int i = 0; i < 8; i++){
			if (fname[0] != '\0') {
				if (curEntry->name[i] != fname[0]) {
					ok = 0;
					break;
				}
				fname++;
			}
			else{
				if (curEntry->name[i] != ' ') {
					ok = 0;
					break;
				}
			}
		}

		if (ok == 0) { j++; continue; }
		// ako smo ovde-> nasli smo fajl entrie
		myTail--;

		if (myTail == 0){
			if (temp){  // ako brisemo poslednji ulaz iz ne-root cluster-a
				myDrive->freeClusters(temp->firstCluster);
				myDrive->updateEntry(temp, tempTag, 0, 0);
			}
			else {  // brisemo poslednji ulaz iz root-a
				curEntry->name[0] = ' ';
				myDrive->writePartCl(myTag, (char*)curEntry, 0, sizeof(Entry));
				myDrive->decDirEntries();
			}
		}
		else {  // ima vise stvari u dir-u, ima pomeranja
			ClusterNo last = myDrive->positionOnLast(tempTag);
			myDrive->readPartCl(last, (char*)curEntry, (myTail%maxEntries)*sizeof(Entry), sizeof(Entry));
			myDrive->writePartCl(myTag, (char*)curEntry, (j%maxEntries)*sizeof(Entry), sizeof(Entry));
			curEntry->name[0] = ' ';
			myDrive->writePartCl(last, (char*)curEntry, (myTail%maxEntries)*sizeof(Entry), sizeof(Entry));

			if (temp) {
				myDrive->updateEntry(temp, tempTag, myTail);
			}
			else {
				myDrive->decDirEntries();
			}
			j = myTail;
			continue;
		}
	}
	if (curEntry) delete curEntry;
	if (temp) delete temp;
	return ok;
}



Entry* RootDir::findPath(char* Fpath, ClusterNo &EntrysCluster){
	char *path;
	path = strchr(Fpath, '\\');
//	path = strstr(Fpath, "\\");
	if (path == 0) {   
		EntrysCluster = myDrive->rootClusterStartsAt();
		return NULL;
	}
	// ovde dolazimo ako ima sigurno bar jedan folder, krece se od root-a
	tag = myDrive->rootClusterStartsAt();
	tail = myDrive->rootEntrySize();
	int i = 0;

	while (i < tail){
		int ok = 1;
		path = Fpath;
		myDrive->readPartCl(tag, (char*)curEntry, i*sizeof(Entry), sizeof(Entry));
		if (curEntry->atributes != 2){ 
			i++;
			continue;
		}
		for (int j = 0; j < 8; j++){
			if (path[0] != '\\'){
				if (path[0] == curEntry->name[j]){
					path++;
				}
				else ok = 0;
			}
			else {
				if ((curEntry->name[j] != ' ') && (curEntry->name[j] != '\0')) ok = 0;
			}
		}
		// kraj provere imena
		if (!ok) {
			i++;
			continue;
		}
		// ako dolazi ovde -> nasli smo Entry, priprema za naredni nivo

		path++;
//		path++;
		Fpath = path;
		path = strchr(Fpath, '\\');
		if (path == NULL){  // nema vise foldera, tj dosli smo do oblika fajlX.dat ili FOLDER
			EntrysCluster = tag;
			Entry *a = new Entry();
			memcpy((char*)a, (char*)curEntry, sizeof(Entry));
			return a;
		}
		else {  // ulazimo u sledeci nivo
			tag = curEntry->firstCluster;
			tail = curEntry->size;
			i = 0;
		}
	}
	EntrysCluster = 0;
	return NULL;
}

