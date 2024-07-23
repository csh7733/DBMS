#include "bpt.h"
#include "file.h"
#include "buffer.h"
#include <lock_table.h>
#include "trx.h"

pthread_mutex_t lock_manager_latch = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t trx_manager_latch = PTHREAD_MUTEX_INITIALIZER;
map<pair<int,int64_t>,hash_table*> m2;
vector<trx_table*> trx_manager;
int cnt2 = 0;

int
init_lock_table()
{
	return 0;
}

lock_t*
lock_acquire(pagenum_t page,int table_id, int64_t key,int trx_id, int lock_mode)
{
	unlock(page,table_id);
	pthread_mutex_lock(&lock_manager_latch);
	pair<int,int64_t> p = {table_id,key};
	lock_t* lock = new lock_t;
	if (m2.find(p) == m2.end()){
		hash_table* hash = new hash_table;
		lock_t* header = new lock_t;
		lock_t* tailer = new lock_t;
		header->next = tailer;
		header->hash_table_entry = hash;
		header->is_sleep = 0;
		header->lock_mode = -1;
		header->owner_trx_id = -1;
		header->prev = NULL;
		tailer->prev = header;
		tailer->hash_table_entry = hash;
		tailer->is_tailer = 1;
		tailer->lock_mode = -1;
		tailer->is_sleep = 0;
		tailer->owner_trx_id = -1;
		tailer->next = NULL;
		hash->header = header;
		hash->tailer = tailer;
		m2[p] = hash;
	}
	lock->cond = PTHREAD_COND_INITIALIZER;
	lock->hash_table_entry = m2[p];
	lock->is_tailer = 0;
	lock->is_sleep = 0;
	lock->table_id = table_id;
	lock->key = key;
	lock->lock_mode = lock_mode;
	lock->owner_trx_id = trx_id;

	lock->next = lock->hash_table_entry->tailer;
	lock->prev = lock->hash_table_entry->tailer->prev;
	lock->hash_table_entry->tailer->prev->next = lock;
	lock->hash_table_entry->tailer->prev = lock;

	deadlock_check(trx_id);

	if(lock->lock_mode == 0){
		if(lock->prev->is_sleep || lock->prev->lock_mode == 1){
			lock->is_sleep = 1;
			trx_wait(lock->prev->owner_trx_id, trx_id);
			pthread_cond_wait(&lock->cond,&lock_manager_latch);
		}
	}
	else if(lock->lock_mode == 1){
		if(lock->prev != lock->hash_table_entry->header){
			lock->is_sleep = 1;
			trx_wait(lock->prev->owner_trx_id, trx_id);
			pthread_cond_wait(&lock->cond,&lock_manager_latch);	
		}
	}
	trx_table* trx_t = trx_manager[trx_id-1];
	lock->trx_prev = trx_t->tailer->trx_prev;
	lock->trx_next = trx_t->tailer;
	lock->trx_prev->trx_next = lock;
	trx_t->tailer->trx_prev = lock;
	pthread_mutex_unlock(&lock_manager_latch);
	return lock;
}
	
int
lock_release(lock_t* lock_obj)
{
	if(lock_obj->is_sleep) printf("error!\n");
	pthread_mutex_lock(&lock_manager_latch);
	lock_t* lock_next = lock_obj->next;

	if(lock_obj->lock_mode == 0){
		if(lock_obj->prev == lock_obj->hash_table_entry->header && lock_obj->next->is_sleep){
			if(lock_next->lock_mode == 1){
				trx_wait_release(lock_next->owner_trx_id, lock_obj->owner_trx_id);
				pthread_cond_signal(&lock_next->cond);
				lock_next->is_sleep = 0;
			}
		}
	}
	else if(lock_obj->lock_mode == 1){
		if(lock_next->lock_mode == 1){
			trx_wait_release(lock_next->owner_trx_id, lock_obj->owner_trx_id);
			pthread_cond_signal(&lock_next->cond);
			lock_next->is_sleep = 0;
		}
		else{
			while(lock_next->lock_mode == 0){
				trx_wait_release(lock_next->owner_trx_id, lock_next->prev->owner_trx_id);
				pthread_cond_signal(&lock_next->cond);
				lock_next->is_sleep = 0;
				lock_next = lock_next->next;
			}
		}
	}

	lock_obj->next->prev = lock_obj->prev;
	lock_obj->prev->next = lock_obj->next;
	delete lock_obj;
	pthread_mutex_unlock(&lock_manager_latch);
	return 0;
}

