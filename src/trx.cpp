#include "dbapi.h"
#include "bpt.h"
#include "file.h"
#include "buffer.h"
#include "trx.h"
#include "lock_table.h"

int trx_count = 1;

int trx_begin(void){
	pthread_mutex_lock(&trx_manager_latch);
	lock_t* header = new lock_t;
	lock_t* tailer = new lock_t;
	trx_table* trx_t = new trx_table;
	header->trx_next = tailer;
	tailer->trx_prev = header;
	trx_t->header = header;
	trx_t->tailer = tailer;
	trx_manager.push_back(trx_t);
	pthread_mutex_unlock(&trx_manager_latch);
	return trx_count++;
}

int trx_commit(int trx_id){
	pthread_mutex_lock(&trx_manager_latch);
	lock_t* now;
	trx_table* trx_t = trx_manager[trx_id-1];
	now = trx_t->header->trx_next;
	while(now != trx_t->tailer){
		lock_t* temp = now;
		now->trx_prev->trx_next = now->trx_next;
		now->trx_next->trx_prev = now->trx_prev;
		now = now->trx_next;
		lock_release(temp);
	}
	pthread_mutex_unlock(&trx_manager_latch);
	return trx_id;
}

int trx_abort(int trx_id){
	return 0;
}
