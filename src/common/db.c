// $Id: db.c,v 1.6 2003/06/29 05:49:37 lemit Exp $
#define MALLOC_DBN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"
#include "malloc.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define ROOT_SIZE 4096
static struct dbn *dbn_root[512], *dbn_free;
static int dbn_root_rest=0,dbn_root_num=0;

static void * malloc_dbn(void)
{
	struct dbn* ret;

	if(dbn_free==NULL){
		if(dbn_root_rest<=0){
			dbn_root[dbn_root_num]=(struct dbn *)aCalloc(ROOT_SIZE,sizeof(struct dbn));
			dbn_root_rest=ROOT_SIZE;
			dbn_root_num++;
		}
		return &(dbn_root[dbn_root_num-1][--dbn_root_rest]);
	}
	ret=dbn_free;
	dbn_free = dbn_free->parent;
	return ret;
}

static void free_dbn(struct dbn* add_dbn)
{
	add_dbn->parent = dbn_free;
	dbn_free = add_dbn;
}

void exit_dbn(void)
{
   int i;

   for (i=0;i<dbn_root_num;i++)
     if (dbn_root[i])
     	free(dbn_root[i]);
   
   dbn_root_rest=0;
   dbn_root_num=0;
   
   return;
}

static int strdb_cmp(struct dbt* table,void* a,void* b)
{
	if(table->maxlen)
		return strncmp(a,b,table->maxlen);
	return strcmp(a,b);
}

static unsigned int strdb_hash(struct dbt* table,void* a)
{
	int i;
	unsigned int h;
	unsigned char *p=a;

	i=table->maxlen;
	if(i==0) i=0x7fffffff;
	for(h=0;*p && --i>=0;){
		h=(h*33 + *p++) ^ (h>>24);
	}
	return h;
}

struct dbt* strdb_init_(int maxlen,const char *file,int line)
{
	int i;
	struct dbt* table;

	table = (struct dbt*)aCalloc(1,sizeof(struct dbt));
	table->cmp=strdb_cmp;
	table->hash=strdb_hash;
	table->maxlen=maxlen;
	for(i=0;i<HASH_SIZE;i++)
		table->ht[i]=NULL;
	table->alloc_file = file;
	table->alloc_line = line;
	table->item_count = 0;
	return table;
}

static int numdb_cmp(struct dbt* table,void* a,void* b)
{
	int ia,ib;

	ia=(int)a;
	ib=(int)b;

	if((ia^ib) & 0x80000000)
		return ia<0 ? -1 : 1;

	return ia-ib;
}

static unsigned int numdb_hash(struct dbt* table,void* a)
{
	return (unsigned int)a;
}

struct dbt* numdb_init_(const char *file,int line)
{
	int i;
	struct dbt* table;

	table = (struct dbt*)aCalloc(1,sizeof(struct dbt));
	table->cmp=numdb_cmp;
	table->hash=numdb_hash;
	table->maxlen=sizeof(int);
	for(i=0;i<HASH_SIZE;i++)
		table->ht[i]=NULL;
	table->alloc_file = file;
	table->alloc_line = line;
	table->item_count = 0;
	return table;
}

void* db_search(struct dbt *table,void* key)
{
	struct dbn *p;
	if( table == NULL ) return NULL;

	for(p=table->ht[table->hash(table,key) % HASH_SIZE];p;){
		int c=table->cmp(table,key,p->key);
		if(c==0)
			return p->data;
		if(c<0)
			p=p->left;
		else
			p=p->right;
	}
	return NULL;
}

static void db_rotate_left(struct dbn *p,struct dbn **root)
{
	struct dbn * y = p->right;
	p->right = y->left;
	if (y->left !=0)
		y->left->parent = p;
	y->parent = p->parent;

	if (p == *root)
		*root = y;
	else if (p == p->parent->left)
		p->parent->left = y;
	else
		p->parent->right = y;
	y->left = p;
	p->parent = y;
}

static void db_rotate_right(struct dbn *p,struct dbn **root)
{
	struct dbn * y = p->left;
	p->left = y->right;
	if (y->right != 0)
		y->right->parent = p;
	y->parent = p->parent;

	if (p == *root)
		*root = y;
	else if (p == p->parent->right)
		p->parent->right = y;
	else
		p->parent->left = y;
	y->right = p;
	p->parent = y;
}

static void db_rebalance(struct dbn *p,struct dbn **root)
{
	p->color = RED;
	while(p!=*root && p->parent->color==RED){ // rootは必ず黒で親は赤いので親の親は必ず存在する
		if (p->parent == p->parent->parent->left) {
			struct dbn *y = p->parent->parent->right;
			if (y && y->color == RED) {
				p->parent->color = BLACK;
				y->color = BLACK;
				p->parent->parent->color = RED;
				p = p->parent->parent;
			} else {
				if (p == p->parent->right) {
					p = p->parent;
					db_rotate_left(p, root);
				}
				p->parent->color = BLACK;
				p->parent->parent->color = RED;
				db_rotate_right(p->parent->parent, root);
			}
		} else {
			struct dbn* y = p->parent->parent->left;
			if (y && y->color == RED) {
				p->parent->color = BLACK;
				y->color = BLACK;
				p->parent->parent->color = RED;
				p = p->parent->parent;
			} else {
				if (p == p->parent->left) {
					p = p->parent;
					db_rotate_right(p, root);
				}
				p->parent->color = BLACK;
				p->parent->parent->color = RED;
				db_rotate_left(p->parent->parent, root);
			}
		}
	}
	(*root)->color=BLACK;
}

static void db_rebalance_erase(struct dbn *z,struct dbn **root)
{
	struct dbn *y = z, *x = NULL, *x_parent = NULL;

	if (y->left == NULL)
		x = y->right;
	else if (y->right == NULL)
		x = y->left;
	else {
		y = y->right;
		while (y->left != NULL)
			y = y->left;
		x = y->right;
	}
	if (y != z) { // 左右が両方埋まっていた時 yをzの位置に持ってきてzを浮かせる
		z->left->parent = y;
		y->left = z->left;
		if (y != z->right) {
			x_parent = y->parent;
			if (x) x->parent = y->parent;
			y->parent->left = x;
			y->right = z->right;
			z->right->parent = y;
		} else
			x_parent = y;
		if (*root == z)
			*root = y;
		else if (z->parent->left == z)
			z->parent->left = y;
		else
			z->parent->right = y;
		y->parent = z->parent;
		{ int tmp=y->color; y->color=z->color; z->color=tmp; }
		y = z;
	} else { // どちらか空いていた場合 xをzの位置に持ってきてzを浮かせる
		x_parent = y->parent;
		if (x) x->parent = y->parent;
		if (*root == z)
			*root = x;
		else if (z->parent->left == z)
			z->parent->left = x;
		else
			z->parent->right = x;
	}
	// ここまで色の移動の除いて通常の2分木と同じ
	if (y->color != RED) { // 赤が消える分には影響無し
		while (x != *root && (x == NULL || x->color == BLACK))
			if (x == x_parent->left) {
				struct dbn* w = x_parent->right;
				if (w->color == RED) {
					w->color = BLACK;
					x_parent->color = RED;
					db_rotate_left(x_parent, root);
					w = x_parent->right;
				}
				if ((w->left == NULL ||
					 w->left->color == BLACK) &&
					(w->right == NULL ||
					 w->right->color == BLACK)) {
					w->color = RED;
					x = x_parent;
					x_parent = x_parent->parent;
				} else {
					if (w->right == NULL ||
						w->right->color == BLACK) {
						if (w->left) w->left->color = BLACK;
						w->color = RED;
						db_rotate_right(w, root);
						w = x_parent->right;
					}
					w->color = x_parent->color;
					x_parent->color = BLACK;
					if (w->right) w->right->color = BLACK;
					db_rotate_left(x_parent, root);
					break;
				}
			} else {                  // same as above, with right <-> left.
				struct dbn* w = x_parent->left;
				if (w->color == RED) {
					w->color = BLACK;
					x_parent->color = RED;
					db_rotate_right(x_parent, root);
					w = x_parent->left;
				}
				if ((w->right == NULL ||
					 w->right->color == BLACK) &&
					(w->left == NULL ||
					 w->left->color == BLACK)) {
					w->color = RED;
					x = x_parent;
					x_parent = x_parent->parent;
				} else {
					if (w->left == NULL ||
						w->left->color == BLACK) {
						if (w->right) w->right->color = BLACK;
						w->color = RED;
						db_rotate_left(w, root);
						w = x_parent->left;
					}
					w->color = x_parent->color;
					x_parent->color = BLACK;
					if (w->left) w->left->color = BLACK;
					db_rotate_right(x_parent, root);
					break;
				}
			}
		if (x) x->color = BLACK;
	}
}

void db_free_lock(struct dbt *table) {
	table->free_lock++;
}

void db_free_unlock(struct dbt *table) {
	if(--table->free_lock == 0) {
		int i;
		for(i = 0; i < table->free_count ; i++) {
			db_rebalance_erase(table->free_list[i].z,table->free_list[i].root);
			if(table->cmp == strdb_cmp) {
				free(table->free_list[i].z->key);
			}
#ifdef MALLOC_DBN
			free_dbn(table->free_list[i].z);
#else
			free(table->free_list[i].z);
#endif
			table->item_count--;
		}
		table->free_count = 0;
	}
}

struct dbn* db_insert(struct dbt *table,void* key,void* data)
{
	struct dbn *p,*priv;
	int c,hash;

	hash = table->hash(table,key) % HASH_SIZE;
	for(c=0,priv=NULL ,p = table->ht[hash];p;){
		c=table->cmp(table,key,p->key);
		if(c==0){ // replace
			if(p->deleted) {
				// 削除されたデータなので、free_list 上の削除予定を消す
				int i;
				for(i = 0; i < table->free_count ; i++) {
					if(table->free_list[i].z == p) {
						memmove(
							&table->free_list[i],
							&table->free_list[i+1],
							sizeof(struct db_free)*(table->free_count - i - 1)
						);
						break;
					}
				}
				if(i == table->free_count || table->free_count <= 0) {
					printf("db_insert: cannnot find deleted db node.\n");
				} else {
					table->free_count--;
					if(table->cmp == strdb_cmp) {
						free(p->key);
					}
				}
			}
			p->key     = key;
			p->data    = data;
			p->deleted = 0;
			return p;
		}
		priv=p;
		if(c<0){
			p=p->left;
		} else {
			p=p->right;
		}
	}
#ifdef MALLOC_DBN
	p=malloc_dbn();
#else
	p=(struct dbn *)aCalloc(1,sizeof(struct dbn));
#endif
	p->parent= NULL;
	p->left  = NULL;
	p->right = NULL;
	p->key   = key;
	p->data  = data;
	p->color = RED;
	p->deleted = 0;
	if(c==0){ // hash entry is empty
		table->ht[hash] = p;
		p->color = BLACK;
	} else {
		if(c<0){ // left node
			priv->left = p;
			p->parent=priv;
		} else { // right node
			priv->right = p;
			p->parent=priv;
		}
		if(priv->color==RED){ // must rebalance
			db_rebalance(p,&table->ht[hash]);
		}
	}
	table->item_count++;
	return p;
}

void* db_erase(struct dbt *table,void* key)
{
	void *data;
	struct dbn *p;
	int c,hash;

	hash = table->hash(table,key) % HASH_SIZE;
	for(c=0,p = table->ht[hash];p;){
		c=table->cmp(table,key,p->key);
		if(c==0)
			break;
		if(c<0)
			p=p->left;
		else
			p=p->right;
	}
	if(!p || p->deleted)
		return NULL;
	data=p->data;
	if(table->free_lock) {
		if(table->free_count == table->free_max) {
			table->free_max += 32;
			table->free_list = (struct db_free*)realloc(table->free_list,sizeof(struct db_free) * table->free_max);
		}
		table->free_list[table->free_count].z    = p;
		table->free_list[table->free_count].root = &table->ht[hash];
		table->free_count++;
		p->deleted = 1;
		p->data    = NULL;
		if(table->cmp == strdb_cmp) {
			if(table->maxlen) {
				char *key = (char*)malloc(table->maxlen);
				memcpy(key,p->key,table->maxlen);
				p->key = key;
			} else {
				p->key = strdup((const char*)p->key);
			}
		}
	} else {
		db_rebalance_erase(p,&table->ht[hash]);
#ifdef MALLOC_DBN
		free_dbn(p);
#else
		free(p);
#endif
		table->item_count--;
	}
	return data;
}

void db_foreach(struct dbt *table,int (*func)(void*,void*,va_list),...)
{
	va_list ap;

	va_start(ap, func);
	db_foreach_sub(table, func, ap);
	va_end(ap);
}

void db_foreach_sub(struct dbt* table,int(*func)(void*,void*,va_list), va_list ap)
{
	int i,sp;
	int count;
	// red-black treeなので64個stackがあれば2^32個ノードまで大丈夫
	struct dbn *p,*pn,*stack[64];
	if( table == NULL ) return ;
	count = table->item_count;

	db_free_lock(table);
	for(i=0;i<HASH_SIZE;i++){
		if((p=table->ht[i])==NULL)
			continue;
		sp=0;
		while(1){
			if(!p->deleted) {
				func(p->key,p->data,ap);
			}
			count--;
			if((pn=p->left)!=NULL){
				if(p->right){
					stack[sp++]=p->right;
				}
				p=pn;
			} else {
				if(p->right){
					p=p->right;
				} else {
					if(sp==0)
						break;
					p=stack[--sp];
				}
			}
		}
	}
	db_free_unlock(table);
	if(count) {
		printf(
			"db_foreach : data lost %d item(s) allocated from %s line %d\n",
			count,table->alloc_file,table->alloc_line
		);
	}
	// else {
	// 	printf("db_foreach : ok\n");
	// }
}

void db_final(struct dbt *table,int (*func)(void*,void*,va_list),...)
{
	int i;
	struct dbn *p;
	va_list ap;

	if( table == NULL ) return ;
	va_start(ap,func);
	for(i=0;i<HASH_SIZE;i++){
		while( ( p = table->ht[i] ) ) {
			while( p->left || p->right ) {
				p = (p->right ? p->right : p->left);
			}
			if( !p->parent ) {
				table->ht[i] = NULL;
			} else if( p->parent->left == p ) {
				p->parent->left  = NULL;
			} else {
				p->parent->right = NULL;
			}
			if( func ) 
				func( p->key, p->data, ap );
#ifdef MALLOC_DBN
			free_dbn(p);
#else
			aFree(p);
#endif
		}
	}
	free(table->free_list);
	free(table);
	va_end(ap);
}

void  linkdb_insert( struct linkdb_node** head, void *key, void* data) {
	struct linkdb_node *node;
	if( head == NULL ) return ;
	node = aMalloc( sizeof(struct linkdb_node) );
	if( *head == NULL ) {
		// first node
		*head      = node;
		node->prev = NULL;
		node->next = NULL;
	} else {
		// link nodes
		node->next    = *head;
		node->prev    = (*head)->prev;
		(*head)->prev = node;
		(*head)       = node;
	}
	node->key  = key;
	node->data = data;
}

void* linkdb_search( struct linkdb_node** head, void *key) {
	int n = 0;
	struct linkdb_node *node;
	if( head == NULL ) return NULL;
	node = *head;
	while( node ) {
		if( node->key == key ) {
			if( node->prev && n > 5 ) {
				// 処理効率改善の為にheadに移動させる
				if(node->prev) node->prev->next = node->next;
				if(node->next) node->next->prev = node->prev;
				node->next = *head;
				node->prev = (*head)->prev;
				(*head)->prev = node;
				(*head)       = node;
			}
			return node->data;
		}
		node = node->next;
		n++;
	}
	return NULL;
}

void* linkdb_erase( struct linkdb_node** head, void *key) {
	struct linkdb_node *node;
	if( head == NULL ) return NULL;
	node = *head;
	while( node ) {
		if( node->key == key ) {
			void *data = node->data;
			if( node->prev == NULL )
				*head = node->next;
			else
				node->prev->next = node->next;
			if( node->next )
				node->next->prev = node->prev;
			aFree( node );
			return data;
		}
		node = node->next;
	}
	return NULL;
}

void linkdb_replace( struct linkdb_node** head, void *key, void *data ) {
	int n = 0;
	struct linkdb_node *node;
	if( head == NULL ) return ;
	node = *head;
	while( node ) {
		if( node->key == key ) {
			if( node->prev && n > 5 ) {
				// 処理効率改善の為にheadに移動させる
				if(node->prev) node->prev->next = node->next;
				if(node->next) node->next->prev = node->prev;
				node->next = *head;
				node->prev = (*head)->prev;
				(*head)->prev = node;
				(*head)       = node;
			}
			node->data = data;
			return ;
		}
		node = node->next;
		n++;
	}
	// 見つからないので挿入
	linkdb_insert( head, key, data );
}

void  linkdb_final( struct linkdb_node** head ) {
	struct linkdb_node *node, *node2;
	if( head == NULL ) return ;
	node = *head;
	while( node ) {
		node2 = node->next;
		aFree( node );
		node = node2;
	}
	*head = NULL;
}

#ifndef NO_CSVDB
// csvdb -- csv のデータ読み込み関数

// csv の読み込み。skip_comment が真のときは、行頭に//がある行を読み飛ばす。
struct csvdb_data* csvdb_open(const char* file, int skip_comment) {
	int  i;
	char buf[8192];
	FILE *fp = fopen(file, "r");
	struct csvdb_data *csv = (struct csvdb_data*)calloc( sizeof(struct csvdb_data), 1);
	csv->file = strdup( file );
	if(fp == NULL) {
		return csv;
	}

	while( fgets( buf, sizeof(buf)-1, fp) ) {
		int max = 0;
		char *s;
		struct csvdb_line *line;
		if( buf[0] == '\n' || !buf[0] )                      continue; // 空行
		if( skip_comment && buf[0] == '/' && buf[1] == '/' ) continue; // コメント
		if( csv->row_count == csv->row_max ) {
			csv->row_max += 64;
			csv->data  = (struct csvdb_line*)realloc(
				csv->data, sizeof(struct csvdb_line) * csv->row_max );
			csv->index = (int*)realloc(
				csv->index, sizeof(int) * csv->row_max );
		}
		max = (int)strlen(buf);
		if( buf[max-1] == '\n' ) buf[--max] = 0;

		// lineにコピーし、行末にNUL を２つ続けさせる
		buf[max  ] = ' '; // ダミー文字
		buf[max+1] = 0;
		line = &csv->data[csv->row_count++];
		line->buf = strdup( buf );
		line->num = 0;
		line->buf[max  ] = 0;
		line->buf[max+1] = 0;
		for(s = line->buf; s[0] && line->num < MAX_CSVCOL; ) {
			if( *s == '"' ) {
				// [""] を ["] に置き換え、[",] を終了の合図とする。
				// 行末にNUL が２つ続くので、k[n] がNUL のとき
				// k[n+1]を参照しても安全。
				char *j = ++s, *k = s;
				while( k[0] ) {
					if( k[0] == '"' ) {
						if( k[1] == ',' || k[1] == 0) break;
						*(j++) = (*k++); k++;
					} else {
						*(j++) = (*k++);
					}
				}
				*j = 0;
				line->data_p[line->num] = s;
				line->data_v[line->num] = strtol(s, NULL, 0);
				line->num++;
				if( k[1] == 0 ) break;
				s = k + 2;
			} else {
				char *p = strchr( s, ',');
				if( p ) *p = 0;
				line->data_p[line->num] = s;
				line->data_v[line->num] = strtol(s, NULL, 0);
				s = p + 1;
				line->num++;
				if( !p ) break;
			}
		}
	}
	fclose(fp);
	for(i = 0; i < csv->row_count; i++) {
		csv->index[i] = i;
	}
	csv->row_notempty = csv->row_count;
	return csv;
}

// 行数を返す
int csvdb_get_rows(struct csvdb_data *csv) {
	return ( csv == NULL ? -1 : csv->row_notempty );
}

// 指定した行の列数を返す
int csvdb_get_columns(struct csvdb_data *csv, int row) {
	return ( csv == NULL || row < 0 || csv->row_notempty <= row ? -1 : csv->data[csv->index[row]].num);
}

// 指定した行、列のデータを整数にして返す
int csvdb_get_num(struct csvdb_data *csv, int row, int col) {
	if( csv == NULL || row < 0 || csv->row_notempty <= row || col < 0 || csv->data[csv->index[row]].num <= col )
		return -1;
	else
		return csv->data[csv->index[row]].data_v[col];
}

// 指定した行、列のデータへのポインタを返す
const char* csvdb_get_str(struct csvdb_data *csv, int row, int col) {
	if( csv == NULL || row < 0 || csv->row_notempty <= row || col < 0 || csv->data[csv->index[row]].num <= col )
		return NULL;
	else
		return csv->data[csv->index[row]].data_p[col];
}

// 先頭の行にある数字がvalue と一致する行を返す
int csvdb_find_num(struct csvdb_data *csv, int col, int value) {
	int i;
	if( csv == NULL || col < 0 || col >= MAX_CSVCOL) return -1;
	for(i = 0; i < csv->row_notempty; i++) {
		struct csvdb_line *line = &csv->data[csv->index[i]];
		if( line->num > col && line->data_v[col] == value ) {
			return i;
		}
	}
	return -1;
}

// 先頭の行にある文字がvalue と一致する行を返す
int csvdb_find_str(struct csvdb_data *csv, int col, const char* value) {
	int i;
	if( csv == NULL || col < 0 || col >= MAX_CSVCOL) return -1;
	for(i = 0; i < csv->row_notempty; i++) {
		struct csvdb_line *line = &csv->data[csv->index[i]];
		if( line->num > col && !strcmp(line->data_p[col], value) ) {
			return i;
		}
	}
	return -1;
}

// csv のリサイズ(ただし、列数は無視される)
int csvdb_resize(struct csvdb_data *csv, int row, int col) {
	int i;
	if( csv == NULL || col < 0 || row < 0 || col >= MAX_CSVCOL) return 0;
	// 行の拡張
	if( csv->row_count < row ) {
		while( csv->row_max <= row ) {
			csv->row_max += 32;
			csv->data = (struct csvdb_line*)realloc(
				csv->data, sizeof(struct csvdb_data) * csv->row_max
			);
			csv->index = (int*)realloc(
				csv->index, sizeof(int) * csv->row_max
			);
		}
		for(i = csv->row_count; i < row; i++) {
			csv->data[i].num = 0;
			csv->data[i].buf = strdup("");
			csv->index[i]    = i;
		}
		csv->row_count = row;
	}
	return 1;
}

// 指定した行、列のデータを設定する
int csvdb_set_num(struct csvdb_data *csv, int row, int col, int num) {
	char buf[256];
	sprintf(buf, "%d", num);
	return csvdb_set_str(csv, row, col, buf);
}

// 指定した行、列のデータを設定する
int csvdb_set_str(struct csvdb_data *csv, int row, int col, const char* str) {
	int i;
	struct csvdb_line *line;
	if( csv == NULL || col < 0 || col >= MAX_CSVCOL || row < 0) return 0;
	if( csv->row_notempty <= row ) {
		if(!csvdb_resize(csv, row + 1, col + 1)) return 0;
		csv->row_notempty = row + 1;
	}
	line = &csv->data[ csv->index[row] ];
	if( line->buf != NULL ) {
		for( i = 0; i < line->num; i++) {
			line->data_p[i] = strdup( line->data_p[i] );
		}
		free( line->buf );
		line->buf = NULL;
	}
	for(i = line->num; i <= col; i++) {
		line->data_p[i] = strdup("");
		line->data_v[i] = 0;
	}
	csv->dirty = 1;
	if(line->num < col + 1)
		line->num = col + 1;
	free(line->data_p[col]);
	line->data_p[col] = strdup(str);
	line->data_v[col] = strtol( str, NULL, 0);
	return 1;
}

// 指定した行、列のデータをクリアする
int csvdb_clear_row(struct csvdb_data *csv, int row) {
	int i;
	struct csvdb_line *line;
	if( csv == NULL || row < 0 || csv->row_notempty <= row ) return 0;
	line = &csv->data[ csv->index[ row ] ];
	if( line->buf == NULL ) {
		for(i = 0; i < line->num; i++) {
			free( line->data_p[i] );
		}
	} else {
		free( line->buf );
	}
	line->buf = strdup("");
	line->num = 0;
	csv->dirty = 1;
	return 1;
}

// 並び替え関数群
static int    csvdb_sort_key = 0;
static struct csvdb_line * csvdb_sort_data = NULL;

static int csvdb_sort_asc(const void *a, const void *b) {
	const struct csvdb_line *p1 = &csvdb_sort_data[*(int*)a];
	const struct csvdb_line *p2 = &csvdb_sort_data[*(int*)b];
	int v1 = (p1->num < csvdb_sort_key ? 0 : p1->data_v[csvdb_sort_key]);
	int v2 = (p2->num < csvdb_sort_key ? 0 : p2->data_v[csvdb_sort_key]);
	return  v1 - v2;
}

static int csvdb_sort_desc(const void *a, const void *b) {
	const struct csvdb_line *p1 = &csvdb_sort_data[*(int*)a];
	const struct csvdb_line *p2 = &csvdb_sort_data[*(int*)b];
	int v1 = (p1->num < csvdb_sort_key ? 0 : p1->data_v[csvdb_sort_key]);
	int v2 = (p2->num < csvdb_sort_key ? 0 : p2->data_v[csvdb_sort_key]);
	return  v2 - v1;
}

int csvdb_sort(struct csvdb_data *csv, int key, int order) {
	if(csv == NULL || key < 0) return 0;
	csvdb_sort_key  = key;
	csvdb_sort_data = csv->data;
	if( order != -1 ) {
		// 昇順
		qsort( csv->index, csv->row_notempty, sizeof(int), csvdb_sort_asc);
	} else {
		// 降順
		qsort( csv->index, csv->row_notempty, sizeof(int), csvdb_sort_desc);
	}
	csv->dirty = 1;
	return 1;
}

// 指定した行を削除する
int csvdb_delete_row(struct csvdb_data *csv, int row) {
	int i; 
	if(csv == NULL || row < 0 || csv->row_notempty <= row) return 0;
	csvdb_clear_row(csv,row);
	i = csv->index[row];
	memmove(&csv->index[row], &csv->index[row+1], sizeof(int)*(csv->row_count-row-1));
	csv->index[csv->row_count-1] = i;
	csv->row_notempty--;
	csv->dirty = 1;
	return 1;
}

// 行を挿入する
int csvdb_insert_row(struct csvdb_data *csv, int row) {
	int i;
	if( csv == NULL || row < 0 || csv->row_notempty <= row) return 0;
	csvdb_resize(csv, csv->row_notempty + 1, 0);
	i = csv->index[csv->row_count-1];
	memmove(&csv->index[row+1],&csv->index[row],sizeof(int)*(csv->row_count-row));
	csv->index[row] = i;
	csv->row_notempty++;
	csv->dirty = 1;
	return 1;
}

// ファイルに書き出す
int csvdb_flush(struct csvdb_data *csv) {
	int  i, j;
	FILE *fp;
	if( csv == NULL ) return 0;
	if( !csv->dirty ) return 1; // 更新されてなければ何もしない
	fp = fopen( csv->file, "w" );
	if( !fp ) return 0;

	for(i = 0; i < csv->row_notempty; i++) {
		struct csvdb_line *line = &csv->data[csv->index[i]];
		for( j = 0; j < line->num; j++) {
			if( strchr( line->data_p[j], '\"' ) || strchr( line->data_p[j], ',' ) ) {
				char *p = line->data_p[j];
				fprintf(fp, "\"");
				for( ; *p; p++) {
					if( *p == '\"' ) {
						fprintf(fp, "\"\"");
					} else {
						fprintf(fp, "%c", *p);
					}
				}
				fprintf(fp, "\",");
			} else {
				fprintf(fp, "%s,", line->data_p[j]);
			}
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
	csv->dirty = 0;

	return 1;
}

// メモリを解放する
void csvdb_close(struct csvdb_data *csv) {
	int i, j;
	if( csv == NULL ) return;

	if( csv->dirty ) {
		csvdb_flush( csv );
	}

	for( i = 0; i < csv->row_count; i++) {
		if( csv->data[i].buf == NULL ) {
			struct csvdb_line *line = &csv->data[i];
			for(j = 0; j < line->num; j++) {
				free( line->data_p[j] );
			}
		} else {
			free( csv->data[i].buf );
		}
	}
	free( csv->file );
	free( csv->data );
	free( csv->index );
	free( csv );
}

// デバッグ用
void csvdb_dump(struct csvdb_data* csv) {
	int i, j;
	struct csvdb_line *line;
	if( csv == NULL ) return;
	printf("csvdb_dump: index\n");
	for(i = 0; i < csv->row_notempty; i++) {
		printf("% 3d", csv->index[i]);
	}
	printf("[");
	for( ; i < csv->row_count; i++) {
		printf("% 3d", csv->index[i]);
	}
	printf("]\n");

	for(i = 0; i < csv->row_count; i++) {
		printf("line% 4d : ", i);
		line = &csv->data[csv->index[i]];
		for( j = 0; j < line->num; j++) {
			printf("[%s],", line->data_p[j]);
		}
		printf("\n");
	}
}
#endif
