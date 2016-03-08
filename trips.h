#include <stdio.h>
#include <iostream>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include "buffer.h"

using namespace std;

typedef int PON;  // -32768 to 32767
typedef int LABEL;
typedef unsigned int COUNT;

#define microsec 1000000.0
#define BUFSIZE 350 // megs
COUNT maxnodes;
int fcounts[100] = {0};

Buffer membuf;

class TimeTracker {
private:
	struct timeval start_time;
	struct timeval stop_time;
	bool  running;
	
public:
	TimeTracker() {
		running=false;
	}

	void Start() {
		gettimeofday(&start_time, (struct timezone *)0);
		running=true;
	}

	double Stop() {
		double st, en;
		if (!running) return(-1.0);
		else {
			gettimeofday(&stop_time, (struct timezone *)0);
			st = start_time.tv_sec + (start_time.tv_usec/microsec);
			en = stop_time.tv_sec + (stop_time.tv_usec/microsec);
			running=false;
			return (double)(en - st);
		}
	}
};

struct Match {
	PON matchpon;
	PON ptr;
};

struct TreeNode {
	PON nps;
	LABEL labseq;
};

class Tree {
	
  public:
	COUNT nodes;
	struct TreeNode *nodelist;
	struct Match *patmatch;
	unsigned int patmatchlen;

	static COUNT dbsize;

	~Tree() {
		nodelist = NULL;
		patmatch = NULL;
		patmatchlen = 0;
	}
};

class Pattern {
  public:
	COUNT nodes;
	PON *nps;
	LABEL *labseq;
	struct Match *patmatch;
	unsigned int patmatchlen;
   	PON *pon;  // Post order number of nodes

	/* Pattern is stored in reverse order of PON and hence the new node is added at the end whose index is (nodes-1). */

	Pattern () {
		nodes = 0;
		patmatchlen = 0;
		nps = NULL;
		labseq = NULL;
		patmatch = NULL;
		pon = NULL;
	}
	
	~Pattern () {
		pon = NULL;
		nps = NULL;
		labseq = NULL;
		pon = NULL;
	}
	void extend ( Pattern &oldpat, LABEL lab_to_add, int posindex );
	void print ( void ) ;
	void print ( FILE *patout ) ;
};

void Pattern::extend ( Pattern &oldpat, LABEL lab_to_add, int posindex ) {
	nodes = oldpat.nodes + 1;

	nps = (PON *) membuf.newbuf( sizeof(PON) * (nodes+1));
	labseq = (LABEL *) membuf.newbuf( sizeof(LABEL) * (nodes+1));
	pon = (PON *) membuf.newbuf( sizeof(PON) * (nodes+1));

	/* 0th entry is NOT used */
	
	if ( posindex == -1 ) {
		labseq[1] = lab_to_add;
		nps[1] = -1;
		pon[1] = 1;
	}
	else {
		int i;
		for ( i=1; i <= oldpat.nodes; i++ ) {
			labseq[i] = oldpat.labseq[i];
			nps[i] = oldpat.nps[i];     // it is actually index into itself. pon[nps[i]] gives the actual NPS
			pon[i] = oldpat.pon[i] + 1;
		}
		labseq[i] = lab_to_add;
		nps[i] = posindex;				// actual NPS is pon[posindex]
		pon[i] = 1;
	}
}

void Pattern::print ( FILE *patout ) {
	fprintf(patout, "%d\n", nodes);
	for ( int i=nodes; i > 1; i-- ) 
		fprintf(patout, "%d  ", labseq[ nps[i] ] );
	fprintf(patout, "0 \n");
	for ( int i=nodes; i > 1; i-- ) 
		fprintf(patout, "%d  ", pon[ nps[i] ] );
	fprintf(patout, "0 \n");
	for ( int i=nodes; i > 0; i-- ) 
		fprintf(patout, "%d  ", labseq[i] );
	fprintf(patout, "\n");
}

void Pattern::print ( void ) {
	printf("LPS: ");
	for ( int i=nodes; i > 1; i-- ) 
		printf("%d  ", labseq[ nps[i] ] );
	printf("0 \n");
	printf("NPS: ");
	for ( int i=nodes; i > 1; i-- ) 
		printf("%d  ", pon[ nps[i] ] );
	printf("0 \n");
	printf("LS: ");
	for ( int i=nodes; i > 0; i-- ) 
		printf("%d  ", labseq[i] );
	printf("\n-----\n");
}

class Embedding {
  public:
  	int tid, start_loc, match_loc;
};

class SupportStructure {
  public:
  	LABEL lab;
	PON position;
	COUNT sup;
	char flag;

	~SupportStructure () {
	}
};

LABEL labcount, *labset; // if labcount = 4, then labels are 0, 1, 2, 3, 4.
Tree *treeset;
COUNT Tree::dbsize = 0;
SupportStructure *supds;
int supsize;
