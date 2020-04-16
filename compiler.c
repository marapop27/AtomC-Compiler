#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>



#define SAFEALLOC(var,Type) if((var=(Type*)malloc(sizeof(Type)))==NULL)err("not enough memory");


enum{ID, END, BREAK, CHAR, DOUBLE, ELSE, FOR, IF, INT, 
	RETURN, STRUCT, VOID, WHILE, CT_INT , CT_REAL, CT_CHAR,
    CT_STRING, COMMA, SEMICOLON, LPAR, RPAR, LBRACKET, RBRACKET,
    LACC, RACC, ADD, SUB, MUL, DIV, DOT, AND, OR, NOT, ASSIGN,
    EQUAL, NOTEQ, LESS, LESSEQ, GREATER, GREATEREQ}; // tokens codes

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

int getNextToken(char *input)
{
    int state=0,nCh;
    char ch ;
    const char *pStartCh, *pCrtCh;
    Token *tk;
    
    while(1){ // infinite loop
        ch = *pCrtCh;

        switch(state){
            case 0: // transitions test for state 0
                if(isalpha(ch)||ch=='_'){
                    pStartCh=pCrtCh; // memorizes the beginning of the ID
                    pCrtCh++; // consume the character
                    state=1; // set the new state
                }
                else if(ch=='='){
                    pCrtCh++;
                    state=3;
                }
                else if(ch==' '||ch=='\r'||ch=='\t'){
                    pCrtCh++; // consume the character and remains in state 0
                }
                else if(ch=='\n'){ // handled separately in order to update the current line
                    line++;
                    pCrtCh++;
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
				else if(ch=='.'){
					pCrtCh++;
					addTk(DOT);
				}
                else if(ch==0){ // the end of the input string
                    addTk(END);
                    return END;
                }
                else 
                    tkerr(addTk(END),"invalid character");
                break;
            case 1:
                if(isalnum(ch)||ch=='_')
                    pCrtCh++;
                else 
                    state=2;
                break;
            case 2:
                nCh=pCrtCh-pStartCh; // the id length
                // keywords tests
                if(nCh==5&&!memcmp(pStartCh,"break",5))
                    tk=addTk(BREAK);
                else if(nCh==4&&!memcmp(pStartCh,"char",4))
                    tk=addTk(CHAR);
                else if(nCh==3&&!memcmp(pStartCh,"int",3))
                    tk=addTk(INT);
                else if(nCh==6&&!memcmp(pStartCh,"double",6))
                    tk=addTk(DOUBLE);
                else if(nCh==4&&!memcmp(pStartCh,"void",4))
                    tk=addTk(VOID);
                else if(nCh==2&&!memcmp(pStartCh,"if",2))
                    tk=addTk(IF);
                else if(nCh==4&&!memcmp(pStartCh,"else",4))
                    tk=addTk(ELSE);
                else if(nCh==3&&!memcmp(pStartCh,"for",3))
                    tk=addTk(FOR);
                else if(nCh==5&&!memcmp(pStartCh,"while",5))
                    tk=addTk(WHILE);
                else if(nCh==6&&!memcmp(pStartCh,"return",6))
                    tk=addTk(RETURN);
                else if(nCh==6&&!memcmp(pStartCh,"struct",6))
                    tk=addTk(STRUCT);
                // … all keywords …
                else{ // if no keyword, then it is an ID
                    tk=addTk(ID);
                    // tk->text=createString(pStartCh,pCrtCh);
                }
                return tk->code;
            case 3:
                if(ch=='='){
                    pCrtCh++;
                    state=4;
                }
                else 
                    state=5;
                break;
            case 4:
                addTk(EQUAL);
                return EQUAL;
            case 5:
                addTk(ASSIGN);
                return ASSIGN;
        }
    }
}



int main(){
    printf("%d \n", BREAK);
    return 0;
}




