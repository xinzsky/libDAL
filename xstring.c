#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>

void trim(char *str)
{
  int pos = 0;
  char *dest = str;

  /* skip leading blanks */
  while (str[pos] <= ' ' && str[pos] > 0)
  {
    pos++;
  }

  while (str[pos])
  {
    *(dest++) = str[pos];
    pos++;
  }

  *(dest--) = '\0'; /* store the null */

  /* remove trailing blanks */
  while (dest >= str && *dest <= ' ' && *dest > 0)
  {
    *(dest--) = '\0';

  }
}

char *strsqueeze(char *str)
{
  char *rp;
  char *wp;
  int spc;
  
  if(!str) return NULL;
  rp = str;
  wp = str;
  spc = 1;
  
  while(*rp != '\0'){
    if(*rp > 0 && *rp <= ' '){
      if(!spc) *(wp++) = *rp;
      spc = 1;
    } else {
      *(wp++) = *rp;
      spc = 0;
    }
    rp++;
  }
  
  *wp = '\0';
  for(wp--; wp >= str; wp--){
    if(*wp > 0 && *wp <= ' '){
      *wp = '\0';
    } else {
      break;
    }
  }
  
  return str;
}

int strfragnum(const char *str, const char *delimiter)
{
  const char *p, *q;
  int num = 0;
  int len;

  len = strlen(delimiter);
  p = str;
  while ((q = strstr(p, delimiter)) != NULL)
  {
    num++;
    p = q;
    p += len;
  }

  if (*p)
    num++;
  return num;
}

int split(char * str, const char *delimiter, char * fragment[], int frags)
{
  int i,len;
  char *tmp = NULL;
  char *brk = NULL;

  if (!str || !*str || !delimiter || !*delimiter)
    return -1;

  len = strlen(delimiter);
  tmp = str;
  for (i = 0; i < frags; i++) {
    brk = strstr(tmp, delimiter);
    fragment[i] = tmp;
    if (!brk) break;
    *brk = '\0';
    tmp = brk + len;
  }

  if (i >= frags)
    return frags;
  else
    return i + 1;
}

char * stristr(const char *StrBase, const char *SubBase)
{

  while (*StrBase)
  {
    if (toupper(*StrBase) == toupper(*SubBase))
    {
      const char * Str, * Sub;
      Str = StrBase + 1;
      Sub = SubBase + 1;
      while (*Sub && toupper(*Sub) == toupper(*Str))
      {
        Sub++;
        Str++;
      }
      if (! *Sub) return((char *)StrBase);
    }
    StrBase++;
  }

  return(NULL);
}

/* Convert a number in the [0, 16) range to
   the ASCII representation of the corresponding hexadecimal digit.
   `+ 0' is there so you can't accidentally use it as an lvalue.  */
#define XNUM_TO_DIGIT(x) ("0123456789ABCDEF"[x] + 0)
#define XNUM_TO_digit(x) ("0123456789abcdef"[x] + 0)
#define ISXDIGIT(x) isxdigit (x)
#define TOUPPER(x) toupper (x)
#define XDIGIT_TO_NUM(h) ((h) < 'A' ? (h) - '0' : TOUPPER (h) - 'A' + 10)
#define X2DIGITS_TO_NUM(h1, h2) ((XDIGIT_TO_NUM (h1) << 4) + XDIGIT_TO_NUM (h2))

char *bin2hex(unsigned char *bin, int binlen, char *hex)
{
  int i;

  for(i=0;i<binlen;i++)
  {
  	*hex = XNUM_TO_digit (bin[i] >> 4);
    hex++;
    *hex = XNUM_TO_digit (bin[i] & 0xf);
    hex++;
  }

  *hex = '\0';
  return hex;
}

int hex2bin(char *hex,unsigned char *bin)
{
  int i;

  i = 0;
  while(*hex)
  {
    if (!hex[1] || !(ISXDIGIT(hex[0]) && ISXDIGIT(hex[1])))
      return 0;

    bin[i] = X2DIGITS_TO_NUM(hex[0], hex[1]);
    hex += 2;
    i++;
  }

  return i;
}


