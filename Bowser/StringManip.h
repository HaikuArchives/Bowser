
#ifndef STRINGMANIP_H_
#define STRINGMANIP_H_

BString GetWord (const char *, int32);
BString RestOfString (const char *, int32);
BString GetNick (const char *);
BString GetIdent (const char *);
BString GetAddress (const char *);
BString TimeStamp (void);
BString ExpandKeyed (const char *, const char *, const char **);

#endif
