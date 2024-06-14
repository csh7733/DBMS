#ifndef TRX_H
#define TRX_H
#include "dbapi.h"
#include "bpt.h"
#include "file.h"
#include "buffer.h"
#include "lock_table.h"

extern int trx_count;

int trx_begin(void);
int trx_commit(int trx_id);
int trx_abort(int trx_id);

#endif
