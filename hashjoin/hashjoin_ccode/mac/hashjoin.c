/*===========================================================================*/
 static char sid[] = "@(#) hashjoin.c   Command Version1.12   10/14/99 16:01:53";
/*===========================================================================*/
/*===========================================================================*/
/* hashjoin.c                                                                */
/*                                                                           */
/* join two files. one is called master file, the other join is file         */
/* output joinfile record(include key) first, master file records are        */
/* appended.                                                                 */
/*                                                                           */
/* Mon Jan 20 22:31:30 MST 1997, Jincai, create.                             */
/* Fri Mar 28 11:15:16 MST 1997, Jincai, modify. add -v flag                 */
/* Thu May 22 11:42:45 MDT 1997, Jincai, upgrade.                            */
/* Fri Feb 13 12:59:32 MST 1998, Jincai, modify. version Release 2.          */
/* Thu Oct 14 15:59:40 MDT 1999, Jincai, modify. add x flag.                 */
/*===========================================================================*/
#define	_LARGEFILE_SOURCE
#define	_FILE_OFFSET_BITS	64
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hash.h"
#include "base.h"

/* translate fixed format to delimeted type. */
void print_usage (char * procname, int exit_code)
{
	errinfo("%s: join two files.", procname);
	errinfo("usage: %s [-ddelm] [-efill] [-rline] [-m] [-v] [-x] \n"
		"\t\t[-j n1:n2,n1+n2,...] [-k n1:n2,n1+n2,...]\n"
		"\t\t[-s n1:n2,n1+n2,...] [-t n1:n2,n1+n2,...]\n"
		"\t\tlookup_file < joining_file > outputfile",
		procname);
	errinfo("\tor: %s [-ddelm] [-efill] [-rline] [-m] [-v] [-x] \n"
		"\t\t[-j n1:n2,n1+n2,...] [-k n1:n2,n1+n2,...]\n"
		"\t\t[-s n1:n2,n1+n2,...] [-t n1:n2,n1+n2,...]\n"
		"\t\tlookup_file joining_file [joining_file ...]",
		procname);
	errinfo("\tin the second case, the first file is a lookup file\n"
		"\tand the rest are joining files.");
	errinfo("\toutput files are named joining_file.lookup_file.\n");

	errinfo("\tdelm : input/output delimeter (default is TAB).");
	errinfo("\tfill : output filler string when keys "
		"are missing from lookup file.");
	errinfo("\t       if it is not specified, lines with missing key\n"
		"\t       will not be output.");
	errinfo("\tline : number of lines to report progress.\n"
		"\t       default is not report progress.\n");
	errinfo("\t-m   : output multiple records when a key "
		"is duplicated in lookup file.");
	errinfo("\t       if it is not specified, it will randomly choose a\n"
		"\t       record to output when there are multiple records\n"
		"\t       with the same key in lookup file.");
	errinfo("\t-v   : put fields in lookup file at the beginning of line.");
	errinfo("\t-x   : output fields only if key is missing "
		"in lookup file.");
	errinfo("\t-j   : key fields in lookup file. (default = 1).");
	errinfo("\t-k   : key fields in joining file. (default = 1).");
	errinfo("\t-s   : fields to be output in lookup file. "
		"(default = no fields).");
	errinfo("\t-t   : fields to be output in joining file. "
		"(default = all fields).");
	errinfo("\tn1:n2 : fields from n1 to n2 (inclusive).");
	errinfo("\tn1+n2 : n2 fields start from n1.");
	exit(exit_code);
}

#define	LBUFSIZ	(1024*1024)
#define	NFIELDS	(1024*64)
int main(int argc, char * argv[])
{
	FILE		*infile = stdin;
	FILE		*outfile = stdout;
	FILE		*masterfile = NULL;

	Hash		ht;
	char		*inp, *outp;
	char		*subs[NFIELDS];
	int		nsub;
	char		delm = '\t';
	char		eflag[32] = "\0";
	int		i, j;
	long long	lineno, outlineno;
	int		nkeys, mkeys;
	char		**flddata;
	int		*fldsize;
	int		rflag = 0, vflag = 0, mflag = 0, xflag = 0;
	char		toutbuff[LBUFSIZ];
	char		notfbuff[LBUFSIZ];	/* not found buff */
	char		soutbuff[LBUFSIZ];
	char		*p1 = soutbuff;
	char		*p2 = toutbuff;
	char		*p3 = notfbuff;
	char		*p4 = toutbuff;

	char		inbuff[LBUFSIZ];
	char		outbuff[LBUFSIZ];
	char		keybuff[LBUFSIZ];
	char		datbuff[LBUFSIZ];
	int		keysize, datsize;
	int		j_elm, k_elm, k_elm_work;
	int		s_elm, t_elm, t_elm_work;
	int		j_max, k_max, s_max, t_max;
	int		*jkey, *kkey, *kkey_work;
	int		*sout, *tout, *tout_work;

	char		joinedfn[FILENAME_MAX];
	char		masterfn[FILENAME_MAX];
	int		fargs;
	int		c;

	if (argc < 2)	print_usage(argv[0], 1);

	j_elm = k_elm = s_elm = t_elm = 0;
	j_max = k_max = s_max = t_max = 0;
	while ((c = getopt(argc, argv, "d:e:r:j:k:s:t:mvxh?")) != EOF)
	{
		switch(c)
		{
		case 'd':	delm = optarg[0];
				break;
		case 'e':	if (strlen(optarg) > 31)
				{
					errout("filler %s to long", optarg);
				}
				strcpy(eflag, optarg);
				break;
		case 'r':	rflag = atoi(optarg);
				if (rflag < 0)	rflag = 0;
				break;
		case 'm':	mflag = 1;		break;
		case 'x':	xflag = 1;		break;
		case 'v':	vflag = 1;
				p1 = toutbuff;
				p2 = soutbuff;
				p3 = toutbuff;
				p4 = notfbuff;
				break;
		case 'j':	j_max = parse_int_ranges(optarg, &jkey, &j_elm);
				break;
		case 'k':	k_max = parse_int_ranges(optarg, &kkey, &k_elm);
				break;
		case 's':	s_max = parse_int_ranges(optarg, &sout, &s_elm);
				break;
		case 't':	t_max = parse_int_ranges(optarg, &tout, &t_elm);
				break;
		case 'h':
		case '?':
		default:
				print_usage(argv[0], 1);
				break;
		}
	}

	if (argc == optind)
	{
		print_usage(argv[0], 1);
	}
	if ((ht = hash_create(0,0)) == NULL)
	{
		errout("cannot create hash table");
	}
	if ((masterfile = fopen(argv[optind], "r")) == NULL)
	{
		errout("cannot open file %s for read", argv[optind]);
	}
	strcpy(masterfn, argv[optind]);

	if (j_elm == 0)
	{
		if ((jkey = (int *) malloc(sizeof (int))) == NULL)
		{
			errout("cannot malloc memory");
		}
		jkey[0] = 0;
		j_max = 0;
		j_elm = 1;
	}
	if (k_elm == 0)
	{
		if ((kkey = (int *) malloc(sizeof (int))) == NULL)
		{
			errout("cannot malloc memory");
		}
		kkey[0] = 0;
		k_max = 0;
		k_elm = 1;
	}
	if (t_elm == 0)
	{
		if ((tout = (int *) malloc(2 * sizeof (int))) == NULL)
		{
			errout("cannot malloc memory");
		}
		tout[0] = 0;
		tout[1] = -1;
		t_max = 0;
		t_elm = 2;
	}
	kkey_work = (int *) malloc(k_elm * sizeof (int));
	tout_work = (int *) malloc(t_elm * sizeof (int));
	if (kkey_work == NULL || tout_work == NULL)
	{
		errout("cannot malloc memory");
	}

	/*===================================================================*/
	/* load the master file.                                             */
	/*===================================================================*/
	lineno = 0;
	if (rflag)
	{
		errinfo("Loading file %s ...", masterfn);
	}
	if (fgets (inbuff, LBUFSIZ, masterfile) == NULL)
	{
		errout("empty file %s", masterfn);
	}
	nsub = str_subs(inbuff, delm, NULL, 0);
	if (nsub <= j_max || nsub <= s_max)
	{
		errout("require %d fields, found %d fields in file %s",
			1 + (j_max > s_max ? j_max : s_max),
			nsub, masterfn);
	}
	j_max = extend_int_ranges(&jkey, &j_elm, nsub);
	s_max = extend_int_ranges(&sout, &s_elm, nsub);
	nsub = 1 + (j_max > s_max ? j_max : s_max);

	/* loading lookup file into memory hash table */
	do
	{
		lineno++;
		keysize = 0;
		datsize = 0;

		/* pick key and data */
		if (str_subs(inbuff, delm, subs, NFIELDS) < nsub)
		{
			errinfo("too few fields in file %s line %lld",
				masterfn, lineno);
			continue;
		}

		for (i = 0; i < j_elm; i++)
		{
			j = jkey[i];
			keysize = combine_end (delm, keybuff,
				keysize, subs[j], strsize(subs[j]));
		}

		for (i = 0; i < s_elm; i++)
		{
			j = sout[i];
			datsize = combine_end (delm, datbuff,
				datsize, subs[j], strsize(subs[j]));
		}

		if (s_elm == 0)
		{
			if (!hash_insert_key(ht, keybuff, keysize, NULL))
			{
				errout("cannot add key at line %lld", lineno);
			}
		}
		else
		{
			if (!hash_insert_data(ht, keybuff, keysize, NULL,
				datbuff, datsize))
			{
				errout("cannot add data at line %lld", lineno);
			}
		}
		if (rflag && (lineno % rflag) == 0)
		{
			errinfo("\tline %lld\tpassed.", lineno);
		}
	}	while (fgets (inbuff, LBUFSIZ, masterfile) != NULL);
	fclose (masterfile);

	if (lineno == 0)
	{
		errout("file %s is empty or has too few fields", masterfn);
	}
	if (rflag)
	{
		errinfo("\t%lld lines loaded.", lineno);
	}

	fargs = optind + 1;
	if (argc > fargs)
	{
		if ((infile = fopen(argv[fargs], "r")) == NULL)
			errout("cannot open file %s for read", argv[fargs]);
		sprintf(joinedfn, "%s.%s", argv[fargs],
			base_filename(masterfn));
		if ((outfile = fopen(joinedfn, "w")) == NULL)
			errout("cannot open file %s for write", joinedfn);
	}

	/* generate an 'not found' string so it does not need to be  */
	/* appended again and again in case there are many not found */
	outp = notfbuff;
	outp[0] = '\0';
	for (i = 0; eflag[0] && i < s_elm; i++)
	{
		*outp++ = delm;
		strcpy(outp, eflag);
		outp += strlen(eflag);
	}
	mkeys = hash_max_ndata(ht);
	flddata = (char **) malloc(mkeys * sizeof (char *));
	fldsize = (int *) malloc(mkeys * sizeof (int));
	if (flddata == NULL || fldsize == NULL)
	{
		errout("cannot malloc memory for retrieving data");
	}

	/*===================================================================*/
	/* join loop, multiple file support, if input is stdio, output  goes */
	/* to stdout, if input file is named like joining_file, output will  */
	/* goes to joining_file.lookup_file. therefore lookup_file only      */
	/* needs to be loaded once for multiple joining_files.               */
	/*===================================================================*/
	while (infile != NULL)
	{
		char	*fn_p;

		fn_p = (infile == stdin) ? "<stdin>" : argv[fargs];
		lineno = 0;
		outlineno = 0;
		if (rflag)
		{
			errinfo("Reading file %s and joining...", fn_p);
		}

		k_elm_work = k_elm;
		t_elm_work = t_elm;
		for (i = 0; i < k_elm; i++)
		{
			kkey_work[i] = kkey[i];
		}
		for (i = 0; i < t_elm; i++)
		{
			tout_work[i] = tout[i];
		}

		if (fgets (inbuff, LBUFSIZ, infile) == NULL)
		{
			errout("empty joining file");
		}
		nsub = str_subs(inbuff, delm, NULL, 0);
		if (nsub <= k_max || nsub <= t_max)
		{
			errout("require %d fields, found %d fields in file %s",
				1 + (k_max > t_max ? k_max : t_max),
				nsub, fn_p);
		}
		j_max = extend_int_ranges(&kkey_work, &k_elm_work, nsub);
		s_max = extend_int_ranges(&tout_work, &t_elm_work, nsub);
		nsub = (j_max > s_max) ? (j_max + 1) : (s_max + 1);

		/* read joining file line by line and output results */
		do
		{
			lineno++;
			if (rflag && (lineno % rflag) == 0)
			{
				errinfo("\tline %lld\tpassed.", lineno);
			}

			keysize = 0;
			outp = toutbuff;	/* hold -t out in toutbuff */
			outp[0] = '\0';
			if (str_subs(inbuff, delm, subs, LBUFSIZ) < nsub)
			{
				errinfo("too few fields in joining file "
					"line %lld.", lineno);
				continue;
			}
			for (i = 0; i < k_elm_work; i++)
			{
				j = kkey_work[i];
				keysize = combine_end (delm, keybuff, keysize,
					subs[j], strsize(subs[j]));
			}
			for (i = 0; i < t_elm_work; i++)
			{
				j = tout_work[i];
				*outp++ = delm;
				strcpy(outp, subs[j]);
				outp += strlen(subs[j]);
			}

			/* joining process. several cases, */
			/* if nkeys==0, not found in lookup file */
			nkeys = hash_key_search_ndata_ptr(ht, keybuff, keysize,
				&mkeys, flddata, fldsize);

			if (nkeys == 0)
			{
				/* case 1, jkey not found, */
				/* only when eflag or xflag is set */
				/* does action be taken. */
				/* otherwise goes silently. */
				if (eflag[0] != '\0' || xflag)
				{
					/* need output */
					outlineno++;
					strcpy(outbuff, p4);
					strcat(outbuff, p3);
					strcat(outbuff, "\n");
					fputs(outbuff+1, outfile);
				}
			}
			else if (mflag == 0 && !xflag)	/* nkeys >= 1 */
			{
				/* case 2, found one or more entries, */
				/* mflag is off, */
				/* eflag is irrelavent. */
				/* only one output is needed. */
				/* retrieve and cat the data to */
				/* generate sout buffer. */
				outp = soutbuff;
				outp[0] = '\0';
				if (s_elm > 0)
				{
					while (decombine(delm, flddata[0], 
						&fldsize[0], &inp) > 0)
					{
						*outp++ = delm;
						strcpy(outp, inp);
						outp += strlen(inp);
					}
				}

				outlineno++;
				strcpy(outbuff, p2);
				strcat(outbuff, p1);
				strcat(outbuff, "\n");
				fputs(outbuff+1, outfile);
			}
			else if (!xflag)	/* nkeys >= 1 && mflag == 1 */
			{
				/* case 2, found one or more entries, */
				/* mflag is on, */
				/* all entries are required to be output. */
				for (i = 0; i < nkeys; i++)
				{
					outp = soutbuff;
					outp[0] = '\0';
					if (s_elm > 0)
					{
						while (decombine(delm,
							flddata[i],
							&fldsize[i], &inp) > 0)
						{
							*outp++ = delm;
							strcpy(outp, inp);
							outp += strlen(inp);
						}
					}

					outlineno++;
					strcpy(outbuff, p2);
					strcat(outbuff, p1);
					strcat(outbuff, "\n");
					fputs(outbuff+1, outfile);
				}
			}
		}	while (fgets(inbuff, LBUFSIZ, infile) != NULL);

		if (rflag)
		{
			errinfo("\tinput %lld lines,\toutput %lld lines.",
				lineno, outlineno);
		}

		if (++fargs >= argc || infile == stdin)	break;

		fclose (infile);
		fclose (outfile);
		if ((infile = fopen(argv[fargs], "r")) == NULL)
		{
			errout("cannot open file %s for read", argv[fargs]);
		}
		sprintf(joinedfn, "%s.%s", argv[fargs],
			base_filename(masterfn));
		if ((outfile = fopen(joinedfn, "w")) == NULL)
		{
			errout("cannot open file %s for write", joinedfn);
		}
	}

	return 0;
}

