#ifndef BUFFER_H
#define BUFFER_H
#include "file.h"

extern map<int64_t,int> m[11];
extern pthread_mutex_t buffer_manager_latch;

extern int N; //N == header , N+1 == tailer
extern int headeridx;
extern int tailer;

typedef struct Buffer {
	page_t* frame;
	int32_t table_id;
	pagenum_t page_num;
	int32_t is_dirty;
	int32_t is_pinned;
	pthread_mutex_t page_latch;
	int32_t next_of_LRU;
	int32_t prev_of_LRU;
	int32_t pin_count;
}Buffer;

extern Buffer* buf;

page_t* read_buffer(pagenum_t pagenum,int table_id,int insert_mode);
void write_buffer(pagenum_t pagenum,page_t* src,int table_id);
void buffer_to_file(pagenum_t pagenum,int table_id);
page_t* file_to_buffer(pagenum_t pagenum,int table_id,int insert_mode);
void set_pin(pagenum_t pagenum,int table_id);
void unset_pin(pagenum_t pagenum,int table_id);
void unlock(pagenum_t pagenum,int table_id);
void dolock(pagenum_t pagenum,int table_id);
void buffer_to_file_in_table(int table_id);
//void update_header(int table_id);
void buffer_header(int table_id);
void buffer_to_file_all(void);

#endif
