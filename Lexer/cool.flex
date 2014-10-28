/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

/*
 *  Add Your own definitions here
 */

int comm=0;		/* Variable for comment nesting */

%}

/*
 * Define names for regular expressions here.
 */

whitespace	[ \f\r\t\v]

ints		[0-9]+
types		[A-Z][a-zA-Z0-9_]*
objects		[a-z][a-zA-Z0-9_]*

SINGLES		"+"|"-"|"*"|"/"|"~"|"<"|"="|"("|")"|"{"|"}"|";"|":"|"."|","|"@"
INVALID		"`"|"!"|"#"|"$"|"%"|"^"|"&"|"_"|"["|"]"|"|"|[\\]|">"|"?"

/*
 * State Definitions
 */
%x comment string escape

%%


 /*
  *  The multiple-character operators.
  */
"=>"			return DARROW;
"<-"			return ASSIGN;
"<="			return LE;

 /*
  * Case Insensitive Keywords
  */
(?i:class)		return CLASS;
(?i:else)		return ELSE;
(?i:fi)			return FI;
(?i:if)			return IF;
(?i:in)			return IN;
(?i:inherits)		return INHERITS;
(?i:let)		return LET;
(?i:loop)		return LOOP;
(?i:pool)		return POOL;
(?i:then)		return THEN;
(?i:while)		return WHILE;
(?i:case)		return CASE;
(?i:esac)		return ESAC;
(?i:of)			return OF;
(?i:new)		return NEW;
(?i:isvoid)		return ISVOID;
(?i:not)		return NOT;

 /*
  * Start Case sensitive booleans
  */
(t)(?i:rue)		{
				cool_yylval.boolean = true;
				return BOOL_CONST;
			}
(f)(?i:alse)		{
				cool_yylval.boolean = false;
				return BOOL_CONST;
			}
	
 /*
  * Identifiers for Ints, Types and Objects
  */
{ints}			{
				cool_yylval.symbol = inttable.add_string(yytext);
				return INT_CONST;
			}
{types}			{
				cool_yylval.symbol = idtable.add_string(yytext);
				return TYPEID;
			}
{objects}|(self)	{
				cool_yylval.symbol = idtable.add_string(yytext);
				return OBJECTID;
			}

 /*
  * Single Character Special Syntactic Symbols
  */
{SINGLES}		return int(yytext[0]);

 /*
  * Single Invalid Characters
  */
{INVALID}		{
				cool_yylval.error_msg = yytext;
				return ERROR;
			}

 /*
  * All Comments handled here
  */

"--"(.)*

"*)"			{
				cool_yylval.error_msg = "Unmatched *)";
				return ERROR;
			}
"(*"			{
				++comm;
				BEGIN(comment);				
			}

<comment>"(*"		++comm;
<comment>"*)"		{
				--comm;
				if(comm==0)
					BEGIN(INITIAL);
				else if(comm<0){
					cool_yylval.error_msg = "Unmatched *)";
					comm=0;
					BEGIN(INITIAL);
					return ERROR;
				}
			}
<comment>\n		++curr_lineno;
<comment>.
<comment>{whitespace}+
<comment><<EOF>>	{
				BEGIN(INITIAL);
				if(comm>0){
					cool_yylval.error_msg = "EOF in comment.";
					comm=0;
					return ERROR;
				}
			}


 /*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  *
  */
"\""			{
				BEGIN(string);
				string_buf_ptr = string_buf;
			}

<string>"\""		{
				if(string_buf_ptr - string_buf > MAX_STR_CONST-1){
					*string_buf = '\0';
					cool_yylval.error_msg = "String constant too long";
					BEGIN(escape);
					return ERROR;
				}
				*string_buf_ptr = '\0';
				cool_yylval.symbol = stringtable.add_string(string_buf);
				BEGIN(INITIAL);
				return STR_CONST;
			}
<string><<EOF>>		{
				cool_yylval.error_msg = "EOF in string constant";
				BEGIN(INITIAL);
				return ERROR;
			}
<string>\0		{
				*string_buf = '\0';
				cool_yylval.error_msg = "String contains null character";
				BEGIN(escape);
				return ERROR;
			}
<string>\n		{
				*string_buf = '\0';
				BEGIN(INITIAL);
				cool_yylval.error_msg = "Unterminated string constant";
				return ERROR;
			}
<string>"\\n"		*string_buf_ptr++ = '\n';
<string>"\\t"		*string_buf_ptr++ = '\t';
<string>"\\b"		*string_buf_ptr++ = '\b';
<string>"\\f"		*string_buf_ptr++ = '\f';
<string>"\\"[^\0]	*string_buf_ptr++ = yytext[1];
<string>.		*string_buf_ptr++ = *yytext;

<escape>[\n|"]		BEGIN(INITIAL);
<escape>[^\n|"]	
	
 /*
  * Skip all Whitespace characters
  */
\n		curr_lineno++;
{whitespace}+

 /*
  * When nothing matches report error text
  */
.		{
			cool_yylval.error_msg = yytext;
			return ERROR;
		}

%%
