#include "dbapi.h"
#include "bpt.h"
#include "file.h"
#include "buffer.h"
#include "log.h"

int init_db (int buf_num,int flag,int log_num,char* log_path,char* logmsg_path){
	for(int i=1;i<=10;i++){
		fd[i] = -1;	
	}
	table_count = 0; // init fd
	N = buf_num;
	buf = new Buffer[N+2];
	headeridx = N;
	tailer = N+1; // 0~N-1 is components, N == header, N+1 == tailer
	for(int i=0;i<N+2;i++){
		buf[i].frame = NULL;
		buf[i].is_dirty = 0;
		buf[i].is_pinned = 0;
		buf[i].pin_count = 0;
		buf[i].table_id = 0;
		buf[i].page_latch = PTHREAD_MUTEX_INITIALIZER;
		buf[i].page_num = -1;
	}
	buf[headeridx].next_of_LRU = 0;
	buf[0].prev_of_LRU = headeridx;
	buf[0].next_of_LRU = 1;
	buf[tailer].prev_of_LRU = N-1;
	buf[N-1].next_of_LRU = tailer;
	buf[N-1].prev_of_LRU = N-2;
	for(int i=1;i<N-1;i++){
		buf[i].next_of_LRU = i+1;
		buf[i].prev_of_LRU = i-1;
	}
	return 0;
}

int open_table(char* pathname){
	int i = 1,flag = 0;
	string path = pathname;
	if(path_table.find(path) != path_table.end()) flag = 1;
	fdt = open(pathname, O_RDWR | O_CREAT, 0644);
	if(fdt == -1) return -1; // fail to open
	if(flag){
		int unique_id = path_table[path];
		fd[unique_id] = fdt;
		// if header is not exist
		if(get_header_number_of_pages(unique_id) == 0) {
		make_header_page(unique_id);
		}
		return unique_id;
	}
	else{
		fd[++table_count] = fdt;
		path_table[path] = table_count;
		// if header is not exist
		if(get_header_number_of_pages(table_count) == 0) make_header_page(table_count);
		return table_count;
		
	}
}


int db_insert(int table_id,int64_t key,char* value){		
	return insert(key,value,table_id);
}

int db_find(int table_id,int64_t key,char* ret_val,int trx_id){
	strcpy(ret_val,find(key,table_id,trx_id));
	if(ret_val == NULL) return -1;
	else{
		return 0;
	} 
}

int db_delete(int table_id,int64_t key){
	return remove(key,table_id);
}

int db_update(int table_id,int64_t key,char* values,int trx_id){
	char result[120];
	strcpy(result,update(key,table_id,trx_id,values));
	return 0;
}

int close_table(int table_id){
	int fd2 = fd[table_id];
	buffer_to_file_in_table(table_id);
	close(fd2);
	return 0;
}

int shutdown_db(void){
	int fd2;
	buffer_to_file_all();
	for(int i=1;i<=table_count;i++){
		fd2 = fd[i];
		close(fd2);
	}
	return 0;
}
