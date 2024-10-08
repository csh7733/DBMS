## 프로젝트 소개

이 프로젝트는 메모리 기반의 B+트리를 먼저 디스크 기반의 B+트리로 전환한 후, 디스크 기반의 B+트리에 버퍼 매니저 계층을 추가하고, 멀티스레드 환경에서 동시성 제어와 트랜잭션 관리, 복구 알고리즘을 구현하는 것입니다.

# 요약

## 1. Memory 기반 B+트리에서 On-Disk B+ 트리로의 전환

### Page
디스크 기반 B+트리를 위해 필요한 페이지

-Header Page: 첫 번째 free page 위치, 루트 페이지 위치, 총 페이지 수

-Free Page: 다음 free page 위치

-Leaf Page: 페이지 헤더 (부모 페이지, is leaf, key 개수, 오른쪽 형제 페이지), 레코드 (key+value)

-Internal Page: 페이지 헤더 (부모 페이지, is leaf, key 개수, 첫 자식 페이지 번호), key와 페이지 번호

### Bpt Manager(bpt.cpp)
Bpt Manager는 B+ 트리 구조를 관리하는 역할을 합니다

데이터 저장: 데이터를 페이지 단위로 B+ 트리에 저장합니다.

삽입, 삭제 및 검색 작업: 데이터를 삽입, 삭제, 검색할 때는 루트 페이지부터 내부 페이지(internal page)를 따라가며, 실제 레코드가 있는 리프 페이지(leaf page)에 접근합니다.
### File Manager(file.cpp)
File Manager는 Data File과 Bpt Manager 사이의 연결 고리 역할을 합니다.

페이지 읽기/쓰기: Bpt Manager의 요청에 따라 디스크에서 페이지를 읽고 씁니다.

프리 페이지 관리: 데이터 파일에 프리 페이지를 할당하거나, 할당된 프리 페이지 중 하나를 Bpt Manager에게 제공합니다.

## 2. 버퍼 매니저 계층 추가
-디스크 직접 읽기/쓰기의 시간 소요를 줄이기 위해 메모리 버퍼를 사용하여 페이지를 저장하고 관리합니다. 각 테이블은 고유의 아이디를 가지며, 다수의 테이블을 관리할 수 있습니다.
### Buffer Manager(buffer.cpp)
Buffer Manager는 Bpt Manager와 File Manager 사이의 중간 단계로서 다음과 같은 역할을 수행합니다

페이지 요청 및 반환: Bpt Manager가 페이지를 요청하면 반환하고, 동시에 필요한 페이지를 File Manager에게 요청합니다.

페이지 관리: Look-Aside, Write-Back 전략을 통해 페이지를 관리합니다.

LRU Policy: LRU 정책에 따라 페이지를 관리하며, Evict가 발생할 경우 디스크에 Flush하여 데이터의 일관성을 유지합니다.

![image](https://github.com/user-attachments/assets/9285123b-5b14-40cf-ab55-ee197cca9310)

![image](https://github.com/user-attachments/assets/491ba4c5-ada0-4bee-b060-2108795bbfbd)


## 3. 동시성 제어를 위한 Lock 테이블
### Lock Table(lock_table.cpp)
Lock Table은 동시성 제어를 위해 사용되며, 여러 스레드가 동시에 데이터를 사용하려 할 때 충돌을 방지합니다. 주요 역할은 다음과 같습니다:

해시 테이블 구조: Lock Table은 해시 테이블을 사용하여 관리됩니다. 해시 테이블의 각 항목은 <table_id, key>로 구분됩니다. 여기서 table_id는 테이블의 식별자이고, key는 레코드의 식별자입니다.

락 리스트 관리: 각 해시 테이블 항목은 락 리스트를 가지며, 이 리스트는 특정 데이터 항목에 대해 걸려 있는 모든 락들을 관리합니다.

락 타입: 락에는 공유 락(Shared Lock, S Lock)과 배타 락(Exclusive Lock, X Lock) 두 가지 종류가 있습니다.

- 공유 락(S Lock): 여러 트랜잭션이 동시에 읽기를 수행할 수 있도록 허용합니다. 다른 트랜잭션이 S Lock을 가지고 있는 동안에도 S Lock을 획득할 수 있습니다.

- 배타 락(X Lock): 한 트랜잭션이 데이터를 갱신할 때 사용됩니다. X Lock이 걸려 있는 동안에는 다른 트랜잭션이 S Lock이나 X Lock을 획득할 수 없습니다.

락 획득 및 대기: 트랜잭션이 S Lock을 획득하려는 경우, 앞에있는 다른 트랜잭션이 X Lock을 가지고 있지 않으면 S Lock을 획득할 수 있습니다.
트랜잭션이 X Lock을 획득하려는 경우, 다른 트랜잭션이 S Lock이나 X Lock을 가지고 있으면 대기해야 합니다.

락 해제 및 깨움: 트랜잭션이 데이터를 사용한 후 락을 해제하면, 대기 중인 다른 트랜잭션이 락을 획득할 수 있도록 깨웁니다.

![image](https://github.com/user-attachments/assets/8e211b56-be60-4e81-b172-644f2125e723)


## 4. 트랜잭션 관리자(trx.cpp)
트랜잭션 관리자는 Begin, Commit, Abort를 지원합니다.

### 트랜잭션 시작 (Begin)
- 트랜잭션이 시작되면, 새로운 트랜잭션 객체가 생성됩니다.
- 트랜잭션 객체는 해당 트랜잭션에서 획득한 락을 추적하기 위해 `lock_list`를 가지고 있습니다.
- 락을 획득할 때마다 해당 락이 `lock_list`에 추가됩니다.

### 트랜잭션 커밋 (Commit)
- 트랜잭션이 커밋될 때, 트랜잭션 객체의 `lock_list`를 따라 모든 락을 해제합니다.
- 트랜잭션이 성공적으로 완료되었음을 의미합니다.

### 트랜잭션 중단 (Abort)
- 트랜잭션 중단은 데드락이 감지되거나 다른 이유로 인해 트랜잭션을 더 이상 진행할 수 없을 때 발생합니다.
- 중단 시, 트랜잭션 객체의 `lock_list`를 따라 하나씩 락을 해제합니다.
- 이때 트랜잭션이 중단되었을 경우, 트랜잭션 동안 수행된 모든 Write작업을 저장된 변경 전 값(old_values)을 바탕으로 롤백합니다.

### 데드락 감지 (Deadlock Detection)
- 시스템은 주기적으로 `deadlock_check()` 함수를 통해 데드락을 감지합니다.
- `wait-for-graph`를 탐색하여 사이클이 발견되면 데드락이 발생한 것으로 간주하고, 해당 트랜잭션을 중단(Abort)시킵니다.

## 5. 복구 알고리즘(log.cpp) - 구현 미완성
-Analysis-Redo-Undo 3단계 복구 알고리즘을 구현하여 DB의 Atomicity와 Durability를 보장합니다. 로그를 통해 변경 사항을 기록하고, WAL (Write Ahead Logging) 원칙을 준수합니다. 로그 타입에는 commit, update, rollback, begin, compensate가 있으며, 복구 시 로그 파일을 읽어와 DB를 원래 상태로 복구합니다.

# 구현 Detail

## Concurrency Control Implementation

동시성 제어는 DBMS에서 여러 transaction을 효과적으로 관리하여 Consistency와 Isolation을 보장합니다. 이는 transaction 간의 충돌로 발생하는 Lost Update, Inconsistent Reads, Dirty Reads와 같은 문제를 해결합니다. 목표는 성능과 데이터 무결성을 유지하면서 conflict serializable한 스케줄로 transaction을 실행하는 것입니다.

1. **Two-Phase Locking (2PL):**
   - **Shared Lock**: 레코드를 읽기 전에 필요하며, 여러 transaction이 동시에 Shared Lock을 가질 수 있습니다.
   - **Exclusive Lock**: 레코드를 쓰기 전에 필요하며, 한 transaction만 Exclusive Lock을 가질 수 있습니다.
   - **Strict 2PL**: transaction이 commit되거나 abort될 때까지 잠금을 유지하여 Cascading Abort를 방지하고 동시성 제어를 강화합니다.

2. **Deadlock 감지 및 해결:**
   - 대기 그래프를 사용하여 사이클을 탐지하고, 이는 Deadlock을 나타냅니다. Deadlock이 발견되면, 해당 transaction을 abort하여 문제를 해결합니다.

#### 구현 단계:
1. **Files and Index Management layer**에서 읽거나 쓸 레코드를 찾기 위해 **Buffer Management layer**에서 해당 페이지를 찾습니다.
2. **Buffer Manager latch**에 대한 lock을 획득하고, 해당 페이지에 대한 lock을 획득한 후 Buffer Manager latch를 해제합니다.
3. 이제 **Lock Manager latch**를 획득한 후, Lock Manager의 해시 테이블을 통해 해당 `<table_id, key>`가 있는 곳으로 이동하여 해당 레코드 lock을 linked list에 매답니다. 이때 대기 그래프와 DFS 탐색을 통해 사이클이 있는지를 확인하고, 사이클이 있다면 해당 transaction을 abort하고, 그렇지 않으면 정상적으로 진행합니다.
4. 상황에 따라 레코드 lock을 바로 획득하거나 잠시 대기 후 깨어나서 레코드 lock을 획득합니다. 이때 Lock Manager latch는 해제됩니다.
5. 해당 레코드에 관련된 read, write를 수행하고, write인 경우 수정된 작업을 Buffer에 갱신합니다.
6. transaction 내 모든 read, write에 대해 1~5 단계를 수행한 후 commit이나 abort를 통해 transaction이 종료되면 해당 transaction의 모든 lock을 해제합니다. 이때 잠들어 있는 lock들을 깨웁니다.
7. 이후 Buffer에 기록된 페이지가 축출되거나 프로그램이 종료되면서 Buffer Management -> Disk space Management -> Database에 정상적으로 갱신됩니다.

동시성 제어를 통해 여러 transaction들이 동시에 수행될 수 있습니다.

## Crash-Recovery Implementation (구현 미완성)
Crash-Recovery는 트랜잭션 매니저를 통해 Files and Index Management, Buffer Management, Disk Space Management 계층에 걸쳐 구현됩니다. 시스템 충돌 이후에도 Atomicity와 Durability를 보장하여 DBMS의 ACID 특성을 유지하는 것이 목표입니다.

복구 과정은 세 가지 주요 단계로 이루어집니다: Analysis, REDO, UNDO.

-------------------
1. Analysis Phase :
Analysis Phase에서는 Checkpoint 이후에 시작된 트랜잭션들의 상태를 분석합니다.
각 트랜잭션의 commit이나 abort 여부를 확인하여, 정상 종료된 트랜잭션은 Winner, 비정상 종료된 트랜잭션은 Loser로 분류합니다.
-------------------
2. REDO Phase:
REDO Phase에서는 Winner 트랜잭션의 변경 사항을 디스크에 다시 반영합니다.
이 과정에서 pageLSN이 현재 로그의 LSN보다 작은 경우에만 REDO 작업(Consider-Redo)를 수행합니다. 즉, 변경 사항이 이미 디스크에 반영된 경우는 REDO를 생략하고, 반영되지 않은 경우에만 REDO를 진행합니다.
정상적으로 롤백된 트랜잭션이나 UNDO가 완료된 트랜잭션에 대해서도 REDO 작업을 수행합니다. 이는 REDO가 UNDO의 효과를 반복하는 것과 동일하므로, 중복되는 UNDO 작업을 방지할 수 있습니다.
-------------------
3. UNDO Phase:
UNDO Phase는 Loser 트랜잭션(중단된 트랜잭션)의 변경 사항을 되돌리는 단계입니다.
Consider-Undo 방식을 사용하여 pageLSN을 기반으로 UNDO 작업을 수행합니다. **CLR(Compensation Log Record)**을 생성하여, 복구 중 Crash가 발생하더라도 다시 UNDO를 이어서 진행할 수 있도록 다음 UndoSeqNo를 기록합니다.
CLR은 다음에 수행할 UNDO 작업을 명시하고, 복구 도중 충돌이 발생해도 이미 수행된 UNDO 작업을 중복으로 처리하지 않도록 보장합니다.
-------------------

이러한 단계들을 통해 DBMS는 crash 이전 상태로 돌아갈 수 있으며, Atomicity와 Durability를 보장하게 됩니다. 여러 transaction들이 동시에 수행되면서도 DBMS가 crash가 나더라도 정상적으로 복구할 수 있습니다.

## In-depth Analysis(1). Workload with many concurrent non-conflicting read-only transactions
![9](https://github.com/csh7733/DBMS/assets/149491102/b9a02d4c-7ec3-447c-bd31-5145e6ce1661)
![10](https://github.com/csh7733/DBMS/assets/149491102/be26f025-ada4-4cbf-8abf-547052825895)
