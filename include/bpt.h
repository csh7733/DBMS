#ifndef __BPT_H__
#define __BPT_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "file.h"
#include "buffer.h"
#include "trx.h"
#include "lock_table.h"
#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

// Default order is 4.
#define LEAF_ORDER 32
#define INTERNAL_ORDER 249

// Minimum order is necessarily 3.  We set the maximum
// order arbitrarily.  You may change the maximum order.
#define MIN_ORDER 3
#define MAX_ORDER 20

// Constants for printing part or all of the GPL license.
#define LICENSE_FILE "LICENSE.txt"
#define LICENSE_WARRANTEE 0
#define LICENSE_WARRANTEE_START 592
#define LICENSE_WARRANTEE_END 624
#define LICENSE_CONDITIONS 1
#define LICENSE_CONDITIONS_START 70
#define LICENSE_CONDITIONS_END 625

// TYPES.

/* Type representing the record
 * to which a given key refers.
 * In a real B+ tree system, the
 * record would hold data (in a database)
 * or a file (in an operating system)
 * or some other information.
 * Users can rewrite this part of the code
 * to change the type and content
 * of the value field.
 */
/* 
(Do not use in this project)

typedef struct record {
	int value;
} record;
*/

/* Type representing a node in the B+ tree.
 * This type is general enough to serve for both
 * the leaf and the internal node.
 * The heart of the node is the array
 * of keys and the array of corresponding
 * pointers.  The relation between keys
 * and pointers differs between leaves and
 * internal nodes.  In a leaf, the index
 * of each key equals the index of its corresponding
 * pointer, with a maximum of order - 1 key-pointer
 * pairs.  The last pointer points to the
 * leaf to the right (or NULL in the case
 * of the rightmost leaf).
 * In an internal node, the first pointer
 * refers to lower nodes with keys less than
 * the smallest key in the keys array.  Then,
 * with indices i starting at 0, the pointer
 * at i + 1 points to the subtree with keys
 * greater than or equal to the key in this
 * node at index i.
 * The num_keys field is used to keep
 * track of the number of valid keys.
 * In an internal node, the number of valid
 * pointers is always num_keys + 1.
 * In a leaf, the number of valid pointers
 * to data is always num_keys.  The
 * last leaf pointer points to the next leaf.
 */

/*
(Do not use in this project)
typedef struct node {
	void** pointers;
	int* keys;
	struct node* parent;
	bool is_leaf;
	int num_keys;
	struct node* next; // Used for queue.
} node;
*/

// GLOBALS.

/* The order determines the maximum and minimum
 * number of entries (keys and pointers) in any
 * node.  Every node has at most order - 1 keys and
 * at least (roughly speaking) half that number.
 * Every leaf has as many pointers to data as keys,
 * and every internal node has one more pointer
 * to a subtree than the number of keys.
 * This global variable is initialized to the
 * default value.
 */
extern int leaf_order;
extern int internal_order;

/* The queue is used to print the tree in
 * level order, starting from the root
 * printing each entire rank on a separate
 * line, finishing with the leaves.
 */
// (Do not use in this project) extern node* queue;

/* The user can toggle on and off the "verbose"
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
// (Do not use in this project) extern bool verbose_output;


// FUNCTION PROTOTYPES.

// Output and utility.

void usage_1(void);
void usage_2(void);

typedef struct Q{
	pagenum_t page;
	struct Q* next;
}Q;

extern Q* queue;

void enqueue(pagenum_t new_node);
pagenum_t dequeue(void);
int height(int table_id);
int path_to_root(pagenum_t child,int table_id);
void print_leaves(int table_id);
void print_tree(int table_id);
pagenum_t find_leaf(int64_t key,int table_id,int trx_id);
char* find(int64_t key,int table_id,int trx_id);
char* update(int64_t key,int table_id,int trx_id,char*values);
int cut(int length);

// Insertion.

pagenum_t make_node(int table_id);
pagenum_t make_leaf(int table_id);
int get_left_index(pagenum_t parent_page, pagenum_t left_page,int table_id);
void insert_into_leaf(pagenum_t leaf_p, int64_t key, char* value,int table_id);
void insert_into_leaf_after_splitting(pagenum_t leaf_p, int64_t key, char* value,int table_id);
void insert_into_node(pagenum_t n_page, int left_index, int64_t key, pagenum_t right_page,int table_id);
void insert_into_node_after_splitting(pagenum_t old_node_page, int left_index, int64_t key, pagenum_t right_page,int table_id);
void insert_into_parent(pagenum_t left_page, int64_t key, pagenum_t right_page,int table_id);
void insert_into_new_root(pagenum_t left_page, int64_t key, pagenum_t right_page,int table_id);
void start_new_tree(int64_t key, char* value,int table_id);
int insert(int64_t key, char* value,int table_id);

// Deletion.

int get_neighbor_index(pagenum_t n_page,int table_id);
pagenum_t remove_entry_from_node(pagenum_t n_page, int64_t key,int table_id);
void adjust_root(pagenum_t root_page,int table_id);
void coalesce_nodes(pagenum_t n_page, pagenum_t neighbor_page, int neighbor_index, int64_t k_prime,int table_id);
void redistribute_nodes(pagenum_t n_page, pagenum_t neighbor_page, int neighbor_index,
	int k_prime_index, int64_t k_prime,int table_id);
void delete_entry(pagenum_t n_page, int64_t key,int table_id);
int remove(int64_t key,int table_id);

// getter && setter

pagenum_t get_root(header_page * header_p);
int32_t get_is_leaf(page_t * page);
int32_t get_number_of_keys(page_t * page);
int64_t get_internal_key_at_i(internal_page * page, int i);
pagenum_t get_page_number_at_i(internal_page * page, int i);
int64_t get_leaf_key_at_i(leaf_page * page, int i);
char* get_leaf_value_at_i(leaf_page * page, int i);
pagenum_t get_right_sibling_page_number(leaf_page * page);
pagenum_t get_parent_page_number(page_t * page);
void set_leaf_key_at_i(leaf_page * page, int i, int64_t insert_key);
void set_leaf_value_at_i(leaf_page * page, int i, char* insert_value);
void set_number_of_keys(page_t * page, int n);
void set_is_leaf(page_t * page, int32_t n);
void set_parent_page_number(page_t * child, pagenum_t parent);
void set_right_sibling_page_number(leaf_page * page, pagenum_t next_page);
void set_internal_key_at_i(internal_page * page, int i, int64_t input_key);
void set_page_number_at_i(internal_page * page, int i, pagenum_t page_n);
void set_root(header_page * header_p, pagenum_t page);
#endif /* __BPT_H__*/
