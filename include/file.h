#ifndef FILE_H
#define FILE_H
#define PAGE_SIZE 4096
#define VALUE_SIZE 120
#define EMPTY 88
#define EMPTY2 4072
#define EMPTY3 4088
#define EMPTY4 3968
#define EMPTY5 8
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <map>
#include <string>
#include <vector>
#include <iostream>

using namespace std;

typedef uint64_t pagenum_t;

typedef struct page_t{
   	pagenum_t parent_p_n;
	int32_t is_leaf;
	int32_t n_of_keys;
	char empty2[EMPTY5];
	int64_t page_lsn;
	char empty[EMPTY];
	pagenum_t leaf_value_or_internal_value;
	char fill[EMPTY4];
	
}page_t;

typedef struct key_value{
   int64_t key;   
   char value[VALUE_SIZE];
}key_value;

typedef struct key_pagenum{
   int64_t key;
   int64_t p_n;
}key_pagenum;

typedef struct leaf_page{
	pagenum_t parent_p_n;
	int32_t is_leaf;
	int32_t n_of_keys;
	char empty2[EMPTY5];
	int64_t page_lsn;
	char empty[EMPTY];
	pagenum_t right_sibling_p_n;
	key_value record[31];
}leaf_page;

typedef struct header_page{
	pagenum_t free_p_n;
	pagenum_t root_p_n;
	pagenum_t n_of_p;
	char empty2[EMPTY2];
}header_page;

typedef struct free_page{
	pagenum_t next_free_p_n;
	char empty3[EMPTY3];
}free_page;

typedef struct internal_page{
	pagenum_t parent_p_n;
	int32_t is_leaf;
	int32_t n_of_keys;
	char empty2[EMPTY5];
	int64_t page_lsn;
	char empty[EMPTY];
	pagenum_t one_more_p_n;
	key_pagenum record[248];
}internal_page;


extern int fd[11];
extern int table_count;
extern int fdt;
extern map<string,int> path_table;

pagenum_t file_alloc_page(int table_id);
void file_free_page(pagenum_t pagenum,int table_id);
void file_read_page(pagenum_t pagenum, page_t* dest,int table_id);
void file_write_page(pagenum_t pagenum,const page_t* src,int table_id);
void make_header_page(int table_id);
pagenum_t header_modify1(int table_id);
pagenum_t header_modify2(int table_id);
pagenum_t get_header_number_of_pages(int table_id);

#endif
