#ifndef __LOCK_TABLE_H__
#define __LOCK_TABLE_H__

#include "bpt.h"
#include "file.h"
#include "buffer.h"
#include "trx.h"

typedef struct lock_t1 lock_t;
typedef struct hash_table1 hash_table;
typedef struct trx_table1 trx_table;


struct lock_t1 {
	lock_t* prev;
	lock_t* next;
	hash_table* hash_table_entry;
	pthread_cond_t cond;
	int lock_mode;
	lock_t* trx_next;
	lock_t* trx_prev;
	int owner_trx_id;
	int is_tailer;
	int table_id;
	int64_t key;
	int is_sleep;
};

struct hash_table1 {
	lock_t* header;
	lock_t* tailer;
};



struct trx_table1 {
	lock_t* header;
	lock_t* tailer;
	int lsn = 0;
};

using namespace std;

extern pthread_mutex_t lock_manager_latch;
extern pthread_mutex_t trx_manager_latch;
extern map<pair<int,int64_t>,hash_table*> m2;
extern vector<trx_table*> trx_manager;
extern int cnt2;

/* APIs for lock table */
int init_lock_table();
lock_t* lock_acquire(pagenum_t page,int table_id, int64_t key,int trx_id, int lock_mode);
int lock_release(lock_t* lock_obj);

#endif /* __LOCK_TABLE_H__ */
