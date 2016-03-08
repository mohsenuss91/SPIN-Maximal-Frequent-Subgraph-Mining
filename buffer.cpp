#include <stdio.h>
#include <stdlib.h>
#include "buffer.h"
#include <string.h>


extern bool donewithgraphs;
//Constructor
Buffer::Buffer(int buf_size)
{
  bufpos=0;
  bufs=0;
  for(int i=0;i<100;i++)
    {
      marks[i]=0;
    }
  BUF_SIZE = buf_size*1024*1024; //passed as megabytes
  bufs = (char *) malloc (sizeof(char) * BUF_SIZE);	//Allocate buffers

}

Buffer::Buffer()
{
	//printf("### resetting the buffer\n");
  	bufs=0;
  	bufpos=0;
}


//Destructor
Buffer::~Buffer()
{
  //if(bufs>0)
  //  {
  //    printf("Deallocating buffer at %d\n",bufs);
  //    free(bufs);
  //  }
  //Free pointer for all buffers

}   

void Buffer::freeBuffer ( void ) {
	if ( bufs != NULL ) {
		free(bufs);
		bufs = NULL;
	}
}

//Returns pointe to a allocated memory location
char * Buffer::newbuf(int size)
{

//	size = size + (4 - size%4);
	size = size + (WORDSIZE - size%WORDSIZE);

//printf("marks is %d, bufs is %d, pos is %d, size is %d\n",marks,bufs,bufpos,BUF_SIZE);
  char * addr = bufs+bufpos;
  bufpos+=size;
  //printf("mark is %d, pos is %d, size is %d\n",marks, bufpos,BUF_SIZE);
  if((int)bufpos>BUF_SIZE)
    {
      printf("*** CODE RED:  WE EXCEEDED OUR BUFFER !!! ***\n");
      exit(0);
    }
  bufs[bufpos]=0;
  //printf("asked for %d bytes, now at pos %d\n",size,bufpos);
    //if((int)bufs==bufpos)
  	//printf("########### CODE RED3!!!\n");
  return addr;
}


void Buffer::mark_buf(int level)
{
if(level>50000)
	printf("########## buffer recursion level exceeded (%d>50000)\n",level);
  marks[level]=bufpos;

  
}


void Buffer::clear_to_mark(int level)
{
  bufpos = marks[level];
}


char * Buffer::realloc( char *old, int oldsize, int newsize ) {

	char *newptr = newbuf ( newsize );
	memcpy ( newptr, old, oldsize );
	return newptr;

}

void Buffer::clearbuf ( void ) {
	if ( bufs != NULL )
		free ( bufs );
	bufs = NULL;
	bufpos = 0;
}

char *Buffer::curr_loc ( void ) {
	return bufs+bufpos;
}
