//----------------------------------------------------------------------------
//
//  Derived from Dave Hylands & http://www.efgh.com/software/gprintf.htm
//  code (public domain)
//
//  The only reason of these functions using is to avoid
//  heavy and huge GCC standard C RTL
//
//----------------------------------------------------------------------------

#include <stddef.h>
#include <stdarg.h>

#define  NO_OPTION       0x00   // No options specified.
#define  MINUS_SIGN	     0x01   // Should we print a minus sign?
#define  RIGHT_JUSTIFY   0x02   // Should field be right justified?
#define  ZERO_PAD        0x04   // Should field be zero padded?
#define  CAPITAL_HEX     0x08   // Did we encounter %X?

#define  IsOptionSet(p,x)     (((p)->options & (x)) != 0)
#define  IsOptionClear(p,x)   (((p)->options & (x)) == 0)
#define  SetOption(p,x)       (p)->options = ((p)->options |  (x))
#define  ClearOption(p,x)     (p)->options = ((p)->options & ~(x))

typedef int (*dprintf_func)(void *outParm, int ch);

typedef struct _PARAMETERS
{
   int  numOutputChars;
   int  options;
   int  minFieldWidth;
   int  editedStringLen;
   int  leadingZeros;
   dprintf_func outFunc;
   void * outParm;
}PARAMETERS;

typedef struct _STRPRINTFPARAMS
{
   char * str;
   int   maxLen;
}STRPRINTFPARAMS;

//-- Local prototypes

static void OutputChar(PARAMETERS *p, int c);
static void OutputField(PARAMETERS *p, char *s);
static int  StrPrintfFunc(void *outParm, int ch);
static int  vStrPrintf(char *outStr, int maxLen, const char *fmt, va_list args);
static int  vStrXPrintf(dprintf_func outFunc, void *outParm,
                        const char *fmt, va_list args);

//----------------------------------------------------------------------------
int tn_snprintf( char *outStr, int maxLen, const char *fmt, ... )
{
   int      rc;
   va_list  args;

   va_start( args, fmt );
   rc = vStrPrintf( outStr, maxLen, fmt, args );
   va_end( args );

   return rc;
}

//----------------------------------------------------------------------------
static int vStrPrintf( char *outStr, int maxLen, const char *fmt, va_list args )
{
   STRPRINTFPARAMS    strParm;

   strParm.str    = outStr;
   strParm.maxLen = maxLen - 1; /* Leave space for temrinating null char   */

   return vStrXPrintf(StrPrintfFunc, &strParm, fmt, args );
}

//----------------------------------------------------------------------------
static int vStrXPrintf( dprintf_func  outFunc, void * outParm,
                 const char * fmt, va_list args)
{
   PARAMETERS  p;
   char        controlChar;

   p.numOutputChars  = 0;
   p.outFunc         = outFunc;
   p.outParm         = outParm;

   controlChar = *fmt++;

   while(controlChar != '\0')
   {
      if(controlChar == '%')
      {
         short precision = -1;
         short longArg   = 0;
         short base      = 0;

         controlChar = *fmt++;
         p.minFieldWidth = 0;
         p.leadingZeros  = 0;
         p.options       = NO_OPTION;

         SetOption( &p, RIGHT_JUSTIFY);

          // Process [flags]

         if(controlChar == '-')
         {
            ClearOption( &p, RIGHT_JUSTIFY );
            controlChar = *fmt++;
         }

         if(controlChar == '0')
         {
            SetOption( &p, ZERO_PAD );
            controlChar = *fmt++;
         }

         // Process [width]

         if(controlChar == '*')
         {
            p.minFieldWidth = (short)va_arg( args, int );
            controlChar = *fmt++;
         }
         else
         {
            while(('0' <= controlChar) && (controlChar <= '9'))
            {
               p.minFieldWidth = p.minFieldWidth * 10 + controlChar - '0';
               controlChar = *fmt++;
            }
         }

          // Process [.precision]

         if(controlChar == '.')
         {
            controlChar = *fmt++;
            if(controlChar == '*')
            {
               precision = (short)va_arg( args, int );
               controlChar = *fmt++;
            }
            else
            {
               precision = 0;
               while(('0' <= controlChar) && (controlChar <= '9'))
               {
                  precision = precision * 10 + controlChar - '0';
                  controlChar = *fmt++;
               }
            }
         }

         //  Process [l]

         if(controlChar == 'l')
         {
            longArg = 1;
            controlChar = *fmt++;
         }

         //  Process type.

         if(controlChar == 'd')
         {
            base = 10;
         }
         else if(controlChar == 'x')
         {
            base = 16;
         }
         else if(controlChar == 'X')
         {
            base = 16;
            SetOption( &p, CAPITAL_HEX );
         }
         else if(controlChar == 'u')
         {
            base = 10;
         }
         else if( controlChar == 'o')
         {
            base = 8;
         }
         else if ( controlChar == 'b' )
         {
            base = 2;
         }
         else if ( controlChar == 'c' )
         {
            base = -1;
            ClearOption( &p, ZERO_PAD );
         }
         else if(controlChar == 's' )
         {
            base = -2;
            ClearOption( &p, ZERO_PAD );
         }

         if(base == 0)  // invalid conversion type
         {
            if(controlChar != '\0')
            {
               OutputChar( &p, controlChar );
               controlChar = *fmt++;
            }
         }
         else
         {
            if(base == -1)            // conversion type c
            {
               char c;
               c = (char)va_arg( args, int );
               p.editedStringLen = 1;
               OutputField(&p, &c);
            }
            else if(base == -2)      // conversion type s
            {
               char *string;
               string = va_arg(args, char*);

               p.editedStringLen = 0;
               while(string[ p.editedStringLen ] != '\0')
               {
                  if((precision >= 0) && (p.editedStringLen >= precision))
                  {
                      // We don't require the string to be null terminated
                      // if a precision is specified.
                     break;
                  }
                  p.editedStringLen++;
               }
               OutputField( &p, string );
            }
            else  // conversion type d, b, o or x
            {
               unsigned long x;

               // Worst case buffer allocation is required for binary output,
               // which requires one character per bit of a long.

               char buffer[ 8 * sizeof(unsigned long) + 4 ]; //-- 4 for align,need 1

               p.editedStringLen = 0;
               if(longArg)
               {
                  x = va_arg( args, unsigned long );
               }
               else if(controlChar == 'd')
               {
                  x = va_arg(args,int);
               }
               else
               {
                  x = va_arg(args,unsigned);
               }

               if((controlChar == 'd') && ((long) x < 0 ))
               {
                  SetOption(&p,MINUS_SIGN);
                  x = - (long) x;
               }

               do
               {
                  int c;
                  c = x % base + '0';
                  if(c > '9')
                  {
                     if(IsOptionSet(&p, CAPITAL_HEX))
                     {
                        c += 'A'-'9'-1;
                     }
                     else
                     {
                        c += 'a'-'9'-1;
                     }
                  }
                  buffer[ sizeof( buffer ) - 1 - p.editedStringLen++ ] = (char)c;
               } while((x /= base) != 0);

               if((precision >= 0) && (precision > p.editedStringLen))
               {
                  p.leadingZeros = precision - p.editedStringLen;
               }
               OutputField(&p, buffer + sizeof(buffer) - p.editedStringLen);
            }
            controlChar = *fmt++;
         }
      }
      else
      {
          // We're not processing a % output. Just output the character that
          // was encountered.

         OutputChar(&p,controlChar);
         controlChar = *fmt++;
      }
   }
   return p.numOutputChars;
}

static void OutputChar(PARAMETERS *p, int c)
{
   if(p->numOutputChars >= 0)
   {
      int n = (*p->outFunc)(p->outParm, c);

      if(n >= 0)
         p->numOutputChars++;
      else
         p->numOutputChars = n;
   }
}

//-----------------------------------------------------------------------------
static void OutputField( PARAMETERS *p, char *s )
{
   int padLen = p->minFieldWidth - p->leadingZeros - p->editedStringLen;

   padLen = p->minFieldWidth - p->leadingZeros - p->editedStringLen;

   if(IsOptionSet(p,MINUS_SIGN))
   {
      if(IsOptionSet(p,ZERO_PAD))
      {
          // Since we're zero padding, output the minus sign now. If we're space
          // padding, we wait until we've output the spaces.
         OutputChar( p, '-' );
      }
      // Account for the minus sign now, even if we are going to output it
      // later. Otherwise we'll output too much space padding.

      padLen--;
   }

   if(IsOptionSet(p,RIGHT_JUSTIFY))
   {
       // Right justified: Output the spaces then the field.
      while ( --padLen >= 0 )
      {
         OutputChar(p, p->options & ZERO_PAD ? '0' : ' ');
      }
   }
   if(IsOptionSet(p,MINUS_SIGN) && IsOptionClear(p,ZERO_PAD))
   {
       // We're not zero padding, which means we haven't output the minus
       // sign yet. Do it now.
      OutputChar( p, '-' );
   }

   while(--p->leadingZeros >= 0)  // Output any leading zeros.
      OutputChar(p,'0');
   while(--p->editedStringLen >= 0) // Output the field itself.
      OutputChar(p,*s++ );

   // Output any trailing space padding. Note that if we output leading
   // padding, then padLen will already have been decremented to zero.

   while(--padLen >= 0)
      OutputChar(p, ' ');
}

//-----------------------------------------------------------------------------
static int StrPrintfFunc( void *outParm, int ch )
{
   STRPRINTFPARAMS * strParm;

   strParm = (STRPRINTFPARAMS *)(outParm);
   if(strParm != NULL)
   {
      if(strParm->maxLen > 0 )
      {
         *strParm->str++ = (char)ch;
         *strParm->str = '\0';
         strParm->maxLen--;

         return 1;
      }
   }
   return -1;
}

//----------------------------------------------------------------------------
int tn_abs(int i)
{
  if(i < 0)
    i = -i;
  return i;
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_strlen
 * �������� :
 * ���������:
 * ���������:
 *----------------------------------------------------------------------------*/
int tn_strlen(const char *str)
{
  int  n = 0;
  const char *s = str;
  
  while (*s) {
    s++;
    n++;
  }
  return n;
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_strcpy
 * �������� :
 * ���������:
 * ���������:
 *----------------------------------------------------------------------------*/
char* tn_strcpy(char *dst, const char *src)
{
  char *d = dst;
  
  while(*src)
    *dst++ = *src++;
  
  *dst = '\0';
  return d;
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_strncpy
 * �������� :
 * ���������:
 * ���������:
 *----------------------------------------------------------------------------*/
char* tn_strncpy(char *dst, const char *src, int n)
{
  char *d = dst;
  
  while (n-- && *src)
    *dst++ = *src++;
  *dst = '\0';
  
  return d;
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_strcat
 * �������� :
 * ���������:
 * ���������:
 *----------------------------------------------------------------------------*/
char* tn_strcat(char *dst, const char *src)
{
  char *d = dst;
  
  while (*dst)
    dst++;
  
  while (*src)
    *dst++ = *src++;
  *dst = '\0';
  
  return d;
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_strncat
 * �������� :
 * ���������:
 * ���������:
 *----------------------------------------------------------------------------*/
char* tn_strncat(char *dst, const char *src, int n)
{
  char *d = dst;
  
  while (*dst)
    dst++;
  while (n-- && *src)
    *dst++ = *src++;
  *dst = '\0';
  return d;
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_strcmp
 * �������� :
 * ���������:
 * ���������:
 *----------------------------------------------------------------------------*/
int tn_strcmp(const char *str1, const char *str2)
{
  register int c1, res;

  for (;;) {
    c1  = *str1++;
    res = c1 - *str2++;
    if (c1 == 0 || res != 0)
      break;
  }
  return res;
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_strncmp
 * �������� :
 * ���������:
 * ���������:
 *----------------------------------------------------------------------------*/
int tn_strncmp(const char *str1, const char *str2, int num)
{
  if (num <= 0)
    return 0;
  
  while (--num && *str1 && *str1 == *str2) {
    str1++;
    str2++;
  }
  
  return (*(unsigned char *)str1 - *(unsigned char *)str2);
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_memset
 * �������� :
 * ���������:
 * ���������:
 *----------------------------------------------------------------------------*/
void* tn_memset(void *dst, int ch, int length)
{
  register char *ptr = (char *)dst - 1;
  
  while (length--)
    *++ptr = ch;
  
  return dst;
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_memcpy
 * �������� :
 * ���������:
 * ���������:
 *----------------------------------------------------------------------------*/
void* tn_memcpy(void *s1, const void *s2, int n)
{
  register int mask = (int)s1 | (int)s2 | n;
  
  if (n == 0)
    return s1;
  
  if (mask & 0x1) {
    register const char *src = (const char *)s2;
    register char *dst = (char*)s1;
  
    do {
      *dst++ = *src++;
    } while (--n);
  
    return s1;
  }
  
  if (mask & 0x2) {
    register const short *src = (const short *)s2;
    register       short *dst = (short *)s1;
  
    do {
      *dst++ = *src++;
    } while ( n -= 2);
  
    return s1;
  }
  else {
    register const int *src = (const int *)s2;
    register       int *dst = (int *)s1;
  
    do {
      *dst++ = *src++;
    } while (n -= 4);
  
    return s1;
  }
}

/*-----------------------------------------------------------------------------*
 * �������� : tn_memcmp
 * �������� :
 * ���������:
 * ���������:
 *----------------------------------------------------------------------------*/
int tn_memcmp(const void *s1, const void *s2, int n)
{
  register const unsigned char *ptr1 = s1;
  register const unsigned char *ptr2 = s2;

  if (n <= 0)
    return 0;
  
  while (--n && *ptr1 == *ptr2) {
    ptr1++;
    ptr2++;
  }
  
  return (*ptr1 - *ptr2);
}

/*-----------------------------------------------------------------------------*
  �������� :  atoi
  �������� :  ����������� ������ � ����� �����.
  ���������:  s - ������������� ������.
  ���������:  ���������� ����� �����.
*-----------------------------------------------------------------------------*/
int tn_atoi(const char *s)
{
  int i, n, sign;
  /* skip white space */
	for (i = 0; s[i] == ' ' || s[i] == '\n' || s[i] == '\t'; i++);
	sign = 1;
	if (s[i] == '+' || s[i] == '-')	/* sign */
		sign = (s[i++] == '+') ? 1 : -1;
	for (n = 0; s[i] >= '0' && s[i] <= '9'; i++)
		n = 10 * n + s[i] - '0';
	return (sign * n);
}

//------------------------------------------------------------------------------
