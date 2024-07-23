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

### Bpt Manager
Bpt Manager는 B+ 트리 구조를 관리하는 역할을 합니다

데이터 저장: 데이터를 페이지 단위로 B+ 트리에 저장합니다.

삽입, 삭제 및 검색 작업: 데이터를 삽입, 삭제, 검색할 때는 루트 페이지부터 내부 페이지(internal page)를 따라가며, 실제 레코드가 있는 리프 페이지(leaf page)에 접근합니다.
### File Manager
File Manager는 Data File과 Bpt Manager 사이의 연결 고리 역할을 합니다.

페이지 읽기/쓰기: Bpt Manager의 요청에 따라 디스크에서 페이지를 읽고 씁니다.

프리 페이지 관리: 데이터 파일에 프리 페이지를 할당하거나, 할당된 프리 페이지 중 하나를 Bpt Manager에게 제공합니다.

## 2. 버퍼 매니저 계층 추가
-디스크 직접 읽기/쓰기의 시간 소요를 줄이기 위해 메모리 버퍼를 사용하여 페이지를 저장하고 관리합니다. 각 테이블은 고유의 아이디를 가지며, 다수의 테이블을 관리할 수 있습니다.
### Buffer Manager
Buffer Manager는 Bpt Manager와 File Manager 사이의 중간 단계로서 다음과 같은 역할을 수행합니다

페이지 요청 및 반환: Bpt Manager가 페이지를 요청하면 반환하고, 동시에 필요한 페이지를 File Manager에게 요청합니다.

페이지 관리: Look-Aside, Write-Back 전략을 통해 페이지를 관리합니다.

LRU Policy: LRU 정책에 따라 페이지를 관리하며, Evict가 발생할 경우 디스크에 Flush하여 데이터의 일관성을 유지합니다.

## 3. 동시성 제어를 위한 Lock 테이블
### Lock Table
Lock Table은 동시성 제어를 위해 사용되며, 여러 스레드가 동시에 데이터를 사용하려 할 때 충돌을 방지합니다. 주요 역할은 다음과 같습니다:

해시 테이블 구조: Lock Table은 해시 테이블을 사용하여 관리됩니다. 해시 테이블의 각 항목은 <table_id, key>로 구분됩니다. 여기서 table_id는 테이블의 식별자이고, key는 레코드의 식별자입니다.

락 리스트 관리: 각 해시 테이블 항목은 락 리스트를 가지며, 이 리스트는 특정 데이터 항목에 대해 걸려 있는 모든 락들을 관리합니다.

락 타입: 락에는 공유 락(Shared Lock, S Lock)과 배타 락(Exclusive Lock, X Lock) 두 가지 종류가 있습니다.

- 공유 락(S Lock): 여러 트랜잭션이 동시에 읽기를 수행할 수 있도록 허용합니다. 다른 트랜잭션이 S Lock을 가지고 있는 동안에도 S Lock을 획득할 수 있습니다.

- 배타 락(X Lock): 한 트랜잭션이 데이터를 갱신할 때 사용됩니다. X Lock이 걸려 있는 동안에는 다른 트랜잭션이 S Lock이나 X Lock을 획득할 수 없습니다.

락 획득 및 대기: 트랜잭션이 S Lock을 획득하려는 경우, 이미 다른 트랜잭션이 X Lock을 가지고 있지 않으면 S Lock을 획득할 수 있습니다.
트랜잭션이 X Lock을 획득하려는 경우, 다른 트랜잭션이 S Lock이나 X Lock을 가지고 있으면 대기해야 합니다.

락 해제 및 깨움: 트랜잭션이 데이터를 사용한 후 락을 해제하면, 대기 중인 다른 트랜잭션이 락을 획득할 수 있도록 깨웁니다.

## 4. 트랜잭션 관리자
-트랜잭션의 시작(begin), 커밋(commit), Abort를 지원합니다. 트랜잭션 시작 시 객체를 생성하고, 락을 휙득할때마다 해당 트랜잭션 객체의 lock_list에 lock을 추가합니다. 만약 commit시 트랜잭션 내의 잡고있던 모든 락을 해제합니다. 만약 데드락이 감지되면 해당 트랜잭션을 즉시 중단하고, trx의 lock리스트를 따라서 하나씩 release를 해주는데 이때 write를 한 경우 저장된 변경전값을 바탕으로 rollback을 합니다.
## 5. 복구 알고리즘 - 구현 미완성
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

Crash 이후의 복구는 Transaction Manager를 통해 Files and Index Management, Buffer Management, Disk Space Management 계층에 걸쳐 구현됩니다. Crash-Recovery의 주요 목적은 Atomicity와 Durability를 보장하여 DBMS의 ACID 특성을 유지하는 것입니다.

1. **Analysis Phase:**
   - 로그 파일을 로드하여 각 transaction의 commit이나 abort 여부를 확인하고, 정상 종료된 transaction은 Winner, 비정상 종료된 transaction은 Loser로 분류합니다.

2. **REDO Phase:**
   - 모든 Winner와 Loser의 Write 및 Compensate Log에 대해 REDO를 수행합니다. 로그에 있는 페이지의 pageLSN이 현재 로그의 LSN보다 크다면 CONSIDER-REDO를 합니다.

3. **UNDO Phase:**
   - Loser들의 모든 Write 및 Compensate Log에 대해 UNDO를 수행하고, 이후 Compensate Log를 생성합니다. Compensate Log에는 NextUndoSeqNo 변수가 있어 복구 후 다시 crash가 발생해도 UNDO할 횟수를 줄일 수 있습니다.

이러한 단계들을 통해 DBMS는 crash 이전 상태로 돌아갈 수 있으며, Atomicity와 Durability를 보장하게 됩니다. 여러 transaction들이 동시에 수행되면서도 DBMS가 crash가 나더라도 정상적으로 복구할 수 있습니다.

## In-depth Analysis(1). Workload with many concurrent non-conflicting read-only transactions
![9](https://github.com/csh7733/DBMS/assets/149491102/b9a02d4c-7ec3-447c-bd31-5145e6ce1661)
![10](https://github.com/csh7733/DBMS/assets/149491102/be26f025-ada4-4cbf-8abf-547052825895)

## In-depth Analysis(2). Workload with many concurrent non-conflicting write-only transactions.
![11](https://github.com/csh7733/DBMS/assets/149491102/50f00943-7f74-4111-bbac-14978ab4759c)
