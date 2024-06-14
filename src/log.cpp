#include "dbapi.h"
#include "bpt.h"
#include "file.h"
#include "buffer.h"
#include "lock_table.h"
#include "trx.h"
#include "log.h"

pthread_mutex_t log_manager_latch = PTHREAD_MUTEX_INITIALIZER;

int MB = 1024 * 1024;
int point = 0;
int prev_point = 0;
char* log_buffer = new char(30 * MB);

void add_bcr(int32_t trx_id,int32_t type){
	pthread_mutex_lock(&log_manager_latch);
	log_bcr bcr;
	bcr.log_size = 28;
	bcr.lsn = point;
	bcr.prev_lsn = 0;
	if(bcr.lsn != prev_point) {
		bcr.prev_lsn = prev_point;
		prev_point += bcr.log_size;
	}
	bcr.trx_id = trx_id;
	bcr.type = type;
	memcpy(log_buffer+point,&bcr,bcr.log_size);
	point += bcr.log_size;
	pthread_mutex_unlock(&log_manager_latch);
}

void add_update(int32_t trx_id,int32_t type,int32_t table_id,pagenum_t page_number,int record_idx,char* old_image,char* new_image){
	pthread_mutex_lock(&log_manager_latch);
	log_update bcr;
	bcr.log_size = 288;
	bcr.lsn = point;
	bcr.prev_lsn = 0;
	if(bcr.lsn != prev_point) {
		bcr.prev_lsn = prev_point;
		prev_point += bcr.log_size;
	}
	bcr.trx_id = trx_id;
	bcr.type = type;
	bcr.table_id = table_id;
	bcr.page_number = page_number;
	bcr.offset = 128+128*record_idx+8;
	bcr.data_len = 120;
	memcpy(bcr.old_image,old_image,120);
	memcpy(bcr.new_image,new_image,120);
	memcpy(log_buffer+point,&bcr,bcr.log_size);
	point += bcr.log_size;
	pthread_mutex_unlock(&log_manager_latch);
}

void add_compensate(int32_t trx_id,int32_t type,int32_t table_id,pagenum_t page_number,int record_idx,char* old_image,char* new_image){
	pthread_mutex_lock(&log_manager_latch);
	log_compensate bcr;
	bcr.log_size = 296;
	bcr.lsn = point;
	bcr.prev_lsn = 0;
	if(bcr.lsn != prev_point) {
		bcr.prev_lsn = prev_point;
		prev_point += bcr.log_size;
	}
	bcr.trx_id = trx_id;
	bcr.type = type;
	bcr.table_id = table_id;
	bcr.page_number = page_number;
	bcr.offset = 128+128*record_idx+8;
	bcr.data_len = 120;
	memcpy(bcr.old_image,old_image,120);
	memcpy(bcr.new_image,new_image,120);
	int temp = trx_manager[trx_id]->lsn;
	bcr.next_undo_lsn = 0;
	if(temp != 0) bcr.next_undo_lsn = temp;
	trx_manager[trx_id]->lsn = bcr.lsn;
	memcpy(log_buffer+point,&bcr,bcr.log_size);
	point += bcr.log_size;
	pthread_mutex_unlock(&log_manager_latch);
}


