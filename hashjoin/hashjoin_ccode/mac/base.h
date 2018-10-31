#ifndef BASE_H
#define BASE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#define	_XOPEN_SOURCE	/* glibc2 needs this */
#include <time.h>

/*===========================================================================*/
/* package 1: ebcdic and packed-decimal conversion.                          */
/* package 2: data type name conversion between enum and strings.            */
/* package 3: string to data type conversion.                                */
/* package 4: string operations.                                             */
/* package 5: file and filename utilities.                                   */
/* package 6: simple error message handling.                                 */
/* package 7: system services.                                               */
/* package 8: command line argument parsing functions.                       */
/*===========================================================================*/

/*===========================================================================*/
/* package 1: ebcdic and packed-decimal conversion.                          */
/*===========================================================================*/
/*===========================================================================*/
/* convert ebcdic bytes in inbuf of size size into ascii bytes and place     */
/* the results in outbuf (outbuf can be inbuf).                              */
/* return size given.                                                        */
/*===========================================================================*/
int  convert_ebcdic_to_ascii(char *inbuf, int size, char *outbuf);
int  convert_asc_to_ebcdic(char *inbuf, int size, char *outbuf);
/*===========================================================================*/
/* data in inbuf is PIC 9(5) comp-3 representation. where 5 is input size.   */
/* return the converted size, which is (2 * size + 3) / 2, input increment   */
/* should be (size + 1) / 2, according to packed-decimal-3 definitions.      */
/* tested on some IBM main frame dump data.                                  */
/* (outbuf must be different from in buff)                                   */
/*===========================================================================*/
int  unpack_packed_decimal(char *inbuf, int size, char *outbuf);

/*===========================================================================*/
/* package 2: data type name conversion between enum and strings.            */
/*===========================================================================*/
typedef	enum {
	DTunknown = 0,
	DTinteger = 1,
	DTboolean = 2,
	DTfloat = 3,
	DTdouble = 4,
	DTstring = 5,
	DTpointer = 6
}	DType;
void DType_convert_to_string(DType dt, char *buf);
DType DType_convert_from_string(char *s);

/*===========================================================================*/
/* package 3: string to data type conversion.                                */
/* convert string s to a number r in the given data type                     */
/* return 0 upon success, 1 for unknown characters, 2 date out of range.     */
/*        3 incorrect date format.                                           */
/*===========================================================================*/
int  convert_integer(const char *s, int *r);
int  convert_long(const char *s, long *r);
int  convert_float(const char *s, float *r);
int  convert_double(const char *s, double *r);
int  convert_boolean(const char *s, int *r);
/* #[#]/#[#]/####, #[#]-#[#]-#### (mm-dd-yyyy) */
/* ####/#[#]/#[#], ####/#[#]/#[#] (yyyy-mm-dd) */
/* #[#]/#[#]/##, #[#]-#[#]-## (mm-dd-yy) */
int  cvt_mm_dd_YYYY_to_int(const char *mm_dd_YYYY, int *int_date);
int  cvt_mm_dd_YYYY_to_tm(const char *mm_dd_YYYY, struct tm *tm_time);
int  cvt_YYYYmmdd_to_int(const char *YYYYmmdd, int *int_date);
int  cvt_YYYYmmdd_to_tm(const char *YYYYmmdd, struct tm *tm_time);
int  cvt_date_to_int(const char *date, int *int_date);
int  cvt_date_to_tm(const char *date, struct tm *tm_time);
int  cvt_int_to_tm(const int int_date, struct tm *tm_time);
int  cvt_tm_to_int(const struct tm *tm_time, int *int_date);
int  cvt_int_to_tm(const int int_date, struct tm *tm_time);
#ifndef id_year
#define	id_mon(int_date)	(((int_date)/100)%100)
#define	id_day(int_date)	((int_date)%100)
#define	id_year(int_date)	((int_date)/10000)
#endif
/* return number of days from id_beg to id_end, both are integer dates */
double id_diff(const int id_end, const int id_beg);

/*===========================================================================*/
/* package 4: string operations.                                             */
/*===========================================================================*/
/*===========================================================================*/
/* change all letters to upper case.                                         */
/*===========================================================================*/
void str_upper(char *s);
/*===========================================================================*/
/* change all letters to lower case.                                         */
/*===========================================================================*/
void str_lower(char *s);
/*===========================================================================*/
/* capitalize a string. return the number of words.                          */
/*===========================================================================*/
int  str_words(char *s);
/*===========================================================================*/
/* translate all letter "from" to "to". return the number of replacements    */
/*===========================================================================*/
int  str_trs(char *s, int from, int to);
/*===========================================================================*/
/* remove all letter "c" from the string, return the number of deletions     */
/*===========================================================================*/
int  str_rm(char *s, int c);
/*===========================================================================*/
/* trim off head/tail white spaces.                                          */
/*===========================================================================*/
void str_trim(char *s);
/*===========================================================================*/
/* test if string contains only spaces.                                      */
/*===========================================================================*/
int  str_empty(const char *s);
/*===========================================================================*/
/* remove all new line characters at the end of string.                      */
/*===========================================================================*/
void str_nonl(char *s);
/*===========================================================================*/
/* given a string in buff which is multichararter "delm" delimited,          */
/* and a pre-allocated string pointer array subs, of nsubs elements.         */
/* this function breaks the string into substrings and assign the substrings */
/* to the string pointers. new-line characters at the end of the last        */
/* substring is also be removed.                                             */
/* if buff contains more than nsubs substrings, the last string pointer      */
/* subs[nsubs-1] will contain the rest of the string without further         */
/* breaking.                                                                 */
/*===========================================================================*/
int  str_tok(char buff[], const char delm[], char *subs[], const int nsubs);
/*===========================================================================*/
/* given a string in buff which is fixed-width record, and a pre-allocated   */
/* string pointer array subs, of nsubs elements.                             */
/* this function breaks the string into substrings and assign the substrings */
/* to the string pointers.                                                   */
/* if buff contains less characters than the columns specified,              */
/* the corresponding subs will be empty strings.                             */
/* the column width w[] is one-based char array so fields is <= 128 bytes.   */
/* return nonempty (including partial) fields that has data.                 */
/* upon return buff is modified, buff is required to be nsubs bytes longer   */
/*===========================================================================*/
int  str_fixedwidth(char buff[], const char w[], char *subs[], const int nsubs);
/*===========================================================================*/
/* given a string in buff which is "delm" delimited, and a pre-allocated     */
/* string pointer array subs, of nsubs elements.                             */
/* this function breaks the string into substrings and assign the substrings */
/* to the string pointers. new-line characters at the end of the last        */
/* substring is also be removed.                                             */
/* if buff contains more than nsubs substrings, the last string pointer      */
/* subs[nsubs-1] will contain the rest of the string without further         */
/* breaking.                                                                 */
/* if ssub is given as NULL, it counts the number of substring in buff.      */
/* return the number of substrings it breaks into or counts.                 */
/*===========================================================================*/
int  str_subs(char buff[], const char delm, char *subs[], const int nsubs);
/*===========================================================================*/
/* str_subs_quote subroutine breaks the input string in buff into            */
/* substrings and assign the resulting substrings into the pointer           */
/* array subs.                                                               */
/* The input string is assumed to be delimited "delm", and substrings        */
/* can be quoted by '"' and contains delimiters inside it.                   */
/* The pointer array subs is pre-allocated and has nsubs elements.           */
/* New-line characters at the end of the last substring is also be removed.  */
/* If buff contains more than nsubs substrings, the last string pointer      */
/* subs[nsubs-1] will contain the rest of the string without further         */
/* breaking and dequoting.                                                   */
/* It returns the number of substrings it breaks into.                       */
/* If ssub is given as NULL, it counts the number of substring in buff,      */
/* in the way that str_subs_quote() would break it,                          */
/* and return the counts.                                                    */
/*===========================================================================*/
int  str_subs_quote(char buff[],const char delm, char *subs[], const int nsubs);
/*===========================================================================*/
/* str_dequote subroutine trims off paired quotes (single or double) from    */
/* input string if quotes are present.                                       */
/*===========================================================================*/
void str_dequote(char *s);
/*===========================================================================*/
/* cat nsubs of string list subs[] to a single string buff with preallocated */
/* size bufsiz. return remaining_size. if return value > 0, there are some   */
/* space left. if return value == 0, the buff is just filled up. if return   */
/* value < 0, the buff is that many bytes short. of course it will not cat   */
/* the last few strings which would overflow the buffer.                     */
/* just passing NULL as buff and 0 as bufsiz cause it to count size required */
/*===========================================================================*/
int  str_cats(char **subs, int nsubs, char delm, char *buff, int bufsiz);
/*===========================================================================*/
/* replace the part of src string from substring p, of length len with t.    */
/* place resulting string in buff of size bufsiz. NULL buff or 0 bufsiz is   */
/* counting (size required). NULL t means cut that part off. other than      */
/* all inputs are assumed to be correct. ie pos >= 0 and len > 0 and src     */
/* none null and src longer than pos + len.                                  */
/*===========================================================================*/
int  str_rpls(char *src, char *p, int len, char *t, char *buff, size_t bufsiz);

/*===========================================================================*/
/* package 5: file and filename utilities.                                   */
/*===========================================================================*/
/*===========================================================================*/
/* open/close file to read/write. exit if the operation failed.              */
/* if filename=NULL or -, return stdin or stdout.                            */
/* if filename ends with .gz or .Z, open with zcat.                          */
/*===========================================================================*/
FILE *open_file_read(const char filename[]);
FILE *open_file_write(const char filename[]);
int  close_file(FILE *fp, const char filename[]);
/*===========================================================================*/
/* int read_table(const char filename[], const char delm, const int nfields, */
/*      void *record_array[], const size_t size,                             */
/*      int (*parse_record)(const int lineno, const int nsubs, char *subs[], */
/*          void *record, void *context), void *context)                     */
/* read a table (an array of struct) from a file by the name filename        */
/*      the file is delimited by 'delm' and has nfields per line.            */
/*      it reads each line of data from the file, break the line into        */
/*      strings, and parse the strings with the parse_record() until         */
/*      the file is dry, or return of parse_record() indicating error.       */
/*      parse_record() converts substring array into data elements in        */
/*      a data structure which is of size "size".                            */
/* input:                                                                    */
/*      char filename[], input filename, NULL or "-" for stdin.              */
/*      char delm, the delimiter to separate the input fields.               */
/*      int nfields, the expected number of fields in the input file.        */
/*          if it is zero, there is no the #fields consistency checks        */
/*          and parser may need to deal with variable-#fields records.       */
/*      void *record_array[], a pointer for returning the output record      */
/*          array. the array is dynamically allocated internally.            */
/*      sizt_t size, the data struct size of each elements of record_array.  */
/*      void *context, the context data structure to be passed to            */
/*          parse_record(), can be NULL if parse_record() do not use it.     */
/*      int (*parse_record)(const int nsubs, char *subs[],                   */
/*          void *record, void *context),                                    */
/*          a function to parse a record from string fields                  */
/*          into elements of the given data structure "record"               */
/*          the inputs / outputs are:                                        */
/*          input:                                                           */
/*               int lineno, the input file line number (starts with 1).     */
/*               int nsubs, the number of input string fields.               */
/*               char *subs[], the string fields of the record.              */
/*               void *context, context passed in by read_table().           */
/*               the context are to be cast to the right type of             */
/*               data struct and can be read and/or modified.                */
/*          output:                                                          */
/*               void *record, a data struct with given space of "size"      */
/*               "record" is to be cast to the right of data struct          */
/*               and to be filled with values based on subs[].               */
/*          return:                                                          */
/*               0,  if parsing is sucessful and record should be kept       */
/*               negative integer between -1 and -nsubs,                     */
/*                   if parsing is sucessful but record should be discarded  */
/*                   because of the value in the field indicated by          */
/*                   the negative of that number                             */
/*                   read_table() will report the value and continue.        */
/*               integer < nsubs,                                            */
/*                   if parsing is sucessful but record should be discarded. */
/*                   read_table() will continue silently.                    */
/*               positive integer between 1 and nsubs,                       */
/*                   parsing error occurred in the field by that number.     */
/*                   read_table() will report the error and exit(5).         */
/*               integer > nsubs,                                            */
/*                   indicating that some general parsing errors occurred    */
/*                   read_table() will exit(5) silently.                     */
/* output:                                                                   */
/*      void *record_array, an internally allocated data struct array        */
/*      of nrecords elements, each element is with size "size" and           */
/*      read, parsed, and kept.                                              */
/*      if nrecords=0, the array is NULL.                                    */
/*                                                                           */
/* return:                                                                   */
/*      the number of records that has been read, parsed and kept.           */
/*      if the open/read/parse/allocation action failed,                     */
/*      read_table() prints adaquate error message to stderr and exits       */
/*           exit(1) if opening file failed                                  */
/*           exit(2) if allocating memory failed                             */
/*           exit(3) #fields mismatch, parsing error, and other errors       */
/*           exit(4) errno set by parse_record().                            */
/*                                                                           */
/* example:
typedef struct {int index; double value;} Content;
int  parse_content(const int NR, const int nsubs, char *subs[],
	void *record, void *context)
{
	if (NR==0) {
		return -nsubs-1;
	} else {
		Content	*content = (Content *) record;
		int	*ndiscared = (int *) context;
		content->index = atoi(subs[0]);
		if (errno) return 1;
		if (content->index <= 0) {(*ndiscared)++;return -1;}
		content->value = atoi(subs[1]);
		if (content->value <= 0) {(*ndiscared)++;return -nsubs-1;}
		if (errno) {return nsubs+1;};
		return 0;
	}
}
int main()
{
	int ncontents, ndiscarded = 0;
	Content	*context;
	ncontents = read_table("two_fields.txt", '\t', 2,
		(void **)&context, sizeof(Content),
		parse_content, (void*)&ndiscarded);
	printf("#records with positve index is %d\n", ncontents);
	printf("#records discared is %d.\n", ndiscarded);
	return 0;
}
*/
int read_table(const char filename[], const char delm, const int nfields,
	void *record_array[], const size_t size,
	int (*parse_record)(const int lineno, const int nsubs, char *subs[],
		void *record, void *context),
	void *context);
/*===========================================================================*/
/* count the number of lines text in a file.                                 */
/* return:  0, empty file; -1, cannot open file for read; other, #lines.     */
/*===========================================================================*/
long long get_file_lines(char *filename);
long long get_file_size(char *filename);
/*===========================================================================*/
/* As fgets, it reads a line from a readable file pointer into a buffer      */
/* with a given size.                                                        */
/* In addtion to fgets, it performs the following:                           */
/*	 skip comments (from # to end-of-line),                              */
/*	 trim off the ending \n and/or \r,                                   */
/*	 skip empty lines,                                                   */
/*	 if lines ends with \, appending next line.                          */
/*       keep track of line number (empty lines and comment lines count).    */
/*===========================================================================*/
char *fgets_line(char *buff, const int bufsiz,
	FILE *fp, const int cchr, long long *lineno);

/*===========================================================================*/
/* given a string as filename, return a pointer to some where in that string */
/* so that the substring is a base name of the original filename.            */
/*===========================================================================*/
char *base_filename(char *filename);

int  get_env_filename(char *env_name, char *filename, char *path);

/*===========================================================================*/
/* package 6: simple error message handling.                                 */
/*===========================================================================*/
void errout(char *s, ...);
void errout_file(char *s);
void errout_malloc(char *s);
void errout_malloc_size(char *s, size_t size);
void errinfo(char *s, ...);
/*===========================================================================*/
/* write message when mline > 0 and nline is multiple of mline.              */
/*===========================================================================*/
void report_status(int mline, int nline, char *s, ...);

/*===========================================================================*/
/* package 7: system services.                                               */
/*===========================================================================*/
/*===========================================================================*/
/* Timer: timer to measure program speed.                                    */
/* set_timer(): set timer to current time.                                   */
/* get_timer(): get delta time from last set_timer to now.                   */
/* sprintf_timer(): print delta time from last set_timer to now. return size */
/*===========================================================================*/
typedef struct
{
	time_t  real_time;	/* wall clock time (tick since machine up). */
	time_t	user_time;	/* user cpu time. */
	time_t	system_time;	/* system cpu time. */
} Timer;

void set_timer(Timer *timer);
void get_timer(Timer *timer, double *realtm, double *usertm, double *systm);
int  sprintf_timer(char *buf, size_t bufsiz, Timer *timer);

/*===========================================================================*/
/* raise soft data memory ulimit up to hard data memory ulimit.              */
/*===========================================================================*/
void raise_memory_ulimit( void );

/* get date in YYYYmmdd format, days_from_today is the number of days from */
/* today. if days_from_today = -n, it return the date of n days ago */
int  get_today_date(const int days_from_today);

/*===========================================================================*/
/* package 8: command line argument parsing functions.                       */
/*===========================================================================*/
/*===========================================================================*/
/* str contain delm separated list of int's/double's, out is a nout          */
/* elements array, preallocated and pre-initialized.                         */
/* this routine updates the element values in out with numbers in the        */
/* corresponding str fields.                                                 */
/* values in the elements corresponding to fields beyond the last one        */
/* and fields without value (such as ",,") will remain untouched.            */
/* return the number of fields in str, or 0 if error occurred.               */
/*===========================================================================*/
int  parse_double_list(const char *str, const char delm,
		double out[], int const nout);
int  parse_int_list(const char *str, const char delm,
		int out[], const int nout);

/*===========================================================================*/
/* int  parse_int_range(char * p, int * start, int * stop, char * errs,      */
/*	char sc, char lc)                                                    */
/* parse command line range specification:                                   */
/* for example: myprogram -b1:2 ...                                          */
/*                                                                           */
/* p, string passed in, legal string has format: (where charactors : and #   */
/* are specified by sc and lc)                                               */
/*                                                                           */
/* (1) a, (2) a:, (3) a#, (4) a:b, (5) a#c                                   */
/* (6) :b, (7) #c                                                            */
/*                                                                           */
/* return 1, null range                                                      */
/* return 2, non-digit character                                             */
/* return 3, range downward                                                  */
/* return 0, success                                                         */
/*                                                                           */
/*    case    string  *start          *stop        check a<b                 */
/*    (1)      a        a               a             y                      */
/*    (2)      a:       a             unchanged       n                      */
/*    (3)      a#       a             unchanged       n                      */
/*    (4)      a:b      a               b             y                      */
/*    (5)      a#c      a             a+c-1           y                      */
/*    (6)      :b     unchanged         b             y                      */
/*    (7)      #c     unchanged       *start+c-1      y                      */
/*   error             unknown         unknown                               */
/*   c, length,  b stop offset, # length separator, : range separator        */
/*===========================================================================*/
int  parse_int_range(const char *p, int *start, int *stop,
		const char sc, const char lc);
/*===========================================================================*/
/* Given a string str in the format "a:b,a+c,...", upon return, *range int   */
/* array contains numbers indicating which fields are in the list in the     */
/* order which is specified. Input *nelm is the length of *range. If input   */
/* *nelm == 0, *range will be allocated. *nelm upon return is the new length */
/* of the int array *range. to-last-field is specified by a:-1.              */
/* return the largest field number (0-based) among all specified.            */
/*===========================================================================*/
int  parse_int_ranges(const char *str, int **range, int *nelm);
/*===========================================================================*/
/* quite useful after parse_int_ranges(), fix to-the-end fields              */
/* maxd is the number of fields this route extends to if it finds an         */
/* open-end field specification. maxd is either from -ddim                   */
/* command-line option or from field parser such as str_subs(), or           */
/* arbituarily given.                                                        */
/* if maxd is given to zero, *range if freed by free().                      */
/* return maximum number of fields specified                                 */
/* like parse_int_ranges(), *nelm upon return is the new length              */
/* of the int array *range.                                                  */
/* like parse_int_ranges(), return the largest field number (0-based)        */
/* among all specified (including the extended part).                        */
/*===========================================================================*/
int  extend_int_ranges(int **range, int *nelm, const int maxd);
/* generate ordered unique field lists after parse_int_ranges */
/* return the maximum number of all fields specified          */
int  unique_int_ranges(int **range, int *nelm, const int maxd);
/* generate switches (1/0) for each field after parse_int_ranges */
/* return the number of fields specified                         */
int  switch_int_ranges(int **range, int *nelm, const int maxd);

#ifdef __cplusplus
}
#endif
#endif
