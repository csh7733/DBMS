#include "file.h"

int fd[11];
int table_count;
int fdt;
map<string,int> path_table;

pagenum_t file_alloc_page(int table_id) {
	int fd2 = fd[table_id];
	pagenum_t first_free_p_n = 0, second_free_p_n = 0, free_flag = 0, next = 0;
	pread(fd2, &free_flag, sizeof(free_flag), 0);
	if (free_flag == 0) { // free page does not exist -> make free page (100 pages)
		pagenum_t cnt_p = 0;
		pread(fd2, &cnt_p, sizeof(cnt_p), 16);
		cnt_p++; //save it
		pwrite(fd2, &cnt_p, sizeof(cnt_p), 0);
		for(int i = cnt_p;i<cnt_p+98;i++){
			next = i+1;
			pwrite(fd2, &next, sizeof(next), 4096*i);
		}
		pagenum_t init = 0;
		pwrite(fd2, &init, sizeof(init), 4096 * next);
		cnt_p = cnt_p + 99;
		pwrite(fd2, &cnt_p, sizeof(cnt_p), 16);
		fsync(fd2); // synchronization
		return cnt_p-100; //take it
	} // if exist, just take
	pread(fd2, &first_free_p_n, sizeof(first_free_p_n), 0);
	pread(fd2, &second_free_p_n, sizeof(second_free_p_n), 4096 * first_free_p_n + 0);
	pwrite(fd2, &second_free_p_n, sizeof(second_free_p_n), 0);
	fsync(fd2); // synchronization
	return first_free_p_n;
}

void file_free_page(pagenum_t pagenum,int table_id) {
	int fd2 = fd[table_id];
	pagenum_t first_free_p_n = 0;
	pread(fd2, &first_free_p_n, sizeof(first_free_p_n), 0);
	pwrite(fd2, &pagenum, sizeof(pagenum), 0);
	pwrite(fd2, &first_free_p_n, sizeof(first_free_p_n), 4096 * pagenum + 0);
	fsync(fd2); // synchronization
}

void file_read_page(pagenum_t pagenum, page_t * dest,int table_id) {
	int fd2 = fd[table_id];
	pread(fd2,dest,PAGE_SIZE,pagenum*PAGE_SIZE);
}

void file_write_page(pagenum_t pagenum, const page_t * src,int table_id) {
	int fd2 = fd[table_id];
	pwrite(fd2, src, PAGE_SIZE, pagenum*PAGE_SIZE);
	fsync(fd2); // synchronization
}

void make_header_page(int table_id) {
	int fd2 = fd[table_id];
	pagenum_t init_free_page_number = 0;
	pagenum_t init_root_page_number = 0;
	pagenum_t init_number_of_pages = 1;
	pwrite(fd2, &init_free_page_number, sizeof(init_free_page_number), 0);
	pwrite(fd2, &init_root_page_number, sizeof(init_root_page_number), 8);
	pwrite(fd2, &init_number_of_pages, sizeof(init_number_of_pages), 16);
	fsync(fd2); // synchronization
}

/* for open_table() function */
pagenum_t get_header_number_of_pages(int table_id) {
	int fd2 = fd[table_id];
	pagenum_t n_of_p = 0;
	pread(fd2, &n_of_p, sizeof(n_of_p), 16);
	return n_of_p;
}

pagenum_t header_modify1(int table_id){
	int fd2 = fd[table_id];
	pagenum_t i1;
	pread(fd2, &i1, sizeof(i1), 0);
	return i1;
}

pagenum_t header_modify2(int table_id){
	int fd2 = fd[table_id];
	pagenum_t i2;
	pread(fd2, &i2, sizeof(i2), 16);
	return i2;
}


