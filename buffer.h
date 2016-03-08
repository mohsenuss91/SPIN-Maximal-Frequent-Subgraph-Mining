

#ifndef _BUFFER
#define _BUFFER
#define WORDSIZE 8
class Buffer {
private:
  bool inited;
  int marks[500];
  char * bufs;	//Buffers
  int bufpos;	//Current buffer number
  int BUF_SIZE;
public:
  Buffer();		 
  Buffer(int);		 
  ~Buffer();
  char * realloc(char *old, int oldsize, int newsize);
  void   mark_buf(int);	
  void   clear_to_mark(int);
  void clearbuf ( void );
  char *curr_loc ( void );
  void freeBuffer ( void );
char * newbuf(int size);
};

#endif
