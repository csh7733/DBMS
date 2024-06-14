/*
 *  bpt.c
 */
#define Version "1.14"

#include "bpt.h"
#include "file.h"
#include "buffer.h"
#include <lock_table.h>

int internal_order = INTERNAL_ORDER;
int leaf_order = LEAF_ORDER;

Q* queue = NULL;

void usage_1(void) {
	printf("B+ Tree of Leaf_Order : %d, Internal_Order : %d.\n", leaf_order, internal_order);
	printf("Following Silberschatz, Korth, Sidarshan, Database Concepts, "
		"5th ed.\n\n"
		"To build a B+ tree of a different order, start again and enter "
		"the order\n"
		"as an integer argument:  bpt <order>  ");
	printf("(%d <= order <= %d).\n", MIN_ORDER, MAX_ORDER);
	printf("To start with input from a file of newline-delimited integers, \n"
		"start again and enter the order followed by the filename:\n"
		"bpt <order> <inputfile> .\n");
}

void usage_2(void) {
	printf("Enter any of the following commands after the prompt > :\n"
		"\ti <k>  -- Insert <k> (an integer) as both key and value).\n"
		"\tf <k>  -- Find the value under key <k>.\n"
		"\tp <k> -- Print the path from the root to key k and its associated "
		"value.\n"
		"\tr <k1> <k2> -- Print the keys and values found in the range "
		"[<k1>, <k2>\n"
		"\td <k>  -- Delete key <k> and its associated value.\n"
		"\tx -- Destroy the whole tree.  Start again with an empty tree of the "
		"same order.\n"
		"\tt -- Print the B+ tree.\n"
		"\tl -- Print the keys of the leaves (bottom row of the tree).\n"
		"\tv -- Toggle output of pointer addresses (\"verbose\") in tree and "
		"leaves.\n"
		"\tq -- Quit. (Or use Ctl-D.)\n"
		"\t? -- Print this help message.\n");
}

void enqueue(pagenum_t new_node) {
	Q* c;
	if (queue == NULL) {
		queue = (Q*)malloc(sizeof(Q));
		queue->page = new_node;
		queue->next = NULL;
	}
	else {
		c = queue;
		while (c->next != NULL) {
			c = c->next;
		}
		Q* temp = (Q*)malloc(sizeof(Q));
		temp->page = new_node;
		temp->next = NULL;
		c->next = temp;
	}
}

pagenum_t dequeue(void) {
	Q* n = queue;
	queue = queue->next;
	pagenum_t result = n->page;
	n->next = NULL;
	free(n);
	return result;
}

void print_leaves(int table_id) {
	int i;
	header_page* header = new header_page;
	header = (header_page*)read_buffer(0,table_id,1);
	set_pin(0,table_id);
	unset_pin(0,table_id);
	pagenum_t c_page = get_root(header);
	if (c_page == 0) {
		printf("Empty tree.\n");
		return;
	}
	internal_page* c_i = new internal_page;
	c_i = (internal_page*)read_buffer(c_page,table_id,1);
	set_pin(c_page,table_id);
	while (!get_is_leaf((page_t*)c_i)){
		unset_pin(c_page,table_id);
		c_page = get_page_number_at_i(c_i, 0);
		c_i = (internal_page*)read_buffer(c_page,table_id,1);
		set_pin(c_page,table_id);
	}
	unset_pin(c_page,table_id);
	leaf_page* c_l = new leaf_page;
	c_l = (leaf_page*)read_buffer(c_page,table_id,1);
	set_pin(c_page,table_id);
	while (true) {
		for (i = 0; i < get_number_of_keys((page_t*)c_l); i++) {
			printf("%d ", get_leaf_key_at_i(c_l, i));
		}
		if (get_right_sibling_page_number(c_l) != 0) {
			printf(" | ");
			unset_pin(c_page,table_id);
			c_page = get_right_sibling_page_number(c_l);
			c_l = (leaf_page*)read_buffer(c_page,table_id,1);
			set_pin(c_page,table_id);
		}
		else
			break;
	}
	unset_pin(c_page,table_id);
	printf("\n");
}


/* Utility function to give the height
 * of the tree, which length in number of edges
 * of the path from the root to any leaf.
 */
int height(int table_id) {
	int h = 0;
	header_page* header = new header_page;
	header = (header_page*)read_buffer(0,table_id,1);
	set_pin(0,table_id);
	pagenum_t c_page = get_root(header);
	unset_pin(0,table_id);
	internal_page* c = new internal_page;
	c = (internal_page*)read_buffer(c_page,table_id,1);
	set_pin(c_page,table_id);
	while (!get_is_leaf((page_t*)c)) {
		unset_pin(c_page,table_id);
		c_page = get_page_number_at_i(c, 0);
		c = (internal_page*)read_buffer(c_page,table_id,1);
		set_pin(c_page,table_id);
		h++;
	}
	unset_pin(c_page,table_id);
	return h;
}


/* Utility function to give the length in edges
 * of the path from any node to the root.
 */
int path_to_root(pagenum_t child_page,int table_id) {
	int length = 0;
	pagenum_t c_page = child_page;
	header_page* header = new header_page;
	header = (header_page*)read_buffer(0,table_id,1);
	set_pin(0,table_id);
	internal_page* c = new internal_page;
	c = (internal_page*)read_buffer(c_page,table_id,1);
	set_pin(c_page,table_id);
	while (c_page != get_root(header)) {
		unset_pin(c_page,table_id);
		c_page = get_parent_page_number((page_t*)c);
		c = (internal_page*)read_buffer(c_page,table_id,1);	
		set_pin(c_page,table_id);
		length++;
	}
	unset_pin(0,table_id);
	unset_pin(c_page,table_id);
	return length;
}


/* Prints the B+ tree in the command
 * line in level (rank) order, with the
 * keys in each node and the '|' symbol
 * to separate nodes.
 * With the verbose_output flag set.
 * the values of the pointers corresponding
 * to the keys also appear next to their respective
 * keys, in hexadecimal notation.
 */
void print_tree(int table_id) {

	pagenum_t n_page = 0;
	int i = 0;
	int rank = 0;
	int new_rank = 0;
	header_page* header = new header_page;
	header = (header_page*)read_buffer(0,table_id,1);
	set_pin(0,table_id);
	pagenum_t root_page = get_root(header);
	unset_pin(0,table_id);

	if (root_page == 0) {
		printf("Empty tree.\n");
		return;
	}
	queue = NULL;
	enqueue(root_page);
	internal_page* n = new internal_page;
	leaf_page* n_l = new leaf_page;
	while (queue != NULL) {
		n_page = dequeue();
		n = (internal_page*)read_buffer(n_page,table_id,1);
		n_l = (leaf_page*)read_buffer(n_page,table_id,1);
		set_pin(n_page,table_id);
		if (get_parent_page_number((page_t*)n) != 0) {
			pagenum_t parent_page = get_parent_page_number((page_t*)n);
			internal_page* parent = new internal_page;
			parent = (internal_page*)read_buffer(parent_page,table_id,1);
			set_pin(parent_page,table_id);
			if(n_page == get_page_number_at_i(parent, 0)){
				new_rank = path_to_root(n_page,table_id);
				if (new_rank != rank) {
					rank = new_rank;
					printf("\n");
				}
			}
			unset_pin(parent_page,table_id);
		}
		printf("<%d> ",n_page);
		for (i = 0; i < get_number_of_keys((page_t*)n); i++) {
			if (get_is_leaf((page_t*)n)) {
				printf("%d ", get_leaf_key_at_i(n_l, i));
			}
			else printf("%d ", get_internal_key_at_i(n, i));
		}
		if (!get_is_leaf((page_t*)n))
			for (i = 0; i <= get_number_of_keys((page_t*)n); i++)
				enqueue(get_page_number_at_i(n, i));
		printf("| ");
		unset_pin(n_page,table_id);
	}
	printf("\n");
}


/* Traces the path from the root to a leaf, searching
 * by key.  Displays information about the path
 * if the verbose flag is set.
 * Returns the leaf containing the given key.
 */
pagenum_t find_leaf(int64_t key,int table_id,int trx_id) {
	int i = 0;
	header_page* header = (header_page*) read_buffer(0,table_id,0); // read header
	pagenum_t c_page = get_root(header);
	unlock(0,table_id);
	if (c_page == 0) {
		printf("Empty tree.\n");
		return c_page;
	}
	internal_page* c = (internal_page*) read_buffer(c_page,table_id,0);
	while (!get_is_leaf((page_t*)c)) {
		i = 0;
		while (i < get_number_of_keys((page_t*)c)) {
			if (key >= get_internal_key_at_i(c, i)) i++;
			else break;
		}
		unlock(c_page,table_id);
		c_page = get_page_number_at_i(c, i);
		c = (internal_page*)read_buffer(c_page,table_id,0);
	}
	unlock(c_page,table_id);
	return c_page;
}


/* Finds and returns the record to which
 * a key refers.
 */
char* find(int64_t key,int table_id,int trx_id) {
	//printf("flag : key %d\n",key);
	int i = 0;
	pagenum_t c_page = find_leaf(key,table_id,trx_id);
	if (c_page == 0) return NULL;
	leaf_page* c =(leaf_page*) read_buffer(c_page,table_id,0);
	for (i = 0; i < get_number_of_keys((page_t*)c); i++){
		if (get_leaf_key_at_i(c, i) == key) break;
	}
	if (i == get_number_of_keys((page_t*)c)){
		unlock(c_page,table_id);
		return NULL;
	}
	else {
		if(trx_id != -1){
			lock_t* l = lock_acquire(c_page,table_id,get_leaf_key_at_i(c, i),trx_id,0);
			dolock(c_page,table_id);
			unlock(c_page,table_id);
		}
		return get_leaf_value_at_i(c, i);
	}
}

char* update(int64_t key,int table_id,int trx_id,char*values){
	int i = 0;
	pagenum_t c_page = find_leaf(key,table_id,trx_id);
	leaf_page* c =(leaf_page*) read_buffer(c_page,table_id,0);
	for (i = 0; i < get_number_of_keys((page_t*)c); i++){
		if (get_leaf_key_at_i(c, i) == key) break;
	}
	if (i == get_number_of_keys((page_t*)c)){
		unlock(c_page,table_id);
		printf("error!\n");
		return NULL;
	}
	else {
		lock_t* l = lock_acquire(c_page,table_id,get_leaf_key_at_i(c, i),trx_id,1);
		dolock(c_page,table_id);
		//add_update(trx_id,1,table_id,c_page,i,get_leaf_value_at_i(c, i),values);
		set_leaf_value_at_i(c,i,values);
		write_buffer(c_page,(page_t*)c,table_id);
		unlock(c_page,table_id);
		return get_leaf_value_at_i(c, i);
	}
}

/* Finds the appropriate place to
 * split a node that is too big into two.
 */
int cut(int length) {
	if (length % 2 == 0)
		return length / 2;
	else
		return length / 2 + 1;
}


// INSERTION

/* Creates a new general node, which can be adapted
 * to serve as either a leaf or an internal node.
 */
pagenum_t make_node(int table_id) {
	pagenum_t new_node_page;
	read_buffer(0,table_id,1);
	new_node_page = file_alloc_page(table_id);
	buffer_header(table_id);
	// set all to default(0)
	page_t* new_node = read_buffer(new_node_page,table_id,1); 
	set_pin(new_node_page,table_id);
	set_is_leaf(new_node, 0);
	set_number_of_keys(new_node, 0);
	set_parent_page_number(new_node, 0);
	write_buffer(new_node_page, new_node,table_id);
	unset_pin(new_node_page,table_id);
	return new_node_page;
}

/* Creates a new leaf by creating a node
 * and then adapting it appropriately.
 */
pagenum_t make_leaf(int table_id) {
	pagenum_t leaf_p = make_node(table_id);
	leaf_page* leaf = (leaf_page*)read_buffer(leaf_p,table_id,1); 
	set_pin(leaf_p,table_id);
	set_is_leaf((page_t*)leaf, 1);
	write_buffer(leaf_p,(page_t*)leaf,table_id);
	unset_pin(leaf_p,table_id);
	return leaf_p;
}


/* Helper function used in insert_into_parent
 * to find the index of the parent's pointer to
 * the node to the left of the key to be inserted.
 */
int get_left_index(pagenum_t parent_page, pagenum_t left_page,int table_id) {

	int left_index = 0;

	internal_page* parent = (internal_page*)read_buffer(parent_page,table_id,1);
	set_pin(parent_page,table_id);

	while (left_index <= get_number_of_keys((page_t*)parent) &&
		get_page_number_at_i(parent, left_index) != left_page)
		left_index++;

	unset_pin(parent_page,table_id);
	return left_index;
}

/* Inserts a new pointer to a record and its corresponding
 * key into a leaf.
 * Returns the altered leaf.
 */
void insert_into_leaf(pagenum_t leaf_p, int64_t key, char* value,int table_id) {

	int i, insertion_point;
	leaf_page* leaf = (leaf_page*)read_buffer(leaf_p,table_id,1);
	set_pin(leaf_p,table_id);

	insertion_point = 0;
	while (insertion_point < get_number_of_keys((page_t*)leaf) && get_leaf_key_at_i(leaf, insertion_point) < key)
		insertion_point++;

	set_number_of_keys((page_t*)leaf, get_number_of_keys((page_t*)leaf) + 1);
	for (i = get_number_of_keys((page_t*)leaf) - 1; i > insertion_point; i--) {
		int64_t insert_key = get_leaf_key_at_i(leaf, i - 1);
		set_leaf_key_at_i(leaf, i, insert_key);
		set_leaf_value_at_i(leaf, i, get_leaf_value_at_i(leaf, i - 1));
	}
	set_leaf_key_at_i(leaf, insertion_point, key);
	set_leaf_value_at_i(leaf, insertion_point, value);
	write_buffer(leaf_p,(page_t*)leaf,table_id);
	unset_pin(leaf_p,table_id);
}


/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
void insert_into_leaf_after_splitting(pagenum_t leaf_p, int64_t key, char* value,int table_id) {

	pagenum_t new_leaf_page;
	int64_t* temp_keys;
	int64_t new_key;
	char** temp_values;
	int insertion_index, split, i, j;

	new_leaf_page = make_leaf(table_id);
	leaf_page* new_leaf = (leaf_page*)read_buffer(new_leaf_page,table_id,1);
	set_pin(new_leaf_page,table_id);

	leaf_page* leaf = (leaf_page*)read_buffer(leaf_p,table_id,1);
	set_pin(leaf_p,table_id);
	/* malloc */
	temp_keys = (int64_t*)malloc(sizeof(int64_t) * leaf_order);
	temp_values = (char**)malloc(sizeof(char*) * leaf_order);

	for (int ii = 0; ii < leaf_order; ii++) {
		temp_values[ii] = (char*)malloc(sizeof(char) * 120);
	}
	/* malloc */
	if (temp_keys == NULL) {
		perror("Temporary keys array.");
		exit(EXIT_FAILURE);
	}

	if (temp_values == NULL) {
		perror("Temporary pointers array.");
		exit(EXIT_FAILURE);
	}

	insertion_index = 0;
	while (insertion_index < leaf_order - 1 && get_leaf_key_at_i(leaf, insertion_index) < key)
		insertion_index++;

	for (i = 0, j = 0; i < get_number_of_keys((page_t*)leaf); i++, j++) {
		if (j == insertion_index) j++;
		temp_keys[j] = get_leaf_key_at_i(leaf, i);
		strcpy(temp_values[j], get_leaf_value_at_i(leaf, i));
	}

	temp_keys[insertion_index] = key;
	strcpy(temp_values[insertion_index], value);

	set_number_of_keys((page_t*)leaf, 0);

	split = cut(leaf_order - 1);

	for (i = 0; i < split; i++) {
		set_number_of_keys((page_t*)leaf, get_number_of_keys((page_t*)leaf) + 1);
		set_leaf_value_at_i(leaf, i, temp_values[i]);
		set_leaf_key_at_i(leaf, i, temp_keys[i]);
	}

	for (i = split, j = 0; i < leaf_order; i++, j++) {
		set_number_of_keys((page_t*)new_leaf, get_number_of_keys((page_t*)new_leaf) + 1);
		set_leaf_value_at_i(new_leaf, j, temp_values[i]);
		set_leaf_key_at_i(new_leaf, j, temp_keys[i]);
	}
	// if free, can problem to write page
	for (int ii = 0; ii < leaf_order; ii++) {
		free(temp_values[ii]);
	}
	free(temp_values);
	free(temp_keys);

	set_right_sibling_page_number(new_leaf, get_right_sibling_page_number(leaf));
	set_right_sibling_page_number(leaf, new_leaf_page);
	set_parent_page_number((page_t*)new_leaf, get_parent_page_number((page_t*)leaf));
	new_key = get_leaf_key_at_i(new_leaf, 0);
	write_buffer(new_leaf_page,(page_t*) new_leaf,table_id);
	write_buffer(leaf_p,(page_t*) leaf,table_id);
	unset_pin(new_leaf_page,table_id);
	unset_pin(leaf_p,table_id);
	return insert_into_parent(leaf_p, new_key, new_leaf_page,table_id);
}


/* Inserts a new key and pointer to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
void insert_into_node(pagenum_t n_page, int left_index, int64_t key, pagenum_t right_page,int table_id) {
	int i;

	internal_page* n = (internal_page*)read_buffer(n_page,table_id,1);
	set_pin(n_page,table_id);

	set_number_of_keys((page_t*)n, get_number_of_keys((page_t*)n) + 1);
	for (i = get_number_of_keys((page_t*)n) - 1; i > left_index; i--) {
		set_page_number_at_i(n, i + 1, get_page_number_at_i(n, i));
		set_internal_key_at_i(n, i, get_internal_key_at_i(n, i - 1));
	}
	set_page_number_at_i(n, left_index + 1, right_page);
	set_internal_key_at_i(n, left_index, key);
	write_buffer(n_page,(page_t*)n,table_id);
	unset_pin(n_page,table_id);

}


/* Inserts a new key and pointer to a node
 * into a node, causing the node's size to exceed
 * the order, and causing the node to split into two.
 */
void insert_into_node_after_splitting(pagenum_t old_node_page, int left_index, int64_t key, pagenum_t right_page,int table_id) {

	int i, j;
	int64_t split, k_prime;
	pagenum_t new_node_page, child_page;
	int64_t* temp_keys;
	pagenum_t* temp_pages;

	internal_page* old_node = (internal_page*)read_buffer(old_node_page,table_id,1);
	set_pin(old_node_page,table_id);

	/* First create a temporary set of keys and pointers
	 * to hold everything in order, including
	 * the new key and pointer, inserted in their
	 * correct places.
	 * Then create a new node and copy half of the
	 * keys and pointers to the old node and
	 * the other half to the new.
	 */

	temp_pages = (pagenum_t*)malloc((internal_order + 1) * sizeof(pagenum_t));
	if (temp_pages == NULL) {
		perror("Temporary pages array for splitting nodes.");
		exit(EXIT_FAILURE);
	}
	temp_keys = (int64_t*)malloc(internal_order * sizeof(int64_t));
	if (temp_keys == NULL) {
		perror("Temporary keys array for splitting nodes.");
		exit(EXIT_FAILURE);
	}

	for (i = 0, j = 0; i < get_number_of_keys((page_t*)old_node) + 1; i++, j++) {
		if (j == left_index + 1) j++;
		temp_pages[j] = get_page_number_at_i(old_node, i);
	}

	for (i = 0, j = 0; i < get_number_of_keys((page_t*)old_node); i++, j++) {
		if (j == left_index) j++;
		temp_keys[j] = get_internal_key_at_i(old_node, i);
	}

	temp_pages[left_index + 1] = right_page;
	temp_keys[left_index] = key;

	/* Create the new node and copy
	 * half the keys and pointers to the
	 * old and half to the new.
	 */
	split = cut(internal_order);
	new_node_page = make_node(table_id);

	internal_page* new_node = (internal_page*)read_buffer(new_node_page,table_id,1);
	set_pin(new_node_page,table_id);

	set_number_of_keys((page_t*)old_node, 0);
	for (i = 0; i < split - 1; i++) {
		set_number_of_keys((page_t*)old_node, get_number_of_keys((page_t*)old_node) + 1);
		set_page_number_at_i(old_node, i, temp_pages[i]);
		set_internal_key_at_i(old_node, i, temp_keys[i]);
	}
	set_page_number_at_i(old_node, i, temp_pages[i]);
	k_prime = temp_keys[split - 1];
	for (++i, j = 0; i < internal_order; i++, j++) {
		set_number_of_keys((page_t*)new_node, get_number_of_keys((page_t*)new_node) + 1);
		set_page_number_at_i(new_node, j, temp_pages[i]);
		set_internal_key_at_i(new_node, j, temp_keys[i]);
	}
	set_page_number_at_i(new_node, j, temp_pages[i]);
	//free(temp_pages);
	//free(temp_keys);
	set_parent_page_number((page_t*)new_node, get_parent_page_number((page_t*)old_node));
	page_t* child;
	for (i = 0; i <= get_number_of_keys((page_t*)new_node); i++) {
		child_page = get_page_number_at_i(new_node, i);
		child = read_buffer(child_page,table_id,1);
		set_pin(child_page,table_id);
		set_parent_page_number(child, new_node_page);
		write_buffer(child_page, child,table_id);
		unset_pin(child_page,table_id);
	}

	/* Insert a new key into the parent of the two
	 * nodes resulting from the split, with
	 * the old node to the left and the new to the right.
	 */
	write_buffer(old_node_page, (page_t*)old_node,table_id);
	write_buffer(new_node_page, (page_t*)new_node,table_id);
	unset_pin(old_node_page,table_id);
	unset_pin(new_node_page,table_id);
	insert_into_parent(old_node_page, k_prime, new_node_page,table_id);
}



/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
void insert_into_parent(pagenum_t left_page, int64_t key, pagenum_t right_page,int table_id) {

	int left_index;
	pagenum_t parent_page;

	page_t* left=read_buffer(left_page,table_id,1);
	set_pin(left_page,table_id);

	parent_page = get_parent_page_number(left);

	/* Case: new root. */

	if (parent_page == 0) {
		unset_pin(left_page,table_id);
		insert_into_new_root(left_page, key, right_page,table_id);
		return;
	}

	/* Case: leaf or node. (Remainder of
	 * function body.)
	 */

	 /* Find the parent's pointer to the left
	  * node.
	  */

	left_index = get_left_index(parent_page, left_page,table_id);


	/* Simple case: the new key fits into the node.
	 */

	internal_page* parent = (internal_page*)read_buffer(parent_page,table_id,1);
	set_pin(parent_page,table_id);

	if (get_number_of_keys((page_t*)parent) < internal_order - 1) {
		unset_pin(left_page,table_id);
		unset_pin(parent_page,table_id);
		insert_into_node(parent_page, left_index, key, right_page,table_id);
		return;
	}
	/* Harder case:  split a node in order
	 * to preserve the B+ tree properties.
	 */
	unset_pin(left_page,table_id);
	unset_pin(parent_page,table_id);
	insert_into_node_after_splitting(parent_page, left_index, key, right_page,table_id);
}


/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
void insert_into_new_root(pagenum_t left_page, int64_t key, pagenum_t right_page,int table_id) {

	page_t* left = read_buffer(left_page,table_id,1);
	set_pin(left_page,table_id);

	page_t* right = read_buffer(right_page,table_id,1);
	set_pin(right_page,table_id);

	pagenum_t root_page = make_node(table_id);
	internal_page* root = (internal_page*)read_buffer(root_page,table_id,1);
	set_pin(root_page,table_id);

	set_number_of_keys((page_t*)root, get_number_of_keys((page_t*)root) + 1);
	set_internal_key_at_i(root, 0, key);
	set_page_number_at_i(root, 0, left_page);
	set_page_number_at_i(root, 1, right_page);
	set_parent_page_number((page_t*)root, 0);
	set_parent_page_number(left, root_page);
	set_parent_page_number(right, root_page);
	header_page* header = (header_page*)read_buffer(0,table_id,1);
	set_pin(0,table_id);
	set_root(header,root_page); // return type is void, so make root now
	write_buffer(root_page,(page_t*)root,table_id);
	write_buffer(left_page, left,table_id);
	write_buffer(right_page, right,table_id);
	write_buffer(0,(page_t*)header,table_id);
	unset_pin(root_page,table_id);
	unset_pin(left_page,table_id);
	unset_pin(right_page,table_id);
	unset_pin(0,table_id);
}



/* First insertion:
 * start a new tree.
 */
void start_new_tree(int64_t key, char* value,int table_id) {

	pagenum_t root_page = make_leaf(table_id);
	leaf_page* root = (leaf_page*)read_buffer(root_page,table_id,1);
	set_pin(root_page,table_id);
	set_number_of_keys((page_t*)root, get_number_of_keys((page_t*)root) + 1);
	set_leaf_key_at_i(root, 0, key);
	set_leaf_value_at_i(root, 0, value);
	set_right_sibling_page_number(root, 0);
	set_parent_page_number((page_t*)root, 0);
	header_page* header = (header_page*)read_buffer(0,table_id,1);
	set_pin(0,table_id);
	set_root(header,root_page); // return type is void, so make root now
	write_buffer(root_page,(page_t*)root,table_id);
	write_buffer(0,(page_t*)header,table_id);
	unset_pin(0,table_id);
	unset_pin(root_page,table_id);
}



/* Master insertion function.
 * Inserts a key and an associated value into
 * the B+ tree, causing the tree to be adjusted
 * however necessary to maintain the B+ tree
 * properties.
 */
int insert(int64_t key, char* value,int table_id) {

	pagenum_t leaf_p;
	if (find(key,table_id,-1) != NULL) // ignore duplicate
		return -1; ///fail
	/* Case: the tree does not exist yet.
	 * Start a new tree.
	 */
	header_page* header = (header_page*)read_buffer(0,table_id,1);
	//printf("%d %d %d",header->free_p_n, header->root_p_n,header->n_of_p);
	set_pin(0,table_id);
	if (get_root(header) == 0) {
		unset_pin(0,table_id);
		start_new_tree(key, value,table_id);
		return 0; //success
	}
	/* Case: the tree already exists.
	 * (Rest of function body.)
	 */

	leaf_p = find_leaf(key,table_id,0);
	leaf_page* leaf = (leaf_page*)read_buffer(leaf_p,table_id,1);
	set_pin(leaf_p,table_id);
	/* Case: leaf has room for key and pointer.
	 */

	if (get_number_of_keys((page_t*)leaf) < leaf_order - 1) {
		unset_pin(leaf_p,table_id);
		insert_into_leaf(leaf_p, key, value,table_id);
		return 0; //success
	}


	/* Case:  leaf must be split.
	 */;
	unset_pin(leaf_p,table_id);
	insert_into_leaf_after_splitting(leaf_p, key, value,table_id);
	return 0; //success
}




// DELETION.

/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index(pagenum_t n_page, int table_id) {

	int i;

	/* Return the index of the key to the left
	 * of the pointer in the parent pointing
	 * to n.
	 * If n is the leftmost child, this means
	 * return -1.
	 */
	page_t* n = new page_t;
	n = read_buffer(n_page, table_id,1);
	set_pin(n_page, table_id);

	pagenum_t parent_page = get_parent_page_number(n);
	internal_page* parent = new internal_page;
	parent = (internal_page*)read_buffer(parent_page, table_id,1);
	set_pin(parent_page, table_id);

	for (i = 0; i <= get_number_of_keys((page_t*)parent); i++) {
		if (get_page_number_at_i(parent, i) == n_page) {
			unset_pin(n_page, table_id);
			unset_pin(parent_page, table_id);
			return i - 1;
		}
	}
	// Error state.
	printf("Search for nonexistent pointer to node in parent.\n");
	printf("Node:  %#ld\n", (unsigned long)n);
	exit(EXIT_FAILURE);
}


pagenum_t remove_entry_from_node(pagenum_t n_page, int64_t key,int table_id) {

	int i;

	// becasue it is same : meaning of compare key == compare value, just compare key instead compare value
	i = 0;
	page_t* n = new page_t;
	n = read_buffer(n_page, table_id,1);
	set_pin(n_page, table_id);
	if (get_is_leaf(n)) {
		leaf_page* n_l = new leaf_page;
		n_l = (leaf_page*)read_buffer(n_page, table_id,1);
		while (get_leaf_key_at_i(n_l, i) != key)
			i++;
		for (++i; i < get_number_of_keys((page_t*)n_l); i++) {
			set_leaf_key_at_i(n_l, i - 1, get_leaf_key_at_i(n_l, i));
			set_leaf_value_at_i(n_l, i - 1, get_leaf_value_at_i(n_l, i));
		}
		set_number_of_keys((page_t*)n_l, get_number_of_keys((page_t*)n_l) - 1);
		write_buffer(n_page, (page_t*)n_l, table_id);
		unset_pin(n_page, table_id);
	}
	else {
		internal_page* n_i = new internal_page;
		n_i = (internal_page*)read_buffer(n_page, table_id,1);
		while (get_internal_key_at_i(n_i, i) != key)
			i++;
		for (++i; i < get_number_of_keys((page_t*)n_i); i++) {
			set_internal_key_at_i(n_i, i - 1, get_internal_key_at_i(n_i, i));
			i++;
			set_page_number_at_i(n_i, i - 1, get_page_number_at_i(n_i, i));
			i--;
		}
		set_number_of_keys((page_t*)n_i, get_number_of_keys((page_t*)n_i) - 1);
		write_buffer(n_page, (page_t*)n_i, table_id);
		unset_pin(n_page, table_id);
	}
	return n_page;
}


void adjust_root(pagenum_t root_page, int table_id) {

	pagenum_t new_root_page;

	/* Case: nonempty root.
	 * Key and pointer have already been deleted,
	 * so nothing to be done.
	 */
	page_t* root = new page_t;
	root = read_buffer(root_page, table_id,1);
	set_pin(root_page, table_id);

	if (get_number_of_keys(root) > 0) {
		unset_pin(root_page, table_id);
		return;
	}

	/* Case: empty root.
	 */

	 // If it has a child, promote 
	 // the first (only) child
	 // as the new root.

	if (!get_is_leaf(root)) {
		internal_page* root_i = new internal_page;
		root_i = (internal_page*)read_buffer(root_page, table_id,1);
		new_root_page = get_page_number_at_i(root_i, 0);
		page_t* new_root = new page_t;
		new_root = read_buffer(new_root_page, table_id,1);
		set_pin(new_root_page, table_id);
		set_parent_page_number(new_root, 0);
		write_buffer(new_root_page, new_root, table_id);
		unset_pin(new_root_page, table_id);
	}

	// If it is a leaf (has no children),
	// then the whole tree is empty.

	else {
		new_root_page = 0;
	}
	header_page* header = new header_page;
	header = (header_page*)read_buffer(0, table_id,1);
	set_pin(0, table_id);
	set_root(header, new_root_page); // make new root
	write_buffer(0, (page_t*)header, table_id);
	unset_pin(0, table_id);
	unset_pin(root_page, table_id);
	/* make old_root page to free page
	free(root->keys);
	free(root->pointers);
	free(root);
	*/
	//file_free_page(root_page, table_id);
}


/* Coalesces a node that has become
 * too small after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 */
void coalesce_nodes(pagenum_t n_page, pagenum_t neighbor_page, int neighbor_index, int64_t k_prime, int table_id) {

	int i, j, neighbor_insertion_index;
	int32_t n_end;
	pagenum_t tmp_page;

	/* Swap neighbor with node if node is on the
	 * extreme left and neighbor is to its right.
	 */

	if (neighbor_index == -1) {
		tmp_page = n_page;
		n_page = neighbor_page;
		neighbor_page = tmp_page;
	}

	/* Starting point in the neighbor for copying
	 * keys and pointers from n.
	 * Recall that n and neighbor have swapped places
	 * in the special case of n being a leftmost child.
	 */
	page_t* n = new page_t;
	n = read_buffer(n_page, table_id,1);
	set_pin(n_page, table_id);

	page_t* neighbor = new page_t;
	neighbor = read_buffer(neighbor_page, table_id,1);
	set_pin(neighbor_page, table_id);

	neighbor_insertion_index = get_number_of_keys(neighbor);

	/* Case:  nonleaf node.
	 * Append k_prime and the following pointer.
	 * Append all pointers and keys from the neighbor.
	 */

	if (!get_is_leaf(n)) {

		/* Append k_prime.
		 */
		internal_page* n_i = new internal_page;
		n_i = (internal_page*)read_buffer(n_page, table_id,1);

		internal_page* neighbor_i = new internal_page;
		neighbor_i = (internal_page*)read_buffer(neighbor_page, table_id,1);

		set_number_of_keys((page_t*)neighbor_i, get_number_of_keys((page_t*)neighbor_i) + 1);
		set_internal_key_at_i(neighbor_i, neighbor_insertion_index, k_prime);


		n_end = get_number_of_keys((page_t*)n_i);

		for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
			set_number_of_keys((page_t*)neighbor_i, get_number_of_keys((page_t*)neighbor_i) + 1);
			set_internal_key_at_i(neighbor_i, i, get_internal_key_at_i(n_i, j));
			set_page_number_at_i(neighbor_i, i, get_page_number_at_i(n_i, j));
			set_number_of_keys((page_t*)n_i, get_number_of_keys((page_t*)n_i) - 1);
		}

		/* The number of pointers is always
		 * one more than the number of keys.
		 */

		set_page_number_at_i(neighbor_i, i, get_page_number_at_i(n_i, j));

		/* All children must now point up to the same parent.
		 */
		page_t* tmp = new page_t;
		for (i = 0; i < get_number_of_keys((page_t*)neighbor_i) + 1; i++) {
			tmp_page = get_page_number_at_i(neighbor_i, i);
			tmp = read_buffer(tmp_page, table_id,1);
			set_pin(tmp_page, table_id);
			set_parent_page_number(tmp, neighbor_page);
			write_buffer(tmp_page, tmp, table_id);
			unset_pin(tmp_page, table_id);
		}
		write_buffer(neighbor_page, (page_t*)neighbor_i, table_id);
		write_buffer(n_page, (page_t*)n_i, table_id);
		unset_pin(neighbor_page, table_id);
		unset_pin(n_page, table_id);
	}

	/* In a leaf, append the keys and pointers of
	 * n to the neighbor.
	 * Set the neighbor's last pointer to point to
	 * what had been n's right neighbor.
	 */

	else {
		leaf_page* n_l = new leaf_page;
		n_l = (leaf_page*)read_buffer(n_page, table_id,1);

		leaf_page* neighbor_l = new leaf_page;
		neighbor_l = (leaf_page*)read_buffer(neighbor_page, table_id,1);

		for (i = neighbor_insertion_index, j = 0; j < get_number_of_keys((page_t*)n_l); i++, j++) {
			set_number_of_keys((page_t*)neighbor_l, get_number_of_keys((page_t*)neighbor_l) + 1);
			set_leaf_key_at_i(neighbor_l, i, get_leaf_key_at_i(n_l, j));
			set_leaf_value_at_i(neighbor_l, i, get_leaf_value_at_i(n_l, j));
		}
		set_right_sibling_page_number(neighbor_l, get_right_sibling_page_number(n_l));
		write_buffer(neighbor_page,(page_t*) neighbor_l, table_id);
		write_buffer(n_page, (page_t*)n_l, table_id);
		unset_pin(neighbor_page, table_id);
		unset_pin(n_page, table_id);
	}
	delete_entry(get_parent_page_number(n), k_prime, table_id); // do not use this function's fourth parameter
	//file_free_page(n_page, table_id);
}


/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the
 * maximum
 */
void redistribute_nodes(pagenum_t n_page, pagenum_t neighbor_page, int neighbor_index,
	int k_prime_index, int64_t k_prime, int table_id) {

	int i;
	pagenum_t tmp_page;

	/* Case: n has a neighbor to the left.
	 * Pull the neighbor's last key-pointer pair over
	 * from the neighbor's right end to n's left end.
	 */

	pagenum_t parent_page;
	internal_page* parent = new internal_page;

	page_t* n = new page_t;
	n = read_buffer(n_page, table_id,1);
	set_pin(n_page, table_id);

	int32_t n_number_of_keys = get_number_of_keys(n) - 1;

	internal_page* n_i = new internal_page;
	internal_page* neighbor_i = new internal_page;
	leaf_page* n_l = new leaf_page;
	page_t* tmp = new page_t;
	leaf_page* neighbor_l = new leaf_page;

	if (neighbor_index != -1) {
		if (!get_is_leaf(n)) {
			n_i = (internal_page*)read_buffer(n_page, table_id,1);

			set_page_number_at_i(n_i, n_number_of_keys + 1, get_page_number_at_i(n_i, n_number_of_keys));
			for (i = n_number_of_keys; i > 0; i--) {
				set_internal_key_at_i(n_i, i, get_internal_key_at_i(n_i, i - 1));
				set_page_number_at_i(n_i, i, get_page_number_at_i(n_i, i - 1));
			}
		}
		else {
			n_l = (leaf_page*)read_buffer(n_page, table_id,1);

			for (i = n_number_of_keys; i > 0; i--) {
				set_leaf_key_at_i(n_l, i, get_leaf_key_at_i(n_l, i - 1));
				set_leaf_value_at_i(n_l, i, get_leaf_value_at_i(n_l, i - 1));
			}
		}
		if (!get_is_leaf(n)) {
			neighbor_i = (internal_page*)read_buffer(neighbor_page, table_id,1);
			set_pin(neighbor_page, table_id);

			set_page_number_at_i(n_i, 0, get_page_number_at_i(neighbor_i, get_number_of_keys((page_t*)neighbor_i)));
			tmp_page = get_page_number_at_i(n_i, 0);

			tmp = read_buffer(tmp_page, table_id,1);
			set_pin(tmp_page, table_id);

			set_parent_page_number(tmp, n_page);
			set_page_number_at_i(neighbor_i, get_number_of_keys((page_t*)neighbor_i), 0);
			set_internal_key_at_i(n_i, 0, k_prime);

			parent_page = get_parent_page_number((page_t*)n_i);
			parent = (internal_page*)read_buffer(parent_page, table_id,1);
			set_pin(parent_page, table_id);

			set_internal_key_at_i(parent, k_prime_index,
				get_internal_key_at_i(neighbor_i, get_number_of_keys((page_t*)neighbor_i) - 1));
			set_number_of_keys((page_t*)neighbor_i, get_number_of_keys((page_t*)neighbor_i) - 1);

			write_buffer(n_page, (page_t*)n_i, table_id);
			write_buffer(neighbor_page, (page_t*)neighbor_i, table_id);
			write_buffer(tmp_page, tmp, table_id);
			write_buffer(parent_page, (page_t*)parent, table_id);
			unset_pin(n_page, table_id);
			unset_pin(neighbor_page, table_id);
			unset_pin(tmp_page, table_id);
			unset_pin(parent_page, table_id);

		}
		else {
			neighbor_l = (leaf_page*)read_buffer(neighbor_page, table_id,1);
			set_pin(neighbor_page, table_id);

			set_leaf_value_at_i(n_l, 0, get_leaf_value_at_i(neighbor_l, get_number_of_keys((page_t*)neighbor_l) - 1));
			set_leaf_value_at_i(neighbor_l, get_number_of_keys((page_t*)neighbor_l) - 1, 0);
			set_leaf_key_at_i(n_l, 0, get_leaf_key_at_i(neighbor_l, get_number_of_keys((page_t*)neighbor_l) - 1));

			parent_page = get_parent_page_number((page_t*)n_l);
			parent = (internal_page*)read_buffer(parent_page, table_id,1);
			set_pin(parent_page, table_id);

			set_internal_key_at_i(parent, k_prime_index, get_leaf_key_at_i(n_l, 0));
			set_number_of_keys((page_t*)neighbor_l, get_number_of_keys((page_t*)neighbor_l) - 1);

			write_buffer(n_page, (page_t*)n_l, table_id);
			write_buffer(neighbor_page, (page_t*)neighbor_l, table_id);
			write_buffer(parent_page, (page_t*)parent, table_id);
			unset_pin(n_page, table_id);
			unset_pin(neighbor_page, table_id);
			unset_pin(parent_page, table_id);
		}
	}

	/* Case: n is the leftmost child.
	 * Take a key-pointer pair from the neighbor to the right.
	 * Move the neighbor's leftmost key-pointer pair
	 * to n's rightmost position.
	 */

	else {
		if (get_is_leaf(n)) {
			n_l = (leaf_page*)read_buffer(n_page, table_id,1);

			neighbor_l = (leaf_page*)read_buffer(neighbor_page, table_id,1);
			set_pin(neighbor_page, table_id);

			set_leaf_key_at_i(n_l, n_number_of_keys, get_leaf_key_at_i(neighbor_l, 0));
			set_leaf_value_at_i(n_l, n_number_of_keys, get_leaf_value_at_i(neighbor_l, 0));

			parent_page = get_parent_page_number((page_t*)n_l);
			parent = (internal_page*)read_buffer(parent_page, table_id,1);
			set_pin(parent_page, table_id);

			set_internal_key_at_i(parent, k_prime_index, get_leaf_key_at_i(neighbor_l, 1));
			for (i = 0; i < get_number_of_keys((page_t*)neighbor_l) - 1; i++) {
				set_leaf_key_at_i(neighbor_l, i, get_leaf_key_at_i(neighbor_l, i + 1));
				set_leaf_value_at_i(neighbor_l, i, get_leaf_value_at_i(neighbor_l, i + 1));
			}
			set_number_of_keys((page_t*)neighbor_l, get_number_of_keys((page_t*)neighbor_l) - 1);

			write_buffer(n_page, (page_t*)n_l, table_id);
			write_buffer(neighbor_page, (page_t*)neighbor_l, table_id);
			write_buffer(parent_page, (page_t*)parent, table_id);
			unset_pin(n_page, table_id);
			unset_pin(neighbor_page, table_id);
			unset_pin(parent_page, table_id);

		}
		else {
			n_i = (internal_page*)read_buffer(n_page, table_id,1);

			neighbor_i = (internal_page*)read_buffer(neighbor_page, table_id,1);
			set_pin(neighbor_page, table_id);

			set_internal_key_at_i(n_i, n_number_of_keys, k_prime);
			set_page_number_at_i(n_i, n_number_of_keys + 1, get_page_number_at_i(neighbor_i, 0));

			tmp_page = get_page_number_at_i(n_i, n_number_of_keys + 1);
			tmp = read_buffer(tmp_page, table_id,1);
			set_pin(tmp_page, table_id);

			set_parent_page_number(tmp, n_page);

			parent_page = get_parent_page_number((page_t*)n_l);
			parent = (internal_page*)read_buffer(parent_page, table_id,1);
			set_pin(parent_page, table_id);

			set_internal_key_at_i(parent, k_prime_index, get_internal_key_at_i(neighbor_i, 0));
			for (i = 0; i < get_number_of_keys((page_t*)neighbor_i) - 1; i++) {
				set_internal_key_at_i(neighbor_i, i, get_internal_key_at_i(neighbor_i, i + 1));
				set_page_number_at_i(neighbor_i, i, get_page_number_at_i(neighbor_i, i + 1));
			}
			set_page_number_at_i(neighbor_i, i, get_page_number_at_i(neighbor_i, i + 1));
			set_number_of_keys((page_t*)neighbor_i, get_number_of_keys((page_t*)neighbor_i) - 1);

			write_buffer(n_page, (page_t*)n_i, table_id);
			write_buffer(neighbor_page, (page_t*)neighbor_i, table_id);
			write_buffer(tmp_page, tmp, table_id);
			write_buffer(parent_page, (page_t*)parent, table_id);
			unset_pin(n_page, table_id);
			unset_pin(neighbor_page, table_id);
			unset_pin(tmp_page, table_id);
			unset_pin(parent_page, table_id);
		}

	}

	/* n now has one more key and one more pointer;
	 * the neighbor has one fewer of each.
	 */

}


/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
void delete_entry(pagenum_t n_page, int64_t key, int table_id) {

	int min_keys;
	pagenum_t neighbor_page;
	int neighbor_index;
	int k_prime_index;
	int64_t k_prime;
	int capacity;

	// Remove key and pointer from node.

	n_page = remove_entry_from_node(n_page, key, table_id);

	/* Case:  deletion from the root.
	 */
	header_page* header = new header_page;
	header = (header_page*)read_buffer(0, table_id,1);
	set_pin(0, table_id);

	if (n_page == get_root(header)) {
		unset_pin(0, table_id);
		adjust_root(n_page, table_id);
		return;
	}
	unset_pin(0, table_id);
	/* Case:  deletion from a node below the root.
	 * (Rest of function body.)
	 */

	 /* Determine minimum allowable size of node,
	  * to be preserved after deletion.
	  */

	  //min_keys = get_is_leaf(n) ? cut(leaf_order - 1) : cut(internal_order) - 1;
	min_keys = 1;// for delayed merge!

	/* Case:  node stays at or above minimum.
	 * (The simple case.)
	 */
	page_t* n = new page_t;
	n = read_buffer(n_page, table_id,1);
	set_pin(n_page, table_id);

	if (get_number_of_keys(n) >= min_keys) {
		unset_pin(n_page, table_id);
		return;
	}
	/* Case:  node falls below minimum.
	 * Either coalescence or redistribution
	 * is needed.
	 */

	 /* Find the appropriate neighbor node with which
	  * to coalesce.
	  * Also find the key (k_prime) in the parent
	  * between the pointer to node n and the pointer
	  * to the neighbor.
	  */
	pagenum_t parent_page = get_parent_page_number(n);

	internal_page* parent = new internal_page;
	parent = (internal_page*)read_buffer(parent_page, table_id,1);
	set_pin(parent_page, table_id);

	neighbor_index = get_neighbor_index(n_page, table_id);
	k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
	k_prime = get_internal_key_at_i(parent, k_prime_index);
	neighbor_page = neighbor_index == -1 ? get_page_number_at_i(parent, 1) :
		get_page_number_at_i(parent, neighbor_index);

	capacity = get_is_leaf(n) ? leaf_order : internal_order - 1;

	/* Coalescence. */

	page_t* neighbor = new page_t;
	neighbor = read_buffer(neighbor_page, table_id,1);
	set_pin(neighbor_page, table_id);

	if (get_number_of_keys(neighbor) + get_number_of_keys(n) < capacity) {
		unset_pin(n_page, table_id);
		unset_pin(parent_page, table_id);
		unset_pin(neighbor_page, table_id);
		coalesce_nodes(n_page, neighbor_page, neighbor_index, k_prime, table_id);
		return;
	}
	/* Redistribution. */

	else {
		unset_pin(n_page, table_id);
		unset_pin(parent_page, table_id);
		unset_pin(neighbor_page, table_id);
		redistribute_nodes(n_page, neighbor_page, neighbor_index, k_prime_index, k_prime, table_id);
		return;
	}
}



/* Master deletion function.
 */
int remove(int64_t key, int table_id) {

	int64_t key_leaf;
	char* key_record;

	key_record = find(key, table_id,-1);
	key_leaf = find_leaf(key, table_id,0);
	if (key_record == NULL) return -1; // fail
	if (key_record != NULL && key_leaf != 0) {
		delete_entry(key_leaf, key, table_id);
		//free(key_record);
	}

	return 0;
}

// getter && setter

pagenum_t get_root(header_page * header_p) {
	return header_p->root_p_n;
}

int32_t get_is_leaf(page_t * page) {
	return page->is_leaf;
}

int32_t get_number_of_keys(page_t * page) {
	return page->n_of_keys;
}

int64_t get_internal_key_at_i(internal_page * page, int i) {
	return page->record[i].key;
}

pagenum_t get_page_number_at_i(internal_page * page, int i) {
	if (i == 0) {
		return	page->one_more_p_n;
	}
	else {
		return page->record[i-1].p_n;
	}
}

int64_t get_leaf_key_at_i(leaf_page * page, int i) {
	return page->record[i].key;
}

char* get_leaf_value_at_i(leaf_page * page, int i) {
	return page->record[i].value;
}

pagenum_t get_right_sibling_page_number(leaf_page * page) {
	return page->right_sibling_p_n;
}

pagenum_t get_parent_page_number(page_t * page) {
	return page->parent_p_n;
}

void set_leaf_key_at_i(leaf_page * page, int i, int64_t insert_key) {
	page->record[i].key = insert_key;
}

void set_leaf_value_at_i(leaf_page * page, int i, char* insert_value) {
	strcpy(page->record[i].value, insert_value);
}

void set_number_of_keys(page_t * page, int n) {
	page->n_of_keys = n;
}

void set_is_leaf(page_t * page, int32_t n) {
	page->is_leaf = n;
}

void set_parent_page_number(page_t * child, pagenum_t parent) {
	child->parent_p_n = parent;
}

void set_right_sibling_page_number(leaf_page * page, pagenum_t next_page) {
	page->right_sibling_p_n = next_page;
}

void set_internal_key_at_i(internal_page * page, int i, int64_t input_key) {
	page->record[i].key = input_key;
}

void set_page_number_at_i(internal_page * page, int i, pagenum_t page_n) {
	if (i == 0) {
		page->one_more_p_n = page_n;
	}
	else {
		page->record[i-1].p_n = page_n;
	}
}

void set_root(header_page * header_p, pagenum_t page) {
	header_p->root_p_n = page;
}

