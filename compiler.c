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



Token *crtTk = NULL;
Token *consumedTk = NULL;

int consume(int code){
	if(crtTk->code == code){
		consumedTk = crtTk;
		crtTk = crtTk->next;
		return 1;
	}
	return 0;
	}

int unit(){
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

	if (!consume(STRUCT)) return 0;
	if (!consume(ID)) tkerr(crtTk, "Expected ID after struct declaration.");
	if (!consume(LACC)) {
		crtTk = startTk;
		return 0;
	}
	while (1) {
		if (declVar()) {}
		else break;
	}
	if (!consume(RACC)) tkerr(crtTk, "Expected '}' after struct declaration.");
	if (!consume(SEMICOLON)) tkerr(crtTk, "Expected ';' after struct declaration.");
	return 1;
}

int declVar(){
	Token *startTk = crtTk;

	if (typeBase()) {
		if (consume(ID)) {
			arrayDecl();
			while (1) {
				if (consume(COMMA)) {
					if (consume(ID)) {
						arrayDecl();
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
int typeBase(){
	Token *startTk = crtTk;

	if (consume(INT)) return 1;
	if (consume(DOUBLE)) return 1;
	if (consume(CHAR)) return 1;
	if (consume(STRUCT)) {
		if (consume(ID)) {
			return 1;
		}
		else tkerr(crtTk, "Expected ID after struct declaration.");
	}
	crtTk = startTk;

	return 0;
}

int arrayDecl(){
	Token *startTk = crtTk;

	if (consume(LBRACKET)) {
		expr();
		if (consume(RBRACKET)) {
			return 1;
		}
		else tkerr(crtTk, "Expected ']' after array declaration.");
	}
	crtTk = startTk;

	return 0;
}

int typeName(){
	Token *startTk = crtTk;
	if (typeBase()) {
		arrayDecl();
		return 1;
	}
	crtTk = startTk;
	return 0;
}

int declFuncRepeatingCode() {
	Token *startTk = crtTk;

	if (consume(ID)) {
		if (consume(LPAR)) {
			if (funcArg()) {
				while (1) {
					if (consume(COMMA)) {
						if (funcArg()) {
							continue;
						}
						else tkerr(crtTk, "There are no argument in function declaration");
					}
					else break;
				}
			}
			if (consume(RPAR)) {
				if (stmCompound()) {
					return 1;
				}
				else tkerr(crtTk, "Expected compund statement for function declaration.");
			}
			else tkerr(crtTk, "Expected ')' for function declaration.");
		}
	}
	crtTk = startTk;

	return 0;
}

int declFunc(){
	Token *startTk = crtTk;

	if (typeBase()) {
		if(consume(MUL);
		if (declFuncRepeatingCode()) {
			return 1;
		}
		else tkerr(crtTk, "Expected ID after function type declaration");
	}
	if (consume(VOID)) {
		if (!declFuncRepeatingCode())
			tkerr(crtTk, "Expected ID after void");
		return 1;
	}
	crtTk = startTk;

	return 0;
}

int funcArg(){
	Token *startTk = crtTk;

	if (typeBase()) {
		if (consume(ID)) {
			arrayDecl();
			return 1;
		}
		else tkerr(crtTk, "Expected ID for function argument.");
	}
	crtTk = startTk;

	return 0;
}

int stm(){
	Token *startTk = crtTk;

	if (stmCompound()) return 1;

	if (consume(IF)) {
		if (consume(LPAR)) {
			if (expr()) {
				if (consume(RPAR)) {
					if (stm()) {
						if (consume(ELSE)) {
							if (stm()) return 1;
							else tkerr(crtTk, "Statement expected after else declaration");
						}
						return 1;
					}
					else tkerr(crtTk, "Statement expected");
				}
				else tkerr(crtTk, "Expected ')' ");
			}
			else tkerr(crtTk, "Expected expresion inside if condition");
		}
		else tkerr(crtTk, "Expected '(' ");
	}
	crtTk = startTk;

	if (consume(WHILE)) {
		if (consume(LPAR)) {
			if (expr()) {
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
	crtTk = startTk;

	if (consume(FOR)) {
		if (consume(LPAR)) {
			expr();
			if (consume(SEMICOLON)) {
				expr();
				if (consume(SEMICOLON)) {
					expr();
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
	crtTk = startTk;

	if (consume(BREAK)) {
		if (consume(SEMICOLON))
			return 1;
		else tkerr(crtTk, "Expected ';' after break statement");
	}
	crtTk = startTk;

	if (consume(RETURN)) {
		expr();
		if (consume(SEMICOLON)) {
			return 1;
		}
		else tkerr(crtTk, "Expected ';' after return statement");
	}
	crtTk = startTk;

	if (consume(SEMICOLON)) {
		return 1;
	}
	else { 
		if (expr()) {
			if (!consume(SEMICOLON)) tkerr(crtTk, "Expected ';' after expression");
			return 1;
		}
	}
	crtTk = startTk;
	return 0;
}

int stmCompound(){
	Token *startTk = crtTk;

	if (consume(LACC)) {
		while (1) {
			if (declVar())
				continue;
			else if (stm())
			    continue;
			else break;
		}
		if (consume(RACC))
			return 1;
		else tkerr(crtTk, "Expected '}' ");
	}
	crtTk = startTk;

	return 0;
}

int expr(){
	Token *startTk = crtTk;

	if (exprAssign())
		return 1;	
	crtTk = startTk;

	return 0;
}

int exprAssign(){
	Token *startTk = crtTk;

	if (exprUnary()) {
		if (consume(ASSIGN)) {
			if (exprAssign()){
				return 1;
			}
			else tkerr(crtTk, "Error at expression assign");
		}
	}
	crtTk = startTk;

	if (exprOr()) return 1;
	crtTk = startTk;

	return 0;
}

int exprOrLite() {
	Token *startTk = crtTk;

	if (consume(OR)) {
		if (exprAnd()) {
			if (exprOrLite()) {
				return 1;
			}
			else tkerr(crtTk, "Error at OR expression");
		}
		else tkerr(crtTk, "Error at OR expression");
	}

	crtTk = startTk;

	return 1;
}

int exprOr(){
	Token *startTk = crtTk;

	if (exprAnd()) {
		if (exprOrLite()) {
			return 1;
		}
		else tkerr(crtTk, "Error at OR expression");
	}

	return 0;
}

int exprAndLite() {
	Token *startTk = crtTk;

	if (consume(AND)) {
		if (exprEq()) {
			if (exprAndLite()) {
				return 1;
			}
			else tkerr(crtTk, "Error at AND expression");
		}
		else tkerr(crtTk, "Error at AND expression");
	}
	crtTk = startTk;

	return 1;
}

int exprAnd(){
	Token *startTk = crtTk;

	if (exprEq()) {
		if (exprAndLite()) {
			return 1;
		}
		else tkerr(crtTk, "Error at AND expression");
	}

	return 0;
}

int exprEqLite() {
	Token *startTk = crtTk;

	if (consume(EQUAL) || consume(NOTEQ)) {
		if (exprRel()) {
			if (exprEqLite()) 
				return 1;
			else tkerr(crtTk, "Error at equality expression");
		}
		else tkerr(crtTk, "Error at equality expression");
	}
	crtTk = startTk;

	return 1;
}

int exprEq(){
	Token *startTk = crtTk;

	if (exprRel()) {
		if (exprEqLite()) {
			return 1;
		}
		else tkerr(crtTk, "Error at equality expression");
	}

	return 0;
}

int exprRelLite() {
	Token *startTk = crtTk;

	if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)) {
		if (exprAdd()) {
			if (exprRelLite()) {
				return 1;
			}
			else tkerr(crtTk, "Error at rel expression");
		}
		else tkerr(crtTk, "Error at rel expression");
	}
	crtTk = startTk;

	return 1;
}

int exprRel(){
	Token *startTk = crtTk;

	if (exprAdd()) {
		if (exprRelLite()) {
			return 1;
		}
		else tkerr(crtTk, "Error at rel expression");
	} 

	return 0;
}

int exprAddLite() {
	Token *startTk = crtTk;

	if (consume(ADD) || consume(SUB)) {
		if (exprMul()) {
			if (exprAddLite()) {
				return 1;
			}
			else tkerr(crtTk, "Error at add expression");
		}
		else tkerr(crtTk, "Error at add expression");
	}
	crtTk = startTk;

	return 1;
}

int exprAdd(){
	Token *startTk = crtTk;

	if (exprMul()) {
		if (exprAddLite())
			return 1;
		else tkerr(crtTk, "Error at add expression");
	}

	return 0;
}

int exprMulLite() {
	Token *startTk = crtTk;

	if (consume(MUL) || consume(DIV)) {
		if (exprCast()) {
			if (exprMulLite()) {
				return 1;
			}
			else tkerr(crtTk, "Error at multiply expression");
		}
		else tkerr(crtTk, "Error at multiply expression");
	}
	crtTk = startTk;

	return 1;
}

int exprMul(){
	Token *startTk = crtTk;

	if (exprCast()) {
		if (exprMulLite()) {
			return 1;
		}
		else tkerr(crtTk, "Error at multiply expression");
	}

	return 0;
}

int exprCast(){
	Token *startTk = crtTk;

	if (consume(LPAR)) {
		if (typeName()) {
			if (consume(RPAR)) {
				if (exprCast()) {
					return 1;
				}
				else tkerr(crtTk, "Error at cast");
			}
			else tkerr(crtTk, "Expected ')' ");
		}
		else tkerr(crtTk, "Expected cast type name");
	}
	if (exprUnary()) {
		return 1;
	}
	crtTk = startTk;

	return 0;
}

int exprUnary(){
	Token *startTk = crtTk;

	if (consume(SUB) || consume(NOT)) {
		if (exprUnary())
			return 1;
		else tkerr(crtTk, "Expected unary expression");
	}
	crtTk = startTk;

	if (exprPostfix()) {
		return 1;
	}
	crtTk = startTk;

	return 0;
}

int exprPostfixLite() {
	Token *startTk = crtTk;

	if (consume(LBRACKET)) {
		if (expr()) {
			if (consume(RBRACKET)) {
				if (exprPostfixLite()) {
					return 1;
				}
				else tkerr(crtTk, "Error at postfix declaration");
			}
			else tkerr(crtTk, "Expected ']' ");
		}
		else tkerr(crtTk, "Expected expression after '[' in postfix expression");
	}
	if (consume(DOT)) {
		if (consume(ID)) {
			if (exprPostfixLite()) {
				return 1;
			}
			else tkerr(crtTk, "Error at postfix declaration");
		}
		else tkerr(crtTk, "ID expected in postfix expression");
	}
	crtTk = startTk;

	return 1;
}

int exprPostfix(){
	Token *startTk = crtTk;

	if (exprPrimary()) {
		if (exprPostfixLite()) {
			return 1;
		}
		else tkerr(crtTk, "Error at postfix declaration");
	}
	crtTk = startTk;

	return 0;
}

int exprPrimary(){
	Token *startTk = crtTk;

	if (consume(ID)) {
		if (consume(LPAR)) {
			if (expr()) {
				while (1) {
					if (consume(COMMA)) {
						if (expr())
							continue;
						else tkerr(crtTk, "Expected expression after ','");
					}
					else break;
				}
			}
			if (consume(RPAR)) {
				return 1;
			}
			else tkerr(crtTk, "Missing RPAR in primary EXPR after ID.");
		}
		return 1;
	}
	if (consume(CT_INT)) 
		return 1;

	if (consume(CT_REAL)) 
		return 1;

	if (consume(CT_CHAR)) 
		return 1;

	if (consume(CT_STRING)) 
		return 1;

	if (consume(LPAR)) {
		if (expr()) {
			if (consume(RPAR)) {
				return 1;
			}
			else tkerr(crtTk, "Expected ')' in primary expression");
		}
		else tkerr(crtTk, "Expected expression after '(' ");
	}
	crtTk = startTk;

	return 0;
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

	syntacticAnalysis();		//Syntactic Analyzer

    return 0;
}