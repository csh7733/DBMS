#ifndef TRX_H
#define TRX_H

#include "dbapi.h"
#include "bpt.h"
#include "file.h"
#include "buffer.h"
#include "lock_table.h"

extern int trx_count;
extern map<int, vector<int>> wait_for_graph;
extern map<tuple<int, int, int64_t>, pair<char*, int>> old_values;

int trx_begin(void);
int trx_commit(int trx_id);
int trx_abort(int trx_id);
void trx_wait(int prev_trx_id, int cur_trx_id);
void trx_wait_release(int waiting_trx_id, int awakened_trx_id);
bool has_cycle_util(int trx_id, set<int>& visited, set<int>& rec_stack);
bool has_cycle();
void deadlock_check(int cur_trx_id);
void save_old_value(int trx_id, int table_id, int64_t key, char* value, int type);
void rollback(int table_id, int64_t key, int trx_id, int type);

#endif // TRX_H
