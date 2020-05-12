#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>


#define SAFEALLOC(var,Type) if((var=(Type*)malloc(sizeof(Type)))==NULL)err("not enough memory");

//	 0	  1		2	  3		 4		5	  6	  7	   8
enum{ID, END, BREAK, CHAR, DOUBLE, ELSE, FOR, IF, INT, 
//	  9		  10	 11		12	   13		14		  15
	RETURN, STRUCT, VOID, WHILE, CT_INT , CT_REAL, CT_CHAR,
//		16		 17		  18	  19	20		21		  22
    CT_STRING, COMMA, SEMICOLON, LPAR, RPAR, LBRACKET, RBRACKET,
//	 23	   24	25	 26	  27   28	29	 30	  31  32	 33
    LACC, RACC, ADD, SUB, MUL, DIV, DOT, AND, OR, NOT, ASSIGN,
//	 34		 35	   36	  37	  38		39
    EQUAL, NOTEQ, LESS, LESSEQ, GREATER, GREATEREQ}; // tokens codes

char *codeNames[40] = {"ID", "END", "BREAK", "CHAR", "DOUBLE", "ELSE", "FOR", 
	"IF", "INT", "RETURN", "STRUCT", "VOID", "WHILE", "CT_INT" , "CT_REAL",
	"CT_CHAR", "CT_STRING", "COMMA", "SEMICOLON", "LPAR", "RPAR", "LBRACKET",
	"RBRACKET", "LACC", "RACC", "ADD", "SUB", "MUL", "DIV", "DOT", "AND",
	"OR", "NOT", "ASSIGN", "EQUAL", "NOTEQ", "LESS", "LESSEQ", "GREATER",
	"GREATEREQ"};


typedef struct _Token{
    int code; // code (name)
    union{
        char *text; // used for ID, CT_STRING (dynamically allocated)
        long int i; // used for CT_INT, CT_CHAR
        double r; // used for CT_REAL
    };
    
    int line; // the input file line
    struct _Token *next; // link to the next token
}Token;

int line = 1;
Token *tokens = NULL;
Token *lastToken = NULL;

Token *crtTk = NULL;
Token *consumedTk = NULL;

Token *addTk(int code)
{
    Token *tk;
    SAFEALLOC(tk,Token);
    tk->code=code;
    tk->line=line;
    tk->next=NULL;
    
    if(lastToken){
        lastToken->next=tk;
    }else{
        tokens=tk;
    }
    
    lastToken=tk;
	printf("%s ", codeNames[tk->code]);
    return tk;
}

void err(const char *fmt,...)
{
    va_list va;
    va_start(va,fmt);
    fprintf(stderr,"error: ");
    vfprintf(stderr,fmt,va);
    fputc('\n',stderr);
    va_end(va);
    
    exit(-1);
}



void tkerr(const Token *tk,const char *fmt,...)
{
    va_list va;
    va_start(va,fmt);
    fprintf(stderr,"error in line %d: ",tk->line);
    vfprintf(stderr,fmt,va);
    fputc('\n',stderr);
    va_end(va);
    exit(-1);
}

char* createString(const char* start, const char* end) {
	int length = end - start + 1;
	char *myString=(char*)malloc(length * sizeof(char));
	snprintf(myString, length, "%s", start);
	return myString;
}

char escapeChar(char ch){
	switch(ch){
		case 'a':
			return '\a';
		case 'b':
			return '\b';
		case 'f':
			return '\f';
		case 'n':
			return '\n';
		case 'r':
			return '\r';
		case 't':
			return '\t';
		case 'v':
			return '\v';
		case '\"':
			return '\"';
		case '\'':
			return '\'';
		case '\\':
			return '\\';
		case '\?':
			return '\?';
		case '\0':
			return '\0';
	}
}

// Symbols

enum { TB_INT, TB_DOUBLE, TB_CHAR, TB_STRUCT, TB_VOID };

struct _Symbol;
typedef struct _Symbol Symbol;
typedef struct {
	Symbol **begin; 	//the beginning of the symbols (or NULL)
	Symbol **end; 		//the position after the last symbol
	Symbol **after; 	//the position after the allocated space
}Symbols;

typedef struct {
	int typeBase; 	//TB_*
	Symbol *s; 		//struct definition for TB_STRUCT
	int nElements; 	//>0 array of given size, 0=array without size, <0 non array
}Type;

typedef struct _Symbol {
	const char *name; 	//reference to the name stored in a token
	int cls; 			//CLS_*
	int mem; 			//MEM*_
	Type type;
	int depth;
	union {
		Symbols args; 		//used only for functions
		Symbols members; 	//used only for structs
	};
}Symbol;



enum { CLS_VAR, CLS_FUNC, CLS_EXTFUNC, CLS_STRUCT };

enum { MEM_GLOBAL, MEM_ARG, MEM_LOCAL };


Symbols symbols;
int crtDepth = 0;
Symbol *crtStruct;
Symbol *crtFunc;

void initSymbols(Symbols *symbols) {
	symbols->begin = NULL;
	symbols->end = NULL;
	symbols->after = NULL;
}

Symbol *addSymbol(Symbols *symbols, const char *name, int cls) {
	Symbol *s;
	if (symbols->end == symbols->after) { //create more room
		int count = symbols->after - symbols->begin;
		int n = count * 2; 		//double the room
		if (n == 0) n = 1; 		//needed for the initial case
		symbols->begin = (Symbol **)realloc(symbols->begin, n * sizeof(Symbol *));
		if (symbols->begin == NULL) err("not enough memory");
		symbols->end = symbols->begin + count;
		symbols->after = symbols->begin + n;
	}
	SAFEALLOC(s, Symbol);
	*symbols->end++ = s;
		printf("ADDED SYMBOL %s WITH CLASS %d and DEPTH %d\n", name, cls,crtDepth);

	s->name = name;
	s->cls = cls;
	s->depth = crtDepth;
	return s;
}

Symbol *findSymbol(Symbols *symbols, const char *name) {
	int size = (symbols->end) - (symbols->begin);
	--size;
	while (size >= 0) {
		if (strcmp(symbols->begin[size]->name, name) == 0) {
			return symbols->begin[size];
		}
		--size;
	}
	return NULL;
}

void deleteSymbolsAfter(Symbols *symbols, Symbol *symbol) {
	int cnt = 0;
	int i;
	int size = symbols->end - symbols->begin;
	int found = size;
	for (cnt = 0; cnt < size; cnt++) {
		if ((symbols->begin[cnt]) == symbol) {
			found = cnt;
			for (cnt++; cnt < size; cnt++) {
				free(symbols->begin[cnt]);
			}
			symbols->end = symbols->begin + found + 1;
		}
	}
}

void addVar(Token *tkName, Type *t){
	Symbol *s;
	if (crtStruct) {
		if (findSymbol(&crtStruct->members, tkName->text))
			tkerr("symbol redefinition: %s", tkName->text);
		s = addSymbol(&crtStruct->members, tkName->text, CLS_VAR);
	}
	else if (crtFunc) {
		s = findSymbol(&symbols, tkName->text);
		if (s&&s->depth == crtDepth)
			tkerr("symbol redefinition: %s", tkName->text);
		s = addSymbol(&symbols, tkName->text, CLS_VAR);
		s->mem = MEM_LOCAL;
	}
	else {
		if (findSymbol(&symbols, tkName->text))
			tkerr("symbol redefinition: %s", tkName->text);
		s = addSymbol(&symbols, tkName->text, CLS_VAR);
		s->mem = MEM_GLOBAL;
	}
	s->type = *t;
}

// End of symbols declaration and functions

// Type

Type createType(int typeBase, int nElements) {
	Type t;
	t.typeBase = typeBase;
	t.nElements = nElements;
	return t;
}

typedef union {
	long int i; 		//int,CHAR
	double d; 			//double
	const char *str; 	//char[]
}CtVal;

typedef struct {
	Type type; 		//type of the result
	int isLVal; 	//if it is a Lval
	int isCtVal; 	//if it is a constant value (int, real, char, char[])
	CtVal ctVal; 	//the constant value
}RetVal;


void cast(Type *dst, Type *src) {
	if (src->nElements > -1) {
		if (dst->nElements > -1) {
			if (src->typeBase != dst->typeBase)
				tkerr(crtTk, "an array can not be converted to an array of another type");
		}
		else {
			tkerr(crtTk, "an array can not be converted to a non-array");
		}
	}
	else {
		if (dst->nElements > -1) {
			tkerr(crtTk, "a non-array cannot be converted to an array");
		}
	}
	switch (src->typeBase) {
		case TB_CHAR:
		case TB_INT:
		case TB_DOUBLE:
			switch (dst->typeBase) {
				case TB_CHAR:
				case TB_INT:
				case TB_DOUBLE:
					return;
			}
		case TB_STRUCT:
			if (dst->typeBase == TB_STRUCT) {
				if (src->s != dst->s) 
					tkerr(crtTk, "a structure cannot be converted to another one");
				return;
			}
	}
	tkerr(crtTk, "Incompatible types");
}

Type getArithType(Type *s1, Type *s2) {
	switch (s1->typeBase) {
		case TB_CHAR: 
			return *s2;
		case TB_INT:
			switch (s2->typeBase) {
				case TB_CHAR:
				case TB_INT: 
					return *s1;
				case TB_DOUBLE: 
					return *s2;
			}
		case TB_DOUBLE: 
			return *s1;
	}
}


Symbol *addExtFunc(const char *name, Type type) {
	Symbol *s = addSymbol(&symbols, name, CLS_EXTFUNC);
	s->type = type;
	initSymbols(&s->args);
	return s;
}

Symbol *addFuncArg(Symbol *func, const char *name, Type type) {
	Symbol *a = addSymbol(&func->args, name, CLS_VAR);
	a->type = type;
	return a;
}

// End of type declaration and functions

int getNextToken(char *file)
{
    int state=0, nCh;
    char ch;
	char* stringRepresentation;
    const char *pStartCh, *pCrtCh = file;
    Token *tk;
    
    while(1){ // infinite loop
        ch = *pCrtCh;
		printf("ce sa mai zic       %c  state:%d\n", ch, state);

        switch(state){
            case 0: // transitions test for state 0
                if(isalpha(ch)||ch=='_'){
                    pStartCh = pCrtCh; // memorizes the beginning of the ID
                    pCrtCh++; // consume the character
                    state = 1; // set the new state
                }
                else if(ch==' '||ch=='\r'||ch=='\t'){
                    pCrtCh++; // consume the character and remains in state 0
                }
                else if(ch=='\n'){ // handled separately in order to update the current line
                    line++;
                    pCrtCh++;
                }
				else if(isdigit(ch) && ch != '0'){
					pStartCh = pCrtCh;
					state = 7;
					pCrtCh++;
				}
				else if(isdigit(ch) && ch == '0'){
					pStartCh = pCrtCh;
					state = 16;
					pCrtCh++;
				}
				else if(ch=='\'') {
					pCrtCh++;
					state = 21;
				}
				else if(ch=='\"') {
					pStartCh = pCrtCh;
					pCrtCh++;
					state = 24;
				}
                else if(ch==','){
					pCrtCh++;
					addTk(COMMA);
				}
				else if(ch==';'){
					pCrtCh++;
					addTk(SEMICOLON);
				}
				else if(ch=='('){
					pCrtCh++;
					addTk(LPAR);
				}
				else if(ch==')'){
					pCrtCh++;
					addTk(RPAR);
				}
				else if(ch=='['){
					pCrtCh++;
					addTk(LBRACKET);
				}
				else if(ch==']'){
					pCrtCh++;
					addTk(RBRACKET);
				}
                else if(ch=='{'){
					pCrtCh++;
					addTk(LACC);
				}
				else if(ch=='}'){
					pCrtCh++;
					addTk(RACC);
				}
				else if(ch=='+'){
					pCrtCh++;
					addTk(ADD);
				}
				else if(ch=='-'){
					pCrtCh++;
					addTk(SUB);
				}
				else if(ch=='*'){
					pCrtCh++;
					addTk(MUL);
				}
                else if(ch=='/') {
					pCrtCh++;
					state=3;
				}
				else if(ch=='.'){
					pCrtCh++;
					addTk(DOT);
				}
                else if(ch=='&'){
					pCrtCh++;
					ch=*pCrtCh;
					if(ch=='&'){
						pCrtCh++;
						addTk(AND);
					}
				else 
					tkerr(tk, "Expected \'&\'binary operator\n");
				}

                else if(ch=='|'){
					pCrtCh++;
					ch=*pCrtCh;
					if(ch=='|'){
						pCrtCh++;
						addTk(AND);
					}
				else 
					tkerr(tk, "Expected \'|\'binary operator\n");
				}
                else if(ch=='!') {
					pCrtCh++;
					ch=*pCrtCh;
					if(ch=='='){
						pCrtCh++;
						addTk(NOTEQ);
					}
					else{
						addTk(NOT);                
					}
				}
                else if(ch == '='){
					pCrtCh++;
					ch = *pCrtCh;
					if(ch == '='){
						pCrtCh++;
						addTk(EQUAL);
					}
					else{
						addTk(ASSIGN);              
					}
				}               
                else if(ch == '<'){
					pCrtCh++;
					ch = *pCrtCh;
					if(ch == '='){
						pCrtCh++;
						addTk(LESSEQ);
					}
					else{
						addTk(LESS);            
					}
				}
				else if(ch == '>'){
					pCrtCh++;
					ch=*pCrtCh;
					if(ch == '='){
						pCrtCh++;
						addTk(GREATEREQ);
					}
					else{
						addTk(GREATER);             
					}
				}
                else if(ch == 0){ // the end of the input string
                    addTk(END);
                    return END;
                }
                else 
                    tkerr(addTk(END),"invalid character");
                break;
            case 1:
                if(isalnum(ch) || ch=='_')
                    pCrtCh++;
                else 
                    state = 2;
                break;
            case 2:
                nCh=pCrtCh-pStartCh; // the id length
                // keywords tests
                if(nCh==5&&!memcmp(pStartCh,"break",5))
                    tk = addTk(BREAK);
                else if(nCh==4&&!memcmp(pStartCh,"char",4))
                    tk = addTk(CHAR);
                else if(nCh==3&&!memcmp(pStartCh,"int",3))
                    tk = addTk(INT);
                else if(nCh==6&&!memcmp(pStartCh,"double",6))
                    tk = addTk(DOUBLE);
                else if(nCh==4&&!memcmp(pStartCh,"void",4))
                    tk = addTk(VOID);
                else if(nCh==2&&!memcmp(pStartCh,"if",2))
                    tk = addTk(IF);
                else if(nCh==4&&!memcmp(pStartCh,"else",4))
                    tk = addTk(ELSE);
                else if(nCh==3&&!memcmp(pStartCh,"for",3))
                    tk = addTk(FOR);
                else if(nCh==5&&!memcmp(pStartCh,"while",5))
                    tk = addTk(WHILE);
                else if(nCh==6&&!memcmp(pStartCh,"return",6))
                    tk = addTk(RETURN);
                else if(nCh==6&&!memcmp(pStartCh,"struct",6))
                    tk = addTk(STRUCT);
                // … all keywords …
                else{ // if no keyword, then it is an ID
                    tk = addTk(ID);
                    tk->text = createString(pStartCh,pCrtCh);
                }
				state = 0;
				break;
                // return tk->code;
            case 3:
                if(ch == '*' ) {
					pCrtCh++;
					state = 4;
				}
				else if(ch == '/'){
					pCrtCh++;
					state = 6;
				}
				else {
					addTk(DIV);
					state = 0;
				}
				break;
            case 4:
                if(ch == '*'){
					pCrtCh++;
					state = 5;
				}
				else {
					pCrtCh++;
                    state = 4;
				}
				break;
            case 5:
				if(ch=='/') {
					pCrtCh++;
					state = 0;
				}
				else if(ch == '*') {
					pCrtCh++;
                    state = 5;
				}
				else if (ch !='*' && ch !='/'){
					pCrtCh++;
					state= 4;
				}
				break;
            case 6:
				if(ch !='\n' && ch !='\r' && ch !='\0'){
					pCrtCh++;
                    state = 6;
				}
				else if(ch=='\n') {
					pCrtCh++;
					state = 0;
					line++;
				}
				else
				{
					pCrtCh++;
					state = 0;
				} 
				break;
			case 7:
				if(isdigit(ch)){
					pCrtCh++;
					state = 7;
				}
				else if(ch == '.'){
					pCrtCh++;
					state = 8;
				}
				else if(ch=='e' || ch=='E'){
					pCrtCh++;
					state = 9;
				}
				else 
					state = 10;
				break;
			case 8:
				if(isdigit(ch)){
					pCrtCh++;
					state = 11;
				}
				else 
					tkerr(tk, "Float number format expected");
				break;
			case 9:
				if(ch=='+' || ch=='-'){
					pCrtCh++;
					state = 14;
				}
				else if(isdigit(ch)){
					pCrtCh++;
					state = 15;
				}
				else {
					pCrtCh++;
					state = 14;
				}
				break;
			case 10:
				tk = addTk(CT_INT);
				stringRepresentation = createString(pStartCh, pCrtCh);
				tk->i = atoi(stringRepresentation);
				state = 0;
				break;
			case 11:
				if(isdigit(ch)){
					pCrtCh++;
					state = 11;
				}
				else if(ch=='e' || ch=='E'){
					pCrtCh++;
					state = 9;
				}
				else
					state = 12;
				break;
			case 12:
				tk = addTk(CT_REAL);
				tk->r = strtod(pStartCh, NULL);
				state = 0;
				break;
			case 14:
				if(isdigit(ch)){
					pCrtCh++;
					state = 15;
				} 
				else
					tkerr(tk, "Number format ERROR");
				break;
			case 15:
				if(isdigit(ch)){
					pCrtCh++;
					state = 15;
				}
				else 
					state = 12;
				break;
			case 16:
				if(ch >= '0' && ch <= '7'){
					pCrtCh++;
					state = 17;
				}
				else if(ch=='x' || ch=='X'){
					pCrtCh++;
					state = 18;
				}
				else if(ch=='.'){
					pCrtCh++;
					state = 8;
				}
				else if(ch=='e' || ch=='E'){
					pCrtCh++;
					state = 9;
				}
				else{
					state = 10;
				}
				break;
			case 17:
				if(ch>='0' && ch <= '7') {
					state = 17;
					pCrtCh++;
				}
				else
					state = 20;
				break;
			case 18:
				if((isdigit(ch)) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')){
					pCrtCh++;
					state = 18;
				}
				else
					state = 19;
				break;
			case 19:
				tk = addTk(CT_INT);
				stringRepresentation = createString(pStartCh, pCrtCh);
				tk->i = strtol(stringRepresentation, NULL, 16);
				state = 0;
				break;
			case 20:
				tk = addTk(CT_INT);
				stringRepresentation = createString(pStartCh, pCrtCh);
				tk->i = strtol(stringRepresentation, NULL, 8);
				state = 0;
				break;
			case 21:
				if(ch=='\\'){
					pCrtCh++;
					state=22;
				}
				else{
					pCrtCh++;
					state=23;
				}
				break;
			case 22:
				if(strchr("abfnrtv\'?\"\\0", ch)){
					pCrtCh++;
					state=23;
				}
				else{
					tkerr(tk, "Invalid character\n");
				}
				break;
			case 23:
				if(ch=='\''){
					tk = addTk(CT_CHAR);
					if(*(pCrtCh -2) == '\\'){
						tk->i = escapeChar(*(pCrtCh -1));
					}
					else
						tk->i = *(pCrtCh -1);
					pCrtCh++;
					state = 0;
				}
				else
					tkerr(tk, "Character expected");
				break;
			case 24:
				if(ch == '\\'){
					pCrtCh++;
					state = 25;
				}
				else{
					state = 26;
				}
				break;
			case 25:
				if(strchr("abfnrtv'?\"\\0", ch)){
					pCrtCh++;
					state = 26;
				}
				else 
					tkerr(tk, "Invalid escape character\n");
				break;
			case 26:
				if(ch == '\"') {
					tk = addTk(CT_STRING);
					tk->text = createString(pStartCh + 1, pCrtCh);
					pCrtCh++;
					state = 0;
				}else {
					state = 24;
					pCrtCh++;
				}
				break;
        }
    }
}


int consume(int code){
	if(crtTk->code == code){
		consumedTk = crtTk;
		crtTk = crtTk->next;
		return 1;
	}
	return 0;
	}

void addExtFuncs() {
	initSymbols(&symbols);
	Symbol *s;

	s = addExtFunc("put_s", createType(TB_VOID, -1));
	addFuncArg(s, "s", createType(TB_CHAR, 0));

	s = addExtFunc("get_s", createType(TB_VOID, -1));
	addFuncArg(s, "s", createType(TB_CHAR, 0));

	s = addExtFunc("put_i", createType(TB_VOID, -1));
	addFuncArg(s, "i", createType(TB_INT, 0));

	s = addExtFunc("get_i", createType(TB_INT, -1));

	s = addExtFunc("put_d", createType(TB_VOID, -1));
	addFuncArg(s, "d", createType(TB_DOUBLE, -1));

	s = addExtFunc("get_d", createType(TB_DOUBLE, -1));

	s = addExtFunc("put_c", createType(TB_VOID, -1));
	addFuncArg(s, "c", createType(TB_CHAR, 0));

	s = addExtFunc("get_c", createType(TB_CHAR, -1));

	s = addExtFunc("seconds", createType(TB_DOUBLE, -1));
}

int unit(){
	addExtFuncs();

	while (1) {
		if (declStruct()) {}
		else if (declFunc()) {}
		else if (declVar()) {}
		else break;
	}
	if (!consume(END)) tkerr(crtTk, "Missing END token");
	return 1;
}

int declStruct(){
	Token *startTk = crtTk;
	Token *tkName = crtTk;;

	if (!consume(STRUCT)) return 0;
	if (!consume(ID)) tkerr(crtTk, "Expected ID after struct declaration.");
	if (!consume(LACC)) {
		crtTk = startTk;
		return 0;
	}
	if (findSymbol(&symbols, tkName->text))
		tkerr("symbol redefinition: %s", tkName->text);
	crtStruct = addSymbol(&symbols, tkName->text, CLS_STRUCT);
	initSymbols(&crtStruct->members);
	while (1) {
		if (declVar()) {}
		else break;
	}
	if (!consume(RACC)) tkerr(crtTk, "Expected '}' after struct declaration.");
	if (!consume(SEMICOLON)) tkerr(crtTk, "Expected ';' after struct declaration.");
	crtStruct = NULL;
	return 1;
}

int declVar(){
	Token *startTk = crtTk;
	Type t;
	Token tkName;

	if (typeBase(&t)) {
		tkName = *crtTk;
		if (consume(ID)) {
			if (!arrayDecl(&t)) {
				t.nElements = -1;
			}
			addVar(&tkName, &t);
			while (1) {
				if (consume(COMMA)) {
					tkName = *crtTk;
					if (consume(ID)) {
						if (!arrayDecl(&t)) {
							t.nElements = -1;
						}
						addVar(&tkName, &t);
					}
					else tkerr(crtTk, "Expected ID after ',' in variable declaration.");
				}
				else break;
			}
			if (consume(SEMICOLON)) return 1;
			else tkerr(crtTk, "Expected ';' after variable declaration.");
		}
		else crtTk = startTk;
	}
	else crtTk = startTk;

	return 0;
}

int typeBase(Type *ret){
	Token *startTk = crtTk;

	if (consume(INT)) {
		ret->typeBase = TB_INT;
	}else if (consume(DOUBLE)) {
		ret->typeBase = TB_DOUBLE;
	}else if (consume(CHAR)) {
		ret->typeBase = TB_CHAR;
	}else if (consume(STRUCT)) {
		Token *tkName = crtTk;
		if (consume(ID)) {
				Symbol *s = findSymbol(&symbols, tkName->text);
				if (s == NULL)tkerr(crtTk, "undefined symbol: %s", tkName->text);
				if (s->cls != CLS_STRUCT)tkerr(crtTk, "%s is not a struct", tkName->text);
				ret->typeBase = TB_STRUCT;
				ret->s = s;
		}
		else tkerr(crtTk, "Expected ID after struct declaration.");
	}
	else {
		return 0;
	}
	return 1;
}

int arrayDecl(Type *ret){
	Token *startTk = crtTk;

	if (consume(LBRACKET)) {
		RetVal rv;
		if (expr(&rv)) {
			if (!rv.isCtVal) tkerr(crtTk, "The array size is not a constant");
			if (rv.type.typeBase != TB_INT) tkerr(crtTk, "The array size is not an integer");
			ret->nElements = rv.ctVal.i;
		}
		else {
			ret->nElements = 0; //for now we do not compute real size
		}
		if (consume(RBRACKET)) {
			return 1;
		}
		else tkerr(crtTk, "Expected ']' after array declaration.");
	}
	crtTk = startTk;

	return 0;
}

int typeName(Type *ret){
	Token *startTk = crtTk;
	if (typeBase(ret)) {
		if (!arrayDecl(ret)) {
			ret->nElements = -1;
		}
		return 1;
	}
	return 0;
}


int declFunc(){
	Token *startTk = crtTk;
	Type t;
	Token *tkName;

	if (typeBase(&t)) {
		if(consume(MUL)){
			t.nElements = 0;
		}
		else {
			t.nElements = -1;
		}
	}else if (consume(VOID)) {
		t.typeBase = TB_VOID;
	}
	else {
		return 0;
	}
	tkName = crtTk;
	if (!(consume(ID))) {
		crtTk = startTk;
		return 0;
	}
	if (!consume(LPAR)) {
		crtTk = startTk;
		return 0;
	}
	if (findSymbol(&symbols, tkName->text))
		tkerr(crtTk, "symbol redefinition: %s", tkName->text);
	crtFunc = addSymbol(&symbols, tkName->text, CLS_FUNC);
	initSymbols(&crtFunc->args);
	crtFunc->type = t;
	++crtDepth;
	if (funcArg()) {
		while (1) {
			if (consume(COMMA)) {
				if (!funcArg()) tkerr(crtTk, "Expected func arg in stm");
			}
			else break;
		}
	}
	if (!consume(RPAR)) tkerr(crtTk, "Expected ')' in func declaration");
	--crtDepth;
	if (!stmCompound()) tkerr(crtTk, "Compound statement expected");
	deleteSymbolsAfter(&symbols, crtFunc);
	crtFunc = NULL;

	return 1;
}

int funcArg(){
	Token *startTk = crtTk;
	Type t;
	Token *tkName;

	if (typeBase(&t)) {
		if (consume(ID)) {
			if (!arrayDecl(&t)) {
				t.nElements = -1;
			}
			Symbol  *s = addSymbol(&symbols, tkName->text, CLS_VAR);
			s->mem = MEM_ARG;
			s->type = t;
			s = addSymbol(&crtFunc->args, tkName->text, CLS_VAR);
			s->mem = MEM_ARG;
			s->type = t;
			return 1;
		}
		else tkerr(crtTk, "Expected ID for function argument.");
	}
	crtTk = startTk;

	return 0;
}

int stm(){
	Token *startTk = crtTk;
	RetVal rv, rv2, rv3;

	if (stmCompound()) {
		return 1;
	}
	else if (consume(IF)) {
		if (consume(LPAR)) {
			if (expr(&rv)) {
				if (rv.type.typeBase == TB_STRUCT) tkerr(crtTk, "A structure cannot be logically tested.");
				if (consume(RPAR)) {
					if (stm()) {
						if (consume(ELSE)) {
							if (stm()) return 1;
							else tkerr(crtTk, "Statement expected after else declaration");
						}
						// return 1;
					}
					else tkerr(crtTk, "Statement expected");
				}
				else tkerr(crtTk, "Expected ')' ");
			}
			else tkerr(crtTk, "Expected expresion inside if condition");
		}
		else tkerr(crtTk, "Expected '(' ");
	}
	else if (consume(WHILE)) {
		if (consume(LPAR)) {
			if (expr(&rv)) {
				if (rv.type.typeBase == TB_STRUCT) tkerr(crtTk, "a structure cannot be logically tested");
				if (consume(RPAR)) {
					if (stm()) {
						return 1;
					}
					else tkerr(crtTk, "Expected statement inside while body");
				}
				else tkerr(crtTk, "Expected ')' ");
			}
			else tkerr(crtTk, "Expected expresion inside while declaration");
		}
		else tkerr(crtTk, "Expected '(' ");
	}
	else if (consume(FOR)) {
		if (consume(LPAR)) {
			expr(&rv);
			if (consume(SEMICOLON)) {
				if(expr(&rv2)){
					if (rv2.type.typeBase == TB_STRUCT) tkerr(crtTk, "a structure cannot be logically tested");
				}
				if (consume(SEMICOLON)) {
					expr(&rv3);
					if (consume(RPAR)) {
						if (stm()) {
							return 1;
						}
						else tkerr(crtTk, "Expected statement inside for body");
					}
				 	else tkerr(crtTk, "Expected ')' ");
				}
				else tkerr(crtTk, "Expected ';' inside for 1");
			}
			else tkerr(crtTk, "Expected ';' inside for 2");
		}
		else tkerr(crtTk, "Expected '(' ");
	}
	else if (consume(BREAK)) {
		if (consume(SEMICOLON))
			return 1;
		else tkerr(crtTk, "Expected ';' after break statement");
	}
	else if (consume(RETURN)) {
		if(expr(&rv)) {
			if (crtFunc->type.typeBase == TB_VOID) tkerr(crtTk, "a void function cannot return a value");
			cast(&crtFunc->type, &rv.type);
			if (consume(SEMICOLON)) {
				return 1;
			}
			else tkerr(crtTk, "Expected ';' after return statement");
		}
	}
	// crtTk = startTk;

	// if (consume(SEMICOLON)) {
	// 	return 1;
	// }
	else { 
		expr(&rv);
		if (!consume(SEMICOLON)) tkerr(crtTk, "Expected ';' after expression");
		return 1;
		
	}
	// crtTk = startTk;
	return 0;
}

int stmCompound(){
	Token *startTk = crtTk;
	Symbol *start = symbols.end[-1];
	if (consume(LACC)) {
		++crtDepth;
		while (1) {
			if (declVar()) continue;
			if (stm())     continue;
			else break;
		}
		if (consume(RACC)) {
			--crtDepth;
			deleteSymbolsAfter(&symbols, start);
			return 1;
		}
		else tkerr(crtTk, "Expected '}' ");
	}
	// crtTk = startTk;

	return 0;
}

int expr(RetVal *rv){
	Token *startTk = crtTk;

	if (exprAssign(rv))
		return 1;	
	// crtTk = startTk;

	return 0;
}


int exprAssign(RetVal *rv){
	Token *startTk = crtTk;
	RetVal rve;

	if (exprUnary(rv)) {
		if (consume(ASSIGN)) {
			if (exprAssign(&rve)){
				if (!rv->isLVal) tkerr(crtTk, "cannot assign to a non-lval");
				if ((rv->type.nElements > -1) || (rve.type.nElements > -1))
					tkerr(crtTk, "the arrays cannot be assigned");
				cast(&rv->type, &rve.type);
				rv->isCtVal = rv->isLVal = 0;
			}
			else tkerr(crtTk, "Error at expression assign");
			return 1;
		}
	}
	crtTk = startTk;

	if (exprOr(rv)) return 1;
	// crtTk = startTk;

	return 0;
}

int exprOrLite(RetVal *rv) {
	Token *startTk = crtTk;
	RetVal rve;

	if (consume(OR)) {
		if (exprAnd(&rve)) {
			if (exprOrLite(rv)) {
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr(crtTk, "a structure cannot be logically tested");
				rv->type = createType(TB_INT, -1);
				rv->isCtVal = rv->isLVal = 0;
			}
			else tkerr(crtTk, "Error at OR expression");
		}
		else tkerr(crtTk, "Error at OR expression");
	}

	// crtTk = startTk;

	return 1;
}

int exprOr(RetVal *rv){
	Token *startTk = crtTk;

	if (exprAnd(rv)) {
		if (exprOrLite(rv)) {
			return 1;
		}
		else tkerr(crtTk, "Error at OR expression");
	}

	return 0;
}

int exprAndLite(RetVal *rv) {
	Token *startTk = crtTk;
	RetVal rve;

	if (consume(AND)) {
		if (exprEq(&rve)) {
			if (exprAndLite(&rv)) {
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr(crtTk, "a structure cannot be logically tested");
				rv->type = createType(TB_INT, -1);
				rv->isCtVal = rv->isLVal = 0;
			}
			else tkerr(crtTk, "Error at AND expression");
		}
		else tkerr(crtTk, "Error at AND expression");
	}
	// crtTk = startTk;

	return 1;
}

int exprAnd(RetVal *rv){
	Token *startTk = crtTk;

	if (exprEq(rv)) {
		if (exprAndLite(rv)) {
			return 1;
		}
		else tkerr(crtTk, "Error at AND expression");
	}

	return 0;
}

int exprEqLite(RetVal *rv) {
	Token *startTk = crtTk;
	RetVal rve;

	if (consume(EQUAL) || consume(NOTEQ)) {
		if (exprRel(&rve)) {
			if (exprEqLite(&rv)) {
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr(crtTk, "a structure cannot be compared");
				rv->type = createType(TB_INT, -1);
				rv->isCtVal = rv->isLVal = 0;
			}
			else tkerr(crtTk, "Error at equality expression");
		}
		else tkerr(crtTk, "Error at equality expression");
	}
	// crtTk = startTk;

	return 1;
}

int exprEq(RetVal *rv){
	Token *startTk = crtTk;

	if (exprRel(rv)) {
		if (exprEqLite(rv)) {
			return 1;
		}
		else tkerr(crtTk, "Error at equality expression");
	}

	return 0;
}

int exprRelLite(RetVal *rv) {
	Token *startTk = crtTk;
	RetVal rve;

	if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)) {
		if (exprAdd(&rve)) {
			if (exprRelLite(rv)) {
				if (rv->type.nElements > -1 || rve.type.nElements > -1)
					tkerr(crtTk, "an array cannot be compared");
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr(crtTk, "a structure cannot be compared");
				rv->type = createType(TB_INT, -1);
				rv->isCtVal = rv->isLVal = 0;
			}
			else tkerr(crtTk, "Error at rel expression");
		}
		else tkerr(crtTk, "Error at rel expression");
	}
	// crtTk = startTk;

	return 1;
}

int exprRel(RetVal *rv){
	Token *startTk = crtTk;

	if (exprAdd(rv)) {
		if (exprRelLite(rv)) {
			return 1;
		}
		else tkerr(crtTk, "Error at rel expression");
	} 

	return 0;
}

int exprAddLite(RetVal *rv) {
	Token *startTk = crtTk;
	RetVal rve;

	if (consume(ADD) || consume(SUB)) {
		if (exprMul(&rve)) {
			if (exprAddLite(rv)) {
				if (rv->type.nElements>-1 || rve.type.nElements>-1)
					tkerr(crtTk, "an array cannot be added or subtracted");
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr(crtTk, "a structure cannot be added or subtracted");
				rv->type = getArithType(&rv->type, &rve.type);
				rv->isCtVal = rv->isLVal = 0;
			}
			else tkerr(crtTk, "Error at add/sub expression");
		}
		else tkerr(crtTk, "Error at add/sub expression");
	}
	// crtTk = startTk;

	return 1;
}

int exprAdd(RetVal *rv){
	Token *startTk = crtTk;

	if (exprMul(rv)) {
		if (exprAddLite(rv))
			return 1;
		else tkerr(crtTk, "Error at add/sub expression");
	}

	return 0;
}

int exprMulLite(RetVal *rv) {
	Token *startTk = crtTk;
	RetVal rve;

	if (consume(MUL) || consume(DIV)) {
		if (exprCast(&rve)) {
			if (exprMulLite(rv)) {
				if (rv->type.nElements>-1 || rve.type.nElements>-1)
					tkerr(crtTk, "an array cannot be multiplied or divided");
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr(crtTk, "a structure cannot be multiplied or divided");
				rv->type = getArithType(&rv->type, &rve.type);
				rv->isCtVal = rv->isLVal = 0;
			}
			else tkerr(crtTk, "Error at multiply/divide expression");
		}
		else tkerr(crtTk, "Error at multiply/divide expression");
	}
	// crtTk = startTk;

	return 1;
}

int exprMul(RetVal *rv){
	Token *startTk = crtTk;

	if (exprCast(rv)) {
		if (exprMulLite(rv)) {
			return 1;
		}
		else tkerr(crtTk, "Error at multiply/divide expression");
	}

	return 0;
}

int exprCast(RetVal *rv){
	Token *startTk = crtTk;
	Type t;
	RetVal rve;

	if (consume(LPAR)) {
		if (typeName(&t)) {
			if (consume(RPAR)) {
				if (exprCast(&rve)) {
					cast(&t, &rve.type);
					rv->type = t;
					rv->isCtVal = rv->isLVal = 0;
					return 1;
				}
				else tkerr(crtTk, "Error at cast");
			}
			else tkerr(crtTk, "Expected ')' ");
		}
		else tkerr(crtTk, "Expected cast type name");
	}
	if (exprUnary(rv)) {
		return 1;
	}
	// crtTkaram = startTk;

	return 0;
}

int exprUnary(RetVal *rv){
	Token *startTk = crtTk;

	if (consume(SUB)) {
		if (exprUnary(rv)) {
			if (rv->type.nElements >= 0)
				tkerr(crtTk, "unary '-' cannot be applied to an array");
			if (rv->type.typeBase == TB_STRUCT)
				tkerr(crtTk, "unary '-' cannot be applied to a struct");
			rv->isCtVal = rv->isLVal = 0;
			// return 1;
		}
		else tkerr(crtTk, "Expected unary expression SUB");
	}
	else if (consume(NOT)) {
		if (exprUnary(rv)) {
			if (rv->type.typeBase == TB_STRUCT)tkerr(crtTk, "'!' cannot be applied to a struct");
			rv->type = createType(TB_INT, -1);
			rv->isCtVal = rv->isLVal = 0;
		}
		else tkerr(crtTk, "Expected unary expression NOT");
	}
	else if (exprPostfix(rv)) {
		return 1;
	}
	else 
		return 0;
	// crtTk = startTk;

	return 1;
}

int exprPostfixLite(RetVal *rv) {
	Token *startTk = crtTk;
	RetVal rve;

	if (consume(LBRACKET)) {
		if (expr(&rve)) {
			if (rv->type.nElements<0)tkerr(crtTk, "only an array can be indexed");
			Type typeInt = createType(TB_INT, -1);
			cast(&typeInt, &rve.type);
			rv->type = rv->type;
			rv->type.nElements = -1;
			rv->isLVal = 1;
			rv->isCtVal = 0;
			if (!consume(RBRACKET)) tkerr(crtTk, "Expected ']' ");
		}
		else tkerr(crtTk, "Expected expression after '[' in postfix expression");
	}
	else if (consume(DOT)) {
		Token *tkName = crtTk;
		if (consume(ID)) {
			Symbol *sStruct = rv->type.s;
			Symbol *sMember = findSymbol(&sStruct->members, tkName->text);
			if (!sMember)
				tkerr(crtTk, "struct %s does not have a member %s", sStruct->name, tkName->text);
			rv->type = sMember->type;
			rv->isLVal = 1;
			rv->isCtVal = 0;
		}
		else tkerr(crtTk, "ID expected in postfix expression");
	}
	else 
		return 1;

	// crtTk = startTk;
	exprPostfixLite(rv);
	return 1;
}

int exprPostfix(RetVal *rv){
	Token *startTk = crtTk;

	if (exprPrimary(rv)) {
		if (exprPostfixLite(rv)) {
			return 1;
		}
		else tkerr(crtTk, "Error at postfix declaration");
	}
	// crtTk = startTk;

	return 0;
}

int exprPrimary(RetVal *rv){
	Token *startTk = crtTk;
	Token *tkName = crtTk;
	RetVal arg;

	if (consume(ID)) {
		Symbol *s = findSymbol(&symbols, tkName->text);
		if (!s)tkerr(crtTk, "undefined symbol %s", tkName->text);
		rv->type = s->type;
		rv->isCtVal = 0;
		rv->isLVal = 1;
		if (consume(LPAR)) {
			Symbol **crtDefArg = s->args.begin;
			if (s->cls != CLS_FUNC && s->cls != CLS_EXTFUNC)
				tkerr(crtTk, "call of the non-function %s", tkName->text);
			if (expr(&arg)) {
				if (crtDefArg == s->args.end)tkerr(crtTk, "too many arguments in call");
				cast(&(*crtDefArg)->type, &arg.type);
				++crtDefArg;
			}
			while (1) {
				if (consume(COMMA)) {
					if (expr(&arg)) {
						if (crtDefArg == s->args.end)tkerr(crtTk, "too many arguments in call");
						cast(&(*crtDefArg)->type, &arg.type);
						++crtDefArg;
					}
					else tkerr(crtTk, "Expected expression after ','");
				}
				else break;
			}
			if (consume(RPAR)) {
				if (crtDefArg != s->args.end)tkerr(crtTk, "too few arguments in call");
				rv->type = s->type;
				rv->isCtVal = rv->isLVal = 0;
			}
			else tkerr(crtTk, "Missing RPAR in primary EXPR after ID.");
		}
		else {
			if (s->cls == CLS_FUNC || s->cls == CLS_EXTFUNC)
				tkerr(crtTk, "missing call for function %s", tkName->text);
		}
	}
	else {
		if (consume(CT_INT)) {
			Token *tki = crtTk;
			rv->type = createType(TB_INT, -1);
			rv->ctVal.i = tki->i;
			rv->isCtVal = 1;
			rv->isLVal = 0;
		}

		if (consume(CT_REAL)) {
			Token *tkr = crtTk;
			rv->type = createType(TB_DOUBLE, -1);
			rv->ctVal.d = tkr->r;
			rv->isCtVal = 1;
			rv->isLVal = 0;
		}

		if (consume(CT_CHAR)) {
			Token *tkc = crtTk;
			rv->type = createType(TB_CHAR, -1);
			rv->ctVal.i = tkc->i;
			rv->isCtVal = 1;
			rv->isLVal = 0;
		}

		if (consume(CT_STRING)) {
			Token *tks = crtTk;
			rv->type = createType(TB_CHAR, 0);
			rv->ctVal.str = tks->text;
			rv->isCtVal = 1;
			rv->isLVal = 0;
		}

		if (consume(LPAR)) {
			if (expr(rv)) {
				if (consume(RPAR)) {
					return 1;
				}
				else tkerr(crtTk, "Expected ')' in primary expression");
			}
			else tkerr(crtTk, "Expected expression after '(' ");
		}
		else {
			return 0;
		}
		// crtTk = startTk;
	}
	return 1;
}


char *putFileContentToString(char *filename){
	char *buffer;
	long int length = 0;

	FILE * fp = fopen (filename, "rb");

	if (!fp) 
		err("Error opening file (%s)", filename);
	
	fseek (fp, 0, SEEK_END);
	length = ftell (fp);
	fseek (fp, 0, SEEK_SET);
	buffer = (char*)malloc(length * sizeof(char));
	if (buffer)
	{
		fread (buffer, 1, length, fp);
	}
	fclose (fp);
	buffer[length] = '\0';

	return buffer;
}

int getFileLength(char *filename) {
	FILE * fp = fopen (filename, "rb");

	if (!fp) 
		err("Error opening file (%s)", filename);
	
	fseek (fp, 0, SEEK_END);
	int length = ftell (fp);
	fclose (fp);
	return length;
}

void printTokens(Token * printer) {
	while(printer!=NULL){
		if(printer->code == ID || printer->code == CT_STRING)
			printf("%s:%s ", codeNames[printer->code], printer->text);
		else if(printer->code == CT_INT)
			printf("%s:%ld ", codeNames[printer->code], printer->i);
		else if(printer->code == CT_REAL)
			printf("%s:%f ", codeNames[printer->code], printer->r);
		else if(printer->code == CT_CHAR)
			printf("%s:%c ", codeNames[printer->code], printer->i);
		else
			printf("%s ", codeNames[printer->code]);
		printf("\n");
		printer=printer->next;
	}
}


void lexicalAnalysis(FILE *fp) {
	char *buffer = (char*)malloc(getFileLength(fp) * sizeof(char));
	buffer = putFileContentToString(fp);

	printf("%s\n", buffer);

	getNextToken(buffer);

	Token *printer = tokens;
	printTokens(printer);
}

void syntacticAnalysis() {
	Token *syn = tokens;
	crtTk = syn;
	if (unit())
		printf("Syntactical Analysis stage DONE\n");
	else tkerr(crtTk, "Error at Syntactical Analysis");
}


int main(int argc, char **argv){

	if (argc < 2) {
		printf("Second argument must be a C file\n");
		exit(1);
	}

	lexicalAnalysis(argv[1]);	//Lexical Analyzer

	syntacticAnalysis();		//Syntactic Analyzer + Symbols

    return 0;
}