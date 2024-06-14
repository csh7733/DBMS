#ifndef LOG_H
#define LOG_H
#define VALUE_SIZE 120
#include "dbapi.h"
#include "bpt.h"
#include "file.h"
#include "buffer.h"
#include "lock_table.h"
#include "trx.h"

extern char* log_buffer;
extern int point;
extern int prev_point;
extern pthread_mutex_t log_manager_latch;

typedef struct log_compensate{
	int32_t log_size;
   	int64_t lsn;
	int64_t prev_lsn;
	int32_t trx_id;
	int32_t type;
	int32_t table_id;
	int32_t page_number;
	int32_t offset;
	int32_t data_len;
	char old_image[VALUE_SIZE];
	char new_image[VALUE_SIZE];
	int64_t next_undo_lsn;
}log_compensate;

typedef struct log_update{
	int32_t log_size;
   	int64_t lsn;
	int64_t prev_lsn;
	int32_t trx_id;
	int32_t type;
	int32_t table_id;
	int32_t page_number;
	int32_t offset;
	int32_t data_len;
	char old_image[VALUE_SIZE];
	char new_image[VALUE_SIZE];
}log_update;

typedef struct log_bcr{
	int32_t log_size;
   	int64_t lsn;
	int64_t prev_lsn;
	int32_t trx_id;
	int32_t type;
}log_bcr;

void add_bcr(int32_t trx_id,int32_t type);
void add_update(int32_t trx_id,int32_t type,int32_t table_id,pagenum_t page_number,int record_idx,char* old_image,char* new_image);
void add_compensate(int32_t trx_id,int32_t type,int32_t table_id,pagenum_t page_number,int record_idx,char* old_image,char* new_image);

#endif
