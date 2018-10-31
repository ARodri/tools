#include "base.h"
#include <float.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/resource.h>

/*===========================================================================*/
/* package 1: ebcdic and packed-decimal conversion.                          */
/* package 2: data type name conversion between enum and strings.            */
/* package 3: string to data type conversion.                                */
/* package 4: string operations.                                             */
/* package 5: file and filename utilities.                                   */
/* package 6: simple error message handling.                                 */
/* package 7: system services.                                               */
/* package 8: command line argument parsing functions.                       */
/* searche for eop                                                           */
/*===========================================================================*/

/*===========================================================================*/
/* package 1: ebcdic and packed-decimal conversion.                          */
/*===========================================================================*/
/*===========================================================================*/
/* convert ebcdic bytes in inbuf of size size into ascii bytes and place     */
/* the results in outbuf (outbuf can be inbuf).                              */
/* return size given.                                                        */
/*===========================================================================*/
int  convert_ebcdic_to_ascii(char *inbuf, int size, char *outbuf)
{
	int	i, byte;
	static unsigned char ebcdic_to_ascii[256] =
	{
		0x00,0x01,0x02,0x03,0x9C,0x09,0x86,0x7F,
		0x97,0x8D,0x8E,0x0B,0x0C,0x0D,0x0E,0x0F,
		0x10,0x11,0x12,0x13,0x9D,0x85,0x08,0x87,
		0x18,0x19,0x92,0x8F,0x1C,0x1D,0x1E,0x1F,
		0x80,0x81,0x82,0x83,0x84,0x0A,0x17,0x1B,
		0x88,0x89,0x8A,0x8B,0x8C,0x05,0x06,0x07,
		0x90,0x91,0x16,0x93,0x94,0x95,0x96,0x04,
		0x98,0x99,0x9A,0x9B,0x14,0x15,0x9E,0x1A,
		0x20,0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,
		0xA7,0xA8,0xD5,0x2E,0x3C,0x28,0x2B,0x7C,
		0x26,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
		0xB0,0xB1,0x21,0x24,0x2A,0x29,0x3B,0x7E,
		0x2D,0x2F,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,
		0xB8,0xB9,0xCB,0x2C,0x25,0x5F,0x3E,0x3F,
		0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,0xC0,0xC1,
		0xC2,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,
		0xC3,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
		0x68,0x69,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,
		0xCA,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,
		0x71,0x72,0x5E,0xCC,0xCD,0xCE,0xCF,0xD0,
		0xD1,0xE5,0x73,0x74,0x75,0x76,0x77,0x78,
		0x79,0x7A,0xD2,0xD3,0xD4,0x5B,0xD6,0xD7,
		0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
		0xE0,0xE1,0xE2,0xE3,0xE4,0x5D,0xE6,0xE7,
		0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
		0x48,0x49,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,
		0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,
		0x51,0x52,0xEE,0xEF,0xF0,0xF1,0xF2,0xF3,
		0x5C,0x9F,0x53,0x54,0x55,0x56,0x57,0x58,
		0x59,0x5A,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,
		0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
		0x38,0x39,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
	};

	for (i = 0; i < size; i++)
	{
		byte = (int) inbuf[i];
		outbuf[i] = (char) ebcdic_to_ascii[byte];
	}

	return i;
}

int  convert_asc_to_ebcdic(char *inbuf, int size, char *outbuf)
{
	int	i, byte;
	static unsigned char ascii_to_ebcdic[256] =
	{
		0x00,0x01,0x02,0x03,0x37,0x2D,0x2E,0x2F,
		0x16,0x05,0x25,0x0B,0x0C,0x0D,0x0E,0x0F,
		0x10,0x11,0x12,0x13,0x3C,0x3D,0x32,0x26,
		0x18,0x19,0x3F,0x27,0x1C,0x1D,0x1E,0x1F,
		0x40,0x5A,0x7F,0x7B,0x5B,0x6C,0x50,0x7D,
		0x4D,0x5D,0x5C,0x4E,0x6B,0x60,0x4B,0x61,
		0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,
		0xF8,0xF9,0x7A,0x5E,0x4C,0x7E,0x6E,0x6F,
		0x7C,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,
		0xC8,0xC9,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,
		0xD7,0xD8,0xD9,0xE2,0xE3,0xE4,0xE5,0xE6,
		0xE7,0xE8,0xE9,0xAD,0xE0,0xBD,0x9A,0x6D,
		0x79,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
		0x88,0x89,0x91,0x92,0x93,0x94,0x95,0x96,
		0x97,0x98,0x99,0xA2,0xA3,0xA4,0xA5,0xA6,
		0xA7,0xA8,0xA9,0xC0,0x4F,0xD0,0x5F,0x07,
		0x20,0x21,0x22,0x23,0x24,0x15,0x06,0x17,
		0x28,0x29,0x2A,0x2B,0x2C,0x09,0x0A,0x1B,
		0x30,0x31,0x1A,0x33,0x34,0x35,0x36,0x08,
		0x38,0x39,0x3A,0x3B,0x04,0x14,0x3E,0xE1,
		0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,
		0x49,0x51,0x52,0x53,0x54,0x55,0x56,0x57,
		0x58,0x59,0x62,0x63,0x64,0x65,0x66,0x67,
		0x68,0x69,0x70,0x71,0x72,0x73,0x74,0x75,
		0x76,0x77,0x78,0x80,0x8A,0x8B,0x8C,0x8D,
		0x8E,0x8F,0x90,0x6A,0x9B,0x9C,0x9D,0x9E,
		0x9F,0xA0,0xAA,0xAB,0xAC,0x4A,0xAE,0xAF,
		0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,
		0xB8,0xB9,0xBA,0xBB,0xBC,0xA1,0xBE,0xBF,
		0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xDA,0xDB,
		0xDC,0xDD,0xDE,0xDF,0xEA,0xEB,0xEC,0xED,
		0xEE,0xEF,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
	};

	for (i = 0; i < size; i++)
	{
		byte = (int) inbuf[i];
		outbuf[i] = (char) ascii_to_ebcdic[byte];
	}

	return i;
}

/*===========================================================================*/
/* data in inbuf is PIC 9(5) comp-3 representation. where 5 is input size.   */
/* return the converted size, which is (2 * size + 3) / 2, input increment   */
/* should be (size + 1) / 2, according to packed-decimal-3 definitions.      */
/* tested on some IBM main frame dump data.                                  */
/* (outbuf must be different from in buff)                                   */
/*===========================================================================*/
int  unpack_packed_decimal(char *inbuf, int size, char *outbuf)
{
	int	i, byte;

	size = (size / 2) * 2 + 1;	/* round even number to upper odd */
	for (i = 0; i <= size; i++)
	{
		byte = (i % 2) ? inbuf[i/2] : (inbuf[i/2] >> 4);
		byte &= 0x0F;
		if (i < size)
		{
		    outbuf[i+1] = '0' + byte;
		}
		else if (byte == 0x0C || byte == 0x0F)
		{
			outbuf[0] = '+';
		}
		else if (byte == 0x0D)
		{
			outbuf[0] = '-';
		}
		else
		{
			outbuf[0] = ' ';
		}
	}

	return i;
}


/*===========================================================================*/
/* package 2: data type name conversion between enum and strings.            */
/*===========================================================================*/
void DType_convert_to_string(DType dt, char *buf)
{
	switch (dt)
	{
		case DTunknown : strcpy(buf, "unknown");	break;
		case DTinteger : strcpy(buf, "integer");	break;
		case DTboolean : strcpy(buf, "boolean");	break;
		case DTfloat   : strcpy(buf, "float");		break;
		case DTdouble  : strcpy(buf, "double");		break;
		case DTstring  : strcpy(buf, "string");		break;
		case DTpointer : strcpy(buf, "pointer");	break;
	}
}

DType DType_convert_from_string(char *s)
{
	if (!strcasecmp(s, "integer") || !strcasecmp(s, "int"))
	{
		return DTinteger;
	}
	else if (!strcasecmp(s, "boolean") || !strcasecmp(s, "bool"))
	{
		return DTboolean;
	}
	else if (!strcasecmp(s, "float") || !strcasecmp(s, "flt"))
	{
		return DTfloat;
	}
	else if (!strcasecmp(s, "double") || !strcasecmp(s, "dbl"))
	{
		return DTdouble;
	}
	else if (!strcasecmp(s, "string") || !strcasecmp(s, "str"))
	{
		return DTstring;
	}
	else if (!strcasecmp(s, "pointer") || !strcasecmp(s, "ptr"))
	{
		return DTpointer;
	}
	else
	{
		return DTunknown;
	}
}


/*===========================================================================*/
/* package 3: string to data type conversion.                                */
/* convert string s to a number r in the given data type                     */
/* return 0 upon success, 1 for unknown characters, 2 for other errors       */
/*===========================================================================*/
int  convert_integer(const char *s, int *r)
{
	char	*endp;

	errno = 0;
	(*r) = (int) strtol(s, &endp, 10);
	if (endp == NULL)
	{
		errno = EINVAL;
		return 1;
	}
	while (*endp && isspace(*endp))
	{
		endp++;
	}
	if (endp[0] != '\0')
	{
		errno = EINVAL;
		return 1;
	}
	else if (errno != 0)
	{
		return 2;
	}
	else
	{
		return 0;
	}
}

int  convert_long(const char *s, long *r)
{
	char	*endp;

	errno = 0;
	(*r) = strtol(s, &endp, 10);
	if (endp == NULL)
	{
		errno = EINVAL;
		return 1;
	}
	while (*endp && isspace(*endp))
	{
		endp++;
	}
	if (endp[0] != '\0')
	{
		errno = EINVAL;
		return 1;
	}
	else if (errno != 0)
	{
		return 2;
	}
	else
	{
		return 0;
	}
}

int  convert_float(const char *s, float *r)
{
	char	*endp;

	errno = 0;
	(*r) = (float) strtod(s, &endp);
	if (endp == NULL)
	{
		errno = EINVAL;
		return 1;
	}
	while (*endp && isspace(*endp))
	{
		endp++;
	}
	if (endp[0] != '\0')
	{
		return 1;
	}
	else if (errno != 0)
	{
		return 2;
	}
	else if (!finite((double)(*r)))
	{
		errno = ERANGE;
		return 3;
	}
	else
	{
		return 0;
	}
}

int  convert_double(const char *s, double *r)
{
	char	*endp;

	errno = 0;
	(*r) = (double) strtod(s, &endp);
	if (endp == NULL)
	{
		errno = EINVAL;
		return 1;
	}
	while (*endp && isspace(*endp))
	{
		endp++;
	}
	if (endp[0] != '\0')
	{
		return 1;
	}
	else if (errno != 0)
	{
		return 2;
	}
	else if (!finite(*r))
	{
		errno = ERANGE;
		return 3;
	}
	else
	{
		return 0;
	}
}

int  convert_boolean(const char *s, int *r)
{
	if (!strcasecmp(s, "yes")  || !strcasecmp(s, "y") ||
	    !strcasecmp(s, "true") || !strcasecmp(s, "t") ||
	    !strcasecmp(s, "on")   || !strcasecmp(s, "1") )
	{
		*r = 1;
		return 0;
	}
	else if (!strcasecmp(s, "no")    || !strcasecmp(s, "n") ||
		 !strcasecmp(s, "false") || !strcasecmp(s, "f") ||
		 !strcasecmp(s, "off")   || !strcasecmp(s, "0") )
	{
		*r = 0;
		return 0;
	}
	else
	{
		*r = 0;
		return 1;
	}
}

/* return number of days from id_beg to id_end, both are integer dates */
double id_diff(const int id_end, const int id_beg)
{
	struct tm T;
	time_t tt_beg, tt_end;

	memset(&T, 0, sizeof(T));
	T.tm_year = id_year(id_beg) - 1900;
	T.tm_mon = id_mon(id_beg) - 1;
	T.tm_mday = id_day(id_beg);
	if ( (tt_beg = mktime(&T)) == (time_t)-1 )
	{
		return DBL_MAX;
	}
	memset(&T, 0, sizeof(T));
	T.tm_year = id_year(id_end) - 1900;
	T.tm_mon = id_mon(id_end) - 1;
	T.tm_mday = id_day(id_end);
	if ( (tt_end = mktime(&T)) == (time_t)-1 )
	{
		return DBL_MAX;
	}

	/* #seconds / 86400 */
	return difftime(tt_end, tt_beg) / 86400.0;
}

/* #[#]/#[#]/####, #[#]-#[#]-#### (mm-dd-yyyy) */
/* ####/#[#]/#[#], ####/#[#]/#[#] (yyyy-mm-dd) */
/* #[#]/#[#]/##, #[#]-#[#]-## (mm-dd-yy) */
int  cvt_mm_dd_YYYY_to_int(const char *mm_dd_YYYY, int *int_date)
{
	int	n1, n2, n3, k1, k2, k3;
	char	d, *p;

	p = (char *) mm_dd_YYYY;

	n1 = k1 = 0;
	while ((*p) >= '0' && (*p) <= '9')
	{
		n1 = 10 * n1 + (*p) - '0';
		k1++;
		p++;
	}
	if (k1 != 1 && k1 != 2 && k1 != 4)
	{
		return 3;
	}

	if ((*p) == '-' || (*p) == '/')
	{
		d = (*p);
		p++;
	}
	else if ((*p) == '\0')
	{
		return 3;
	}
	else
	{
		return 1;
	}

	n2 = k2 = 0;
	while ((*p) >= '0' && (*p) <= '9')
	{
		n2 = 10 * n2 + (*p) - '0';
		k2++;
		p++;
	}
	if (k2 != 1 && k2 != 2)
	{
		return 3;
	}

	if ((*p) == d)
	{
		p++;
	}
	else if ((*p) == '\0')
	{
		return 3;
	}
	else
	{
		return 1;
	}

	n3 = k3= 0;
	while ((*p) >= '0' && (*p) <= '9')
	{
		n3 = 10 * n3 + (*p) - '0';
		k3++;
		p++;
	}
	if (k3 != 1 && k3 != 2 && k3 != 4)
	{
		return 3;
	}

	if ((*p) != '\0')
	{
		return 1;
	}

	if (k1 <= 2 && k3 == 4)
	{
		/* assume mm/dd/yyyy */
	}
	else if (k3 <= 2 && k1 == 4)
	{
		/* assume yyyy/mm/dd */
		k1 = n1;
		n1 = n2;
		n2 = n3;
		n3 = k1;
	}
	else if (k1 <= 2 && k3 == 2)
	{
		/* assume mm/dd/yy */
		if (n3 > 50)
		{
			n3 = 1900 + n3;
		}
		else
		{
			n3 = 2000 + n3;
		}
	}
	else
	{
		return 3;
	}

	if (n1 < 1 || n1 > 12 || n2 < 1 || n2 > 31)
	{
		return 2;
	}

	(*int_date) = n3 * 10000 + n1 * 100 + n2;

	return 0;
#if 0
	if (strlen(mm_dd_YYYY) != 10)
	{
		return  1;
	}
	else if (mm_dd_YYYY[6] < '0' || mm_dd_YYYY[6] > '9' ||
	    mm_dd_YYYY[7] < '0' || mm_dd_YYYY[6] > '9' ||
	    mm_dd_YYYY[8] < '0' || mm_dd_YYYY[8] > '9' ||
	    mm_dd_YYYY[9] < '0' || mm_dd_YYYY[9] > '9' ||
	    mm_dd_YYYY[0] < '0' || mm_dd_YYYY[0] > '9' ||
	    mm_dd_YYYY[1] < '0' || mm_dd_YYYY[1] > '9' ||
	    mm_dd_YYYY[3] < '0' || mm_dd_YYYY[3] > '9' ||
	    mm_dd_YYYY[4] < '0' || mm_dd_YYYY[4] > '9')
	{
		return  2;
	}
	else if (mm_dd_YYYY[2] != mm_dd_YYYY[5] ||
	    (mm_dd_YYYY[2] != '-' && mm_dd_YYYY[2] != '/'))
	{
		return  3;
	}
	else
	{
		(*int_date) =
		(mm_dd_YYYY[6]-'0') * 10000000 +
		(mm_dd_YYYY[7]-'0') * 1000000 +
		(mm_dd_YYYY[8]-'0') * 100000 +
		(mm_dd_YYYY[9]-'0') * 10000 +
		(mm_dd_YYYY[0]-'0') * 1000 +
		(mm_dd_YYYY[1]-'0') * 100 +
		(mm_dd_YYYY[3]-'0') * 10 +
		(mm_dd_YYYY[4]-'0');

		return 0;
	}
#endif
}

int  cvt_mm_dd_YYYY_to_tm(const char *mm_dd_YYYY, struct tm *tm_time)
{
	int	n1, n2, n3, k1, k2, k3;
	char	d, *p;

	p = (char *) mm_dd_YYYY;

	n1 = k1 = 0;
	while ((*p) >= '0' && (*p) <= '9')
	{
		n1 = 10 * n1 + (*p) - '0';
		k1++;
		p++;
	}
	if (k1 != 1 && k1 != 2 && k1 != 4)
	{
		return 3;
	}

	if ((*p) == '-' || (*p) == '/')
	{
		d = (*p);
		p++;
	}
	else if ((*p) == '\0')
	{
		return 3;
	}
	else
	{
		return 1;
	}

	n2 = k2 = 0;
	while ((*p) >= '0' && (*p) <= '9')
	{
		n2 = 10 * n2 + (*p) - '0';
		k2++;
		p++;
	}
	if (k2 != 1 && k2 != 2)
	{
		return 3;
	}

	if ((*p) == d)
	{
		p++;
	}
	else if ((*p) == '\0')
	{
		return 3;
	}
	else
	{
		return 1;
	}

	n3 = k3= 0;
	while ((*p) >= '0' && (*p) <= '9')
	{
		n3 = 10 * n3 + (*p) - '0';
		k3++;
		p++;
	}
	if (k3 != 1 && k3 != 2 && k3 != 4)
	{
		return 3;
	}

	if ((*p) != '\0')
	{
		return 1;
	}

	if (k1 <= 2 && k3 == 4)
	{
		/* assume mm/dd/yyyy */
	}
	else if (k3 <= 2 && k1 == 4)
	{
		/* assume yyyy/mm/dd */
		k1 = n1;
		n1 = n2;
		n2 = n3;
		n3 = k1;
	}
	else if (k1 <= 2 && k3 == 2)
	{
		/* assume mm/dd/yy */
		if (n3 > 50)
		{
			n3 = 1900 + n3;
		}
		else
		{
			n3 = 2000 + n3;
		}
	}
	else
	{
		return 3;
	}

	if (n1 < 1 || n1 > 12 || n2 < 1 || n2 > 31)
	{
		return 2;
	}

	memset(tm_time, 0, sizeof(*tm_time));
	tm_time->tm_year = n3 - 1900;
	tm_time->tm_mon  = n1 - 1;
	tm_time->tm_mday = n2;
	if (mktime(tm_time) == (time_t)-1)
	{
		return 2;
	}

	return 0;
}

int  cvt_YYYYmmdd_to_int(const char *YYYYmmdd, int *int_date)
{
	if (strlen(YYYYmmdd) != 8)
	{
		return  1;
	}
	else if (YYYYmmdd[0] < '0' || YYYYmmdd[0] > '9' ||
	    YYYYmmdd[1] < '0' || YYYYmmdd[1] > '9' ||
	    YYYYmmdd[2] < '0' || YYYYmmdd[2] > '9' ||
	    YYYYmmdd[3] < '0' || YYYYmmdd[3] > '9' ||
	    YYYYmmdd[4] < '0' || YYYYmmdd[4] > '9' ||
	    YYYYmmdd[5] < '0' || YYYYmmdd[5] > '9' ||
	    YYYYmmdd[6] < '0' || YYYYmmdd[6] > '9' ||
	    YYYYmmdd[7] < '0' || YYYYmmdd[7] > '9')
	{
		return  2;
	}
	else
	{
		*int_date =
		(YYYYmmdd[0]-'0') * 10000000 +
		(YYYYmmdd[1]-'0') * 1000000 +
		(YYYYmmdd[2]-'0') * 100000 +
		(YYYYmmdd[3]-'0') * 10000 +
		(YYYYmmdd[4]-'0') * 1000 +
		(YYYYmmdd[5]-'0') * 100 +
		(YYYYmmdd[6]-'0') * 10 +
		(YYYYmmdd[7]-'0');

		return 0;
	}
}

int  cvt_YYYYmmdd_to_tm(const char *YYYYmmdd, struct tm *tm_time)
{
	if (strlen(YYYYmmdd) != 8)
	{
		return  1;
	}
	else if (YYYYmmdd[0] < '0' || YYYYmmdd[0] > '9' ||
	    YYYYmmdd[1] < '0' || YYYYmmdd[1] > '9' ||
	    YYYYmmdd[2] < '0' || YYYYmmdd[2] > '9' ||
	    YYYYmmdd[3] < '0' || YYYYmmdd[3] > '9' ||
	    YYYYmmdd[4] < '0' || YYYYmmdd[4] > '9' ||
	    YYYYmmdd[5] < '0' || YYYYmmdd[5] > '9' ||
	    YYYYmmdd[6] < '0' || YYYYmmdd[6] > '9' ||
	    YYYYmmdd[7] < '0' || YYYYmmdd[7] > '9')
	{
		return  2;
	}
	else
	{
		memset(tm_time, 0, sizeof(*tm_time));
		tm_time->tm_year = (YYYYmmdd[0]-'0') * 1000 +
			     (YYYYmmdd[1]-'0') * 100 +
			     (YYYYmmdd[2]-'0') * 10 +
			     (YYYYmmdd[3]-'0') - 1900;
		tm_time->tm_mon  = (YYYYmmdd[4]-'0') * 10 +
			     (YYYYmmdd[5]-'0') - 1;
		tm_time->tm_mday = (YYYYmmdd[6]-'0') * 10 +
			     (YYYYmmdd[7]-'0');
		if (mktime(tm_time) == (time_t)-1)
		{
			return  2;
		}
	}

	return 0;
}

int  cvt_date_to_int(const char *date, int *int_date)
{
	return (cvt_YYYYmmdd_to_int(date, int_date) ?
		cvt_mm_dd_YYYY_to_int(date, int_date) : 0);
}

int  cvt_date_to_tm(const char *date, struct tm *tm_time)
{
	return (cvt_YYYYmmdd_to_tm(date, tm_time) ?
		cvt_mm_dd_YYYY_to_tm(date, tm_time) : 0);
}

int  cvt_int_to_tm(const int int_date, struct tm *tm_time)
{
	int	d;

	memset(tm_time, 0, sizeof(*tm_time));
	d = int_date / 10000;
	tm_time->tm_year = d - 1900;
	d = int_date - d * 10000;
	tm_time->tm_mon  = d / 100 - 1;
	tm_time->tm_mday = d % 100;
	if (mktime(tm_time) == (time_t)-1)
	{
		return  2;
	}

	return 0;
}

int  cvt_tm_to_int(const struct tm *tm_time, int *int_date)
{
	if (tm_time->tm_year < 0 || tm_time->tm_year > 9999 ||
	    tm_time->tm_mon < 0 || tm_time->tm_mon > 11 ||
	    tm_time->tm_mday < 1 || tm_time->tm_mday > 31)
	{
		return  2;
	}
	(*int_date) = tm_time->tm_mday +
		(tm_time->tm_mon + 1) * 100 +
		(tm_time->tm_year + 1900) * 10000;

	return 0;
}

/*===========================================================================*/
/* package 4: string operations.                                             */
/*===========================================================================*/
/*===========================================================================*/
/* change all letters to upper case.                                         */
/*===========================================================================*/
void str_upper(char *s)
{
	while (*s)
	{
		(*s) = toupper(*s);
		s++;
	}
}

/*===========================================================================*/
/* change all letters to lower case.                                         */
/*===========================================================================*/
void str_lower(char *s)
{
	while (*s)
	{
		(*s) = tolower(*s);
		s++;
	}
}

/*===========================================================================*/
/* capitalize a string. return the number of words.                          */
/*===========================================================================*/
int  str_words(char *s)
{
	int	nwords, to_up;

	to_up = 1;
	nwords = 0;
	while (*s)
	{
		if (to_up)
		{
			if (isalnum(*s))
			{
				(*s) = toupper(*s);
				nwords++;
				to_up = 0;
			}
		}
		else
		{
			if (isalnum(*s))
			{
				(*s) = tolower(*s);
			}
			else
			{
				to_up = 1;
			}
		}
		s++;
	}

	return nwords;
}

/*===========================================================================*/
/* translate all letter "from" to "to". return the number of replacements    */
/*===========================================================================*/
int  str_trs(char *s, int from, int to)
{
	int	nreplaces;

	nreplaces = 0;
	while (*s)
	{
		if ((*s) == from)
		{
			nreplaces++;
			(*s) = to;
		}
		s++;
	}

	return nreplaces;
}

/*===========================================================================*/
/* remove all letter "c" from the string, return the number of deletions     */
/*===========================================================================*/
int  str_rm(char *s, int c)
{
	int	i, nrms;

	i = nrms = 0;
	while (*s && (*s) != c)
	{
		s++;
		i++;
	}
	while (*s)
	{
		if ((*s) != c)
		{
			s[i++] = *s;
		}
		else
		{
			nrms++;
		}
		s++;
	}

	return nrms;
}

/*===========================================================================*/
/* trim off head/tail white spaces.                                          */
/*===========================================================================*/
void str_trim(char *s)
{
	char 	*head, *tail;

	if ((head = s) != NULL)
	{
		while (*head && isspace(*head))
		{
			head++;
		}
		tail = head + strlen(head) - 1;
		while (tail >= head && isspace(*tail))
		{
			tail--;
		}
		*(tail+1) = '\0';
		if (s != head)
		{
			while ( (*s++ = *head++) )
			{
				;
			}
		}
	}
}

/*===========================================================================*/
/* test if string contains only spaces.                                      */
/*===========================================================================*/
int  str_empty(const char *s)
{
	if (s != NULL)
	{
		while (*s && isspace(*s))
		{
			s++;
		}
	}

	return (s == NULL || (*s) == '\0');
}

/*===========================================================================*/
/* remove all new line characters at the end of string.                      */
/*===========================================================================*/
void str_nonl(char *s)
{
	char	*p;

	if (s != NULL)
	{
		p = s + strlen(s) - 1;
		while (p >= s && (*p == '\n' || *p == '\r'))
		{
			p--;
		}
		*(p+1) = '\0';
	}
}

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
int  str_tok(char buff[], const char delm[], char *subs[], const int nsubs)
{
	int	l1, l2, n = 0;
	char	*b, *p;

	/* split the substrings and count them */
	subs[n++] = p = buff;
	while (n < nsubs)
	{
		l1 = strcspn(p, delm);
		b = p + l1;
		l2 = strspn(b, delm);
		*b = '\0';
		if (l2 == 0)
		{
			break;
		}
		subs[n++] = p = b + l2;
	}
	p = subs[n-1] + strlen(subs[n-1]) - 1;
	while (p >= subs[n-1] && (*p == '\n' || *p == '\r'))
	{
		p--;
	}
	*(p+1) = '\0';

	return n;
}

#ifdef TEST_STR_TOKS
int  main(int argc, char *argv[])
{
	int	i, n;
	char	*subs[5];
	char	d[]=" 	";

	n = str_tok(argv[1], d, subs, 5);
	for (i = 0; i < n; i++)
	{
		printf("<%s>\n", subs[i]);
	}

	return 1;
}
#endif

/*===========================================================================*/
/* given a string in buff which is "delm" delimited, and a pre-allocated     */
/* string pointer array subs, of nsubs elements.                             */
/* this function breaks the string into substrings and assign the substrings */
/* to the string pointers. new-line characters at the end of the last        */
/* substring is also be removed.                                             */
/* if buff contains more than nsubs substrings, the last string pointer      */
/* subs[nsubs-1] will contain the rest of the string without further         */
/* breaking.                                                                 */
/* if subs is given as NULL, it counts the number of substring in buff.      */
/* return the number of substrings it breaks into or counts.                 */
/*===========================================================================*/
int  str_subs(char buff[], const char delm, char *subs[], const int nsubs)
{
	int	n = 0;
	char	*p = buff;

	if (p != NULL && subs != NULL)
	{
		/* split the substrings and count them */
		subs[n++] = p;
		while ((p = strchr(p, delm)) != NULL && n < nsubs)
		{
			*p++ = '\0';
			subs[n++] = p;
		}
		p = subs[n-1] + strlen(subs[n-1]) - 1;
		while (p >= subs[n-1] && (*p == '\n' || *p == '\r'))
		{
			p--;
		}
		*(p+1) = '\0';
	}
	else
	{
		/* count the number of substrings only */
		while (p != NULL)
		{
			n++;
			p = strchr(p+1, delm);
		}
	}

	return n;
}

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
int  str_fixedwidth(char buff[], const char w[], char *subs[], const int nsubs)
{
	int	i, b, n, ll, len;

	len = strlen(buff);
	while (len > 0 && (buff[len-1] == '\n' || buff[len-1] == '\r'))
		len--;

	/* pointing to the right positions */
	ll = b = 0;
	for (n = 0; n < nsubs; n++)
	{
		subs[n] = buff + b;
		if (b >= len)
		{
			break;
		}
		ll = (b + w[n] > len) ? len - b : w[n];
		b += ll;
	}
	if ( --n == -1)
	{
		return 0;
	}

	memmove(subs[n] + n, subs[n], ll);
	subs[n] += n;
	subs[n][ll] = '\0';
	for (i = n+1; i < nsubs; i++)
	{
		subs[i] = subs[n] + ll;
	}

	/* terminate strings */
	for (i = n-1; i >= 0; i--)
	{
		memmove(subs[i] + i, subs[i], w[i]);
		subs[i] += i;
		subs[i][(int)w[i]] = '\0';
	}

	return n+1;
}

#ifdef	TEST_STR_FIXEDW_SUBS
int main()
{
	int	n;
	char	w[] = {3,4,6,2};
	char	*subs[4];
	char	buff[4096];

	while(fgets(buff, sizeof(buff),  stdin))
	{
		n = str_fixedwidth(buff, w, subs, 4);
		printf("%d\t%s\t%s\t%s\t%s\n",
			n, subs[0], subs[1], subs[2], subs[3]);
	}

	return 0;
}
#endif

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
/* If subs is given as NULL, it counts the number of substring in buff,      */
/* in the way that str_subs_quote() would break it,                          */
/* and return the counts.                                                    */
/*===========================================================================*/
int str_subs_quote(char buff[], const char delm, char *subs[], const int nsubs)
{
	int	q, n = 1;
	char	*p = buff;

	if (subs != NULL)
	{
		/* split the substrings and count them */
		subs[0] = p;
		q = *p == '"';
		while ((p = strchr(p, delm)) != NULL && n < nsubs)
		{
			if (q && *(p-1) != '"')
			{
				++p;
			}
			else
			{
				*p++ = '\0';
				subs[n++] = p;
				q = *p == '"';
			}
		}
		p = subs[n-1] + strlen(subs[n-1]) - 1;
		while (p >= subs[n-1] && (*p == '\n' || *p == '\r'))
		{
			p--;
		}
		*(p+1) = '\0';
	}
	else
	{
		/* split the substrings and count them */
		q = *p == '"';
		while ((p = strchr(p, delm)) != NULL)
		{
			if (q && *(p-1) != '"')
			{
				p++;
			}
			else
			{
				n++; p++;
				q = *p == '"';
			}
		}
	}

	return n;
}

/*===========================================================================*/
/* str_dequote subroutine trims off paired quotes (single or double) from    */
/* input string if quotes are present.                                       */
/*===========================================================================*/
void str_dequote(char *s)
{
	char 	*tail, *head = s;

	tail = head + strlen(head) - 1;
	while (tail > head &&
	       ((*head == '"' && *tail == '"') ||
		(*head == '\'' && *tail == '\'')) )
	{
		head++;
		tail--;
	}
	*(tail+1) = '\0';
	if (s != head)
	{
		while ( (*s++ = *head++) )
		{
			;
		}
	}
}

/*===========================================================================*/
/* cat nsubs of string list subs[] to a single string buff with preallocated */
/* size bufsiz. return remaining_size. if return value > 0, there are some   */
/* space left. if return value == 0, the buff is just filled up. if return   */
/* value < 0, the buff is that many bytes short. of course it will not cat   */
/* the last few strings which would overflow the buffer.                     */
/* just passing NULL as buff and 0 as bufsiz cause it to count size required */
/*===========================================================================*/
int  str_cats(char **subs, int nsubs, char delm, char *buff, int bufsiz)
{
	size_t	len;
	int	n = 0;
	char	*p = buff;

	if (buff != NULL && bufsiz > 0)
	{
		*buff = '\0';
		while (n < nsubs && (len = strlen(subs[n])) < bufsiz)
		{
			strncpy(p, subs[n], len);
			p += len;
			*p++ = delm;
			bufsiz -= (len + 1);
			n++;
		}
		if (p != buff)
		{
			*(p-1) = '\0';
		}
	}
	while (n < nsubs)
	{
		bufsiz -= (1 + strlen(subs[n]));
		n++;
	}

	return bufsiz;	/* return remaining bufsiz */
}

/*===========================================================================*/
/* replace the part of src string from substring p, of length len with t.    */
/* place resulting string in buff of size bufsiz. NULL buff or 0 bufsiz is   */
/* counting (size required). NULL t means cut that part off. other than      */
/* all inputs are assumed to be correct. ie pos >= 0 and len > 0 and src     */
/* none null and src longer than pos + len.                                  */
/*===========================================================================*/
int  str_rpls(char *src, char *p, int len, char *t, char *buff, size_t bufsiz)
{
	size_t	tsiz, tlen, tsrc;
	char	*r, *q, *s;

	if (t == NULL)
	{
		t = "";
	}
	tlen = strlen(t);
	tsrc = strlen(src);
	tsiz = tsrc + tlen - len + 1;
	if (buff == NULL || bufsiz < tsiz)
	{
		return (int) tsiz;
	}
	/* copy the tail of src, could reuse the src as buffer */
	if (tlen > len)
	{
		/* tail moving */
		r = buff + tsiz - 1;
		s = src + tsrc;
		q = p + len;
		while (s >= q)
		{
			*r-- = *s--;
		}
	}
	else if (tlen < len)
	{
		/* head moving */
		r = buff + (p - src) + tlen;
		s = p + len;
		while ((*r++ = *s++))
			;
	}
	r = buff;
	s = src;
	while (s < p)
	{
		*r++ = *s++;
	}
	s = t;
	while (*s)
	{
		*r++ = *s++;
	}

	return (int) (bufsiz - tsiz);
}


/*===========================================================================*/
/* package 5: file and filename utilities.                                   */
/*===========================================================================*/
/*===========================================================================*/
/* open/close file to read/write. exit if the operation failed.              */
/* if filename=NULL or -, return stdin or stdout.                            */
/* if filename ends with .gz or .Z, open with zcat.                          */
/*===========================================================================*/
FILE *open_file_read(const char filename[])
{
	int	len;
	FILE	*fp;

	if (filename == NULL || !strcmp(filename, "-"))
	{
		return stdin;
	}

	len = strlen(filename);
	if (len < FILENAME_MAX - 6
		&& ((len > 3 && filename[len-3] == '.'
		    && filename[len-2] == 'g'
		    && filename[len-1] == 'z')
		 || (len > 2 && filename[len-2] == '.'
		    && filename[len-1] == 'Z')))
	{
		char command[FILENAME_MAX];
		sprintf(command, "zcat %s", filename);
		fp = popen(command, "r");
	}
	else
	{
		fp = fopen(filename, "r");
	}

	if (fp == NULL)
	{
		fprintf(stderr, "cannot open file %s for read: %s\n",
			filename, strerror(errno));
		exit(1);
	}

	return fp;
}

FILE *open_file_write(const char filename[])
{
	int	len;
	FILE	*fp;

	if (filename == NULL || !strcmp(filename, "-"))
	{
		return stdout;
	}

	len = strlen(filename);
	if (len < FILENAME_MAX - 6
		&& ((len > 3 && filename[len-3] == '.'
		    && filename[len-2] == 'g'
		    && filename[len-1] == 'z')
		 || (len > 2 && filename[len-2] == '.'
		    && filename[len-1] == 'Z')))
	{
		char command[FILENAME_MAX];
		sprintf(command, "gzip > %s", filename);
		fp = popen(command, "w");
	}
	else
	{
		fp = fopen(filename, "w");
	}

	if (fp == NULL)
	{
		fprintf(stderr, "cannot open file %s for write: %s\n",
			filename, strerror(errno));
		exit(1);
	}

	return fp;
}

int  close_file(FILE *fp, const char filename[])
{
	int	rtn_code, len;

	if (fp == stdin || fp == stdout || fp == stderr)
	{
		return 0;
	}
	if (filename == NULL)
	{
		return fclose(fp);
	}

	len = strlen(filename);
	if (len < FILENAME_MAX - 6
		&& ((len > 3 && filename[len-3] == '.'
		    && filename[len-2] == 'g'
		    && filename[len-1] == 'z')
		 || (len > 2 && filename[len-2] == '.'
		    && filename[len-1] == 'Z')))
	{
		if ( (rtn_code = pclose(fp)) )
		{
			fprintf(stderr, "failed to open %s, return code=%d, "
				"error=%s\n", filename, rtn_code,
				strerror(rtn_code));
			return EOF;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return fclose(fp);
	}
}

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
	printf("#records discarded is %d.\n", ndiscarded);
	return 0;
}
*/
int read_table(const char filename[], const char delm, const int nfields,
	void *record_array[], const size_t size,
	int (*parse_record)(const int lineno, const int nsubs, char *subs[],
		void *record, void *context),
	void *context)
{
	int	n, alloc;
	char	*fn, *r, *p, buff[65536];
	FILE	*fp;
	long long lineno = 0;

	fp = open_file_read(filename);
	fn = (filename == NULL || !strcmp(filename, "-")) ?
		"<stdin>": (char *) filename;

	alloc = 2;
	if ((p = (char *) calloc(size, alloc)) == NULL)
	{
		fprintf(stderr, "file %s line %lld, "
			"failed to allocate %lld bytes "
			"memory in read_table()\n",
			fn, lineno, (long long) size * alloc);
		exit(2);
	}

	n = 0;
	r = p;
	errno = 0;
	while (fgets(buff, sizeof(buff), fp))
	{
		int	fld, nsubs;
		char	*subs[16384];

		lineno++;
		nsubs = str_subs(buff, delm, subs, 16384);
		if (nfields > 0 && nsubs != nfields)
		{
			fprintf(stderr, "file %s line %lld, "
				"#fields %d is not equal to %d "
				"as expected\n",
				fn, lineno, nsubs, nfields);
			exit(3);
		}
		fld = parse_record(lineno, nsubs, subs, (void *)r, context);
		if (fld == 0)
		{
			if (errno)
			{
				fprintf(stderr, "file %s line %lld, "
					"parse error: %s\n",
					fn, lineno, strerror(errno));
				exit(4);
			}
			/* good record, keep */
			n++;
			r += size;
			if (n < alloc)
			{
				continue;
			}
			alloc *= 2;
			if ((p = (char *) realloc(p, size * alloc)) == NULL)
			{
				fprintf(stderr, "file %s line %lld, "
					"failed to allocate %lld bytes memory "
					"in read_table()\n",
					fn, lineno, (long long) size * alloc);
				exit(2);
			}
			r = p + size * n;
		}
		else if (fld <= -1 && fld >= -nsubs)
		{
			/* discarded with notice */
			fprintf(stderr, "file %s line %lld, "
				"discarded for value of field %d is \"%s\"\n",
				fn, lineno, -fld, subs[-fld-1]);
		}
		else if (fld < 0)
		{
			/* discarded without notice */
		}
		else if (fld >= 1 && fld <= nsubs)
		{
			fprintf(stderr, "file %s line %lld, "
				"cannot parse field %d \"%s\"\n",
				fn, lineno, fld, subs[fld-1]);
			exit(5);
		}
		else if (fld > 0)
		{
			exit(5);
		}
	}

	close_file(fp, filename);
	if (n == 0)
	{
		free(p);
		(*record_array) = NULL;
	}
	else if (n != alloc)
	{
		(*record_array) = realloc(p, size * n);
	}
	else
	{
		(*record_array) = (void *) p;
	}

	return n;
}

/*===========================================================================*/
/* count the number of lines text in a file.                                 */
/* return:  0, empty file; -1, cannot open file for read; other, #lines.     */
/*===========================================================================*/
long long get_file_lines(char *filename)
{
	long long lines = 0;
	char	buff[BUFSIZ];
	FILE	*fp;

	if ((fp = fopen(filename, "r")) == NULL &&
		(fp = popen(filename, "r")) == NULL)
	{
		return -1;
	}
	while (fgets(buff, BUFSIZ, fp) != NULL)
	{
		lines++;
	}
	fclose(fp);

	return lines;
}

long long get_file_size(char *filename)
{
	struct stat	statb;

	if (stat(filename, &statb) == -1)
	{
		return -1;
	}
	else
	{
		return (long long) statb.st_size;
	}
}

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
	FILE *fp, const int cchr, long long *lineno)
{
	char	*p, *bp = buff;

	while (fgets(bp, bufsiz, fp) != NULL)
	{
		(*lineno)++;

		/* remove comment */
		if ((p = strchr(bp, cchr)) != NULL)
		{
			*p = '\0';
		}

		/* remove newline */
		for (p = bp; *p != '\0' && *p != '\n' && *p != '\r'; p++)
		{
			;
		}
		*p = '\0';

		/* if it is continuation line, keep reading */
		if (p > bp && *(p-1) == '\\')
		{
			/* continue line, appending */
			bp = p;
			continue;
		}

		/* if it is empty line, keep reading */
		for (p = bp; *p != '\0' && isspace(*p); p++)
		{
			;
		}
		if (*p == '\0')
		{
			/* empty string, try again */
			continue;
		}

		return buff;
	}

	return NULL;
}

/*===========================================================================*/
/* given a string as filename, return a pointer to some where in that string */
/* so that the substring is a base name of the original filename.            */
/*===========================================================================*/
char *base_filename(char *filename)
{
	char	*p, *subfn = filename;

	while ((p = strchr(subfn, '/')) != NULL)
	{
		subfn = p+1;
	}

	for (p = subfn; *p && !isspace(*p); p++)
	{
		;
	}
	*p = '\0';

	return subfn;
}

/*===========================================================================*/
/* using value in environment variable env_name as directory name, and       */
/* filename as a given base filename, it copies full path into path.         */
/* return 0 for success, -1 if env_name==NULL, env_name is not defined,      */
/* env_name value is too long.                                               */
/* if filename == NULL, path is just the directory part with '/' at the end. */
/*===========================================================================*/
int  get_env_filename(char *env_name, char *filename, char *path)
{
	char        	*dir;
	size_t		len;

	if (env_name == NULL || (dir = getenv(env_name)) == NULL ||
		strlen(dir) + strlen(filename) >= FILENAME_MAX - 2)
	{
		return 1;
	}

	strcpy(path, dir);
	if ((len = strlen(path)) && path[len - 1] != '/')
	{
		strcat(path, "/");
	}
	if (filename != NULL)
	{
		strcat(path, filename);
	}

	return 0;
}


/*===========================================================================*/
/* package 6: simple error message handling.                                 */
/*===========================================================================*/
void errout(char *s, ...)
{
	char	buff[BUFSIZ];
	va_list	args;

	va_start(args, s);
	vsprintf(buff, s, args);
	strcat (buff, "\n");
	fputs(buff, stderr);
	va_end(args);

#ifdef NDEBUG
	exit(1);
#else
	abort();
#endif
}

void errout_file(char *s)
{
	fprintf(stderr, "cannot open file \"%s\"\n", s);
#ifdef NDEBUG
	exit(2);
#else
	abort();
#endif
}

void errout_malloc(char *s)
{
	if (s != NULL)
	{
		fprintf(stderr, "cannot malloc memory for %s\n", s);
	}
	else
	{
		fprintf(stderr, "cannot malloc memory\n");
	}
#ifdef NDEBUG
	exit(3);
#else
	abort();
#endif
}

void errout_malloc_size(char *s, size_t size)
{
	if (s != NULL)
	{
		fprintf(stderr,
			"cannot malloc memory of size %lu for %s\n",
			(unsigned long) size, s);
	}
	else
	{
		fprintf(stderr,
			"cannot malloc memory of size %lu\n",
			(unsigned long) size);
	}
#ifdef NDEBUG
	exit(3);
#else
	abort();
#endif
}

void errinfo(char *s, ...)
{
	char	buff[BUFSIZ];
	va_list	args;

	va_start(args, s);
	vsprintf(buff, s, args);
	strcat (buff, "\n");
	fputs(buff, stderr);
	va_end(args);
}

/*===========================================================================*/
/* write message when mline > 0 and nline is multiple of mline.              */
/*===========================================================================*/
void report_status(int mline, int nline, char *s, ...)
{
	if (mline > 0 && (nline % mline == 0))
	{
		char		buff[BUFSIZ];
		va_list		args;

		va_start(args, s);
		vsprintf(buff, s, args);
		strcat(buff, "\n");
		fputs(buff, stderr);
		va_end(args);
	}
}


/*===========================================================================*/
/* package 7: system services.                                               */
/*===========================================================================*/
/*===========================================================================*/
/* Timer: timer to measure program speed.                                    */
/* set_timer(): set timer to current time.                                   */
/* get_timer(): get delta time from last set_timer to now.                   */
/* sprintf_timer(): print delta time from last set_timer to now. return size */
/*===========================================================================*/
void set_timer(Timer *timer)
{
	struct tms	now;

	if ((timer->real_time = times(&now)) == -1)
	{
		timer->user_time = 0;
		timer->system_time = 0;
		perror("Failed to set timer times()");
		return;
	}
	timer->user_time = now.tms_utime;
	timer->system_time = now.tms_stime;
}

void get_timer(Timer *timer, double *realtime, double *usertime, double *systime)
{
	time_t	real_now;
	struct tms	now;

	if (timer->real_time == -1)
	{
		(*realtime) = 0;
		(*usertime) = 0;
		(*systime)  = 0;
		return;
	}

	if ((real_now = times(&now)) == -1)
	{
		perror("Failed to get timer times()");
		(*realtime) = 0;
		(*usertime) = 0;
		(*systime)  = 0;
		return;
	}
	(*realtime) = (double) (real_now - timer->real_time) / CLK_TCK;
	(*usertime) = (double) (now.tms_utime - timer->user_time) / CLK_TCK;
	(*systime) = (double) (now.tms_stime - timer->system_time) / CLK_TCK;
}

int  sprintf_timer(char *buf, size_t bufsiz, Timer *timer)
{
	int	realhour, realmin;
	int	userhour, usermin;
	int	syshour, sysmin;
	double	realsec, usersec, syssec;

	get_timer(timer, &realsec, &usersec, &syssec);

	realhour = (int) (realsec / 3600.0);
	realsec -= 3600.0 * realhour;
	realmin = (int) (realsec / 60.0);
	realsec -= 60.0 * realmin;

	userhour = (int) (usersec / 3600.0);
	usersec -= 3600.0 * userhour;
	usermin = (int) (usersec / 60.0);
	usersec -= 60.0 * usermin;

	syshour = (int) (syssec / 3600.0);
	syssec -= 3600.0 * syshour;
	sysmin = (int) (syssec / 60.0);
	syssec -= 60.0 * sysmin;

	/* Time: Real=0h0m0.00s\tUser=0h0m0.00s\tSystem=0h0m0.00s */
	return snprintf(buf, bufsiz,
	    "Time: Real=%dh%dm%.2fs\tUser=%dh%dm%.2fs\tSystem=%dh%dm%.2fs",
	    (int) realhour, (int) realmin, realsec,
	    (int) userhour, (int) usermin, usersec,
	    (int) syshour, (int) sysmin, syssec);
}

/*===========================================================================*/
/* raise soft data memory ulimit up to hard data memory ulimit.              */
/*===========================================================================*/
void raise_memory_ulimit( void )
{
	struct rlimit rlp;

	if (getrlimit(RLIMIT_DATA, &rlp))
	{
		perror("raise_memory_ulimit()->getrlimit()");
	}
	rlp.rlim_cur = rlp.rlim_max;
	if (setrlimit(RLIMIT_DATA, &rlp))
	{
		perror("raise_memory_ulimit()->setrlimit()");
	}
}

/* get date in YYYYmmdd format, days_from_today is the number of days from */
/* today. if days_from_today = -n, it return the date of n days ago */
int  get_today_date(const int days_from_today)
{
	int	date;
	time_t	tt;
	struct tm *st;

	time(&tt);
	st = localtime(&tt);
	st->tm_mday += days_from_today;

	if (mktime(st) == (time_t)-1)
	{
		return 0;
	}

	date = (st->tm_year + 1900) * 10000 +
		(st->tm_mon+1) * 100 + st->tm_mday;

	return date;
}

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
int  parse_int_list(const char *str, const char delm,
		int *out, const int nout)
{
	int	i, nsub;
	char	*subs[BUFSIZ];
	char	buff[BUFSIZ];

	strncpy(buff, str, BUFSIZ);
	buff[BUFSIZ-1] = '\0';
	nsub = str_subs(buff, delm, subs, BUFSIZ);
	if (nsub > nout)
	{
		nsub = nout;
	}
	for (i = 0; i < nsub; i++)
	{
		char	*endp;

		if (strlen(subs[i]) > 0)
		{
			out[i] = (int) strtol(subs[i], &endp, 10);
			if (endp == NULL)
			{
				return -1;
			}
			while (*endp && isspace(*endp))
			{
				endp++;
			}
			if (*endp != '\0')
			{
				return -1;
			}
		}
	}

	return i;
}

int  parse_double_list(const char *str, const char delm,
		double *out, int const nout)
{
	int	i, nsub;
	char	*subs[BUFSIZ];
	char	buff[BUFSIZ];

	strncpy(buff, str, BUFSIZ);
	buff[BUFSIZ-1] = '\0';
	nsub = str_subs(buff, delm, subs, BUFSIZ);
	if (nsub > nout)
	{
		nsub = nout;
	}
	for (i = 0; i < nsub; i++)
	{
		char	*endp;

		if (strlen(subs[i]) > 0)
		{
			out[i] = strtod(subs[i], &endp);
			if (endp == NULL)
			{
				return -1;
			}
			while (*endp && isspace(*endp))
			{
				endp++;
			}
			if (*endp != '\0')
			{
				return -1;
			}
		}
	}

	return i;
}

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
		const char sc, const char lc)
{
	char	buff[BUFSIZ];
	char	*psave, *pint, *ptmp;
	int	i = 0, len;

	if (p == NULL || *p == '\0')
	{
		return 1;
	}

	/* get rid of spaces */
	for (psave = (char *)p, i = 0; *psave != '\0'; psave++)
	{
		if (!isspace(*psave))
		{
			buff[i++] = *psave;
		}
	}
	buff[i] = '\0';

	if (buff[0] == '\0')
	{
		return 1;
	}

	if ((pint = strchr(buff, (int) sc)) == NULL)
	{
		pint = strchr(buff, (int) lc);
	}

	if (pint == NULL)
	{
		/* case (1) */
		*stop = *start = (int) strtol(buff, &ptmp, 10);
		return (*ptmp) ? 2 : 0;
	}
	else if (pint == buff && *pint == sc && *(pint+1) != '\0')
	{
		/* case (6) */
		*stop = (int) strtol(pint+1, &ptmp, 10);
		return (*ptmp) ? 2 : 0;
	}
	else if (pint == buff && *pint == lc && *(pint+1) != '\0')
	{
		/* case (7) */
		len = (int) strtol(pint+1, &ptmp, 10);
		if (*ptmp != '\0')
		{
			return 2;
		}
		else if (len < 0)
		{
			return 3;
		}
		else
		{
			*stop = *start + len - 1;
			return 0;
		}
	}
	else if (*pint == sc && *(pint+1) != '\0')
	{
		/* case (4) */
		*start = (int) strtol(buff, &ptmp, 10);
		if (ptmp != pint)
		{
			return 2;
		}

		*stop = (int) strtol(pint+1, &ptmp, 10);
		if (*ptmp != '\0')
		{
			return 2;
		}
		else if (*stop < *start)
		{
			return 3;
		}
		else
		{
			return 0;
		}
	}
	else if (*pint == lc && *(pint+1) != '\0')
	{
		/* case (5) */
		*start = (int) strtol(buff, &ptmp, 10);
		if (ptmp != pint)
			return 2;

		len = (int) strtol(pint+1, &ptmp, 10);
		if (*ptmp != '\0')
		{
			return 2;
		}
		else if (len < 0)
		{
			return 3;
		}
		else
		{
			*stop = *start + len - 1;
			return 0;
		}
	}
	else if (pint != buff)
	{
		/* case (2) and (3) */
		*start = (int) strtol(buff, &ptmp, 10);
		return (ptmp == pint) ? 0 : 2;
	}
	else
	{
		/* must be sc or lc along */
		return 2;
	}

	return 0;
}

/*===========================================================================*/
/* Given a string str in the format "a:b,a+c,...", upon return, *range int   */
/* array contains numbers indicating which fields are in the list in the     */
/* order which is specified. Input *nelm is the length of *range. If input   */
/* *nelm == 0, *range will be allocated. *nelm upon return is the new length */
/* of the int array *range. to-last-field is specified by a:-1.              */
/* return the largest field number (0-based) among all specified.            */
/*===========================================================================*/
int  parse_int_ranges(const char *str, int **range, int *nelm)
{
	int	i, len, cnt;
	int	alld;	/* allocated dimension */
	int	numd;	/* number of dimension */
	int	beg, end;
	char	*p;
	char	buff[BUFSIZ];

	strcpy(buff, str);
	p = strtok(buff, ",; \t");
	alld = numd = ((*nelm) > 0 && (*range) != NULL) ? *nelm : 0;
	cnt = -1;
	while (p != NULL)
	{
		beg = 1;
		end = -1;
		if (parse_int_range(p, &beg, &end, ':', '+') != 0)
		{
			errout("cannot parse field specification %s", p);
		}
		if (beg < 1 || (end != -1 && end < beg))
		{
			errout("%s out of acceptable range", p);
		}

		/* maintein the array allocation */
		if (alld == 0 || *range == NULL)
		{
			alld = BUFSIZ;
			(*range) = (int *)malloc(alld * sizeof (int));
			if ((*range) == NULL)
			{
				errout("cannot malloc memory of size %d", alld);
			}
		}
		len = (end == -1) ? 2 : (end - beg + 1);
		while (numd + len > alld)
		{
			alld += BUFSIZ;
			(*range) = (int *)realloc(*range, alld*sizeof(int));
			if ((*range) == NULL)
			{
				errout("cannot malloc memory of size %d", alld);
			}
		}

		/* update the list */
		if (end == -1)
		{
			(*range)[numd++] = beg - 1;
			(*range)[numd++] = -1;
			end = beg;
		}
		else
		{
			for (i = beg - 1; i < end; i++)
			{
				(*range)[numd++] = i;
			}
		}
		if (cnt < end-1)
		{
			cnt = end-1;
		}
		p = strtok (NULL, ",; \t");
	}

	if (numd == 0)
	{
		free((*range));
		(*range) = NULL;
	}
	else
	{
		(*range) = (int *)realloc((*range), numd * sizeof (int));
	}
	*nelm = numd;

	return cnt;
}

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
int  extend_int_ranges(int **range, int *nelm, const int maxd)
{
	int	i, j, len, cnt;
	int	beg;
	int	alld, numd;
	int	* old_range;

	if (*range == NULL || *nelm <= 0)
	{
		return -1;
	}
	if (maxd == 0)
	{
		free(*range);
		*nelm = 0;
		return -1;
	}

	/* duplicate the *range passed in. */
	alld = (*nelm);
	if ((old_range = (int *) malloc(alld * sizeof(int))) == NULL)
	{
		errout("cannot malloc memory of size %d", alld);
	}
	for (i = 0; i < alld; i++)
	{
		old_range[i] = (*range)[i];
	}

	/* reconstruct the *range from old_range */
	cnt = -1;
	for (numd = beg = 0, i = 0; i < *nelm; i++)
	{
		len = (old_range[i] < 0) ? (maxd - beg - 1) : 1;
		while (numd + len > alld)
		{
			alld += BUFSIZ;
			*range = (int *) realloc (*range, alld * sizeof (int));
			if (*range == NULL)
			{
				errout("cannot malloc memory of size %d", alld);
			}
		}
		if (old_range[i] < 0)
		{
			for (j = beg + 1; j < maxd; j++)
			{
				(*range)[numd++] = j;
			}
			if (cnt < maxd-1)
			{
				cnt = maxd-1;
			}
		}
		else if (old_range[i] < maxd)
		{
			(*range)[numd++] = old_range[i];
			beg = old_range[i];
			if (cnt < beg)
			{
				cnt = beg;
			}
		}
		else
		{
			beg = old_range[i];
		}
	}

	free (old_range);
	(*nelm) = numd;
	if (numd == 0)
	{
		free(*range);
		*range = NULL;
	}
	else
	{
		*range = (int *) realloc(*range, numd * sizeof(int));
	}

	return cnt;
}

/* generate ordered unique field lists after parse_int_ranges() */
/* return the maximum number of all fields specified          */
int  unique_int_ranges(int **range, int *nelm, const int maxd)
{
	int	i, j;
	int	beg;
	int	*old_range;

	if (*range == NULL || *nelm == 0)
	{
		return -1;
	}
	if (maxd == 0)
	{
		free (*range);
		*nelm = 0;
		return -1;
	}

	/* reconstruct the *range from old_range */
	if ((old_range = (int *) calloc (maxd, sizeof (int))) == NULL)
	{
		errout("cannot malloc memory of size %d", maxd);
	}
	for (beg = 0, i = 0; i < *nelm; i++)
	{
		if ((*range)[i] < 0)
		{
			for (j = beg + 1; j < maxd; j++)
			{
				old_range[j] = 1;
			}
		}
		else if ((*range)[i] < maxd)
		{
			old_range[(*range)[i]] = 1;
			beg = (*range)[i];
		}
		else
		{
			beg = (*range)[i];
		}
	}
	for (beg = 0, i = 0; i < maxd; i++)
	{
		beg += old_range[i];
	}
	*nelm = beg;
	if (*nelm == 0)
	{
		free (old_range);
		free (*range);
		return -1;
	}

	if ((*range = (int *) realloc (*range, *nelm * sizeof (int))) == NULL)
	{
		errout("cannot malloc memory of size %d", *nelm);
	}
	for (beg = 0, i = 0; i < maxd; i++)
	{
		if (old_range[i] == 1)
		{
			(*range)[beg++] = i;
		}
	}
	free (old_range);
	*nelm = beg;

	return (*range)[beg-1];
}

/* generate switches (1/0) for each field after parse_int_ranges() */
/* return the number of fields specified                         */
int  switch_int_ranges(int **range, int *nelm, const int maxd)
{
	int	i, j;
	int	beg;
	int	alld;
	int	*old_range;

	if (*range == NULL || *nelm == 0)
	{
		free (*range);
		*range = NULL;
		*nelm = maxd;
		if (maxd != 0 &&
			(*range = (int *) calloc (maxd, sizeof (int))) == NULL)
		{
			errout("cannot malloc memory of size %d", maxd);
		}
		return 0;
	}
	if (maxd == 0)
	{
		*nelm = 0;
		free (*range);
		return 0;
	}
	/* duplicate the *range passed in. */
	alld = *nelm;
	if ((old_range = (int *) malloc (alld * sizeof (int))) == NULL)
	{
		errout("cannot malloc memory of size %d", alld);
	}
	for (i = 0; i < alld; i++)
	{
		old_range[i] = (*range)[i];
	}

	/* reconstruct the *range from old_range */
	free (*range);
	if ((*range = (int *) calloc(maxd, sizeof (int))) == NULL)
	{
		errout("cannot malloc memory of size %d", maxd);
	}
	for (beg = 0, i = 0; i < *nelm; i++)
	{
		if (old_range[i] < 0)
		{
			for (j = beg + 1; j < maxd; j++)
			{
				(*range)[j] = 1;
			}
		}
		else if (old_range[i] < maxd)
		{
			(*range)[old_range[i]] = 1;
			beg = old_range[i];
		}
		else
		{
			beg = old_range[i];
		}
	}
	free (old_range);
	for (beg = 0, j = 0; j < maxd; j++)
	{
		beg += (*range)[j];
	}
	*nelm = maxd;

	return beg;
}

/* eop */

/*===========================================================================*/
/* testing functions.                                                        */
/*===========================================================================*/
#ifdef TEST
int main(int argc, char ** argv)
{
	int	rtn;
	int	i, ln;
	int	maxd;
	int	* range;
	FILE	* fp;
	char	rbuff[BUFSIZ];
	char	cbuff[BUFSIZ];

	if (argc != 2)
		errout("usage: %s filename", argv[0]);
	if ((fp = fopen(argv[1], "r")) == NULL)
		errout("cannot open %s for read", argv[1]);
	maxd = 0;
	while (gets(cbuff) != NULL)
	{
		if (!strcmp(cbuff, "quit"))
		{
			exit(0);
		}
		else if (!strcmp(cbuff, "uniq"))
		{
			rtn = unique_int_ranges (&range, &maxd, 13);
			for (i = 0; i < maxd; i++)
			printf("%d;", range[i]);
			printf("(%d)\n", rtn);
			free(range);
			range = NULL;
			maxd = 0;
		}
		else if (!strcmp(cbuff, "ext"))
		{
			rtn = extend_int_ranges (&range, &maxd, 13);
			for (i = 0; i < maxd; i++)
			printf("%d;", range[i]);
			printf("(%d)\n", rtn);
			free(range);
			range = NULL;
			maxd = 0;
		}
		else if (!strcmp(cbuff, "sw"))
		{
			rtn = switch_int_ranges (&range, &maxd, 13);
			for (i = 0; i < maxd; i++)
			printf("%d;", range[i]);
			printf("(%d)\n", rtn);
			free(range);
			range = NULL;
			maxd = 0;
		}
		else
		{
			rtn = parse_int_ranges(cbuff, &range, &maxd);
			for (i = 0; i < maxd; i++)
			printf("%d;", range[i]);
			printf("(%d)\n", rtn);
		}
	}
	return 0;
}
#endif
