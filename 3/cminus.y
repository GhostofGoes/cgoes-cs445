%{
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdarg.h>
#include "types.h"
#include "cminus.h"

#define YYERROR_VERBOSE

// Flex's Lexer (heh)
extern int yylex();

// Flex's line numbering
extern int yylineno; 

// Input file stream
extern FILE * yyin;

// Global abstract syntax tree pointer
TreeNode *syntaxTree = NULL;

// Global Annotated Syntax Tree pointer 
TreeNode *annotatedTree = NULL;

// Globally keep track of warnings and errors
int warnings = 0;
int errors = 0;

// Defining yyerror
static void yyerror(const char *);

%}

%code requires {
	#define MAXCHILDREN 3
	#include "types.h"
	#include "cminus.h"
}

// TODO: If i'm using a single type, can i just use the API?
// %define api.value.type { Treenode *tree } 
// Above would allow use of %token <TreeNode> instead of "tree". Unsure of the value, just a random thing.
%union 
{
	TreeNode *tree;
}

%token <tree> ID
%token <tree> NUMCONST
%token <tree> STRINGCONST
%token <tree> CHARCONST 
%token <tree> ERROR
%token <tree> BOOLCONST
%token <tree> ADDASS SUBASS MULASS DIVASS INC DEC LESSEQ GRTEQ EQ NOTEQ STATIC INT BOOL CHAR IF ELSE WHILE FOREACH IN RETURN BREAK
%token <tree> SEMICOLON LPAREN RPAREN LBRACKET RBRACKET OR AND NOT ASSIGN PLUS MINUS MULTIPLY DIVIDE MODULUS QUESTION LTHAN GTHAN LBRACE RBRACE COMMA COLON 

%type <tree> program
%type <tree> declaration-list
%type <tree> declaration
%type <tree> var-declaration
%type <tree> scoped-var-declaration
%type <tree> var-decl-list
%type <tree> var-decl-initialize
%type <tree> var-decl-id
%type <tree> scoped-type-specifier
%type <tree> type-specifier
%type <tree> fun-declaration
%type <tree> params
%type <tree> param-list
%type <tree> param-type-list
%type <tree> param-id-list
%type <tree> param-id
%type <tree> statement
%type <tree> compound-stmt
%type <tree> local-declarations
%type <tree> statement-list
%type <tree> expression-stmt
%type <tree> return-stmt
%type <tree> break-stmt
%type <tree> expression
%type <tree> simple-expression
%type <tree> and-expression
%type <tree> unary-rel-expression
%type <tree> rel-expression
%type <tree> relop
%type <tree> sum-expression
%type <tree> sumop
%type <tree> term
%type <tree> mulop
%type <tree> unary-expression
%type <tree> unaryop
%type <tree> factor
%type <tree> mutable
%type <tree> immutable
%type <tree> call
%type <tree> args
%type <tree> arg-list
%type <tree> constant
%type <tree> matched unmatched  
%type <tree> matched-while-stmt  matched-foreach-stmt matched-selection-stmt
%type <tree> unmatched-while-stmt unmatched-selection-stmt unmatched-foreach-stmt
/* need to put a error incrementer/warning incrementer for ERROR */

%%
	
program:	
	declaration-list	
		{ $$ = $1; syntaxTree = $$; }
	;

declaration-list: 
	declaration-list declaration
		{ $$ = linkSiblings(2, $1, $2); }
	| declaration
		{ $$ = $1; }
	;

declaration: 
	var-declaration 
		{ $$ = $1; }
	| fun-declaration
		{ $$ = $1; }
	;

var-declaration:
	type-specifier var-decl-list SEMICOLON
        { $$ = makeNode( DeclK, VarK, $1->nodetype, $1->lineno, $2->str, $2->token ); }
	;

/* Note: need to keep track of scope! */
scoped-var-declaration:
	scoped-type-specifier var-decl-list SEMICOLON
        { 
            $$ = makeNode( DeclK, VarK, $1->nodetype, $1->lineno, $2->str, $2->token );
            //$$->isScoped = true; 
        }
	;

var-decl-list:
	var-decl-list COMMA var-decl-initialize
	| var-decl-initialize 
		{ $$ = $1; }
	;				

var-decl-initialize:
	var-decl-id
		{ $$ = $1; }
	| var-decl-id COLON simple-expression
		{ $$ = $1; }
	;

var-decl-id:
	ID
		{ $$ = $1; }
	| ID LBRACKET NUMCONST RBRACKET
		{ 
            $$ = $1;
            $$->isArray = true;
        }
	;

scoped-type-specifier:
	STATIC type-specifier
		{ 
            $$ = makeNode( DeclK, VarK, $2->nodetype, $2->lineno, $2->str, $2->token); 
            $$->isStatic = true; 
            //$$->isScoped = true;
           }
	| type-specifier
		{ $$ = $1; }
	;

type-specifier:
	INT 
		{ $$ = $1; $$->nodetype = Integer; }
	| BOOL 
		{ $$ = $1; $$->nodetype = Boolean; }
	| CHAR 
		{ $$ = $1; $$->nodetype = Character; }
	;

fun-declaration:
	type-specifier ID LPAREN params RPAREN statement
		{ 
			$$ = makeNode( DeclK, FunK, $1->nodetype, $2->lineno, $2->str, $2->token );
			addChildren( $$, 2, $4, $6 );
		}
	| ID LPAREN params RPAREN statement
		{ 
			$$ = makeNode( DeclK, FunK, $1->nodetype, $1->lineno, $1->str, $1->token );
			addChildren( $$, 2, $3, $5);
		}
	;

params:
	param-list 
		{ $$ = $1; }
	| /* empty */
		{ $$ = NULL; }
	;

param-list:
	param-list SEMICOLON param-type-list 
		{  
			$$ = linkSiblings(2, $1, $3);
		}
	| param-type-list
		{ $$ = $1; }
	;
	
param-type-list:
	type-specifier param-id-list 
		{ 
			$$ = makeNode( DeclK, ParamK, $1->nodetype, $1->lineno, $2->str, $2->token );
            addChildren( $$, 1, $2 );
		}
	;
	
param-id-list:
	param-id-list COMMA param-id 
		{ $$ = linkSiblings(2, $1, $3); }
	| param-id 
		{ $$ = $1; }
	;
	
param-id:
	ID 		
		{ $$ = $1; }
	| ID LBRACKET RBRACKET
		{ 
            $$ = $1; 
            $$->isArray = true; 
        }
	;
	
statement:
	matched
		{ $$ = $1; }
	| unmatched
		{ $$ = $1; }
	;

matched:
	expression-stmt 
	| compound-stmt 
	| matched-selection-stmt 
	| matched-while-stmt 
	| matched-foreach-stmt
	| return-stmt 
	| break-stmt 
	;

unmatched:
	unmatched-selection-stmt
	| unmatched-while-stmt
	| unmatched-foreach-stmt
	;
	
matched-selection-stmt:
	IF LPAREN simple-expression RPAREN matched ELSE matched
        { 
            $$ = makeNode( StmtK, IfK, $1->nodetype, $1->lineno, NULL, $1->token );
            addChildren( $$, 3, $3, $5, $7 );
        }
	;

unmatched-selection-stmt:
	IF LPAREN simple-expression RPAREN statement
        { 
            $$ = makeNode( StmtK, IfK, $1->nodetype, $1->lineno, NULL, $1->token );
            addChildren( $$, 2, $3, $5 );
        }
	| IF LPAREN simple-expression RPAREN matched ELSE unmatched 
        { 
            $$ = makeNode( StmtK, IfK, $1->nodetype, $1->lineno, NULL, $1->token );
            addChildren( $$, 3, $3, $5, $7 );
        }
	;
	
matched-while-stmt:
	WHILE LPAREN simple-expression RPAREN matched
        { 
            $$ = makeNode( StmtK, WhileK, $1->nodetype, $1->lineno, NULL, $1->token );
            addChildren( $$, 2, $3, $5 );
        }
	;

unmatched-while-stmt:
	WHILE LPAREN simple-expression RPAREN unmatched
        { 
            $$ = makeNode( StmtK, WhileK, $1->nodetype, $1->lineno, NULL, $1->token );
            addChildren( $$, 2, $3, $5 );
        }
	;
	
matched-foreach-stmt:
	FOREACH LPAREN mutable IN simple-expression RPAREN matched
        { 
            $$ = makeNode( StmtK, ForeachK, $1->nodetype, $1->lineno, NULL, $1->token );
            addChildren( $$, 3, $3, $5, $7 );
        }
	;
	
unmatched-foreach-stmt:
	FOREACH LPAREN mutable IN simple-expression RPAREN unmatched
        { 
            $$ = makeNode( StmtK, ForeachK, $1->nodetype, $1->lineno, NULL, $1->token );
            addChildren( $$, 3, $3, $5, $7 );
        }
	;
	
compound-stmt:
	LBRACE local-declarations statement-list RBRACE
        { $$ = linkSiblings(2, $2, $3); }
	;

local-declarations:
	local-declarations scoped-var-declaration 
		{ $$ = linkSiblings(2, $1, $2); }
	| /* empty */ 
		{ $$ = NULL; }
	;
	
statement-list:
	statement-list statement
        { $$ = linkSiblings(2, $1, $2); }
	| /* empty */
		{ $$ = NULL; }
	;
	
expression-stmt:
	expression SEMICOLON 
        { $$ = $1; /* does expression occur at semicolon for line counting? */ }
	| SEMICOLON
		{ // null? 
            $$ = $1;
        }
	;
	
return-stmt:
	RETURN SEMICOLON 
        { $$ = $1; }
	| RETURN expression SEMICOLON
        {
            $$ = makeNode( StmtK, ReturnK, $2->nodetype, $1->lineno, $1->str, $1->token );
            addChildren( $$, 1, $2 );
        }
	;
	
break-stmt:
	BREAK SEMICOLON
        { $$ = $1; } 
	;
	
expression:
	mutable ASSIGN expression
        { 
            $$ = $2;
            addChildren( $$, 2, $1, $3 );
        }
	| mutable ADDASS expression
        { 
            $$ = $2;
            addChildren( $$, 2, $1, $3 );
        }
	| mutable SUBASS expression
        { 
            $$ = $2;
            addChildren( $$, 2, $1, $3 );
        }
	| mutable MULASS expression
        { 
            $$ = $2;
            addChildren( $$, 2, $1, $3 );
        }
	| mutable DIVASS expression
        { 
            $$ = $2;
            addChildren( $$, 2, $1, $3 );
        }
	| mutable INC
        { 
            $$ = $2;
            addChildren( $$, 1, $1 );
        }
	| mutable DEC
        { 
            $$ = $2;
            addChildren( $$, 1, $1 );
        }
	| simple-expression
		{ $$ = $1; }
	;
	
simple-expression:
	simple-expression OR and-expression 
        { 
            $$ = $2;
            addChildren( $$, 2, $1, $3 );
        }
	| and-expression 
		{ $$ = $1; }
	;
	
and-expression:
	and-expression AND unary-rel-expression
        { 
            $$ = $2;
            addChildren( $$, 2, $1, $3 );
        }
	| unary-rel-expression 
		{ $$ = $1; }
	;
	
unary-rel-expression:
	NOT unary-rel-expression
        { 
            $$ = $1;
            addChildren( $$, 1, $2 );
        }
	| rel-expression 
		{ $$ = $1; }
	;
	
rel-expression:
	sum-expression relop sum-expression
        { 
            $$ = $2;
            addChildren( $$, 2, $1, $3 );
        } 
	| sum-expression 
		{ $$ = $1; }
	;
	
relop:
	LESSEQ
		{ $$ = $1; }
	| LTHAN
		{ $$ = $1; }
	| GTHAN
		{ $$ = $1; }
	| GRTEQ 
		{ $$ = $1; }
	| EQ 
		{ $$ = $1; }
	| NOTEQ 
		{ $$ = $1; }
	;
	
sum-expression:
	sum-expression sumop term
        { 
            $$ = $2;
            addChildren( $$, 2, $1, $3 );
        } 
	| term 
		{ $$ = $1; }
	;
	
sumop:
	PLUS
		{ $$ = $1; }
	| MINUS
		{ $$ = $1; }
	;
	
term:
	term mulop unary-expression
        { 
            $$ = $2;
            addChildren( $$, 2, $1, $3 );
        }
	| unary-expression 
		{ $$ = $1; }
	;
	
mulop:
	MULTIPLY 
		{ $$ = $1; }
	| DIVIDE 
		{ $$ = $1; }
	| MODULUS 
		{ $$ = $1; }
	;
	
unary-expression:
	unaryop unary-expression
        { 
            $$ = $1;
            addChildren( $$, 1, $2 );
        }
	| factor 
		{ $$ = $1; }
	;
	
unaryop:
	MINUS
		{ $$ = $1; }
	| MULTIPLY
		{ $$ = $1; }
	| QUESTION
		{ $$ = $1; }
	;
	
factor:
	immutable 
		{ $$ = $1; }
	| mutable 
		{ $$ = $1; }
	;
	
mutable:
	ID
		{ $$ = $1; }
	| ID LBRACKET expression RBRACKET
        {
            $$ = $1;
            $$->isArray = true; 
            addChildren( $$, 1, $3 );
        }
	;
	
immutable:
	LPAREN expression RPAREN
        { $$ = $2; }
	| call
		{ $$ = $1; }	
	| constant 
		{ $$ = $1; }	
	;
	
call:
	ID LPAREN args RPAREN
		{ 
			$$ = makeNode( ExpK, CallK, $1->nodetype, $1->lineno, $1->str, $1->token );
			addChildren($$, 1, $3);
		}
	;
	
args:
	arg-list
		{ $$ = $1; }
	| /* empty */
		{ $$ = NULL; }
	;
	
arg-list:
	arg-list COMMA expression
		{ 
		    $$ = linkSiblings(2, $1, $3); 
		}
	| expression
		{ $$ = $1; }	
	;
	
constant:
	NUMCONST 
		{ $$ = $1; }
	| CHARCONST 
		{ $$ = $1; }
	| STRINGCONST
		{ $$ = $1; }
	| BOOLCONST
		{ $$ = $1; }
	;
	

%%

int main( int argc, char* argv[] ) {
	
	// FILE * output = stdout;
	int option;
	opterr = 0;
	
	// Flags
	bool test_mode = false;
	bool abstract_tree = false;
	bool annotated_tree = false;
	bool code_generation = false;
	
	// Command line options. Only handles "-d" debug option currently.
	while( (option = getopt(argc, argv, "dpPt")) != EOF ) {
		switch(option) {
			case 'd': 
				yydebug = 1;
				break;
			case 'p':
				abstract_tree = true;
				break;
			case 'P':
				annotated_tree = true;
				break;
			case 't':
				test_mode = true;
				break;
			default:
				break;
		}
	}
	
	// Slightly hacky way to get input filename, without using a option
	if( argc > 1 && optind < argc ) {
		yyin = fopen( argv[optind], "r" );
	}

	// Main parsing loop. Goes until end of input
	do {
		yyparse();
	} while (!feof(yyin));
	
	
	// Prints out the entire syntax tree recursivly, from the global root node
	if(abstract_tree) {
		printAbstractTree(syntaxTree, 0);
	}
	
	if(annotated_tree) {
		semanticAnalysis(annotatedTree);
		printAnnotatedTree(annotatedTree);
	}
	
	if(code_generation) {
		// TODO import I/O library
		generateCode();
	}
	
	// How many bad things happened. TODO: when do we want to do this, or not?
	printf( "Number of warnings: %d\n", warnings );
	printf( "Number of errors: %d\n", errors );	
	
	fclose(yyin);
	return 0;
}

// Defines yyerror for bison
static void yyerror(const char *msg)
{
	fflush(stdout);
    printf("ERROR(%d): %s\n", yylineno, msg);
}

/*
// Silly "typing saving" helper function
void easy( int linenum, char * svalue ) { 
	printf("Line %d Token: %s\n", linenum, svalue );
}


// Print spaces at end of strings if neccessary. 
// Assumes we're printing to STDOUT. If we need a file, just redirect at OS level.
void printAbstractTree(TreeNode * tree, int indent_count) {
	
	// Keeps track of siblings
	int sibling_count = 0;

	// Prints all nodes of the tree
	while( tree != NULL ) {
		
		for(int i = 0; i < indent_count; i++) {
			printf("|   ");
		}
		if(sibling_count > 0) {
			// two spaces after child num
			printf( "|Sibling: %d  ", sibling_count);
		}
		
		if( tree->typespecifier ) {
			if( tree->value == ID ) {
				
			}
		}
		
		// Print the line number + newline
		printf( "[line: %d]\n", tree->lineno );
		
		// Check if there are children
		if( tree->numChildren > 0 ) {
			// "tab" space for children
			printf( "   ");
			
			for ( int i = 0; i < tree->numChildren; i++ ) {
				// Two spaces after child num
				printf( "Child: %d  ", i);
				printAbstractTree(tree->child[i], indent_count + 1);
			}			
		}
		
		tree = tree->sibling;
		sibling_count++;
		
	} // end while
	
}

// TODO: placeholder Prints the Annotated Syntax Tree 
void printAnnotatedTree( TreeNode * tree ) {
	;
}

// TODO: placeholder Performs semantic analysis, generating the Annotated Syntax Tree
void semanticAnalysis( TreeNode * tree ) {
	;
}


// TODO: placeholder Generates executable code 
void generateCode() {
	;
}


TreeNode * makeNode( int value, int numChildren, ... ) {
	
	// Allocate a new node
	TreeNode * tempNode = allocNode();
	
	// Attach children
	tempNode->numChildren = numChildren;
	if(numChildren > 0) { // Congratulations, its a TreeNode pointer!
		va_list children; 
		va_start (children, numChildren); // Load children from arguments
		
		for(int i = 0; i < numChildren; i++) {
			tempNode->child[i] = va_arg(children, TreeNode*);
		}
		
		va_end(children); // End var list
	}
	
	return tempNode;
}


// Creates a new node for the syntax tree
void addChildren( TreeNode * parent, int numChildren,...) {
	
	// Attach children
	parent->numChildren = numChildren;
	if(numChildren > 0) { // Congratulations, its a TreeNode pointer!
		va_list children; 
		va_start (children, numChildren); // Load children from arguments
		
		for(int i = 0; i < numChildren; i++) {
			parent->child[i] = va_arg(children, TreeNode*);
		}
		
		va_end(children); // End var list
	}
}

TreeNode * linkSiblings( int numSiblings, ... ) {
	
	va_list siblings;
	va_start (siblings, numSiblings);
	
	TreeNode * prev = va_arg(siblings, TreeNode*);
	TreeNode * temp = prev;
	TreeNode * next = NULL;
	if(temp = NULL; )
	while (prev->sibling != NULL ) { prev = prev->sibling; }
	
	for(int i = 0; i < numSiblings; i++) {
		next = va_arg(siblings, TreeNode*);
		if(prev != NULL) {
			prev->sibling = next;
		}
		prev = next;
	}
	prev->sibling = NULL;
	va_end(siblings);
	
	return temp;
	
}

*/