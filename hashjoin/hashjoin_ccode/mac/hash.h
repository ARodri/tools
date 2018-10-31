/*===========================================================================*/
/* @(#) hash.h   Utility Version3.15   1/24/02 11:41:43 */
/*===========================================================================*/
/*===========================================================================*/
/* hash.h : hash table object header file.                                   */
/* include data type definitions and public function protocols.              */
/*                                                                           */
/* Sun Dec 15 20:14:56 MST 1996, Jincai, create. 1st version.                */
/* Sat May 10 05:36:10 MDT 1997, Jincai, modify. 2nd version.                */
/* Wed Feb 11 19:28:27 MST 1998, Jincai, modify. 3rd version.                */
/* Sat Aug  1 22:29:01 CDT 1998, Jincai, modify. remove double hash          */
/*===========================================================================*/
#ifndef  _HASH_H
#define  _HASH_H
#ifdef __cplusplus
extern "C" {
#endif

#ifdef   _THREAD_SAFE
#include <pthread.h>
#endif
#include <stdio.h>
/*LINTLIBRARY*/

/*===========================================================================*/
/* constants and macros for common use.                                      */
/*===========================================================================*/
#define	RACK_STORAGE_RATIO	8	/* allocated size per data entry. */
#define RACK_PURGING_RATIO	25	/* purge when 25% of cells deleted */
#define RACK_INCSIZE_BOUND	67108864	/* 64MB */
#ifndef	strsize
#define strsize(s)		((int)(strlen(s)+1))
#endif
#ifndef	IncSize
#define IncSize(s)		((s)+(s)/(1+(s)/RACK_INCSIZE_BOUND))
#endif
				/*         1 : BOUND     -> size * 2     */
				/*     BOUND : 2 * BOUND -> size * 1.5   */
				/* 2 * BOUND : 3 * BOUND -> size * 1.33  */
				/* 3 * BOUND : 4 * BOUND -> size * 1.25  */

/*===========================================================================*/
/* Rack data structure, object for internal byte-data storage.               */
/* it is always programed to be a subclass of a container so that handles    */
/* and sizes returned by this object can be organized and data be retrieved. */
/*===========================================================================*/
typedef char *HashData;		/* try to make keys and data opague */
typedef struct
{
	size_t		size;		/* space allocated in byte */
	int		fpos;		/* free space position */
	char		*base;		/* space starting address */
}	_Rack, *Rack;

/*===========================================================================*/
/* for creation operation. for operations returning Rack, return NULL if the */
/* operation is failed. for operations returning int, return <= 0 if the     */
/* operation is failed.                                                      */
/*===========================================================================*/
Rack rack_setup(size_t size);		/* create a new rack */
Rack rack_copy(Rack r);			/* duplicate exactly a rack */
int  rack_clean(Rack r);		/* delete a rack */
int  rack_expand(Rack r, int addon);	/* expand r to at least "addon" bytes */
int  rack_save(Rack r, FILE *fp);	/* return size of structure written */
Rack rack_load(FILE *fp);		/* create and read file */

/*===========================================================================*/
/* for purging rack, start with call rack_move to create a new empty one.    */
/* followed by calling rack_reon, rack_reput, and reack_rehook until the     */
/* purging operation is done. the rack_move return NULL if failed. the other */
/* function return a new handle or return -1 if failed. rack_rehook will     */
/* update *hook for being passed in to hook next piece of data.              */
/*===========================================================================*/
Rack rack_move(Rack r);
int  rack_reon(Rack t, Rack r, int handle, int size);
int  rack_reput(Rack t, Rack r, int handle);
int  rack_rehook(Rack t, Rack r, int handle, int *hook);

/*===========================================================================*/
/* for inserting data. return handle for later retrieving, or 0 if the       */
/* insertion is failed. r must not be NULL, data must not be NULL size must  */
/* be greater than or equal to 0. for rack_hook, if *hook < 0, start a new   */
/* chain and return new handle and update the *hook. otherwise *hook will    */
/* be used as hook and be updated and return a positive number.              */
/*===========================================================================*/
int  rack_on(Rack r, const char *data, const int size);
int  rack_put(Rack r, char *data, int size);
int  rack_hook(Rack r, int *hook, char *data, int size);

/*===========================================================================*/
/* for retrieving data. _ptr function retrives pointer while other functions */
/* retrieve data to the input buffer passed in. for functions returning int  */
/* the returned value is the size or -1 if failed. for functions returning   */
/* char *, the returned value is the data pointer or NULL if failed.         */
/* rack_off must have proper sized buffer. others must have proper-sized     */
/* buffer or NULL as char *data argument.                                    */
/* if functions pass in int *handle the handle can be used for next call.    */
/* rack_size --- returns size of a particular hook/put, not the overall size */
/* rack_pick_all --- return all data in a chain. not allocating space.       */
/* rack_counts --- returns number of hooks in a particular chain             */
/* rack_unhook --- returns new handle, hook is updated if unhook the last    */
/*                 link. nth = 1 .. num_of_hooks. if nth = 0 or              */
/*                 nth > num_of_hooks means to unhook the last link.         */
/*===========================================================================*/
int  rack_off(Rack r, int handle, char *data, int size);
char *rack_off_ptr(Rack r, int handle);
int  rack_get(Rack r, int handle, char *data);
int  rack_get_ptr(Rack r, int handle, char **datap);
int  rack_pick(Rack r, int *handle, char *data);
int  rack_pick_ptr(Rack r, int *handle, char **datap);
int  rack_pick_all(Rack r, int handle, char **data, int *sizes);
int  rack_pick_all_ptr(Rack r, int handle, char **datap, int *sizes);
int  rack_size(Rack r, int handle);
int  rack_counts(Rack r, int handle);
int  rack_unhook(Rack r, int handle, int *hook, int nth);

/*===========================================================================*/
/* generic sorting definition. may be shared by list etc.                    */
/*===========================================================================*/
typedef struct				/* sorting structure */
{
	char		*object;	/* object, in this case is Hash *ht */
	char		*key;
	char		*data;
	unsigned int	position;	/* index to the position in table */
	unsigned int	pindex;		/* position index accessed by index */
	unsigned int	index;
	unsigned int	counts;
	unsigned int	datasize;
	unsigned short	keysize;
	unsigned short	ndata;
}	SortEntry;

typedef enum
{
	Sort_ran,	/* random, not sorted */
	Sort_asc,
	Sort_des
}	SortOrder;

typedef enum
{
	Sort_normal,			/* hash table state */
	Sort_index,			/* for traverse through index order */
	Sort_key_counts,		/* #insertions of a key */
	Sort_key_string,		/* keys are strings */
	Sort_key_integer,		/* keys are integers (sizeof int) */
	Sort_key_float,			/* keys are floats (sizeof(float) */
	Sort_key_double,		/* keys are doubles (sizeof(double) */
	Sort_key_strint,		/* keys are sprintf'ed integer */
	Sort_key_strflt,
	Sort_key_strdbl,
	Sort_data_counts,		/* #data, equal ot key_counts if */
					/* there are all insert_data */
	Sort_data_string,
	Sort_data_integer,
	Sort_data_float,
	Sort_data_double,
	Sort_data_strint,
	Sort_data_strflt,
	Sort_data_strdbl,
	Sort_comparator			/* sort on user-provided comparator */
					/* see example in end of this file */
}	SortState;
typedef int (*SortComp)(SortEntry *a, SortEntry *b);
typedef int (*HashProc)(SortEntry *e, size_t stp, int nst, void *in, void *out);

/*===========================================================================*/
/* Hash data structure.                                                      */
/* it has following features:                                                */
/*     indexing field for the most common use of hash table.                 */
/*     sorted list conversion. automatic binary search.                      */
/*     built-in simple linked-list structure for data.                       */
/*===========================================================================*/
/*====================================================*/
/* note that the status of a cell are:                */
/*                                                    */
/*   counts keysize status     insert     search      */
/* ------------------------------------------------   */
/*    = 0    = 0    empty       stop      stop        */
/*    = 0    > 0    deleted     stop      continue    */
/*    > 0    >= 0   occupied   continue   continue    */
/*====================================================*/
typedef struct
{
	unsigned int	counts;		/* count = 0, deleted/empty cell */
	unsigned int	keysize;	/* keysize = 0, empty cell */
	unsigned int	pindex;		/* position index */
	unsigned int	index;		/* index of when it is in */
	int		khandle;	/* key handle */
	int		dhandle;	/* data handle */
	int		dhook;		/* data hook */
}	HashCell;

typedef struct
{
	/* hash table size and mask data */
	size_t		ment;		/* max number of entries */
	size_t		nent;		/* current number of entries */
	size_t		pent;		/* permittable number of entries */
	size_t		lent;		/* last time number of records */
	unsigned char	dens;		/* percentage of occupied cell */
	unsigned char	byte;		/* number of bytes for xkey */
	unsigned char	nbyt;		/* sizeof(int) - byte */
	unsigned char	nref;		/* reference counter */
	/* hash table data structure */
	HashCell	*cell;		/* hash_cells */
	Rack		rack;		/* translate key and put position */
	/* hash table maintenence structure */
	char		slck;		/* soft lock for traversing */
	int		ndel;		/* number deletions */
	int		mdel;		/* maximum #deletions before purge */
	unsigned int	ginx;		/* global index */
	unsigned int	trav;		/* traverser position */
	int		cpck;		/* entry traverser */
	/* for sorting etc */
	SortState	stat;		/* indicating how is sorted */
	SortOrder	sord;		/* sorting order. */
	SortEntry	*sent;		/* sorting entries. */
	SortComp	scmp;		/* sorting comparator. */
}	_Hash, *Hash;

/*===========================================================================*/
/* creation/deletion operation.                                              */
/* return either Hashtable=success / NULL=failure, or 1=success / 0=failure  */
/*===========================================================================*/
/* allocate space for a hash table and return the points. */
/* size is the initial size. density is maximum table density in density% */
/* if size=0, size=13. if density=0, density=75 (max 75% full) */
Hash hash_create(const int size, const int density);

/* create and read a complete hash table from an open file pointer fp */
/* which is written by hash_write() */
Hash hash_read(FILE *fp);

/* write the complete hash table to an open file which */
/* can be read later by hash_read() */
int  hash_write(Hash ht, FILE *fp);

/* create and read a hash table, string key and index only, */
/* from an open file pointer */
/* assume the format is string_key<delm>integer_index\n */
Hash hash_load(const char delm, FILE *fp);

/* save hash table string key and index pair into an open file */
/* in the format string_key<delm>integer_index\n */
int  hash_save(Hash ht, const char delm, FILE *fp);

/* increment reference counter of ht and return ht itself */
/* or NULL if counter reaches maximum value */
Hash hash_share(Hash ht);	/* increment reference counter of ht. */

/* clone a hash table and return new hash table */
/* or return NULL if something is wrong */
Hash hash_copy(Hash ht);

/* decrement hash table reference counter. if the counter reaches 0 */
/* destroy it and reclaim all the spaces. */
int  hash_delete(Hash ht);

/* re-claim some spaces after may deletion operation */
int  hash_purge(Hash ht);

/* replace index with position of the cell (position increase by 1). */
/* if map is not NULL (must pre-allocated), map[old_index] = new_index */
/* must have some kind of sorting in advance, won't index Sort_normal */
int  hash_reindex(Hash ht, int  *map);

/* sort hash table entries according to SortState stt and SortOrder ord */
/* ord can be Sort_ran (won't sort), Sort_asc (ascendingly), and Sort_des */
/* stt can be one of the following */
/*    Sort_normal : not sort, hashing order */
/*    Sort_index : not sort, just tell traverse() to use index order */
/*    Sort_key_counts : sort by key counts */
/*    Sort_key_string : sort by string key */
/*    Sort_key_integer : sort by integer key */
/*    Sort_key_float : sort by float key */
/*    Sort_key_double : sort by double key */
/*    Sort_key_strint : sort by key that is integer in string */
/*    Sort_key_strflt : sort by key that is float in string */
/*    Sort_key_strdbl : sort by key that is double in string */
/*    Sort_data_counts : sort by number pieces of data */
/*    Sort_data_string : sort by string data */
/*    Sort_data_integer : sort by integer data */
/*    Sort_data_float : sort by float data */
/*    Sort_data_double : sort by double data */
/*    Sort_data_strint : sort by data that is integer in string */
/*    Sort_data_strflt : sort by data that is float in string */
/*    Sort_data_strdbl : sort by data that is double in string */
/*    Sort_comparator : sort by comparator which was set by */
/*        hash_sort_comparator() */
int  hash_sort(Hash ht, const SortState stt, const SortOrder ord);

/* set comparator so that hash_sort(ht, Sort_comparator, ) works */
int  hash_sort_comparator(Hash ht, SortComp scmp);

/*===========================================================================*/
/* insertion operations. return counts (>=1) or 0 if insertion failed.       */
/*===========================================================================*/
/* insert key, and increment key counter, if inx != NULL, return index too */
int  hash_insert_key(Hash ht, const char *key, const int keysize, int *inx);

/* insert key only if the key has not been inserted. otherwise return 0 */
int  hash_insert_key_unique(Hash ht, const char *key, const int keysize, int *inx);

/* insert key only if the key has been inserted. otherwise return 0. */
int  hash_insert_key_multiple(Hash ht, const char *key, const int keysize, int *inx);

/* insert key and data */
int  hash_insert_data(Hash ht, const char *key, const int keysize, int *inx,
	char *data, const int datasize);

/*===========================================================================*/
/* retrieving operations. return counts or 0 if search not found.            */
/*===========================================================================*/
/* given key/keysize, search for inx */
int  hash_search_index(Hash ht, const char *key, const int keysize, int *inx);

/* given index, search for and copy back key, set keysize if it is not NULL */
int  hash_search_key(Hash ht, const int inx, char *key, int *keysize);

/* given index, search for and set pointer to key, set keysize if not NULL */
int  hash_search_key_ptr(Hash ht, const int inx, char **keyptr, int *keysize);

/* given index, search for and copy back the 1st data, */
/* set datasize if it is not NULL */
int  hash_index_search_data(Hash ht, const int inx, char *data, int *datasize);

/* given index, search for and set pointer to the 1st data, */
/* set datasize if it is not NULL */
int  hash_index_search_data_ptr(Hash ht, const int inx,
	char **dataptr, int *datasize);

/* given index, search for and copy back all data, */
/* set datasize if it is not NULL */
int  hash_index_search_ndata(Hash ht, const int inx,
	int *ndata, char **data, int *datasize);

/* given index, search for and set pointer in pointer array to all data, */
/* set datasize in datasize array, if it is not NULL */
int  hash_index_search_ndata_ptr(Hash ht, const int inx,
	int *ndata, char **dataptr, int *datasize);

/* given key/keysize, search for and copy back the 1st data, */
/* set datasize if it is not NULL */
int  hash_key_search_data(Hash ht, const char *key, const int keysize,
	char *data, int *datasize);

/* given key/keysize, search for and set pointer to the 1st data, */
/* set datasize if it is not NULL */
int  hash_key_search_data_ptr(Hash ht, const char *key, const int keysize,
	char **dataptr, int *datasize);

/* given key/keysize, search for and copy back all data, */
/* set datasize if it is not NULL */
int  hash_key_search_ndata(Hash ht, const char *key, const int keysize,
	int *ndata, char **data, int *datasize);

/* given key/keysize, search for and set pointer in pointer array to */
/* all data, set datasize in datasize array, if it is not NULL */
int  hash_key_search_ndata_ptr(Hash ht, const char *key, const int keysize,
	int *ndata, char **dataptr, int *datasize);

/*===========================================================================*/
/* modification operations. return counts or 0 if modification failed.       */
/*===========================================================================*/

/*===========================================================================*/
/* if key/kesysize is in table, search and set inx (if it is not NULL),      */
/* and return 2. (hash_search())                                             */
/* if key/keysize is not in table, insert the key/keysize, set inx (if it    */
/* is not NULL), and return 1. (hash_insert())                               */
/* return 0 if failed.                                                       */
/*===========================================================================*/
int  hash_locate_key(Hash ht, const char *key, const int keysize, int *inx);

/*===========================================================================*/
/* if key/keysize is in talbe, and it has data already, replace the old      */
/* data with the one passed in, and return the key counts.                   */
/* if key/keysize is not in table, insert the key/keysize, set *inx, and     */
/* attach the data to the entry. (hash_insert_data())                        */
/*                                                                           */
/* by calling this function, key always gets one piece of data in the table. */
/*===========================================================================*/
int  hash_key_replace_data(Hash ht, const char *key, const int keysize,
	int *inx, char *data, int datasize);

/*===========================================================================*/
/* if found an entry match the key, the entry has data already, and the      */
/* first data has the same size as passed-in datasize, replace it with the   */
/* data passed in, otherwise return failed.                                  */
/* useful when get a data and update it and put it back in.                  */
/*===========================================================================*/
int  hash_key_update_data(Hash ht, const char *key, const int keysize,
	char *data, int datasize);

int  hash_index_update_data(Hash ht, const int inx, char *data, int datasize);


/*===========================================================================*/
/* search a key in the table. if it                                          */
/* (1) exists with data: pass out the data and return 2.                     */
/* (2) exists but without data: insert the passed-in data and return 1.      */
/* (3) does not exist: insert the key and the passed-in data and return 1.   */
/* (4) cannot insert data or key and data: return 0.                         */
/* useful for conditional insertion (insertion depends on retrieving).       */
/* the hash_index_unique_data works slightly different. it takes care of     */
/* case (1) and (2) and treats (3) as (4).                                   */
/*===========================================================================*/
int  hash_key_unique_data(Hash ht, const char *key, const int keysize,
	char *data, int insize, char **dataptr, int *outsize);

int  hash_index_unique_data(Hash ht, const int inx,
	char *data, int insize, char **dataptr, int *outsize);

/*===========================================================================*/
/* deletion operations. return counts or 0 if deletion failed.               */
/*===========================================================================*/
/* given key/keysize, remove the entry from hash table */
int  hash_key_remove_key(Hash ht, const char *key, const int keysize);

/* given key/keysize, remove the 1st data associated with it from hash table */
int  hash_key_remove_data(Hash ht, const char *key, const int keysize);

/* given key/keysize, remove the all data associated with it from hash table */
int  hash_key_remove_ndata(Hash ht, const char *key, const int keysize);

/* given index, remove the entry from hash table */
int  hash_index_remove_key(Hash ht, const int inx);

/* given index, remove the 1st data associated with it from hash table */
int  hash_index_remove_data(Hash ht, const int inx);

/* given index, remove the all data associated with it from hash table */
int  hash_index_remove_ndata(Hash ht, const int inx);

/*===========================================================================*/
/* traversing operation. return  counts or 0 if modification failed.         */
/*===========================================================================*/
int  hash_traverse_start(Hash ht);

int  hash_traverse_key(Hash ht, char *key, int *keysize, int *inx);

int  hash_traverse_key_ptr(Hash ht, char **keyptr, int *keysize, int *inx);

int  hash_traverse_data(Hash ht, char *key, int *keysize, int *inx,
	char *data, int *datasize);

int  hash_traverse_data_ptr(Hash ht, char **keyptr, int *keysize, int *inx,
	char **dataptr, int *datasize);

int  hash_traverse_ndata(Hash ht, char *key, int *keysize, int *inx,
	int *ndata, char **data, int *datasize);

int  hash_traverse_ndata_ptr(Hash ht, char **keyptr, int *keysize, int *inx,
	int *ndata, char **dataptr, int *datasize);

int  hash_traverse_end(Hash ht);

int  hash_next_data(Hash ht, char *data);

int  hash_next_data_ptr(Hash ht, char **dataptr);

int  hash_walk(Hash ht, HashProc proc, void *in, void *out);

int  hash_edit(Hash ht, HashProc proc, void *in, void *out);

/*===========================================================================*/
/* query functions                                                           */
/*===========================================================================*/
int  hash_state(Hash ht);
int  hash_max_index(Hash ht);	/* if no removing, 1 <= x <= # has data */
int  hash_num_keys(Hash ht);	/* number of occupied cells */
int  hash_max_keys(Hash ht);	/* total number of cells */
int  hash_max_keysize(Hash ht);	/* max of all keysize among all keys */
int  hash_sum_ndata(Hash ht);	/* total number pieces of data */
int  hash_max_ndata(Hash ht);	/* max number of ndata among all keys */
int  hash_generate_hashkey(Hash ht, char *key, int keysize);

/* associative array */
#define	Smap	Hash
#define sm_create(size)	hash_create((size),0)
#define sm_delete(sm)	hash_delete(sm)
#define sm_max(sm)	hash_num_keys(sm)	/* return number keys */
int  sm_cnt(Hash sm, const char key[]);		/* return counts */
int  sm_inx(Hash sm, const char key[]);		/* return index and insert */
int  sm_sinx(Hash sm, const char key[]);	/* return index or 0 not found*/
int  sm_ninx(Hash sm, const char key[], int *n); /* index with new flag */
char *sm_key(Hash sm, const int inx);		/* return key or NULL not found */

#define	Imap	Hash
#define im_create(size)	hash_create((size),0)
#define im_delete(im)	hash_delete(im)
#define im_max(im)	hash_num_keys(im)
int  im_cnt(Hash im, const int key);
int  im_inx(Hash im, const int key);
int  im_sinx(Hash im, const int key);
int  im_ninx(Hash im, const int key, int *n);
int  im_key(Hash im, const int inx);

/*===========================================================================*/
/* field composition and decomposition, may shared by list etc.              */
/* compose_* do not use delimeter therefore they can be used for any type of */
/* data but they are slower. combine_* need delimeter. paste_* do not need   */
/* and run fast but they just paste two pieces of data together adjacently.  */
/* argument d is delimeter, it must not appear in any keys.                  */
/* compose_front --- return consequent key size, put skey in front of ckey   */
/* compose_end --- return consequent key size, put skey in front of ckey     */
/* decompose --- return sub key size or 0, set *skey                         */
/*===========================================================================*/
int  compose_front  (char *ckey, int csiz, char *skey, int ssiz);
int  compose_end    (char *ckey, int csiz, char *skey, int ssiz);
int  decompose      (char *ckey, int *csiz, char **skey);
int  combine_front  (char d, char *ckey, int csiz, char *skey, int ssiz);
int  combine_end    (char d, char *ckey, int csiz, char *skey, int ssiz);
int  decombine      (char d, char *ckey, int *csiz, char **skey);
int  paste_front    (char *ckey, int csiz, char *skey, int ssiz);
int  paste_end      (char *ckey, int csiz, char *skey, int ssiz);

/*===========================================================================*/
/* associative array (array indexed by string) functions                     */
/*===========================================================================*/
#define ASSOC_MISSING_COUNT	0
#define ASSOC_MISSING_DEFAULT	1
typedef	enum
{
	Assoc_string,
	Assoc_integer
}	AssocType;
typedef struct
{
	Hash		ht;
	AssocType	styp;	/* ASSOC_STRING or ASSOC_INT */
	int		miss;	/* missing handle */
	int		indx;	/* value for auto index */
	int		idef;	/* default integer */
	char		*sdef;	/* default string */
}	_Assoc, *Assoc;

Assoc iassoc_create(int miss_handle, int miss_default);
Assoc iassoc_load(FILE *fp);
int   iassoc_delete(Assoc aa);
int   iassoc_save(Assoc aa, FILE *fp);
int   iassoc_index(Assoc aa, char *inx, int value);
int   iassoc_value(Assoc aa, char *inx);
Assoc sassoc_create(int miss_handle, char *miss_default);
Assoc sassoc_load(FILE *fp);
int   sassoc_delete(Assoc aa);
int   sassoc_save(Assoc aa, FILE *fp);
int   sassoc_index(Assoc aa, char *inx, char *value);
char *sassoc_value(Assoc aa, char *inx);
Hash  assoc_hash(Assoc aa);

#ifdef __cplusplus
}
#endif

#if 0
/* example of user-provided sort comparators */
int  sort_keysize_key_string(SortEntry *e1, SortEntry *e2)
{

	/*===================================================================*/
	/* in this example. keysize is the first order, and key as strings   */
	/* is the second order. use hash_sort_comparator() to cache it       */
	/* inside the hash table and then call                               */
	/* hash_sort(ht, Sort_comparator, Sort_asc/Sort_des) to sort the     */
	/* table. After sorting, traverseing table will be in the sorted     */
	/* order (user-defined comparator sorting will not help the speed    */
	/* of searching , it is only good for ordered traversing).           */
	/*                                                                   */
	/* NOTE that the object, key and data of SortEntry are pointers      */
	/* to hash table internal data and must be read only. the execution  */
	/* of the comparator is under pthread lock therefore other hash      */
	/* functions such as hash rehash(), hash_write() etc cannot be       */
	/* called inside the comparator. the object of SortEntry is "this"   */
	/* hashtable. it can be casted to Hash but member must be read only. */
	/*                                                                   */
	/* when the data of a hash table is pointer to user-defined          */
	/* structure and it is desired to access them in order of a member   */
	/* casting liking following will do:                                 */
	/*     struct mytype	*mt1, *mt2;                                  */
	/*     mt1 = (struct mytype *) e1->data;                             */
	/*     mt2 = (struct mytype *) e2->data;                             */
	/*     if (mt1->a > mt2->a)	return 1;                            */
	/*     else if (mt1->a < mt2->a)	return -1;                   */
	/*     else                    	return 0;                            */
	/*===================================================================*/

	if (e1->keysize > e2->keysize)
		return 1;
	else if (e1->keysize < e2->keysize)
		return -1;
	else
		return strcmp(e1->key, e2->key);
}

/* example of hash walking procedures */
#include <string.h>
#include "hash.h"

int  traverser(SortEntry *ent, int mstep, int nstep, void *in, void *out)
{
	int	i;
	char	*prefix = (char *) in;
	char	***listing = (char ***) out;
	char	buff[BUFSIZ];

	if (nstep == 0)
	{
		if (((*listing) = (char **) malloc(mstep *
			sizeof(char *))) == NULL)
		{
			return 0;
		}
	}
	sprintf(buff, "%s%d inx=%d pos=%d cnt=%d ndat=%d key=\"%s\" dat=\"%s\"",
		prefix, nstep + 1, ent->index, ent->position,
		ent->counts, ent->ndata, ent->key, ent->data);
	if (((*listing)[nstep] = strdup(buff)) == NULL)
	{
		return 0;
	}

	return 1;
}

int  main(int argc, char *argv[])
{
	int	i, line;
	char	*title = "Cell ";
	char	**content;
	FILE	*fp;
	Hash	ht;

	if (argc != 2)
	{
		fprintf(stderr, "usage: tst hashtablefile\n");
		exit(1);
	}
	if ((fp = fopen(argv[1], "r")) == NULL)
	{
		fprintf(stderr, "cannot open file %s for read\n", argv[1]);
		exit(2);
	}
	if ((ht = hash_read(fp)) == 0)
	{
		fprintf(stderr, "file %s is not hashtable file\n", argv[1]);
		exit(3);
	}
	fclose(fp);

	line = hash_walk(ht, traverser, title, &content);
	for (i = 0; i < line; i++)
	{
		fprintf(stdout, "%s\n", content[i]);
		free(content[i]);
	}
	if (content != NULL)
	{
		free(content);
	}

	return 0;
}
#endif
#endif
