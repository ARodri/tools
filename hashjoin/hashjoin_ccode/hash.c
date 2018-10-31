/*===========================================================================*/
/* hash.c : hash table object program file.                                  */
/* include implemention of all functions defined in hash.h.                  */
/*                                                                           */
/* Sun Dec 15 20:14:56 MST 1996, Jincai, create. 1st version.                */
/* Sat May 10 06:25:01 MDT 1997, Jincai, modify. 2st version.                */
/* Wed Feb 11 22:36:23 MST 1998, Jincai, modify. 3rd version.                */
/* Fri Jul 31 16:28:16 MDT 1998, Jincai, modify. replace double hash         */
/* Fri Nov  9 10:21:26 MST 2001, Jincai, modify. fix warning/cosmetic/_share */
/*===========================================================================*/
#include "hash.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
/*LINTLIBRARY*/

/*===========================================================================*/
/* part 1: rack class implementation.                                        */
/*===========================================================================*/
/*===========================================================================*/
/* Rack, a fundemental data structure. it provides storage for data,         */
/* optionally store the size of the data, and optionally chain a number of   */
/* related pieces of data together. An integer index (handle) is returned    */
/* for other data structure to keep and to organize.                         */
/* Three types of storages are supported:                                    */
/*     on/off:    on/off type saves data only.                               */
/*     put/get:   put/get type saves data and its size.                      */
/*     hook/pick: hook/pick type saves an array of data and their sizes.     */
/*                in a linked list fashion.                                  */
/*                                                                           */
/* implementation notes:                                                     */
/*                                                                           */
/* structure:                                                                */
/*     base ---> memory pointer of the memory storage, zero based.           */
/*     size ---> allocated storage size. there are some overhead.            */
/*     fpos ---> first free position. start from zero.                       */
/*                                                                           */
/*     on/off type storage:                                                  */
/*                                                                           */
/*                                    +---------------+                      */
/*                   handle ------>   |               |                      */
/*                                    |   data        |                      */
/*                                    |               |                      */
/*                                    +---------------+                      */
/*                                                                           */
/*     put/get type storage:                                                 */
/*                                                                           */
/*                                    +---------------+                      */
/*                   handle ------>   | size of data  |                      */
/*                                    |---------------|                      */
/*                                    |               |                      */
/*                                    |   data        |                      */
/*                                    |               |                      */
/*                                    +---------------+                      */
/*                                                                           */
/*     hook/pick type storage:                                               */
/*                                                                           */
/*                                    +---------------+                      */
/*                   handle ------>   | size of data  |                      */
/*                                    |---------------|                      */
/*                                    |               |                      */
/*                                    |   data        |                      */
/*                                    |               |                      */
/*                                    |---------------|                      */
/*                     hook ------->  | next location | ------+              */
/*                                    +---------------+       |              */
/*                                                            |              */
/*                                    +---------------+       |              */
/*                                    | size of data  | <-----+              */
/*                                    |---------------|                      */
/*                                    |               |                      */
/*                                    |   data        |                      */
/*                                    |               |                      */
/*                                    |---------------|                      */
/*       cellp->datatail --------->   | -1 for ending |                      */
/*                                    +---------------+                      */
/*                                                                           */
/*===========================================================================*/
/* create a new rack, return NULL = failed. */
Rack rack_setup(size_t size)
{
	Rack	r;

	if ((r = (_Rack *) malloc(sizeof(_Rack))) == NULL)
	{
		return NULL;
	}
	if ((r->base = (char *) calloc(1, size)) == NULL)
	{
		free(r);
		return NULL;
	}
	r->size = size;
	r->fpos = 0;

	return r;
}

/* destroy a rack, return size in the rack */
int  rack_clean(Rack r)
{
	int	size = 0;

	if (r != NULL)
	{
		size = r->size;
		if (r->base != NULL)
		{
			free(r->base);
		}
		free(r);
	}

	return size;
}

Rack rack_copy(Rack r)
{
	Rack	t;

	if ((t = rack_setup(r->size)) == NULL)
	{
		return NULL;
	}
	t->fpos = r->fpos;
	memcpy(t->base, r->base, r->fpos);

	return t;
}

/* expand rack to accommodate "addon" bytes, growing by a factor of 2 */
/* return 0 = failed (the rack is untouched), otherwise return new size */
int  rack_expand(Rack r, int addon)
{
	char	*newbase;
	size_t	newsize;

	/* figure out size first */
	newsize = IncSize(r->size);
	if (newsize < r->size + addon)
	{
		newsize = r->size + addon + 1;
	}
	/* not use realloc, so that the existing base will */
	/* not be disturbed in case of allocation failure. */
	if ((newbase = (char *) calloc(1, (size_t) newsize)) == NULL)
	{
		return 0;
	}
	memcpy(newbase, r->base, r->fpos);
	free(r->base);
	r->base = newbase;
	r->size = newsize;

	return newsize;
}

#define	RACK_IO_ID	"#Rack Binary Save 1.1"
/* save rack in binary format, return 0 = failed, return bytes written */
int  rack_save(Rack r, FILE *fp)
{
	if (fwrite(RACK_IO_ID, 1, sizeof(RACK_IO_ID), fp) != sizeof(RACK_IO_ID))
	{
		return 0;
	}
	if (fwrite(&r->fpos, sizeof(r->fpos), 1, fp) != 1)
	{
		return 0;
	}
	if (fwrite(&r->size, sizeof(r->size), 1, fp) != 1)
	{
		return 0;
	}
	if (fwrite(r->base, 1, r->fpos, fp) != (size_t) r->fpos)
	{
		return 0;
	}
	if (fwrite(RACK_IO_ID, 1, sizeof(RACK_IO_ID), fp) != sizeof(RACK_IO_ID))
	{
		return 0;
	}

	return sizeof(RACK_IO_ID) +
	       sizeof(r->fpos) +
	       sizeof(r->size) +
	       r->fpos +
	       sizeof(RACK_IO_ID);
}

/* create/load rack written by rack_save, with limited verificataion */
/* return NULL = failed, return created and filled rack. */
Rack rack_load(FILE *fp)
{
	int	fpos;
	size_t	size;
	char	buff[80];
	Rack	r;

	if (fread(buff, 1, sizeof(RACK_IO_ID), fp) != sizeof(RACK_IO_ID) ||
	    memcmp(buff, RACK_IO_ID, sizeof(RACK_IO_ID)))
	{
		return NULL;
	}
	if (fread(&fpos, sizeof(fpos), 1, fp) != 1)
	{
		return 0;
	}
	if (fread(&size, sizeof(size), 1, fp) != 1)
	{
		return 0;
	}
	if ((r = rack_setup(size)) == NULL)
	{
		return NULL;
	}
	if (fread(r->base, 1, fpos, fp) != (size_t) fpos)
	{
		rack_clean(r);
		return NULL;
	}
	r->fpos = fpos;
	if (fread(buff, 1, sizeof(RACK_IO_ID), fp) != sizeof(RACK_IO_ID) ||
	    memcmp(buff, RACK_IO_ID, sizeof(RACK_IO_ID)))
	{
		rack_clean(r);
		return NULL;
	}

	return r;
}
#undef	RACK_IO_ID

/* start move one rack to a new one for purge, return = NULL failed */
Rack rack_move(Rack r)
{
	Rack	t;

	if ((t = rack_setup(r->size)) == NULL)
	{
		return NULL;
	}

	return t;
}

/* move data from r rack to t rack. returen new handle, set *hook to new hook */
int  rack_rehook(Rack t, Rack r, int handle, int *hook)
{
	int	size, thandle, phandle;

	thandle = -1;
	*hook = -1;
	while (handle >= 0 && handle < (int) r->size)
	{
		phandle = t->fpos;
		if (thandle == -1)
		{
			thandle = phandle;
		}
		memcpy((char *) &size, r->base + handle, sizeof(int));
		size += sizeof(int);
		*hook = phandle + size;
		t->fpos = *hook + sizeof(int);
		memcpy(t->base + phandle, r->base + handle, size);
		memcpy(t->base + (*hook), (char *) &(t->fpos), sizeof(int));
		memcpy((char *) &handle, r->base + handle + size, sizeof(int));
	}
	if (*hook != -1)
	{
		size = -1;
		memcpy(t->base + (*hook), (char *) &(size), sizeof(int));
	}

	return thandle;
}

/* move data from r rack to t rack. returen new handle */
int  rack_reput(Rack t, Rack r, int handle)
{
	int	size, thandle;

	thandle = -1;
	if (handle >= 0 && handle < (int) r->size)
	{
		thandle = t->fpos;
		memcpy((char *) &size, r->base + handle, sizeof(int));
		size += sizeof(int);
		t->fpos += size;
		memcpy(t->base + thandle, r->base + handle, size);
	}

	return thandle;
}

/* return new handle */
int  rack_reon(Rack t, Rack r, int handle, int size)
{
	int	thandle = -1;

	if (handle >= 0 && handle < (int) r->size)
	{
		thandle = t->fpos;
		memcpy(t->base + thandle, r->base + handle, size);
		t->fpos += size;
	}

	return thandle;
}

/* data on the rack, (without saving size). */
/* return -1 if failed, return handle for later retrieve. */
int  rack_on(Rack r, const char *data, const int size)
{
	int	handle = -1;

	if (size >= 0 && data != NULL)
	{
		if ((size + r->fpos) >= (int) r->size)
		{
			if (!rack_expand(r, size))
			{
				return -1;
			}
		}
		handle = r->fpos;
		memcpy(r->base + handle, data, size);
		r->fpos += size;
	}

	return handle;
}

/* not allow data to be NULL */
int  rack_off(Rack r, int handle, char *data, int size)
{
	if (handle >= 0 && handle < (int) r->size)
	{
		memcpy(data, r->base + handle, size);
		return size;
	}
	else
	{
		return -1;
	}
}

char *rack_off_ptr(Rack r, int handle)
{
	if (handle >= 0 && handle < (int) r->size)
	{
		return r->base + handle;
	}
	else
	{
		return NULL;
	}
}

/* internal use only, garrentee that handle in range, speed up process */
#define rack_cmp_ptr(r,handle,key,keysiz) \
	(memcmp((r)->base+(handle), (key), (keysiz)))

/* return -1 means failed to put the data on the rack (cannot alloc space)   */
/* otherwise return handle for later fetch.                                  */
int  rack_put(Rack r, char *data, int size)
{
	int	rsiz, handle;
	char	*bp;

	handle = -1;
	if (size >= 0 && data != NULL)
	{
		rsiz = size + sizeof(int);
		if (rsiz + r->fpos >= (int) r->size)
		{
			if (!rack_expand(r, rsiz))
			{
				return -1;
			}
		}
		handle = r->fpos;
		bp = r->base + handle;
		memcpy(bp, (char *) &size, sizeof(int));
		bp += sizeof(int);
		memcpy(bp, data, size);
		r->fpos += rsiz;
	}

	return handle;
}

/* allow data to be NULL */
int  rack_get(Rack r, int handle, char *data)
{
	int	size = -1;
	char	*bp;

	if (handle >= 0 && handle < (int) r->size)
	{
		bp = r->base + handle;
		memcpy((char *) &size, bp, sizeof(int));
		if (data != NULL)
		{
			bp += sizeof(int);
			memcpy(data, bp, size);
		}
	}

	return size;
}

int  rack_get_ptr(Rack r, int handle, char **datap)
{
	int	size = -1;
	char	*bp;

	if (handle >= 0 && handle < (int) r->size)
	{
		bp = r->base + handle;
		memcpy((char *) &size, bp, sizeof(int));
		if (datap != NULL)
		{
			(*datap) = bp + sizeof(int);
		}
	}

	return size;
}

/* if (*tail) is zero, start a new chain, (cannot hook on zero position).    */
/* always return new handle (may not be useful for the caller to keep)       */
/* update the hook so that next time it can be passed in to hook new data.   */
int  rack_hook (Rack r, int *hook, char *data, int size)
{
	int	rsiz, handle;

	handle = -1;
	if (size >= 0 && data != NULL)
	{
		rsiz = size + sizeof(int) + sizeof(int);
		if (rsiz + r->fpos >= (int) r->size)
		{
			if (!rack_expand(r, rsiz))
			{
				return -1;
			}
		}
		handle = r->fpos;
		if (*hook >= 0)
		{
			/* need to hook to the last one */
			memcpy(r->base + *hook, (char *) &handle, sizeof(int));
		}
		*hook = handle + size + sizeof (int);
		r->fpos += rsiz;
		memcpy(r->base + handle, (char *) &size, sizeof(int));
		memcpy(r->base + handle + sizeof(int), data, size);
		size = -1;
		memcpy(r->base + *hook, (char *) &size, sizeof(int));
	}

	return handle;
}

/* return new handle, hook is updated if unhook the last link.nth = 1 ..  */
/* num_of_hooks, nth == 0 || nth > num_of_hooks means unhooking last link */
int  rack_unhook (Rack r, int handle, int *hook, int nth)
{
	int	size;
	int	phandle, phook, chook;

	phandle = handle;
	phook = chook = -1;
	if (nth == 0)
	{
		nth--;
	}
	while (nth-- && phandle >= 0 && phandle < (int) r->size)
	{
		phook = chook;
		memcpy((char *) &size, r->base + phandle, sizeof(int));
		chook = phandle + size + sizeof(int);
		memcpy((char *) &phandle, r->base + chook, sizeof(int));
	}
	if (phook != -1)
	{
		/* not remove first */
		if (phandle == -1)
		{
			/* remove last one */
			*hook = phook;
		}
		memcpy(r->base + phook, (char *) &phandle, sizeof(int));
	}
	else if (phandle != -1)
	{
		/* remove first */
		handle = phandle;
	}
	else
	{
		*hook = -1;
		handle = -1;
	}

	return handle;
}

/* return size, handle is updated for the next call */
int  rack_pick(Rack r, int *handle, char *data)
{
	int	size = -1;
	int	phandle = *handle;
	char	*bp;

	if (phandle >= 0 && phandle < (int) r->size)
	{
		bp = r->base + phandle;
		memcpy((char *) &size, bp, sizeof(int));
		bp += sizeof(int);
		if (data != NULL)
		{
			memcpy(data, bp, size);
		}
		bp += size;
		memcpy((char *) handle, bp, sizeof(int));
	}

	return size;
}

int  rack_pick_ptr(Rack r, int *handle, char **datap)
{
	int	size = -1;
	int	phandle = *handle;
	char	*bp;

	if (phandle >= 0 && phandle < (int) r->size)
	{
		bp = r->base + phandle;
		memcpy((char *) &size, bp, sizeof(int));
		bp += sizeof(int);
		if (datap != NULL)
		{
			(*datap) = bp;
		}
		bp += size;
		memcpy((char *) handle, bp, sizeof(int));
	}

	return size;
}

/* return ndata, allow size == NULL. */
int  rack_pick_all(Rack r, int handle, char **data, int *sizes)
{
	int	count = 0;

	if (data != NULL && sizes != NULL)
	{
		while (handle >= 0 && handle < (int) r->size)
		{
			sizes[count] = rack_pick(r, &handle, data[count]);
			count++;
		}
	}
	else if (sizes != NULL)
	{
		while (handle >= 0 && handle < (int) r->size)
		{
			sizes[count] = rack_pick(r, &handle, NULL);
			count++;
		}
	}
	else if (data != NULL)
	{
		while (handle >= 0 && handle < (int) r->size)
		{
			rack_pick(r, &handle, data[count]);
			count++;
		}
	}

	return count;
}

int  rack_pick_all_ptr(Rack r, int handle, char **datap, int *sizes)
{
	int	count = 0;

	if (datap != NULL && sizes != NULL)
	{
		while (handle >= 0 && handle < (int) r->size)
		{
			sizes[count] = rack_pick_ptr(r, &handle, datap + count);
			count++;
		}
	}
	else if (sizes != NULL)
	{
		while (handle >= 0 && handle < (int) r->size)
		{
			sizes[count] = rack_pick(r, &handle, NULL);
			count++;
		}
	}
	else if (datap != NULL)
	{
		while (handle >= 0 && handle < (int) r->size)
		{
			rack_pick_ptr(r, &handle, datap + count);
			count++;
		}
	}


	return count;
}

/* returns size of a particular hook/put, not the overall size */
int  rack_size(Rack r, int handle)
{
	int	size = -1;

	if (handle >= 0 && handle < (int) r->size)
	{
		memcpy((char *) &size, r->base + handle, sizeof(int));
	}

	return size;
}

int  rack_counts(Rack r, int handle)
{
	int	size;
	int	count = 0;

	while (handle >= 0 && handle < (int) r->size)
	{
		memcpy((char *) &size, r->base + handle, sizeof(int));
		size += handle + sizeof(int);
		memcpy((char *) &handle, r->base + size, sizeof(int));
		count++;
	}

	return count;
}
/*===========================================================================*/
/* End of part 1: rack class implementation.                                 */
/*===========================================================================*/




/*===========================================================================*/
/* part 2: hash class implementation.                                        */
/*     section 1: hash key functions.                                        */
/*     section 2: hash engine/static.                                        */
/*     section 3: hash maintenence functions.                                */
/*     section 4: sorting functions.                                         */
/*     section 5: hash creation/deletion, I/O operations.                    */
/*     section 6: insertion functions.                                       */
/*     section 7: retrieving functions.                                      */
/*     section 8: modification functions.                                    */
/*     section 9: traversing functions.                                      */
/*     section 10: querying functions.                                       */
/*     section 11: hash convenient functions.                                */
/*     section 12: assoc functions.                                          */
/*     section 13: built-in testing main program.                            */
/*===========================================================================*/
#define HASH_DEFAULT_ENTRIES	13	/* 13 cells to start with */
#define HASH_DEFAULT_PREVOUS	8	/* previous number in series is 8 */
#define HASH_DEFAULT_DENSITY	75	/* allow 75% full before realloc */

/*===========================================================================*/
/*     section 1: hash key functions.                                        */
/*===========================================================================*/
/*===========================================================================*/
/* random number lookup table.                                               */
/* number of random numbers = 256.                                           */
/* random number range = [0..255].                                           */
/*===========================================================================*/
static unsigned char	hash_xkey_table[256] =
{
	235,54,35,73,112,122,62,16,80,49,99,131,152,239,207,234,
	26,222,214,102,150,133,103,250,186,37,44,172,97,17,223,6,
	153,91,242,100,252,142,18,165,253,98,66,117,14,24,206,245,
	130,147,106,128,212,154,126,51,64,42,48,113,77,159,163,75,
	84,19,12,82,249,185,141,9,161,149,243,114,205,68,134,157,
	225,101,15,160,85,202,93,57,107,254,238,74,89,95,90,210,
	55,231,227,47,155,190,166,193,2,229,22,72,170,129,111,148,
	5,124,7,201,255,182,105,59,173,171,189,244,1,11,83,104,
	180,145,34,204,13,221,31,199,146,8,79,92,217,56,183,169,
	29,65,187,208,30,69,123,125,63,248,143,139,60,138,236,135,
	41,27,230,88,215,241,140,224,209,216,46,178,43,136,132,121,
	251,67,21,36,179,3,76,53,71,246,115,211,176,25,33,137,
	233,203,4,61,195,174,168,200,70,237,50,58,156,28,127,94,
	220,38,119,116,118,87,198,151,110,32,40,192,78,20,167,191,
	240,175,81,162,232,219,86,23,196,213,120,177,184,188,144,52,
	194,226,96,39,181,0,197,45,228,158,247,109,108,10,164,218
};

static
int  hash_xxkey(Hash ht, const char *key, const int keysize, int p, int *k)
{
	int	n = ++(*k);

	if (n < keysize)
	{
		n = keysize - (++(*k));
		n = hash_xkey_table[(unsigned char) key[n + 1]] +
			hash_xkey_table[(unsigned char) key[n]];
		return ((p + n * n) % ht->ment);
	}
	else if (n <= 255)
	{
		n = hash_xkey_table[n];
		return ((p + n * n) % ht->ment);
	}
	else
	{
		return ((p + 1) % ht->ment);
	}
}

static
int  hash_xkey(Hash ht, const char *key, const int keysize, int p, int *k)
{
	int		i, j;
	unsigned char	*ukey = (unsigned char *) &p;

	for (p = 0, i = ht->nbyt; i < (int) sizeof(int); i++)
	{
		for (j = i - ht->nbyt; j < keysize; j += ht->byte)
		{
			ukey[i] = (ukey[i] << 1) ^ key[j];
		}
		ukey[i] = hash_xkey_table[ukey[i]];
	}

	return (p % ht->ment);
}

#define	NotKey(celp, rack, key, ksiz)	\
((ksiz) != (int)(celp).keysize || rack_cmp_ptr((rack),(celp).khandle,(key),(ksiz)))
/*===========================================================================*/
/*     End of section 1: hash key functions.                                 */
/*===========================================================================*/

/*===========================================================================*/
/*     section 2: hash engine/static.                                        */
/*===========================================================================*/
static
int  hash_compare_entries(const void *a, const void *b)
{
	SortEntry	*e1 = (SortEntry *) a;
	SortEntry	*e2 = (SortEntry *) b;
	Hash		ht = (Hash) e1->object;
	if (ht->sord == Sort_des)
	{
		/* descendent */
		e1 = (SortEntry *) b;
		e2 = (SortEntry *) a;
	}

	return (ht->scmp)(e1, e2);
}

static
int  hash_expand(Hash ht)
{
	size_t		rsiz;
	int		i, k, p, keysize;
	char		*key;
	Rack		rack = ht->rack;
	HashCell	*cell = ht->cell, *newcell;

	rsiz = ht->ment;
	ht->ment += ht->lent;
	ht->lent = rsiz;
	/*===================================================================*/
	/* don't worry about the highest bit of a position translated from   */
	/* a key, hashing process will stop here (in malloc) way before      */
	/* the translation function runs into that problem.                  */
	/*===================================================================*/
	if ((newcell = (HashCell *) calloc(ht->ment, sizeof(HashCell))) == NULL)
	{
		return 0;
	}
	ht->pent = ht->ment * ht->dens / 100;
	for (ht->nbyt = sizeof(int), i = ht->ment - 1; i > 0; ht->nbyt--)
		i >>= CHAR_BIT;
	ht->byte = sizeof(int) - ht->nbyt;

	/* rehash all entries */
	for (i = 0; i < (int) rsiz; i++)
	{
		if (cell[i].counts)
		{
			keysize = cell[i].keysize;
			key = rack_off_ptr(rack, cell[i].khandle);
			p = k = 0;
			p = hash_xkey(ht, key, keysize, p, &k);
			while (newcell[p].counts)
			{
				p = hash_xxkey(ht, key, keysize, p, &k);
			}
			newcell[cell[i].index-1].pindex = p + 1;
			newcell[p].index = cell[i].index;
			newcell[p].counts = cell[i].counts;
			newcell[p].keysize = cell[i].keysize;
			newcell[p].khandle = cell[i].khandle;
			newcell[p].dhandle = cell[i].dhandle;
			newcell[p].dhook = cell[i].dhook;
		}
	}
	free(cell);
	ht->cell = newcell;

	return 1;
}

static
int  hash_insert_position(Hash ht, const char *key, const int keysize, int *pos)
{
	int		p, k;
	Rack		rack = ht->rack;
	HashCell	*cell = ht->cell;

	p = k = 0;
	p = hash_xkey(ht, key, keysize, p, &k);
	while (cell[p].counts && NotKey(cell[p], rack, key, keysize))
	{
		p = hash_xxkey(ht, key, keysize, p, &k);
	}
	if (cell[p].counts)
	{
		/* old key */
		*pos = p;
		cell[p].counts++;
		return cell[p].counts;
	}

	/* new item inserted */
	if (ht->nent >= ht->pent)
	{
		if (!hash_expand(ht))
		{
			return 0;
		}
		cell = ht->cell;
		rack = ht->rack;
		p = k = 0;
		p = hash_xkey(ht, key, keysize, p, &k);
		while (cell[p].counts && NotKey(cell[p], rack, key, keysize))
		{
			p = hash_xxkey(ht, key, keysize, p, &k);
		}
	}
	if ((cell[p].khandle = rack_on(rack, key, keysize)) < 0)
	{
		return 0;
	}
	cell[p].keysize = keysize;
	cell[p].dhandle = -1;
	cell[p].dhook = -1;
	cell[ht->ginx].pindex = p + 1;
	cell[p].index = ++(ht->ginx);
	ht->nent++;
	cell[p].counts++;
	*pos = p;

	return cell[p].counts;
}

static
int  hash_key_search_position(Hash ht, const char *key,
	const int keysize, int *pos)
{
	int		p, k;
	Rack		rack = ht->rack;
	HashCell	*cell = ht->cell;

	p = k = 0;
	p = hash_xkey(ht, key, keysize, p, &k);
	while (cell[p].keysize && NotKey(cell[p], rack, key, keysize))
	{
		p = hash_xxkey(ht, key, keysize, p, &k);
	}
	*pos = p;
	ht->cpck = cell[p].dhandle;
	return cell[p].counts;
}

static
int  hash_index_search_position(Hash ht, int inx, int *pos)
{
	int		p;
	HashCell	*cell = ht->cell;

	if (inx > 0 && inx <= ht->ginx && (p = cell[inx-1].pindex))
	{
		*pos = --p;
		ht->cpck = cell[p].dhandle;
		return cell[p].counts;
	}
	else
	{
		return 0;
	}
}

static
int  hash_traverse_position(Hash ht, int *pos)
{
	int		p;
	HashCell	*cell = ht->cell;

	switch (ht->stat)
	{
	case Sort_normal :
		p = ht->trav;
		while (p < ht->ment && !cell[p].counts)
		{
			p++;
		}
		if (p >= ht->ment)
		{
			return 0;
		}
		ht->trav = p + 1;
		break;
	case Sort_index :
		p = ht->trav;
		while (p < ht->ginx && !cell[p].pindex)
		{
			p++;
		}
		if (p >= ht->ginx)
		{
			return 0;
		}
		ht->trav = p + 1;
		p = cell[p].pindex - 1;
		break;
	default:
	case Sort_key_counts :
	case Sort_key_string :
	case Sort_key_integer :
	case Sort_key_float :
	case Sort_key_double :
	case Sort_key_strint :
	case Sort_key_strflt :
	case Sort_key_strdbl :
	case Sort_data_counts :
	case Sort_data_string :
	case Sort_data_integer :
	case Sort_data_float :
	case Sort_data_double :
	case Sort_data_strint :
	case Sort_data_strflt :
	case Sort_data_strdbl :
	case Sort_comparator :
		if ((p = ht->trav) >= ht->nent)
		{
			return 0;
		}
		p = ht->sent[p].position;
		ht->trav++;
		break;
	}
	*pos = p;
	ht->cpck = cell[p].dhandle;
	return cell[p].counts;
}
#undef	NotKey
/*===========================================================================*/
/*     End of section 2: hash engine/static.                                 */
/*===========================================================================*/

/*===========================================================================*/
/*     section 3: hash maintenence functions.                                */
/*===========================================================================*/
static
int  hash_rehash(Hash ht)
{
	int		i, k, p, keysize, ment = ht->ment;
	char		*key;
	Rack		rack = ht->rack;
	HashCell	*cell = ht->cell, *newcell;

	if ((newcell = (HashCell *) calloc(ment, sizeof(HashCell))) == NULL)
	{
		return 0;
	}
	for (i = 0; i < ment; i++)
	{
		if (cell[i].counts)
		{
			keysize = cell[i].keysize;
			key = rack_off_ptr(rack, cell[i].khandle);
			p = k = 0;
			p = hash_xkey(ht, key, keysize, p, &k);
			while (newcell[p].counts)
			{
				p = hash_xxkey(ht, key, keysize, p, &k);
			}
			newcell[cell[i].index-1].pindex = p + 1;
			newcell[p].index = cell[i].index;
			newcell[p].counts = cell[i].counts;
			newcell[p].keysize = cell[i].keysize;
			newcell[p].khandle = cell[i].khandle;
			newcell[p].dhandle = cell[i].dhandle;
			newcell[p].dhook = cell[i].dhook;
		}
	}
	free(ht->cell);
	ht->cell = newcell;

	return 1;
}

static
int  hash_compact(Hash ht)
{
	int		i, j = 0;
	int		ment = ht->ment;
	HashCell	*cell = ht->cell;

	while (j < ment && cell[j].counts)
	{
		j++;
	}
	for (i = j + 1; i < ment; i++)
	{
		if (cell[i].counts)
		{
			memcpy((char *) (cell + j), (char *) (cell + i),
				sizeof(HashCell));
			memset((char *) (cell + i), 0, sizeof(HashCell));
			j++;
		}
	}
	if (ht->sent != NULL)
	{
		free(ht->sent);
		ht->sent = NULL;
	}

	return	1;
}

/* frequently delete needs purge, purge rack and cell (for first insertion) */
static
int  hash_clean(Hash ht)
{
	int		i, p, k, keysize;
	int		nent, ginx, ment = ht->ment;
	char		*key;
	Rack		rack = ht->rack, track;
	HashCell	*cell = ht->cell, *newcell;

	if ((newcell = (HashCell *) calloc(ment, sizeof(HashCell))) == NULL)
	{
		return 0;
	}
	if ((track = rack_move(rack)) == NULL)
	{
		free(newcell);
		return 0;
	}

	/* rehash all entries, reput data on new rack */
	for (ginx = 0, nent = 0, i = 0; i < ment; i++)
	{
		if (cell[i].counts)
		{
			keysize = cell[i].keysize;
			key = rack_off_ptr(rack, cell[i].khandle);
			p = k = 0;
			p = hash_xkey(ht, key, keysize, p, &k);
			while (newcell[p].counts)
			{
				p = hash_xxkey(ht, key, keysize, p, &k);
			}
			newcell[p].index = cell[i].index;
			newcell[p].counts = cell[i].counts;
			newcell[p].keysize = keysize;
			newcell[p].khandle = rack_reon(track, rack,
				cell[i].khandle, keysize);
			newcell[p].dhandle = rack_rehook(track, rack,
				cell[i].dhandle, &(newcell[p].dhook));
			newcell[cell[i].index-1].pindex = p + 1;
			nent++;
			if (ginx < cell[i].index)
			{
				ginx = cell[i].index;
			}
		}
	}
	free(cell);
	rack_clean(rack);
	if (ht->sent != NULL)
	{
		free(ht->sent);
		ht->sent = NULL;
	}
	ht->nent = nent;
	ht->ginx = ginx;
	ht->cell = newcell;
	ht->rack = track;
	ht->ndel = 0;
	ht->stat = Sort_normal;

	return 1;
}

int  hash_purge(Hash ht)
{
	int	rtn;

	if (ht == NULL)
	{
		return 0;
	}
	rtn = hash_clean(ht);

	return rtn;
}

/* must have some kind of sorting in advance, won't index normal */
int  hash_reindex(Hash ht, int *map)
{
	int		i, p, rtn;

	if (ht == NULL || ht->sent == NULL)
	{
		return 0;
	}
	if (map != NULL)
	{
		for (i = 0; i <= ht->ginx; i++)
		{
			map[i] = 0;
		}
	}
	ht->ginx = 0;
	for (i = 0; i < ht->nent; i++)
	{
		p = ht->sent[i].position;
		if (map != NULL)
		{
			map[ht->cell[p].index] = ht->ginx;
		}
		ht->cell[ht->ginx].pindex = p + 1;
		ht->cell[p].index = ++(ht->ginx);
	}
	rtn = ht->ginx;

	return rtn;
}
/*===========================================================================*/
/*     End of section 3: hash maintenence functions.                         */
/*===========================================================================*/

/*===========================================================================*/
/*     section 4: sorting functions.                                         */
/*===========================================================================*/
static int sort_key_counts(SortEntry *e1, SortEntry *e2)
{
	if (e1->counts > e2->counts)		return 1;
	else if (e1->counts < e2->counts)	return -1;
	else	return 0;
}

static int sort_key_string(SortEntry *e1, SortEntry *e2)
{
	return strcmp(e1->key, e2->key);
}

static int sort_key_integer(SortEntry *e1, SortEntry *e2)
{
	int		a = 0, b = 0;

	memcpy ((char *) &a, e1->key, sizeof(int));
	memcpy ((char *) &b, e2->key, sizeof(int));
	if (a > b)	return 1;
	else if (a < b)	return -1;
	else	return 0;
}

static int sort_key_float(SortEntry *e1, SortEntry *e2)
{
	float		a = 0.0, b = 0.0;

	memcpy ((char *) &a, e1->key, sizeof(float));
	memcpy ((char *) &b, e2->key, sizeof(float));
	if (a > b)	return 1;
	else if (a < b)	return -1;
	else	return 0;
}

static int sort_key_double(SortEntry *e1, SortEntry *e2)
{
	double		a = 0.0, b = 0.0;

	memcpy ((char *) &a, e1->key, sizeof(double));
	memcpy ((char *) &b, e2->key, sizeof(double));
	if (a > b)	return 1;
	else if (a < b)	return -1;
	else	return 0;
}

static int sort_key_strint(SortEntry *e1, SortEntry *e2)
{
	int		a = 0, b = 0;

	a = (int) atol(e1->key);
	b = (int) atol(e2->key);
	if (a > b)	return 1;
	else if (a < b)	return -1;
	else	return 0;
}

static int sort_key_strflt(SortEntry *e1, SortEntry *e2)
{
	float		a = 0.0, b = 0.0;

	a = (float) atof(e1->key);
	b = (float) atof(e2->key);
	if (a > b)	return 1;
	else if (a < b)	return -1;
	else	return 0;
}

static int sort_key_strdbl(SortEntry *e1, SortEntry *e2)
{
	double		a = 0.0, b = 0.0;

	a = atof(e1->key);
	b = atof(e2->key);
	if (a > b)	return 1;
	else if (a < b)	return -1;
	else	return 0;
}

static int sort_data_counts(SortEntry *e1, SortEntry *e2)
{
	if (e1->ndata > e2->ndata)	return 1;
	else if (e1->ndata < e2->ndata)	return -1;
	else	return 0;
}

static int sort_data_string(SortEntry *e1, SortEntry *e2)
{
	return strcmp(e1->data, e2->data);
}

static int sort_data_integer(SortEntry *e1, SortEntry *e2)
{
	int		a = 0, b = 0;

	memcpy((char *) &a, e1->data, sizeof(int));
	memcpy((char *) &b, e2->data, sizeof(int));
	if (a > b)	return 1;
	else if (a < b)	return -1;
	else	return 0;
}

static int sort_data_float(SortEntry *e1, SortEntry *e2)
{
	float		a = 0.0, b = 0.0;

	memcpy((char *) &a, e1->data, sizeof(float));
	memcpy((char *) &b, e2->data, sizeof(float));
	if (a > b)	return 1;
	else if (a < b)	return -1;
	else	return 0;
}

static int sort_data_double(SortEntry *e1, SortEntry *e2)
{
	double		a = 0.0, b = 0.0;

	memcpy((char *) &a, e1->data, sizeof(double));
	memcpy((char *) &b, e2->data, sizeof(double));
	if (a > b)	return 1;
	else if (a < b)	return -1;
	else	return 0;
}

static int sort_data_strint(SortEntry *e1, SortEntry *e2)
{
	int		a = 0, b = 0;

	a = (int) atol(e1->data);
	b = (int) atol(e2->data);
	if (a > b)	return 1;
	else if (a < b)	return -1;
	else	return 0;
}

static int sort_data_strflt(SortEntry *e1, SortEntry *e2)
{
	float		a = 0.0, b = 0.0;

	a = (float) atof(e1->data);
	b = (float) atof(e2->data);
	if (a > b)	return 1;
	else if (a < b)	return -1;
	else	return 0;
}

static int sort_data_strdbl(SortEntry *e1, SortEntry *e2)
{
	double		a = 0.0, b = 0.0;

	a = atof(e1->data);
	b = atof(e2->data);
	if (a > b)	return 1;
	else if (a < b)	return -1;
	else	return 0;
}

static int hash_create_entries(Hash ht)
{
	int		i, j;
	int		handle, ment;
	Rack		rack;
	HashCell	*cell;
	SortEntry	*sent;

	ment = ht->ment;
	rack = ht->rack;
	cell = ht->cell;
	if ((sent = (SortEntry *) calloc(ht->nent, sizeof(SortEntry))) == NULL)
	{
		return 0;
	}
	for (j = 0, i = 0; i < ment; i++)
	{
		if (cell[i].counts)
		{
			sent[j].object = (char *) ht;
			sent[j].position = i;
			sent[j].pindex = cell[i].pindex;
			sent[j].index = cell[i].index;
			sent[j].counts = cell[i].counts;
			sent[j].key = rack_off_ptr(rack, cell[i].khandle);
			sent[j].keysize = cell[i].keysize;
			sent[j].ndata = rack_counts(rack, cell[i].dhandle);
			handle = cell[i].dhandle;
			sent[j].datasize = rack_pick_ptr(rack,
				&handle, &sent[j].data);
			j++;
		}
	}
	if (ht->sent != NULL)
	{
		free(ht->sent);
	}
	ht->sent = sent;

	return	1;
}

int  hash_sort_comparator(Hash ht, SortComp scmp)
{
	if (ht == NULL)
	{
		return 0;
	}
	ht->scmp = scmp;

	return 1;
}

int  hash_sort(Hash ht, const SortState stt, const SortOrder ord)
{
	if (ht == NULL)
	{
		return 0;
	}
	switch (stt)
	{
	case Sort_normal :
	case Sort_index :
		if (ht->sent != NULL)
		{
			free(ht->sent);
			ht->sent = NULL;
		}
		ht->stat = stt;
		return 1;
	case Sort_key_counts  :	ht->scmp = sort_key_counts;	break;
	case Sort_key_string  :	ht->scmp = sort_key_string;	break;
	case Sort_key_integer :	ht->scmp = sort_key_integer;	break;
	case Sort_key_float   :	ht->scmp = sort_key_float;	break;
	case Sort_key_double  :	ht->scmp = sort_key_double;	break;
	case Sort_key_strint  :	ht->scmp = sort_key_strint;	break;
	case Sort_key_strflt  :	ht->scmp = sort_key_strflt;	break;
	case Sort_key_strdbl  :	ht->scmp = sort_key_strdbl;	break;
	case Sort_data_counts :	ht->scmp = sort_data_counts;	break;
	case Sort_data_string :	ht->scmp = sort_data_string;	break;
	case Sort_data_integer:	ht->scmp = sort_data_integer;	break;
	case Sort_data_float  :	ht->scmp = sort_data_float;	break;
	case Sort_data_double :	ht->scmp = sort_data_double;	break;
	case Sort_data_strint :	ht->scmp = sort_data_strint;	break;
	case Sort_data_strflt :	ht->scmp = sort_data_strflt;	break;
	case Sort_data_strdbl :	ht->scmp = sort_data_strdbl;	break;
	case Sort_comparator  :	if (ht->scmp == NULL)
				{
					return 0;
				}
				break;
	default:	
				return 0;
	}
	ht->sord = ord;
	if (ord == Sort_ran)
	{
		return 0;
	}
	if (ht->nent == 0)
	{
		return 1;
	}
	if (!hash_create_entries(ht))
	{
		return 0;
	}
	qsort((void *) ht->sent, ht->nent,
		sizeof(SortEntry), hash_compare_entries);
	ht->stat = stt;

	return 1;
}
/*===========================================================================*/
/*     End of section 4: sorting functions.                                  */
/*===========================================================================*/


/*===========================================================================*/
/*     section 5: hash creation/deletion, I/O operations.                    */
/*===========================================================================*/
#define	HASH_IO_ID	"#Hash Binary Save 3.1"
int  hash_write(Hash ht, FILE *fp)
{
	if (ht == NULL || fp == NULL)
	{
		return 0;
	}
	if (fwrite(HASH_IO_ID, 1, sizeof(HASH_IO_ID), fp) != sizeof(HASH_IO_ID))
	{
		return 0;
	}
	if (fwrite(ht, sizeof(_Hash), 1, fp) != 1)
	{
		return 0;
	}
	if (!hash_compact(ht))
	{
		return 0;
	}
	if (fwrite(ht->cell, sizeof(HashCell), ht->nent, fp) != ht->nent)
	{
		return 0;
	}
	if (!rack_save(ht->rack, fp))
	{
		return 0;
	}
	if (fwrite(HASH_IO_ID, 1, sizeof(HASH_IO_ID), fp) != sizeof(HASH_IO_ID))
	{
		return 0;
	}
	fflush(fp);

	if (!hash_rehash(ht))
	{
		return 0;
	}
	if (ht->sent != NULL)
	{
		free(ht->sent);
		ht->sent = NULL;
	}
	ht->stat = Sort_normal;

	return 1;
}

Hash hash_read(FILE *fp)
{
	char	buff[80];
	Hash	ht;

	if (fp == NULL)
	{
		return NULL;
	}
	if (fread(buff, 1, sizeof(HASH_IO_ID), fp) != sizeof(HASH_IO_ID) ||
	    memcmp(buff, HASH_IO_ID, sizeof(HASH_IO_ID)))
	{
		return NULL;
	}
	if ((ht = (Hash) malloc(sizeof (_Hash))) == NULL)
	{
		return NULL;
	}
	if (fread(ht, sizeof(_Hash), 1, fp) != 1)
	{
		free(ht);
		return NULL;
	}
	if ((ht->cell = (HashCell *)calloc(ht->ment, sizeof(HashCell))) == NULL)
	{
		free(ht);
		return NULL;
	}
	if (fread(ht->cell, sizeof(HashCell), ht->nent, fp) != ht->nent)
	{
		free(ht->cell);
		free(ht);
		return NULL;
	}
	if ((ht->rack = rack_load(fp)) == NULL)
	{
		free(ht->cell);
		free(ht);
		return NULL;
	}
	if (fread(buff, 1, sizeof(HASH_IO_ID), fp) != sizeof(HASH_IO_ID) ||
	    memcmp(buff, HASH_IO_ID, sizeof(HASH_IO_ID)))
	{
		rack_clean(ht->rack);
		free(ht->cell);
		free(ht);
		return NULL;
	}

	if (!hash_rehash(ht))
	{
		rack_clean(ht->rack);
		free(ht->cell);
		free(ht);
		return NULL;
	}
	ht->nref = 1;	/* not for multiple reference even was when write */
	ht->slck = 0;
	ht->trav = 0;
	ht->cpck = 0;
	ht->stat = Sort_normal;
	ht->sent = NULL;
	ht->scmp = NULL;

	return ht;
}
#undef	HASH_IO_ID

/* save hash table string key and index pair into an open file */
/* in the format string_key<delm>integer_index\n */
int  hash_save(Hash ht, const char delm, FILE *fp)
{
	int	inx, keysize;
	char	key[BUFSIZ];

	if (ht == NULL || fp == NULL)
	{
		return 0;
	}
	hash_traverse_start(ht);
	while (hash_traverse_key(ht, key, &keysize, &inx))
	{
		if (keysize > 0 && key[keysize-1] != '\0')
		{
			key[keysize] = '\0';
		}
		if (fprintf(fp, "%s%c%d\n", key, delm, inx) <= 0)
		{
			hash_traverse_end(ht);
			return 0;
		}
	}
	hash_traverse_end(ht);
	fflush(fp);

	return 1;
}

/* create and read a hash table, string key and index only, */
/* from an open file pointer */
/* assume the format is string_key<delm>integer_index\n */
Hash hash_load(const char delm, FILE *fp)
{
	int	p, inx, ginx = 0;
	char	*str, buff[BUFSIZ];
	Hash	ht;

	if (fp == NULL)
	{
		return NULL;
	}
	if ((ht = hash_create(0, 0)) == NULL)
	{
		return NULL;
	}
	while (fgets(buff, BUFSIZ, fp) != NULL)
	{
		if ((str = strchr(buff, delm)) == NULL)
		{
			hash_delete(ht);
			return NULL;
		}
		*str++ = '\0';
		if ((inx = (int) atol(str)) <= 0)
		{
			hash_delete(ht);
			return NULL;
		}
		ginx = (ginx > inx) ? ginx : inx;
		while (ginx >= ht->ment)
		{
			if (!hash_expand(ht))
			{
				hash_delete(ht);
				return NULL;
			}
		}
		if (!hash_insert_position(ht, buff, strsize(buff), &p))
		{
			hash_delete(ht);
			return NULL;
		}
		ht->cell[p].index = inx;
		ht->cell[inx-1].pindex = p + 1;
	}
	ht->ginx = ginx;

	return ht;
}

/* increment reference counter of ht and return ht itself */
/* or NULL if counter reaches maximum value */
Hash hash_share(Hash ht)
{
	if (ht != NULL)
	{
		if (ht->nref >= UCHAR_MAX)
		{
			return NULL;
		}
		ht->nref++;
	}

	return ht;
}

/* clone a hash table and return new hash table */
/* or return NULL if something is wrong */
Hash hash_copy(Hash ht)
{
	int	rsiz;
	Hash	newht;

	if ((newht = (Hash) malloc(sizeof(_Hash))) == NULL)
	{
		return NULL;
	}
	memcpy((char *) newht, (char *) ht, sizeof(_Hash));
	rsiz = ht->ment * sizeof(HashCell);
	if ((newht->cell = (HashCell *) malloc(rsiz)) == NULL)
	{
		free(newht);
		return NULL;
	}
	memcpy((char *) newht->cell, (char *) ht->cell, rsiz);
	if ((newht->rack = rack_copy(ht->rack)) == NULL)
	{
		free(newht->cell);
		free(newht);
		return NULL;
	}
	if (ht->sent != NULL)
	{
		newht->sent = NULL;
		hash_create_entries(newht);
	}
	newht->nref = 1;
	newht->slck = 0;

	return newht;
}

Hash hash_create(const int size, const int dens)
{
	size_t		rsiz;
	Hash		ht;

	if ((ht = (Hash) calloc(1, sizeof(_Hash))) == NULL)
	{
		return NULL;
	}
	/* round size to next fas series number */
	ht->ment = HASH_DEFAULT_ENTRIES;
	ht->lent = HASH_DEFAULT_PREVOUS;
	while (ht->ment < size)
	{
		rsiz = ht->ment;
		ht->ment += ht->lent;
		ht->lent = rsiz;
	}
	if ((ht->cell = (HashCell *)calloc(ht->ment, sizeof(HashCell))) == NULL)
	{
		free(ht);
		return NULL;
	}
	if ((ht->rack = rack_setup((RACK_STORAGE_RATIO * ht->ment))) == NULL)
	{
		free(ht->cell);
		free(ht);
		return NULL;
	}
	ht->dens = ((dens > 0 && dens < 100) ? dens : HASH_DEFAULT_DENSITY);
	ht->nent = 0;
	ht->pent = ht->dens * ht->ment / 100;
	for (ht->nbyt = sizeof(int), rsiz = ht->ment-1; rsiz > 0; ht->nbyt--)
		rsiz >>= CHAR_BIT;
	ht->byte = sizeof(int) - ht->nbyt;

	ht->nref = 1;			/* only one user of this hashtable */
	ht->slck = 0;
	ht->ginx = 0;
	ht->trav = 0;
	ht->cpck = 0;
	ht->ndel = 0;
	ht->mdel = RACK_PURGING_RATIO * ht->ment;
	ht->stat = Sort_normal;		/* indicating how is sorted */
	ht->sord = Sort_ran;		/* sorting order is random. */
	ht->sent = NULL;		/* sorting entries. */
	ht->scmp = NULL;		/* sorting comparator. */

	return ht;
}

int  hash_delete(Hash ht)
{
	int	ment = 0;

	if (ht != NULL)
	{
		ment = ht->ment;
		if (ht->nref <= 1)
		{
			rack_clean(ht->rack);
			if (ht->cell != NULL)
			{
				free(ht->cell);
			}
			if (ht->sent != NULL)
			{
				free(ht->sent);
			}
			free(ht);
		}
		else
		{
			ht->nref--;
		}
	}

	return ment;
}
/*===========================================================================*/
/*     End of section 5: hash creation/deletion, I/O operations.             */
/*===========================================================================*/

/*===========================================================================*/
/*     section 6: insertion functions.                                       */
/*===========================================================================*/
int  hash_insert_key(Hash ht, const char *key, const int keysize, int *inx)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_insert_position(ht, key, keysize, &p)))
	{
		if (inx != NULL)
		{
			*inx = ht->cell[p].index;
		}
	}

	return rtn;
}

int  hash_insert_data(Hash ht, const char *key, const int keysize, int *inx,
	char *data, int datasize)
{
	int	p, handle, hook, rtn;

	if (ht == NULL || datasize <= 0 || data == NULL)
	{
		return 0;
	}
	if ((rtn = hash_insert_position(ht, key, keysize, &p)))
	{
		hook = ht->cell[p].dhook;
		handle = rack_hook(ht->rack, &hook, data, datasize);
		if (handle < 0)
		{
			return 0;
		}
		if (ht->cell[p].dhandle == -1)
		{
			ht->cell[p].dhandle = handle;
		}
		ht->cell[p].dhook = hook;
		ht->cpck = handle;
		if (inx != NULL)
		{
			*inx = ht->cell[p].index;
		}
	}

	return rtn;
}

/*===========================================================================*/
/* insert only if the key has not been inserted.                             */
/*===========================================================================*/
int  hash_insert_key_unique(Hash ht, const char *key, const int keysize,
	int *inx)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if (hash_key_search_position(ht, key, keysize, &p))
	{
		rtn = 0;
	}
	else if ((rtn = hash_insert_position(ht, key, keysize, &p)))
	{
		if (inx != NULL)
		{
			*inx = ht->cell[p].index;
		}
	}

	return rtn;
}

/*===========================================================================*/
/* insert only if the key has been inserted.                                 */
/*===========================================================================*/
int  hash_insert_key_multiple(Hash ht, const char *key, const int keysize, int *inx)
{
	int		p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if (!hash_key_search_position(ht, key, keysize, &p))
	{
		rtn = 0;
	}
	else if ((rtn = hash_insert_position(ht, key, keysize, &p)))
	{
		if (inx != NULL)
		{
			*inx = ht->cell[p].index;
		}
	}

	return rtn;
}
/*===========================================================================*/
/*     End of section 6: insertion functions.                                */
/*===========================================================================*/

/*===========================================================================*/
/*     section 7: retrieving functions.                                      */
/*===========================================================================*/
int  hash_search_index(Hash ht, const char *key, const int keysize, int *inx)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_key_search_position(ht, key, keysize, &p)))
	{
		if (inx != NULL)
		{
			*inx = ht->cell[p].index;
		}
	}

	return rtn;
}

int  hash_search_key(Hash ht, int inx, char *key, int *keysize)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_index_search_position(ht, inx, &p)))
	{
		if (keysize != NULL)
		{
			*keysize = ht->cell[p].keysize;
		}
		if (key != NULL)
		{
			rack_off(ht->rack, ht->cell[p].khandle,
				key, ht->cell[p].keysize);
		}
	}

	return rtn;
}

int  hash_search_key_ptr(Hash ht, int inx, char **keyptr, int *keysize)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_index_search_position(ht, inx, &p)))
	{
		if (keysize != NULL)
		{
			*keysize = ht->cell[p].keysize;
		}
		if (keyptr != NULL)
		{
			*keyptr = rack_off_ptr(ht->rack, ht->cell[p].khandle);
		}
	}

	return rtn;
}

int  hash_key_search_data(Hash ht, const char *key, const int keysize,
	char *data, int *datasize)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_key_search_position(ht, key, keysize, &p)))
	{
		if (datasize != NULL)
		{
			*datasize = rack_pick(ht->rack, &ht->cpck, data);
		}
		else
		{
			rack_pick(ht->rack, &ht->cpck, data);
		}
	}

	return rtn;
}

int  hash_key_search_data_ptr(Hash ht, const char *key, const int keysize,
	char **dataptr, int *datasize)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_key_search_position(ht, key, keysize, &p)))
	{
		if (datasize != NULL)
		{
			*datasize = rack_pick_ptr(ht->rack, &ht->cpck, dataptr);
		}
		else
		{
			rack_pick_ptr(ht->rack, &ht->cpck, dataptr);
		}
	}

	return rtn;
}

int  hash_key_search_ndata(Hash ht, const char *key, const int keysize,
	int *ndata, char **data, int *datasize)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_key_search_position(ht, key, keysize, &p)))
	{
		if (ndata != NULL)
		{
			*ndata = rack_pick_all(ht->rack,
				ht->cpck, data, datasize);
		}
		else
		{
			rack_pick_all(ht->rack, ht->cpck, data, datasize);
		}
	}

	return rtn;
}

int  hash_key_search_ndata_ptr(Hash ht, const char *key, const int keysize,
	int *ndata, char **dataptr, int *datasize)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_key_search_position(ht, key, keysize, &p)))
	{
		if (ndata != NULL)
		{
			*ndata = rack_pick_all_ptr(ht->rack,
				ht->cpck, dataptr, datasize);
		}
		else
		{
			rack_pick_all_ptr(ht->rack,
				ht->cpck, dataptr, datasize);
		}
	}

	return rtn;
}

int  hash_index_search_data(Hash ht, int inx, char *data, int *datasize)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_index_search_position(ht, inx, &p)))
	{
		if (datasize != NULL)
		{
			*datasize = rack_pick(ht->rack, &ht->cpck, data);
		}
		else
		{
			rack_pick(ht->rack, &ht->cpck, data);
		}
	}

	return rtn;
}

int  hash_index_search_data_ptr(Hash ht, int inx, char **dataptr, int *datasize)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_index_search_position(ht, inx, &p)))
	{
		if (datasize != NULL)
		{
			*datasize = rack_pick_ptr(ht->rack, &ht->cpck, dataptr);
		}
		else
		{
			rack_pick_ptr(ht->rack, &ht->cpck, dataptr);
		}
	}

	return rtn;
}

int  hash_index_search_ndata(Hash ht, int inx,
	int *ndata, char **data, int *datasize)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_index_search_position(ht, inx, &p)))
	{
		if (ndata != NULL)
		{
			*ndata = rack_pick_all(ht->rack,
				ht->cpck, data, datasize);
		}
		else
		{
			rack_pick_all(ht->rack, ht->cpck, data, datasize);
		}
	}

	return rtn;
}

int  hash_index_search_ndata_ptr(Hash ht, int inx,
	int *ndata, char **dataptr, int *datasize)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_index_search_position(ht, inx, &p)))
	{
		if (ndata != NULL)
		{
			*ndata = rack_pick_all_ptr(ht->rack,
				ht->cpck, dataptr, datasize);
		}
		else
		{
			rack_pick_all_ptr(ht->rack, ht->cpck,
				dataptr, datasize);
		}
	}

	return rtn;
}
/*===========================================================================*/
/*     End of section 7: retrieving functions.                               */
/*===========================================================================*/


/*===========================================================================*/
/*     section 8: modification functions.                                    */
/*===========================================================================*/

/*===========================================================================*/
/* if found an entry match the key and the entry has data already,           */
/* replace the old data with the one passed in. otherwise, this function     */
/* is equivalent to hash_insert_dada.                                        */
/* in this way, you always get one piece of data per key in the table.       */
/*                                                                           */
/* often a search key and insert key combination(conditional insert key)     */
/* appears in a program. it is inefficient to have 2 calls operation.        */
/*===========================================================================*/
int  hash_locate_key(Hash ht, const char *key, const int keysize, int *inx)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_insert_position(ht, key, keysize, &p)))
	{
		if (rtn >= 2)
		{
			ht->cell[p].counts--;
			rtn = 2;
		}
		else
		{
			rtn = 1;
		}
		if (inx != NULL)
		{
			*inx = ht->cell[p].index;
		}
	}

	return rtn;
}

/*===========================================================================*/
/* if key/keysize is in talbe, and it has data already, replace the old      */
/* data with the one passed in, and return the key counts.                   */
/* if key/keysize is not in table, insert the key/keysize, set *inx, and     */
/* attach the data to the entry. (hash_insert_data())                        */
/*                                                                           */
/* by calling this function, key always gets one piece of data in the table. */
/*===========================================================================*/
int  hash_key_replace_data(Hash ht, const char *key, const int keysize,
	int *inx, char *data, int datasize)
{
	int		handle, hook = -1;
	int		p, rtn;
	HashCell	*cell;

	if (ht == NULL || datasize <= 0 || data == NULL)
	{
		return 0;
	}
	if ((rtn = hash_key_search_position(ht, key, keysize, &p)))
	{
		cell = ht->cell + p;
		handle = rack_hook(ht->rack, &hook, data, datasize);
		if (handle < 0)
		{
			return 0;
		}
		cell->dhandle = handle;
		cell->dhook = hook;
		ht->cpck = handle;
		if (inx != NULL)
		{
			*inx = cell->index;
		}
	}
	else if ((rtn = hash_insert_position(ht, key, keysize, &p)))
	{
		cell = ht->cell + p;
		hook = cell->dhook;
		handle = rack_hook(ht->rack, &hook, data, datasize);
		if (handle < 0)
		{
			return 0;
		}
		if (cell->dhandle == -1)
		{
			cell->dhandle = handle;
		}
		cell->dhook = hook;
		ht->cpck = handle;
		if (inx != NULL)
		{
			*inx = cell->index;
		}
	}

	return rtn;
}

/*===========================================================================*/
/* if found an entry match the key, the entry has data already, and the      */
/* first data has the same size as passed-in datasize, replace it with the   */
/* data passed in, otherwise return failed.                                  */
/* useful when get a data and update it and put it back in.                  */
/*===========================================================================*/
int  hash_key_update_data(Hash ht, const char *key, const int keysize,
	char *data, int datasize)
{
	char	*dataptr;
	int	p, size, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_key_search_position(ht, key, keysize, &p)))
	{
		size = rack_pick_ptr(ht->rack, &ht->cpck, &dataptr);
		if (size == datasize)
		{
			memcpy(dataptr, data, size);
			ht->cpck = ht->cell[p].dhandle;
		}
		else
		{
			rtn = 0;
		}
	}

	return rtn;
}

int  hash_index_update_data(Hash ht, int inx, char *data, int datasize)
{
	char	*dataptr;
	int	p, size, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_index_search_position(ht, inx, &p)))
	{
		size = rack_pick_ptr(ht->rack, &ht->cpck, &dataptr);
		if (size == datasize)
		{
			memcpy(dataptr, data, size);
			ht->cpck = ht->cell[p].dhandle;
		}
		else
		{
			rtn = 0;
		}
	}

	return rtn;
}

/*===========================================================================*/
/* search a key in the table. found :                                        */
/* (1). with data: pass out the data and return 2.                           */
/* (2). no data but exists: insert the passed-in data and return 1.          */
/* (3). not exists: insert the key and the passed-in data and return 1.      */
/* (4). cannot insert data or key and data: return 0.                        */
/* useful for conditional insertion (insertion depends on retrieving).       */
/* the hash_index_unique_data works slightly different. it takes care of     */
/* case (1) and (2) and treats (3) as (4).                                   */
/*===========================================================================*/
int  hash_key_unique_data(Hash ht, const char *key, const int keysize,
	char *data, int insize, char **dataptr, int *outsize)
{
	int		p, size, handle, hook, rtn = 1;
	HashCell	*cell;

	if (ht == NULL)
	{
		return 0;
	}
	if (hash_key_search_position(ht, key, keysize, &p))
	{
		handle = ht->cpck;
		size = rack_pick_ptr(ht->rack, &handle, dataptr);
		if (size > 0)
		{
			if (outsize != NULL)
			{
				*outsize = size;
			}
			return 2;
		}

		/* no data yet, insert the new */
		cell = ht->cell + p;
		cell->dhandle = rack_hook(ht->rack, &cell->dhook, data, insize);
		if (cell->dhandle < 0)
		{
			return 0;
		}
		handle = cell->dhandle;
		size = rack_pick_ptr(ht->rack, &handle, dataptr);
		if (outsize != NULL)
		{
			*outsize = size;
		}
		ht->cpck = cell->dhandle;
	}
	else if (hash_insert_position(ht, key, keysize, &p))
	{
		cell = ht->cell + p;
		hook = cell->dhook;
		handle = rack_hook(ht->rack, &hook, data, insize);
		if (handle < 0)
		{
			return 0;
		}
		if (cell->dhandle == -1)
		{
			cell->dhandle = handle;
		}
		cell->dhook = hook;
		handle = cell->dhandle;
		rack_pick_ptr(ht->rack, &handle, dataptr);
		if (outsize != NULL)
		{
			*outsize = insize;
		}
		ht->cpck = ht->cell[p].dhandle;
	}
	else
	{
		rtn = 0;
	}

	return rtn;
}

int  hash_index_unique_data(Hash ht, int inx,
	char *data, int insize, char **dataptr, int *outsize)
{
	int		p, size, handle, rtn = 1;
	HashCell	*cell;

	if (ht == NULL)
	{
		return 0;
	}
	if (hash_index_search_position(ht, inx, &p))
	{
		handle = ht->cpck;
		size = rack_pick_ptr(ht->rack, &handle, dataptr);
		if (size > 0)
		{
			if (outsize != NULL)
			{
				*outsize = size;
			}
			return 2;
		}

		/* no data yet, insert the new */
		cell = ht->cell + p;
		handle = rack_hook(ht->rack, &cell->dhook, data, insize);
		if (handle < 0)
		{
			return 0;
		}
		cell->dhandle = handle;
		size = rack_pick_ptr(ht->rack, &handle, dataptr);
		if (outsize != NULL)
		{
			*outsize = size;
		}
		ht->cpck = cell->dhandle;
	}
	else
	{
		rtn = 0;
	}

	return rtn;
}

int  hash_key_remove_key(Hash ht, const char *key, const int keysize)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_key_search_position(ht, key, keysize, &p)))
	{
		ht->cell[p].counts = 0;
		ht->cell[p].dhandle = -1;
		ht->cell[p].dhook = -1;
		ht->cell[ht->cell[p].index-1].pindex = 0;
		ht->nent--;
		if (++(ht->ndel) >= ht->mdel &&
			ht->sent == NULL &&
			ht->slck == 0)
		{
			hash_clean(ht);
		}
	}

	return rtn;
}

int  hash_key_remove_data(Hash ht, const char *key, const int keysize)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_key_search_position(ht, key, keysize, &p)))
	{
		ht->cell[p].dhandle = rack_unhook(ht->rack,
			ht->cell[p].dhandle, &ht->cell[p].dhook, 1);
		if (ht->cell[p].counts > 1)
		{
			ht->cell[p].counts--;
		}
		if (++(ht->ndel) >= ht->mdel &&
			ht->sent == NULL &&
			ht->slck == 0)
		{
			hash_clean(ht);
		}
	}

	return rtn;
}

int  hash_key_remove_ndata(Hash ht, const char *key, const int keysize)
{
	int		p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_key_search_position(ht, key, keysize, &p)))
	{
		ht->cell[p].counts = 1;
		ht->cell[p].dhandle = -1;
		ht->cell[p].dhook = -1;
		if (++(ht->ndel) >= ht->mdel &&
			ht->sent == NULL &&
			ht->slck == 0)
		{
			hash_clean(ht);
		}
	}

	return rtn;
}

int  hash_index_remove_key(Hash ht, int inx)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_index_search_position(ht, inx, &p)))
	{
		ht->cell[p].counts = 0;
		ht->cell[p].dhandle = -1;
		ht->cell[p].dhook = -1;
		ht->cell[inx].pindex = 0;
		ht->nent--;
		if (++(ht->ndel) >= ht->mdel &&
			ht->sent == NULL &&
			ht->slck == 0)
		{
			hash_clean(ht);
		}
	}

	return rtn;
}

int  hash_index_remove_data(Hash ht, int inx)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_index_search_position(ht, inx, &p)))
	{
		ht->cell[p].dhandle = rack_unhook(ht->rack,
			ht->cell[p].dhandle, &ht->cell[p].dhook, 1);
		if (ht->cell[p].counts > 1)
		{
			ht->cell[p].counts--;
		}
		if (++(ht->ndel) >= ht->mdel &&
			ht->sent == NULL &&
			ht->slck == 0)
		{
			hash_clean(ht);
		}
	}

	return rtn;
}

int  hash_index_remove_ndata(Hash ht, int inx)
{
	int	p, rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if ((rtn = hash_index_search_position(ht, inx, &p)))
	{
		ht->cell[p].counts = 1;
		ht->cell[p].dhandle = -1;
		ht->cell[p].dhook = -1;
		if (++(ht->ndel) >= ht->mdel &&
			ht->sent == NULL &&
			ht->slck == 0)
		{
			hash_clean(ht);
		}
	}

	return rtn;
}
/*===========================================================================*/
/*     End of section 8: modification functions.                             */
/*===========================================================================*/

/*===========================================================================*/
/*     section 9: traversing functions.                                      */
/*===========================================================================*/
int  hash_traverse_start(Hash ht)
{
	int	rtn;

	if (ht == NULL)
	{
		return 0;
	}
	if (ht->slck == 1 || ht->nent == 0)
	{
		return 0;
	}
	ht->trav = 0;
	rtn = hash_max_keysize(ht);
	ht->slck = 1;
	/* notice : DO NOT UNLOCK!. */
	/* it is safe, though it is not efficient */

	return rtn;
}

int  hash_traverse_end(Hash ht)
{
	int	rtn;

	if (ht == NULL || ht->slck == 0)
	{
		return 0;
	}
	rtn = ht->trav;
	ht->trav = 0;
	ht->slck = 0;

	return rtn;
}

int  hash_traverse_key(Hash ht, char *key, int *keysize, int *inx)
{
	int	p, rtn;

	if (ht == NULL || ht->slck == 0)
	{
		return 0;
	}
	if ((rtn = hash_traverse_position(ht, &p)))
	{
		rack_off(ht->rack, ht->cell[p].khandle,
			key, ht->cell[p].keysize);
		if (keysize != NULL)
		{
			 *keysize = ht->cell[p].keysize;
		}
		if (inx != NULL)
		{
			*inx = ht->cell[p].index;
		}
	}

	return rtn;
}

int  hash_traverse_key_ptr(Hash ht, char **keyptr, int *keysize, int *inx)
{
	int	p, rtn;

	if (ht == NULL || ht->slck == 0)
	{
		return 0;
	}
	if ((rtn = hash_traverse_position(ht, &p)))
	{
		*keyptr = rack_off_ptr(ht->rack, ht->cell[p].khandle);
		if (keysize != NULL)
		{
			*keysize = ht->cell[p].keysize;
		}
		if (inx != NULL)
		{
			*inx = ht->cell[p].index;
		}
	}

	return rtn;
}

int  hash_traverse_data(Hash ht, char *key, int *keysize, int *inx,
	char *data, int *datasize)
{
	int	p, rtn;

	if (ht == NULL || ht->slck == 0)
	{
		return 0;
	}
	if ((rtn = hash_traverse_position(ht, &p)))
	{
		rack_off(ht->rack, ht->cell[p].khandle,
			key, ht->cell[p].keysize);
		if (datasize != NULL)
		{
			*datasize = rack_pick(ht->rack, &ht->cpck, data);
		}
		else
		{
			rack_pick(ht->rack, &ht->cpck, data);
		}
		if (keysize != NULL)
		{
			*keysize = ht->cell[p].keysize;
		}
		if (inx != NULL)
		{
			*inx = ht->cell[p].index;
		}
	}

	return rtn;
}

int  hash_traverse_data_ptr(Hash ht, char **keyptr, int *keysize, int *inx,
	char **dataptr, int *datasize)
{
	int	p, rtn;

	if (ht == NULL || ht->slck == 0)
	{
		return 0;
	}
	if ((rtn = hash_traverse_position(ht, &p)))
	{
		*keyptr = rack_off_ptr(ht->rack, ht->cell[p].khandle);
		if (datasize != NULL)
		{
			*datasize = rack_pick_ptr(ht->rack, &ht->cpck, dataptr);
		}
		else
		{
			rack_pick_ptr(ht->rack, &ht->cpck, dataptr);
		}
		if (keysize != NULL)
		{
			*keysize = ht->cell[p].keysize;
		}
		if (inx != NULL)
		{
			*inx = ht->cell[p].index;
		}
	}

	return rtn;
}

int  hash_traverse_ndata(Hash ht, char *key, int *keysize, int *inx,
	int *ndata, char **dataptr, int *datasize)
{
	int	p, rtn;

	if (ht == NULL || ht->slck == 0)
	{
		return 0;
	}
	if ((rtn = hash_traverse_position(ht, &p)))
	{
		rack_off(ht->rack, ht->cell[p].khandle,
			key, ht->cell[p].keysize);
		if (ndata != NULL)
		{
			*ndata = rack_pick_all(ht->rack, ht->cell[p].dhandle,
				dataptr, datasize);
		}
		else
		{
			rack_pick_all(ht->rack, ht->cell[p].dhandle,
				dataptr, datasize);
		}
		if (keysize != NULL)
		{
			*keysize = ht->cell[p].keysize;
		}
		if (inx != NULL)
		{
			*inx = ht->cell[p].index;
		}
	}

	return rtn;
}

int  hash_traverse_ndata_ptr(Hash ht, char **keyptr, int *keysize, int *inx,
	int *ndata, char **dataptr, int *datasize)
{
	int	p, rtn;

	if (ht == NULL || ht->slck == 0)
	{
		return 0;
	}
	if ((rtn = hash_traverse_position(ht, &p)))
	{
		*keyptr = rack_off_ptr(ht->rack, ht->cell[p].khandle);
		if (ndata != NULL)
		{
			*ndata = rack_pick_all_ptr(ht->rack,
				ht->cell[p].dhandle, dataptr, datasize);
		}
		else
		{
			rack_pick_all_ptr(ht->rack, ht->cell[p].dhandle,
				dataptr, datasize);
		}
		if (keysize != NULL)
		{
			*keysize = ht->cell[p].keysize;
		}
		if (inx != NULL)
		{
			*inx = ht->cell[p].index;
		}
	}

	return rtn;
}

/* the following two functions are thread-unsafe. use it only in single */
/* threaded environment, or between hash_traverse_start/end.              */
int  hash_next_data(Hash ht, char *data)
{
	int	rtn;

	if ((rtn = rack_pick(ht->rack, &ht->cpck, data)) == -1)
	{
		rtn = 0;
	}

	return rtn;
}

int  hash_next_data_ptr(Hash ht, char **dataptr)
{
	int	rtn;

	if ((rtn = rack_pick_ptr(ht->rack, &ht->cpck, dataptr)) == -1)
	{
		rtn = 0;
	}

	return rtn;
}

int  hash_walk(Hash ht, HashProc proc, void *in, void *out)
{
	int		p, step;
	HashCell	*cell;
	SortEntry	sent;

	if (ht == NULL || ht->slck == 1)
	{
		return 0;
	}
	ht->trav = step = 0;
	while (hash_traverse_position(ht, &p))
	{
		cell = ht->cell + p;
		sent.object = (char *) ht;
		sent.position = p;
		sent.pindex = cell->pindex;
		sent.index = cell->index;
		sent.counts = cell->counts;
		sent.key = rack_off_ptr(ht->rack, cell->khandle);
		sent.keysize = cell->keysize;
		sent.ndata = rack_counts(ht->rack, cell->dhandle);
		sent.datasize = rack_pick_ptr(ht->rack, &ht->cpck, &sent.data);
		if ((*proc)(&sent, ht->nent, step, in, out) == 0)
		{
			break;
		}
		step++;
	}

	return step;
}

int  hash_edit(Hash ht, HashProc proc, void *in, void *out)
{
	int		p, step, handle, hook;
	Rack		rack;
	HashCell	*cell;
	SortEntry	sent, sent_copy;

	if (ht == NULL || ht->slck == 1)
	{
		return 0;
	}
	rack = ht->rack;
	ht->trav = step = 0;
	while (hash_traverse_position(ht, &p))
	{
		cell = ht->cell + p;
		sent.object = (char *) ht;
		sent.position = p;
		sent.pindex = cell->pindex;
		sent.index = cell->index;
		sent.counts = cell->counts;
		sent.key = rack_off_ptr(ht->rack, cell->khandle);
		sent.keysize = cell->keysize;
		sent.ndata = rack_counts(ht->rack, cell->dhandle);
		sent.datasize = rack_pick_ptr(ht->rack, &ht->cpck, &sent.data);
		memcpy((char *) &sent_copy, (char *) &sent, sizeof(SortEntry));
		if ((*proc)(&sent, ht->nent, step, in, out) == 0)
		{
			break;
		}
		/* editing modified items in sent_copy. */
		/* sent entry object position */
		/* are read-only items. */
		cell->pindex = sent.pindex;
		cell->index = sent.index;
		cell->counts = sent.counts;
		if (sent.keysize != sent_copy.keysize ||
			sent.key != sent_copy.key)
		{
			if ((cell->khandle = rack_on(rack,
				sent.key, sent.keysize)) < 0)
			{
				return 0;
			}
			cell->keysize = sent.keysize;
		}
		/* sent.ndata is basically read-only item except */
		/* that it can be set to zero for eliminating data.   */
		if (sent.ndata != sent_copy.ndata && sent.ndata == 0)
		{
			cell->dhandle = -1;
			cell->dhook = -1;
		}
		if (sent.datasize != sent_copy.datasize ||
			sent.data != sent_copy.data)
		{
			hook = -1;
			if ((handle = rack_hook(ht->rack, &hook,
				sent.data, sent.datasize)) < 0)
			{
				return 0;
			}
			cell->dhandle = handle;
			cell->dhook = hook;
			ht->cpck = handle;
		}
		step++;
	}
	hash_clean(ht);

	return step;
}
/*===========================================================================*/
/*     End of section 9: traversing functions.                               */
/*===========================================================================*/

/*===========================================================================*/
/*     section 10: querying functions.                                       */
/*===========================================================================*/
int  hash_max_index(Hash ht)
{
	int	rtn;

	if (ht == NULL)
	{
		return 0;
	}
	rtn = ht->ginx;

	return rtn;
}

int  hash_num_keys(Hash ht)
{
	int	rtn;

	if (ht == NULL)
	{
		return 0;
	}
	rtn = ht->nent;

	return rtn;
}

int  hash_max_keys(Hash ht)
{
	int	rtn;

	if (ht == NULL)
	{
		return 0;
	}
	rtn = ht->ment;

	return rtn;
}

int  hash_max_keysize(Hash ht)
{
	int	i, max_keysize = 0;

	if (ht == NULL)
	{
		return 0;
	}
	for (i = 0; i < ht->ment; i++)
	{
		if (ht->cell[i].counts)
		{
			if (max_keysize < ht->cell[i].keysize)
			{
				max_keysize = ht->cell[i].keysize;
			}
		}
	}

	return	max_keysize;
}

int  hash_sum_ndata(Hash ht)
{
	int		i, num_ndata = 0;

	if (ht == NULL)
	{
		return 0;
	}
	for (i = 0; i < ht->ment; i++)
	{
		num_ndata += ht->cell[i].counts;
	}

	return	num_ndata;
}

int  hash_max_ndata(Hash ht)
{
	int		i, max_ndata = 0;

	if (ht == NULL)
	{
		return 0;
	}
	for (i = 0; i < ht->ment; i++)
	{
		if (max_ndata < ht->cell[i].counts)
		{
			max_ndata = ht->cell[i].counts;
		}
	}

	return	max_ndata;
}

int  hash_generate_hashkey (Hash ht, char *key, int keysize)
{
	int	k = 0;

	if (ht == NULL)
	{
		return 0;
	}
	return hash_xkey(ht, key, keysize, 0, &k);
}
/*===========================================================================*/
/*     End of section 10: querying functions.                                */
/*===========================================================================*/
int  sm_inx(Hash sm, const char key[])
{
	int	inx;
	if (!hash_insert_key(sm, key, strsize(key), &inx))
	{
		fprintf(stderr, "failed to insert key in sm_inx()\n");
		exit(1);
	}
	return inx;
}

int  sm_ninx(Hash sm, const char key[], int *n)
{
	int	inx;
	if (!((*n) = hash_insert_key(sm, key, strsize(key), &inx)))
	{
		fprintf(stderr, "failed to insert key in sm_inx()\n");
		exit(1);
	}
	return inx;
}

int  sm_sinx(Hash sm, const char key[])
{
	int	inx;
	if (!hash_search_index(sm, key, strsize(key), &inx))
	{
		return 0;
	}
	return inx;
}

int  sm_cnt(Hash sm, const char key[])
{
	return hash_search_index(sm, key, strsize(key), NULL);
}

char *sm_key(Hash sm, const int inx)
{
	char	*p;
	if (!hash_search_key_ptr(sm, inx, &p, NULL))
	{
		return NULL;
	}
	return p;
}

int  im_inx(Hash im, const int key)
{
	int	inx;
	if (!hash_insert_key(im, (char *)&key, sizeof(int), &inx))
	{
		fprintf(stderr, "failed to insert key in im_inx()\n");
		exit(1);
	}
	return inx;
}

int  im_ninx(Hash im, const int key, int *n)
{
	int	inx;
	if (!((*n) = hash_insert_key(im, (char *)&key, sizeof(int), &inx)))
	{
		fprintf(stderr, "failed to insert key in im_inx()\n");
		exit(1);
	}
	return inx;
}

int  im_sinx(Hash im, const int key)
{
	int	inx;
	if (!hash_search_index(im, (char *)&key, sizeof(int), &inx))
	{
		return 0;
	}
	return inx;
}

int  im_cnt(Hash im, const int key)
{
	return hash_search_index(im, (char *)&key, sizeof(int), NULL);
}

int  im_key(Hash im, const int inx)
{
	int	*p;
	if (!hash_search_key_ptr(im, inx, (char **)&p, NULL))
	{
		return INT_MAX;
	}
	return (*p);
}

/*===========================================================================*/
/*     section 11: hash convenient functions.                                */
/*===========================================================================*/
/*===========================================================================*/
/* compose_front().                                                          */
/* compose_end().                                                            */
/* decompose().                                                              */
/*                                                                           */
/* composed key look like :                                                  */
/* key(n) + ksiz(n) + key(n-1) + ksiz(n-1) + ...                             */
/* backward storage for fast decomposing.                                    */
/*                                                                           */
/* zero length subkey is not allowed (return 0 indicate failure).            */
/* zero length compkey means start a new combination process.                */
/* result is undefined if subkey and compkey overlap in space.               */
/* result is undefined if compkey does not have enough space.                */
/*===========================================================================*/
/* return consequent key size, put skey in front of ckey */
int  compose_front(char *ckey, int csiz, char *skey, int ssiz)
{
	int	rsiz;

	if (csiz < 0 || ssiz < 0)
	{
		return -1;
	}

	/* in the front means put the skey at the end */
	rsiz = csiz + ssiz;
	memmove (ckey + csiz, skey, ssiz);
	memmove (ckey + rsiz, (char *) &ssiz, sizeof(int));

	return rsiz + sizeof (int);
}

int  compose_end(char *ckey, int csiz, char *skey, int ssiz)
{
	int	rsiz;

	if (csiz < 0 || ssiz < 0)
	{
		return -1;
	}

	/* in the end means put the skey in the front. */
	/* hope that memmove do right thing for overlapping memory copy */
	rsiz = ssiz + sizeof(int);
	memmove (ckey + rsiz, ckey, csiz);
	memmove (ckey, skey, ssiz);
	memmove (ckey + ssiz, (char *) &ssiz, sizeof(int));

	return csiz + rsiz;
}

/* return sub key size or 0 (no more keys) */
int  decompose(char *ckey, int *csiz, char **skey)
{
	int	ssiz;
	int	cs = *csiz - sizeof(int);

	if (cs < sizeof(int))
	{
		return -1;
	}
	memcpy((char *) &ssiz, ckey + cs, sizeof(int));
	cs -= ssiz;
	*csiz = cs;
	*skey = ckey + cs;

	return ssiz;
}

/*===========================================================================*/
/* combine_front ().                                                         */
/* combine_end ().                                                           */
/* decombine ().                                                             */
/*                                                                           */
/* compare to hash_(de)compose, save 3 bytes per subkey. (run slower)        */
/* d means delimeter, it has to be selected that it does appear in any key.  */
/* composed key look like :                                                  */
/* d + key(n) + d + key(n-1) + ...  (not ended with d)                       */
/* backward storage for fast decomposing.                                    */
/*                                                                           */
/* zero length subkey is not allowed (return 0 indicate failure).            */
/* zero length compkey means start a new combination process.                */
/* result is undefined if subkey and compkey overlap in space.               */
/* result is undefined if compkey does not have enough space.                */
/*===========================================================================*/
int  combine_front(char d, char *ckey, int csiz, char *skey, int ssiz)
{
	int	rsiz;

	if (csiz < 0 || ssiz < 0)
	{
		return -1;
	}

	/* in the front means put the skey at the end */
	rsiz = csiz + 1;
	memset (ckey + csiz, d, 1);
	memmove (ckey + rsiz, skey, ssiz);

	return rsiz + ssiz;
}

int  combine_end(char d, char *ckey, int csiz, char *skey, int ssiz)
{
	int	rsiz;

	if (csiz < 0 || ssiz < 0)
	{
		return -1;
	}

	/* in the end means put the skey in the front */
	/* hope that memmove do right thing for overlapping memory copy */
	rsiz = ssiz + 1;
	memmove (ckey + rsiz, ckey, csiz);
	memset (ckey, d, 1);
	memmove (ckey + 1, skey, ssiz);

	return csiz + rsiz;
}

int  decombine(char d, char *ckey, int *csiz, char **skey)
{
	int	posi = *csiz - 1;
	int	ssiz;

	if (posi <= 0)
	{
		return -1;
	}

	while (posi >= 0 && ckey[posi] != d)
		posi--;
	(*skey) = ckey + posi + 1;
	ssiz = *csiz - posi - 1;
	*csiz = posi;

	return ssiz;
}

/*===========================================================================*/
/* paste_front ().                                                           */
/* paste_end ().                                                             */
/*                                                                           */
/* simply copy two keys together adjacently.                                 */
/* works for recombine(without delimeter) two combined or composed keys.     */
/*===========================================================================*/
/* return consequent key size, put skey in front of ckey */
int  paste_front(char *ckey, int csiz, char *skey, int ssiz)
{
	if (csiz < 0 || ssiz < 0)
	{
		return -1;
	}

	/* in the front means put the skey at the end */
	/* so it will be retrieved earlier (as if     */
	/* they put in earlier).                      */
	memmove (ckey + csiz, skey, ssiz);

	return csiz + ssiz;
}

int  paste_end(char *ckey, int csiz, char *skey, int ssiz)
{
	if (csiz < 0 || ssiz < 0)
	{
		return -1;
	}

	/* in the end means put the skey in the front. */
	/* hope that memmove do right thing for overlapping memory copy */
	memmove (ckey + ssiz, ckey, csiz);
	memmove (ckey, skey, ssiz);

	return csiz + ssiz;
}
/*===========================================================================*/
/*     End of section 11: hash convenient functions.                         */
/*===========================================================================*/





/*===========================================================================*/
/*     section 12: assoc functions.                                          */
/*===========================================================================*/
Assoc iassoc_create(int miss_handle, int miss_default)
{
	Assoc		aa;

	if ((aa = (Assoc) malloc(sizeof(_Assoc))) == NULL)
	{
		return NULL;
	}
	if ((aa->sdef = (char *)malloc(strsize("_missing_"))) == NULL)
	{
		free(aa);
		return NULL;
	}
	if ((aa->ht = hash_create(0, 0)) == NULL)
	{
		free(aa->sdef);
		free(aa);
		return NULL;
	}
	aa->styp = Assoc_integer;
	aa->miss = miss_handle;
	aa->idef = miss_default;
	aa->indx = miss_default + 1;
	strcpy(aa->sdef, "_missing_");

	return aa;
}

Assoc sassoc_create(int miss_handle, char *miss_default)
{
	Assoc		aa;

	if ((aa = (Assoc) malloc(sizeof(_Assoc))) == NULL)
	{
		return NULL;
	}
	if (miss_default != NULL)
	{
		if ((aa->sdef = (char *)malloc(strsize(miss_default))) == NULL)
		{
			free(aa);
			return NULL;
		}
		strcpy(aa->sdef, miss_default);
	}
	else
	{
		aa->sdef = NULL;
	}
	if ((aa->ht = hash_create(0, 0)) == NULL)
	{
		if (aa->sdef != NULL)
			free(aa->sdef);
		free(aa);
		return NULL;
	}
	aa->styp = Assoc_string;
	aa->miss = miss_handle;
	aa->indx = 1;
	aa->idef = 0;

	return aa;
}

int  iassoc_delete(Assoc aa)
{
	int	ment = 0;

	if (aa != NULL)
	{
		ment = hash_delete(aa->ht);
		if (aa->sdef != NULL)
			free(aa->sdef);
		free(aa);
	}

	return ment;
}

int  sassoc_delete (Assoc aa)
{
	return iassoc_delete(aa);
}

#define ASSOC_IDENTIFIER	"#Assoc"
#define ASSOC_ENDSTRING		"#End_of_Assoc"
#define ASSOC_NO_DEF_STRING	"No_Default_Translation_String"
int  iassoc_save (Assoc aa, FILE *fp)
{
	int		ment;
	int		nent;
	int		map;
	char		*sdef;
	char		key[BUFSIZ];

	if (aa == NULL || fp == NULL)
	{
		return 0;
	}
	if (!hash_sort(aa->ht, Sort_data_integer, Sort_asc))
	{
		return 0;
	}
	ment = hash_max_keys(aa->ht);
	nent = hash_num_keys(aa->ht);
	sdef = aa->sdef == NULL ? (char *)ASSOC_NO_DEF_STRING : aa->sdef;
	if (fprintf(fp, "%s %d %d %d %d %d %d %s\n",
		ASSOC_IDENTIFIER, ment, nent,
		aa->styp, aa->miss, aa->indx, aa->idef, sdef) <= 0)
	{
		return 0;
	}
	hash_traverse_start(aa->ht);
	while (hash_traverse_data(aa->ht, key, NULL, NULL, (char *) &map, NULL))
	{
		if (fprintf(fp, "%d\n%s\n", map, key) <= 0)
		{
			return 0;
		}
	}
	hash_traverse_end(aa->ht);
	if (fprintf(fp,"%s\n", ASSOC_ENDSTRING) <= 0)
	{
		return 0;
	}
	fflush(fp);

	return 1;
}

int  sassoc_save (Assoc aa, FILE *fp)
{
	int		ment;
	int		nent;
	char		*keyp;
	char		*datp;
	char		*sdef;

	if (aa == NULL || fp == NULL)
	{
		return 0;
	}
	if (!hash_sort(aa->ht, Sort_data_string, Sort_asc))
	{
		return 0;
	}
	ment = hash_max_keys(aa->ht);
	nent = hash_num_keys(aa->ht);
	sdef = aa->sdef == NULL ? (char *)ASSOC_NO_DEF_STRING : aa->sdef;
	if (fprintf(fp, "%s %d %d %d %d %d %d %s\n",
		ASSOC_IDENTIFIER, ment, nent,
		aa->styp, aa->miss, aa->indx, aa->idef, sdef) <= 0)
	{
		return 0;
	}
	hash_traverse_start(aa->ht);
	while (hash_traverse_data_ptr(aa->ht, &keyp, NULL, NULL, &datp, NULL))
	{
		if (fprintf(fp, "%s\n%s\n", keyp, datp) <= 0)
		{
			return 0;
		}
	}
	hash_traverse_end(aa->ht);
	if (fprintf(fp,"%s\n", ASSOC_ENDSTRING) <= 0)
	{
		return 0;
	}
	fflush(fp);

	return 1;
}

Assoc iassoc_load (FILE *fp)
{
	int		i;
	int		len;
	int		map;
	char		*p;
	char		buff[BUFSIZ];
	char		data[BUFSIZ];
	char		iden[80];
	int		ment, nent;
	int		styp;
	int		miss;
	int		indx;
	int		idef;
	Assoc		aa;

	if (fp == NULL)
	{
		return NULL;
	}
	if (fgets(buff, BUFSIZ, fp) == NULL)
	{
		return NULL;
	}
	if (sscanf(buff, "%s %d %d %d %d %d %d %s",
		iden, &ment, &nent, &styp, &miss, &indx, &idef, data) != 8)
	{
		return NULL;
	}
	if (styp != Assoc_integer)
	{
		return NULL;
	}
	if ((aa = (Assoc) malloc(sizeof(_Assoc))) == NULL)
	{
		return NULL;
	}
	if ((aa->ht = hash_create(ment, 0)) == NULL)
	{
		free(aa);
		return NULL;
	}
	if (!strcmp(data, ASSOC_NO_DEF_STRING))
	{
		aa->sdef = NULL;
	}
	else if ((aa->sdef = (char *)malloc(strsize(data))) == NULL)
	{
		hash_delete(aa->ht);
		free(aa);
		return NULL;
	}
	else
	{
		strcpy(aa->sdef, data);
	}
	aa->styp = (AssocType) styp;
	aa->miss = miss;
	aa->indx = indx;
	aa->idef = idef;

	for (i = 0; i < nent; i++)
	{
		if (fgets(buff, BUFSIZ, fp) == NULL ||
			fgets(data, BUFSIZ, fp) == NULL)
		{
			if (aa->sdef)
				free(aa->sdef);
			hash_delete(aa->ht);
			free(aa);
			return NULL;
		}

		map = (int) strtol(buff, &p, 10);
		if (*p != 0 && *p != '\n')
		{
			if (aa->sdef)
				free(aa->sdef);
			hash_delete(aa->ht);
			free(aa);
			return NULL;
		}

		/* get rid of '\n' */
		len = strlen(data);
		if (len > 0 && data[len-1] == '\n')
			data[len-1] = '\0';

		if (!hash_insert_data(aa->ht, data, strsize(data), NULL,
			(char *) &map, sizeof(int)))
		{
			if (aa->sdef)
				free(aa->sdef);
			hash_delete(aa->ht);
			free(aa);
			return NULL;
		}
	}

	if (fgets(buff, BUFSIZ, fp) == NULL ||
		strncmp(buff, ASSOC_ENDSTRING, strlen(ASSOC_ENDSTRING)))
	{
		if (aa->sdef)
			free(aa->sdef);
		hash_delete(aa->ht);
		free(aa);
		return NULL;
	}

	return aa;
}

Assoc sassoc_load (FILE *fp)
{
	int		i;
	int		len;
	char		buff[BUFSIZ];
	char		data[BUFSIZ];
	char		iden[80];
	int		ment;
	int		nent;
	int		styp;
	int		miss;
	int		indx;
	int		idef;
	Assoc		aa;

	if (fp == NULL)
	{
		return NULL;
	}
	if (fgets(buff, BUFSIZ, fp) == NULL)
	{
		return NULL;
	}
	if (sscanf(buff, "%s %d %d %d %d %d %d %s",
		iden, &ment, &nent, &styp, &miss, &indx, &idef, data) != 8)
	{
		return NULL;
	}
	if ((aa = (Assoc) malloc(sizeof(_Assoc))) == NULL)
	{
		return NULL;
	}
	if ((aa->ht = hash_create(ment, 0)) == NULL)
	{
		free(aa);
		return NULL;
	}
	if (!strcmp(data, ASSOC_NO_DEF_STRING))
	{
		aa->sdef = NULL;
	}
	else if ((aa->sdef = (char *)malloc(strsize(data))) == NULL)
	{
		hash_delete(aa->ht);
		free(aa);
		return NULL;
	}
	else
	{
		strcpy(aa->sdef, data);
	}
	aa->styp = Assoc_string;	/* ignore type saved */
	aa->miss = miss;
	aa->indx = indx;
	aa->idef = idef;

	for (i = 0; i < nent; i++)
	{
		if (fgets(buff, BUFSIZ, fp) == NULL ||
			fgets(data, BUFSIZ, fp) == NULL)
		{
			if (aa->sdef)
				free(aa->sdef);
			hash_delete(aa->ht);
			free(aa);
			return NULL;
		}

		/* get rid of '\n' */
		len = strlen(buff);
		if (len > 0 && buff[len-1] == '\n')
			buff[len-1] = '\0';

		/* get rid of '\n' */
		len = strlen(data);
		if (len > 0 && data[len-1] == '\n')
			data[len-1] = '\0';

		if (!hash_insert_data(aa->ht, buff, strsize(buff), NULL,
			data, strsize(data)))
		{
			if (aa->sdef)
				free(aa->sdef);
			hash_delete(aa->ht);
			free(aa);
			return NULL;
		}
	}

	if (fgets(buff, BUFSIZ, fp) == NULL ||
		strncmp(buff, ASSOC_ENDSTRING, strlen(ASSOC_ENDSTRING)))
	{
		if (aa->sdef)
			free(aa->sdef);
		hash_delete(aa->ht);
		free(aa);
		return NULL;
	}

	return aa;
}
#undef ASSOC_IDENTIFIER
#undef ASSOC_ENDSTRING
#undef ASSOC_NO_DEF_STRING

int  iassoc_value (Assoc aa, char *inx)
{
	int	rtn;
	int	*map;

	if (aa->miss == ASSOC_MISSING_COUNT)
	{
		rtn = hash_key_unique_data(aa->ht, inx, strsize(inx),
			(char *) &aa->indx, sizeof(int),
			(char **) &map, NULL);

		switch(rtn)
		{
		default:
		case 0:		/* failed */
			return aa->idef;
		case 1:		/* inserted */
			aa->indx++;
			return *map;
		case 2:		/* retrieved */
			return *map;
		}
	}

	else if (hash_key_search_data_ptr(aa->ht, inx, strsize(inx),
		(char **) &map, NULL))
	{
		return *map;
	}
	else
	{
		return aa->idef;
	}
}

char *sassoc_value (Assoc aa, char *inx)
{
	int	rtn;
	char	*p;
	char	data[BUFSIZ];

	if (aa->miss == ASSOC_MISSING_COUNT)
	{
		sprintf(data, "%s%d", aa->sdef, aa->indx);
		rtn = hash_key_unique_data(aa->ht, inx, strsize(inx),
			data, strsize(data), &p, NULL);

		switch(rtn)
		{
		default:
		case 0:		/* failed */
			return aa->sdef;
		case 1:		/* inserted */
			aa->indx++;
			return p;
		case 2:		/* retrieved */
			return p;
		}
	}

	else if (hash_key_search_data_ptr(aa->ht, inx, strsize(inx), &p, NULL))
	{
		return p;
	}
	else
	{
		return aa->sdef;
	}
}

int  iassoc_index (Assoc aa, char *inx, int value)
{
	return hash_key_replace_data(aa->ht, inx, strsize(inx), NULL,
		(char *) &value, sizeof(int));
}

int  sassoc_index (Assoc aa, char *inx, char *value)
{
	return hash_key_replace_data(aa->ht, inx, strsize(inx), NULL,
		value, strsize(value));
}

Hash assoc_hash (Assoc aa)
{
	if (aa != NULL)
		return aa->ht;
	else	
		return NULL;
}
/*===========================================================================*/
/*     End of section 12: assoc functions.                                   */
/*===========================================================================*/






/*===========================================================================*/
/*     section 13: built-in testing main program.                            */
/*===========================================================================*/
/*===========================================================================*/
/* the following is testing driver                                           */
/* count word frequencies in a file.                                         */
/* usage:                                                                    */
/*      pipeline: tst < in_filename > outfilename                            */
/*      accept filename: tst filename1 filename2 ... > out_file_name         */
/*===========================================================================*/
#ifdef STAND_ALONE

#if 1		/* test regular Hash */
int  main(int argc, char *argv[])
{
	int	i, inx, line, bsiz, dsiz;
	char	*bp, *dp, buff[BUFSIZ], data[BUFSIZ], save[BUFSIZ];
	FILE	*hash_fp, *fp = stdin;
	int	size;
	Hash	ht, cht;

	if ((ht = hash_create(10, 0)) == NULL)
	{
		fprintf(stderr, "cannot create hash table.\n");
		exit(1);
	}
	line = 1;
	strcpy(save,"nothing");
	for (i = 1; argc == 1 || i < argc; i++)
	{
		if (argc != 1 && (fp = fopen(argv[i], "r")) == NULL)
		{
			fprintf(stderr, "cannot open file <%s>.\n", argv[i]);
			exit (1);
		}
		while (fgets(buff, BUFSIZ, fp) != NULL)
		{
			bp = buff;
			dp = strchr(bp, '\t');
			*dp++ = '\0';
			bp = dp;
			dp = strchr(bp, '\t');
			*dp++ = '\0';
			if (!hash_insert_data(ht, buff, strsize(buff), &inx,
				bp, strsize(bp)))
			{
				printf("cannot insert entry %s\n", buff);
				exit(1);
			}
#if 0
			if (line % 3)
			{
				if (!hash_key_remove_key(ht, save,
					strsize(save)))
					printf("cannot remove %s\n", save);
				else
					printf("removed %s\n", save);
				hash_purge(ht);
				strcpy(save, buff);
			}
			line++;
#endif
		}
		if (argc == 1)	break;
		fclose(fp);
	};

	if (!hash_sort(ht, Sort_key_string, Sort_asc))
	{
		return 0;
	}
	if ((hash_fp = fopen("tst.hashtable", "w")) != NULL)
	{
		hash_write(ht, hash_fp);
		fclose(hash_fp);
	}
	if ((hash_fp = fopen("tst.hashtable", "r")) != NULL)
	{
		cht = hash_read(hash_fp);
		fclose(hash_fp);
	}
	hash_traverse_start(ht);
	while (hash_traverse_data_ptr(ht, &bp, &bsiz, &inx, &dp, &dsiz))
	{
		printf("%s\t%s\t%d\t", bp, dp, inx);
		if (hash_key_search_data(cht, bp, bsiz, data, &dsiz))
		{
			printf("===>%s\t%s\n", bp, data);
		}
		else
		{
			printf("===>????????????\n");
		}
	}
	hash_traverse_end(ht);

	printf("hash table statistics:\n");
	printf("\tmax number of index   = %d\n", hash_max_index(ht));
	printf("\tnumber of unique keys = %d\n", hash_num_keys(ht));
	printf("\ttotal number of cells = %d\n", hash_max_keys(ht));
	printf("\tmax number of entries = %d\n", hash_max_ndata(ht));
	printf("\ttotal number of entries = %d\n", hash_sum_ndata(ht));
	printf("\tmaximum key size = %d\n", hash_max_keysize(ht));
	hash_delete(ht);
	hash_delete(cht);
	return 0;
}

#elif 0		/* test Assoc */

int  main(int argc, char *argv[])
{
	int		i;
	int		map;
	int		bsiz;
	int		dsiz;
	char		buff[BUFSIZ];
	char		data[BUFSIZ];
	char		*bp;
	char		*dp;
	FILE		*fp = stdin;
	FILE		*hash_fp;

	Assoc		aa;
	Hash		ht;

	if ((aa = iassoc_create(ASSOC_MISSING_COUNT, -1)) == NULL)
	{
		fprintf(stderr, "cannot create Assoc.\n");
		exit(1);
	}

	for (i = 1; argc == 1 || i < argc; i++)
	{
		if (argc != 1 && (fp = fopen(argv[i], "r")) == NULL)
		{
			fprintf(stderr, "cannot open file <%s>.\n", argv[i]);
			exit (1);
		}

		/*===========================================================*/
		while (fscanf(fp, "%s", buff) == 1)
		{
			if (iassoc_value(aa, buff) == -2)
			{
				printf("cannot insert entry %s\n", buff);
				exit(1);
			}
		}
		/*===========================================================*/

		if (argc == 1)	break;
		fclose(fp);
	};

	if ((hash_fp = fopen("tst.hashtable", "w")) != NULL)
	{
		iassoc_save(aa, hash_fp);
		fclose(hash_fp);
	}

	if ((hash_fp = fopen("tst.hashtable", "r")) != NULL)
	{
		iassoc_delete(aa);
		aa = iassoc_load(hash_fp);
		fclose(hash_fp);
	}

	ht = assoc_hash (aa);
	hash_set_traverse(ht);
	while (hash_traverse_data(ht, buff, NULL, (char *) &map, NULL))
	{
		printf("%s\t%d\n", buff, map);
	}

	printf("hash table statistics:\n");
	printf("\tnumber of unique keys = %d\n", hash_num_keys(ht));
	printf("\tnumber of entries = %d\n", hash_num_ndata(ht));
	printf("\ttotal number of cells = %d\n", hash_max_keys(ht));
	printf("\tmaximum key size = %d\n", hash_max_keysize(ht));
	printf("\tmaximum counts = %d\n", hash_max_ndata(ht));
	iassoc_delete(aa);
	return 0;
}

#elif 0		/* test hash_(de)compose_key */

int  main(int argc, char *argv[])
{
	int		i;
	FILE		*fp = stdin;
	char		buff[BUFSIZ];

	int		sksiz = 0;
	int		cksiz = 0;
	char		*keyp;
	char		ckey[BUFSIZ];

	for (i = 1; argc == 1 || i < argc; i++)
	{
		if (argc != 1 && (fp = fopen(argv[i], "r")) == NULL)
		{
			fprintf(stderr, "cannot open file <%s>.\n", argv[1]);
			exit (1);
		}

		/*===========================================================*/
		while (fscanf(fp, "%s", buff) == 1)
		{
			cksiz = compose_end(ckey, cksiz,
				buff, strsize(buff));
		}
		/*===========================================================*/

		if (argc == 1)	break;
		fclose(fp);
	};

	while ((sksiz = decompose(ckey, &cksiz, &keyp)) >= 0)
		printf("|%s", keyp);

	return 0;
}

#else

/* test hash_(de)combine_key */
int  main(int argc, char *argv[])
{
	int		i;
	FILE		*fp = stdin;
	char		buff[BUFSIZ];
	char		*keyp;

	int		sksiz = 0;
	int		cksiz = 0;
	char		ckey[BUFSIZ];

	for (i = 1; argc == 1 || i < argc; i++)
	{
		if (argc != 1 && (fp = fopen(argv[i], "r")) == NULL)
		{
			fprintf(stderr, "cannot open file <%s>.\n", argv[1]);
			exit (1);
		}

		/*===========================================================*/
		while (fscanf(fp, "%s", buff) == 1)
		{
			cksiz = combine_end('\t', ckey, cksiz,
				buff, strsize(buff));
		}
		/*===========================================================*/

		if (argc == 1)	break;
		fclose(fp);
	};

	while ((sksiz = decombine('\t', ckey, &cksiz, &keyp)) >= 0)
		printf("|%s", keyp);

	return 0;
}
#endif
#endif
/*===========================================================================*/
/*     End of section 13: built-in testing main program.                     */
/*===========================================================================*/
/*===========================================================================*/
/* End of part 2: hash class implementation.                                 */
/*===========================================================================*/
