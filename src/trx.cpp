#include "dbapi.h"
#include "bpt.h"
#include "file.h"
#include "buffer.h"
#include "trx.h"
#include "lock_table.h"

// 트랜잭션 간 대기 그래프와 이전 값 저장을 위한 자료 구조
map<int, vector<int>> wait_for_graph;
map<tuple<int, int, int64_t>, pair<char*, int>> old_values; // <(trx_id, table_id, key), (old_value, type)> 

int trx_count = 1;

// 새로운 트랜잭션 시작
int trx_begin(void) {
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

// 트랜잭션 커밋
int trx_commit(int trx_id) {
    pthread_mutex_lock(&trx_manager_latch);
    lock_t* now;
    trx_table* trx_t = trx_manager[trx_id-1];
    now = trx_t->header->trx_next;
    while(now != trx_t->tailer) {
        lock_t* temp = now;
        now->trx_prev->trx_next = now->trx_next;
        now->trx_next->trx_prev = now->trx_prev;
        now = now->trx_next;
        lock_release(temp);
    }
    pthread_mutex_unlock(&trx_manager_latch);
    return trx_id;
}

// 트랜잭션 대기 상태 추가
void trx_wait(int prev_trx_id, int cur_trx_id) {
    pthread_mutex_lock(&trx_manager_latch);
    wait_for_graph[prev_trx_id].push_back(cur_trx_id);
    pthread_mutex_unlock(&trx_manager_latch);
}

// 트랜잭션 대기 상태 해제
void trx_wait_release(int waiting_trx_id, int awakened_trx_id) {
    pthread_mutex_lock(&trx_manager_latch);

    if (wait_for_graph.find(waiting_trx_id) != wait_for_graph.end()) {
        auto& vec = wait_for_graph[waiting_trx_id];
        vec.erase(remove(vec.begin(), vec.end(), awakened_trx_id), vec.end());
    }

    pthread_mutex_unlock(&trx_manager_latch);
}

// 사이클 검출을 위한 유틸리티 함수
bool has_cycle_util(int trx_id, set<int>& visited, set<int>& rec_stack) {
    if (visited.find(trx_id) == visited.end()) {
        visited.insert(trx_id);
        rec_stack.insert(trx_id);

        if (wait_for_graph.find(trx_id) != wait_for_graph.end()) {
            for (int neighbor : wait_for_graph[trx_id]) {
                if (visited.find(neighbor) == visited.end() && has_cycle_util(neighbor, visited, rec_stack)) {
                    return true;
                } else if (rec_stack.find(neighbor) != rec_stack.end()) {
                    return true;
                }
            }
        }
    }
    rec_stack.erase(trx_id);
    return false;
}

// 사이클 검출
bool has_cycle() {
    set<int> visited;
    set<int> rec_stack;

    for (const auto& entry : wait_for_graph) {
        if (has_cycle_util(entry.first, visited, rec_stack)) {
            return true;
        }
    }
    return false;
}

// 데드락 검출
void deadlock_check(int cur_trx_id) {
    if (has_cycle()) {
        printf("Deadlock detected! Aborting transaction %d\n", cur_trx_id);
        trx_abort(cur_trx_id);
    }
}

// 기존 값 저장
void save_old_value(int trx_id, int table_id, int64_t key, char* value, int type) {
    if (type == 0 || type == 1) { // type 0 (update) or type 1 (delete)
        old_values[{trx_id, table_id, key}] = make_pair(strdup(value), type);
    } else if (type == 2) { // type 2 (insert)
        old_values[{trx_id, table_id, key}] = make_pair(nullptr, type);
    }
}

// 롤백 수행
void rollback(int table_id, int64_t key, int trx_id, int type) {
    if (type == 0 || type == 1) {
        auto it = old_values.find({trx_id, table_id, key});
        if (it != old_values.end()) {
            if (type == 0) {
                // update
                update(key, table_id, trx_id, it->second.first);
            } else if (type == 1) {
                // insert(delete의 반대)
                insert(key, it->second.first, table_id);
            }
            free(it->second.first);
            old_values.erase(it);
        }
    } else if (type == 2) {
        // delete(insert의 반대)
        remove(key, table_id);
    }
}

// 트랜잭션 중단
int trx_abort(int trx_id) {
    pthread_mutex_lock(&trx_manager_latch);
    lock_t* now;
    trx_table* trx_t = trx_manager[trx_id - 1];
    now = trx_t->header->trx_next;
    
    while (now != trx_t->tailer) {
        lock_t* temp = now;
        now->trx_prev->trx_next = now->trx_next;
        now->trx_next->trx_prev = now->trx_prev;
        now = now->trx_next;
        
        if (temp->lock_mode == 1) { // xlock인 경우 rollback 호출
            auto it = old_values.find({trx_id, temp->table_id, temp->key});
            if (it != old_values.end()) {
                rollback(temp->table_id, temp->key, trx_id, it->second.second);
            }
        }
        
        lock_release(temp);
    }
    
    pthread_mutex_unlock(&trx_manager_latch);
    return trx_id;
}
