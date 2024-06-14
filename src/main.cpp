#include "bpt.h"
#include "file.h"
#include "dbapi.h"
#include "buffer.h"

// MAIN

int main( int argc, char ** argv ) {

    int input;
    char instruction;
    char value[120];
    char pathname[120]; // test
    int unique_id;
    //usage_1();  
    //usage_2();
    int tableid;
    int buf_num;

    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
	case 'o':
	    cin >> pathname;
	    unique_id = open_table(pathname);
	    printf("open success! table_id is %d\n", unique_id);
	    break;
        case 'd':
            scanf("%d", &input);
            scanf("%d", &tableid);
            db_delete(tableid,input);
            print_tree(tableid);
            break;
        case 'i':
            scanf("%d", &input);
	    scanf("%s", value);
            scanf("%d", &tableid);
            db_insert(tableid,input, value);
            print_tree(tableid);
            break;
        case 'f':
	    scanf("%d", &input);
            scanf("%d", &tableid);
	    if(db_find(tableid,input,value,0) == 0) printf("Key : %d, Value : %s\n", input,value);
	    else printf("Not Found!\n");
	    break;
        case 'l':
            scanf("%d", &tableid);
            print_leaves(tableid);
            break;
        case 'q':
            while (getchar() != (int)'\n');
	    shutdown_db();
            return 0;
            break;
	case 'n':
	    scanf("%d", &buf_num);
	    //init_db(buf_num);
	    printf("init success <buf_num : %d>\n", buf_num);
	    break;
        case 't':
            scanf("%d", &tableid);
            print_tree(tableid);
            break;
	case 'c':
            scanf("%d", &tableid);
	    close_table(tableid);
	    printf("close success <table_id : %d>\n", tableid);
	    break;
	case 'r':
	    scanf("%d", &tableid);
            print_tree(tableid);
	    printf("flag : %d\n",get_header_number_of_pages(tableid));
	    break;
        default:
            break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");

    return 0;
}

