#include "file.h"
#include "buffer.h"

map<int64_t,int> m[11];
pthread_mutex_t buffer_manager_latch = PTHREAD_MUTEX_INITIALIZER;

int N = 0;
int headeridx = 0;
int tailer = 0;

Buffer* buf = NULL;

page_t* read_buffer(pagenum_t pagenum,int table_id,int insert_mode) {
	pthread_mutex_lock(&buffer_manager_latch);
	if (m[table_id].find(pagenum) != m[table_id].end()) { // success to find!
		int idx = m[table_id][pagenum];
		pthread_mutex_lock(&buf[idx].page_latch);

		int prev = buf[idx].prev_of_LRU;
		int next = buf[idx].next_of_LRU;
		buf[prev].next_of_LRU = next;
		buf[next].prev_of_LRU = prev;
		int header_next = buf[headeridx].next_of_LRU;
		buf[header_next].prev_of_LRU = idx;
		buf[idx].next_of_LRU = header_next;
		buf[idx].prev_of_LRU = headeridx;
		buf[headeridx].next_of_LRU = idx;
		pthread_mutex_unlock(&buffer_manager_latch);
		if(insert_mode) pthread_mutex_unlock(&buf[idx].page_latch);
		return buf[m[table_id][pagenum]].frame;
	}
	return file_to_buffer(pagenum,table_id,insert_mode);
}

void write_buffer(pagenum_t pagenum,page_t* src,int table_id) {
	buf[m[table_id][pagenum]].frame = src;
	buf[m[table_id][pagenum]].is_dirty = 1;
}


void buffer_to_file(pagenum_t pagenum,int table_id) {
	file_write_page(pagenum, buf[m[table_id][pagenum]].frame,table_id);
	delete buf[m[table_id][pagenum]].frame;
}

page_t* file_to_buffer(pagenum_t pagenum,int table_id,int insert_mode) {
	int idx = buf[tailer].prev_of_LRU;
	pthread_mutex_lock(&buf[idx].page_latch);

	if (buf[idx].is_dirty) buffer_to_file(buf[idx].page_num,buf[idx].table_id);
	int prev = buf[idx].prev_of_LRU;
	int next = buf[idx].next_of_LRU;
	buf[prev].next_of_LRU = next;
	buf[next].prev_of_LRU = prev;
	int header_next = buf[headeridx].next_of_LRU;
	buf[header_next].prev_of_LRU = idx;
	buf[idx].next_of_LRU = header_next;
	buf[idx].prev_of_LRU = headeridx;
	buf[headeridx].next_of_LRU = idx;


	page_t* page = new page_t;
	file_read_page(pagenum, page, table_id);
	buf[idx].frame = page;
	buf[idx].is_dirty = 0;
	buf[idx].is_pinned = 0;
	buf[idx].pin_count = 0;
	if(buf[idx].page_num != -1 && buf[idx].table_id != 0) m[buf[idx].table_id].erase(buf[idx].page_num);
	buf[idx].table_id = table_id;
	buf[idx].page_num = pagenum;
	m[table_id][pagenum] = idx;
	pthread_mutex_unlock(&buffer_manager_latch);
	if(insert_mode) pthread_mutex_unlock(&buf[idx].page_latch);
	return buf[idx].frame;
}

void set_pin(pagenum_t pagenum,int table_id) {
	//buf[m[table_id][pagenum]].is_pinned = 1;
	//buf[m[table_id][pagenum]].pin_count ++;
	return;
}

void unset_pin(pagenum_t pagenum,int table_id) {
	return;
}

void unlock(pagenum_t pagenum,int table_id) {
	int idx = m[table_id][pagenum];
	pthread_mutex_unlock(&buf[idx].page_latch);
}

void dolock(pagenum_t pagenum,int table_id) {
	int idx = m[table_id][pagenum];
	pthread_mutex_lock(&buf[idx].page_latch);
}

void buffer_to_file_in_table(int table_id){
	for(int i =0;i<N;i++){
		if(buf[i].table_id == table_id) {
			buffer_to_file(buf[i].page_num,table_id);
			m[table_id].erase(buf[i].page_num);
			buf[i].frame = NULL;
			buf[i].is_dirty = 0;
			buf[i].is_pinned = 0;
			buf[i].pin_count = 0;
			buf[i].table_id = 0;
			buf[i].page_num = -1;
		}
	}
}

void buffer_to_file_all(void){
	for(int i =0;i<N;i++){
		if(buf[i].page_num != -1 && buf[i].table_id != 0) buffer_to_file(buf[i].page_num,buf[i].table_id);
	}
}

void buffer_header(int table_id){
	int idx = m[table_id][0];
	header_page* page = (header_page*)buf[idx].frame;
	page->free_p_n = header_modify1(table_id);
	page->n_of_p = header_modify2(table_id);
	buf[idx].frame = (page_t*)page;
}

