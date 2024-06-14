## 프로젝트 소개

이 프로젝트는 메모리 기반의 B+트리를 먼저 디스크 기반의 B+트리로 전환한 후, 디스크 기반의 B+트리에 버퍼 매니저 계층을 추가하고, 멀티스레드 환경에서 동시성 제어와 트랜잭션 관리, 복구 알고리즘을 구현하는 것입니다.

## 1. Memory 기반 B+트리에서 On-Disk B+ 트리로의 전환

### Page
디스크 기반 B+트리를 위해 필요한 페이지

-Header Page: 첫 번째 free page 위치, 루트 페이지 위치, 총 페이지 수

-Free Page: 다음 free page 위치

-Leaf Page: 페이지 헤더 (부모 페이지, is leaf, key 개수, 오른쪽 형제 페이지), 레코드 (key+value)

-Internal Page: 페이지 헤더 (부모 페이지, is leaf, key 개수, 첫 자식 페이지 번호), key와 페이지 번호

### Insert
-자식 노드 포인터를 페이지 번호로 수정

-새 페이지 필요 시 free page 사용

-페이지 번호를 매개변수로 전달

-split 시 페이지 정보 갱신


### Delete
-성능 저하 방지를 위해 delayed merge 적용

-페이지가 비면 free page 정보 갱신

-coalescence 발생 시 페이지 정보 갱신

## 2. 버퍼 매니저 계층 추가
-디스크 직접 읽기/쓰기의 시간 소요를 줄이기 위해 메모리 버퍼를 사용하여 페이지를 저장하고 관리합니다. 각 테이블은 고유의 아이디를 가지며, 다수의 테이블을 관리할 수 있습니다.

## 3. 동시성 제어를 위한 Lock 테이블
-해시 테이블에 테이블 아이디와 레코드 아이디로 구분된 노드를 삽입하고, 각 노드는 락 리스트를 가집니다. 여러 스레드가 동시에 데이터를 사용하려 할 때 충돌을 방지하기 위해 mutex를 사용하여 락을 관리합니다. 사용자가 데이터를 사용 중이면 다른 스레드는 대기하고, 사용이 끝나면 대기 중인 스레드를 깨웁니다.

## 4. 트랜잭션 관리자
-트랜잭션의 시작(begin), 커밋(commit), 데이터 업데이트 및 조회(find) 기능을 지원합니다. 트랜잭션 시작 시 객체를 생성하고, 종료 시 커밋을 통해 락을 해제합니다. 데드락이 감지되면 해당 트랜잭션을 즉시 중단하고, 모든 락을 해제합니다. 동시성 제어를 위해 추가적인 mutex (버퍼 래치, 트랜잭션 래치)를 사용합니다.

## 5. 복구 알고리즘
-Analysis-Redo-Undo 3단계 복구 알고리즘을 구현하여 DB의 Atomicity와 Durability를 보장합니다. 로그를 통해 변경 사항을 기록하고, WAL (Write Ahead Logging) 원칙을 준수합니다. 로그 타입에는 commit, update, rollback, begin, compensate가 있으며, 복구 시 로그 파일을 읽어와 DB를 원래 상태로 복구합니다.

성적 : A+


![5](https://github.com/csh7733/DBMS/assets/149491102/a815eeef-f1bb-4ee5-af74-db983f38bb9e)
![6](https://github.com/csh7733/DBMS/assets/149491102/7ab10607-16d4-4fd5-98be-e475d89046ca)
![7](https://github.com/csh7733/DBMS/assets/149491102/097736b1-9dd0-47f3-8f35-2c211e6ff3eb)
![8](https://github.com/csh7733/DBMS/assets/149491102/7e5f6dea-4a06-4341-b6a4-9eb4da2b8554)
![9](https://github.com/csh7733/DBMS/assets/149491102/b9a02d4c-7ec3-447c-bd31-5145e6ce1661)
![10](https://github.com/csh7733/DBMS/assets/149491102/be26f025-ada4-4cbf-8abf-547052825895)
![11](https://github.com/csh7733/DBMS/assets/149491102/50f00943-7f74-4111-bbac-14978ab4759c)
