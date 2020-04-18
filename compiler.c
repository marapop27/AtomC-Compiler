#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>


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

char* createString(char* start, char* end) {
	int length = end - start + 1;
	char *myString=(char*)malloc(length * sizeof(char));
	snprintf(myString, length, "%s", start);
	return myString;
}

int getNextToken(char *file)
{
    int state=0, nCh;
    char ch ;
    const char *pStartCh, *pCrtCh = file;
    Token *tk;
    
    while(1){ // infinite loop
        ch = *pCrtCh;

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
				else if(isdigit(ch)){
					pStartCh = pCrtCh;
					state = 7;
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
                else if(ch=='='){
					pCrtCh++;
					ch=*pCrtCh;
					if(ch=='='){
						pCrtCh++;
						addTk(EQUAL);
					}
					else{
						addTk(ASSIGN);              
					}
				}               
                else if(ch=='<'){
					pCrtCh++;
					ch=*pCrtCh;
					if(ch=='='){
						pCrtCh++;
						addTk(LESSEQ);
					}
					else{
						addTk(LESS);            
					}
				}
				else if(ch=='>'){
					pCrtCh++;
					ch=*pCrtCh;
					if(ch=='='){
						pCrtCh++;
						addTk(GREATEREQ);
					}
					else{
						addTk(GREATER);             
					}
				}
                else if(ch==0){ // the end of the input string
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
                    state=2;
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
                    tk->text=createString(pStartCh,pCrtCh);
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
					state=4;
				}
				break;
            case 6:
				if(ch !='\n' && ch !='\r' && ch !='\0'){
					pCrtCh++;
                    state = 6;
				}
				else if(ch=='\n') {
					pCrtCh++;
					state=0;
					line++;
				}
				else
				{
					pCrtCh++;
					state=0;
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
					state=11;
				}
				else 
					tkerr(tk, "Float number format expected");
				break;
			case 9:
				if(ch=='+' || ch=='-'){
					pCrtCh++;
					state=14;
				}
				else if(isdigit(ch)){
					pCrtCh++;
					state=15;
				}
				else {
					pCrtCh++;
					state=14;
				}
				break;
			case 10:
				tk = addTk(CT_INT);
				char* integer = createString(pStartCh, pCrtCh);
				tk->i = atoi(integer);
				state = 0;
				break;
			case 11:
				if(isdigit(ch)){
					pCrtCh++;
					state = 11;
				}
				else if(ch=='e' || ch=='E'){
					pCrtCh++;
					state=9;
				}
				else
					state = 12;
				break;
			// case 13:
			// 	if(ch=='e' || ch=='E'){
			// 		pCrtCh++;
			// 		state=9;
			// 	}
			// 	else 
			// 		state = 13;
			// 	break;
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

        }
    }
}

char *putFileContentToString(char *filename ){
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

void printTokens(Token * printer) {
	while(printer!=NULL){
		if(printer->code == ID)
			printf("%d:%s ", printer->code, printer->text);
		else if(printer->code == CT_INT)
			printf("%d:%d ", printer->code, printer->i);
		else if(printer->code == CT_REAL)
			printf("%d:%f ", printer->code, printer->r);
		else
			printf("Code:%d ", printer->code);
		printf("\n");
		printer=printer->next;
	}
}


int main(int argc, char **argv){

	if (argc < 2) {
		printf("Second argument must be a C file\n");
		exit(1);
	}
	char *buffer = putFileContentToString(argv[1]);
	printf("nevasta\n\n");
	printf("%s\n", buffer);

	getNextToken(buffer);

	Token *printer = tokens;
	printTokens(printer);
	

	// FILE *fp;

	// fp = fopen(argv[1], "r");

	// if (!fp) 
	// 	err("Error opening file (%s)", argv[1]);


	// fclose(fp);

    return 0;
}




