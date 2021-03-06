
// cminus.cpp
// Author:      Christopher Goes 
// Contact:     ghostofgoes@gmail.com
// Github:      https://github.com/GhostofGoes/cgoes-cs445.git

// C++ Libraries
#include <iostream>
#include <string>
#include <cstdlib>
#include <stack>

// C libraries
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>

// Local headers
#include "cminus.h"
#include "cminus.tab.h"
#include "types.h"
#include "toker.h"
#include "symbolTable.h"
#include "printing.h"
#include "trees.h"
#include "semantic_errors.h"
#include "codegenClassTM.h"

// Tree pointers
TreeNode * syntaxTree = NULL;
TreeNode * annotatedTree = NULL;

// Globally keep track of warnings and errors
int warnings = 0;
int errors = 0;

// Debugging/Testing flags
bool testing = false; // TODO: use a #define so gcc can optimze out

// Used for semantic analysis
bool return_found = false;
TreeNode * func = NULL;

// Enter loop, push true. Leave, pop. If there is a break, checks the top of stack
std::stack<bool> loopDepth; 

// memorySizing globals
int global_offset = 0;
int param_count = 0;


// NOTE: this thing leaks memory a lot. destructors would help, but...it's a compiler. one-time run. 
// TODO: '=' requires that if one operand is an array so must the other...
// In: everything06.c-, everything06linear.c-
// TODO: hamlet.c- invalid input character, hard.c- charconst, sample.c- stringconst
// syntax-err.c-


// TODO: free tokens matched by error terminal in bison
int main ( int argc, char * argv[] )
{
    // getopt flags
    int option;
    opterr = 0;

    // Flags
    bool print_syntax_tree = false;
    bool print_annotated_tree = false;
    bool code_generation = true;
    int parseStatus = 0;
    
    // Filenames
    std::string outfileTM;
    std::string infile;
    
    // yyerrorhelper initialization...
    initTokenMaps(); 
    
    // Get the input source file (*.c-))
    if(argc > 1)
    {
        infile = basename(argv[1]);
        size_t lastindex = infile.find_last_of(".");
        outfileTM = infile.substr(0, lastindex);
        outfileTM += ".tm";
        yyin = fopen(argv[1], "r");
    }

    // Command line option parsing
    // Nice to have: -h/--help flag listing options
    while ((option = getopt(argc, argv, "dpPsto:")) != EOF)
    {
        switch (option) {
        case 'd':
            yydebug = 1;
            break;
        case 'p':
            print_syntax_tree = true;
            break;
        case 'P':
            print_annotated_tree = true;
            break;
        case 't':
            testing = true;
            break;
        case 'o':
            code_generation = true;
            if ( optarg == "-" )
                outfileTM = "stdout";
            else
            {
                if ( optarg != NULL )
                    outfileTM.assign(optarg);
                else
                    std::cerr << "NULL optarg!" << std::endl;
            }
            break;
        default:
            break;
        }
    }

    parseStatus = yyparse(); // Bison builds the syntax tree for us
    if(testing) { std::cout << "yyparse() status: " << parseStatus << std::endl; }
    
    if ( print_syntax_tree )
        printAbstractTree(syntaxTree);
    
    if ( errors == 0 )
    {
        TreeNode * io = buildIOLibrary();
        linkSiblings(io, syntaxTree);
        annotatedTree = io;
        semanticAnalysis(annotatedTree);

        if ( print_annotated_tree )
            printAnnotatedTree(annotatedTree);
    }
 
    if ( code_generation && (errors == 0) )
        generateCode(outfileTM, infile);

    if(print_annotated_tree)
        printf("Offset for end of global space: %d\n", global_offset);
    
    printf("Number of warnings: %d\n", warnings);
    printf("Number of errors: %d\n", errors);

    fclose(yyin);
    return 0;
}

// Performs semantic analysis, generating the Annotated Syntax Tree
void semanticAnalysis ( TreeNode * og )
{
    SymbolTable * symtable = new SymbolTable();
    SymbolTable * typetable = new SymbolTable();
    TreeNode * tree = og;
    
    // *** Type annotation *** //
    typeResolution(NULL, tree, typetable);

    
    // *** Semantic Analysis *** //
    treeParse(NULL, tree, symtable);

    if ( symtable->lookup("main") == NULL )
    {
        printf("ERROR(LINKER): Procedure main is not defined.\n");
        errors++;
    }
    
    // *** Memory allocation *** //
    memorySizing(annotatedTree, symtable, 0);
    
    delete symtable;
    delete typetable;
}

// Like a ninja...silent insertion into symbol table and annotating of types. Few errors.
// TODO: rewrite into class?
void typeResolution ( TreeNode * par, TreeNode * node, SymbolTable * symtable )
{
    TreeNode * tree, parent;
    tree = node;
    (par == NULL) ? parent = tree : parent = par;

    while (tree != NULL)
    {
        TreeNode * temp;            // Temporary TreeNode
        int sibling_count = 0;      // Keeps track of siblings
        int line = tree->lineno;    // Node's line number
        std::string tree_svalue = svalResolve(tree);

        switch (tree->nodekind)
          {
            case DeclK:
              switch (tree->kind)
                {
                  case VarK:
                    if ( !symtable->insert(tree_svalue, tree) )
                    {
                        if(testing)
                            std::cout << "Failed to insert DeclK:VarK:" << tree_svalue << std::endl;
                    }
                    break;

                  case ParamK:
                    if ( !symtable->insert(tree_svalue, tree) )
                    {
                        if(testing)
                            std::cout << "Failed to insert DeclK:ParamK:" << tree_svalue << std::endl;                            
                    }                 
                    break;

                  case FunK:
                    if ( !symtable->insert(tree_svalue, tree) )
                    {
                        if(testing)
                            std::cout << "Failed to insert DeclK:FunK:" << tree_svalue << std::endl;
                    }
                    symtable->enter("Function " + tree_svalue);
                    break;
                  case IdK:
                    if ( testing )
                    {
                        std::cout << "Hit DeclK:IdK:" << tree_svalue << std::endl;
                    }
                    temp = (TreeNode *) symtable->lookup(tree_svalue);
                    if ( temp != NULL )
                    {
                        tree->nodetype = temp->nodetype;
                        if ( temp->isArray && temp->child[0] == NULL )
                        {
                            tree->isArray = true;
                        }
                        if(temp->isIndex)
                        {
                            tree->isIndex = true;
                        }
                        if ( temp->isStatic )
                        {
                            tree->isStatic = true;
                        }

                    } else
                    {
                        tree->nodetype = Undef;
                    }
                    break;
                }
              break;

            case StmtK:
              switch (tree->kind)
                {
                  case CompoundK:
                    if(parent->kind != FunK)
                        symtable->enter("Compound" + line);
                    else
                        tree->isFuncCompound = true;

                    break;

                  case IdK:
                    temp = (TreeNode *) symtable->lookup(tree_svalue);
                    if ( temp != NULL )
                    {
                        tree->nodetype = temp->nodetype;
                        if(temp->isArray && temp->child[0] == NULL)
                            tree->isArray = true;
                        if(temp->isIndex)
                            tree->isIndex = true;
                        if ( temp->isStatic )
                            tree->isStatic = true;
                    } else
                    {
                        tree->nodetype = Undef;
                    }
                    break;
                }
              break;
            case ExpK:
              switch (tree->kind)
                {
                  case CallK:
                    temp = (TreeNode *) symtable->lookup(tree_svalue);
                    if ( temp != NULL && temp->kind == FunK )
                        tree->nodetype = temp->nodetype;
                    else
                        tree->nodetype = Undef;
                    break;
                  case IdK:
                    temp = (TreeNode *) symtable->lookup(tree_svalue);
                    if ( temp != NULL )
                    {
                        tree->nodetype = temp->nodetype;
                        // TODO: could it have a non-null child, and still be an array?
                        if ( temp->isArray && temp->child[0] == NULL ) 
                            tree->isArray = true;
                        else
                            tree->isArray = false;
                        if ( temp->isIndex )
                            tree->isIndex = true;
                        if ( temp->isStatic )
                            tree->isStatic = true;                      
                    } else
                    {
                        tree->nodetype = Undef;
                    }
                    break;
                }
              break;
          }

        // Check if there are children
        if ( tree->numChildren > 0 )
        {
            for (int i = 0; i < tree->numChildren; i++)
            {
                if ( tree->child[i] != NULL )
                    typeResolution(tree, tree->child[i], symtable);
            }
        }

        if ( (tree->kind == CompoundK && parent->kind != FunK) || tree->kind == FunK )
        {
            symtable->leave();
        } 
        else if ( tree->kind == OpK )
        {
            switch (tree->token->bval)
              {
                case MULTIPLY:
                case PLUS:
                case MINUS:
                case DIVIDE:
                case MODULUS:
                  tree->nodetype = Integer;
                  break;
                default:
                  tree->nodetype = Boolean;
                  break;
              }
        } 
        else if(tree->kind == AssignK )
        {  
            if ( tree->token->bval == ASSIGN )
                tree->nodetype = tree->child[0]->nodetype;
            else
                tree->nodetype = Integer;
        } 
        else if ( tree->kind == UnaryK )
        {
            switch(tree->token->bval)
              {
                case NOT:
                case AND:
                case OR:
                    tree->nodetype = Boolean;
                    break;
                default:
                    tree->nodetype = Integer;
                    break;
              }
        }

        tree = tree->sibling; // Jump to the next sibling
        sibling_count++;
    } // end while
}


// TODO: put different error types into their own methods in a "errors.cpp" file.
void treeParse ( TreeNode * par, TreeNode * node, SymbolTable * symtable )
{
    TreeNode * tree, parent;
    tree = node;
    (par == NULL) ? parent = tree : parent = par;

    while (tree != NULL)
    {
        TreeNode * tmp;             // Temporary TreeNode
        int sibling_count = 0;      // Keeps track of siblings
        int line = tree->lineno;    // Node's line number

        Type lhs = Undef; // Left hand side  (child[0])'s type
        Type rhs = Undef; // Right hand side (child[1])'s type

        std::string child0_sval;
        std::string child1_sval;
        std::string tree_svalue = svalResolve(tree);
        std::string op = opToStr(tree->token);

        if ( tree->child[0] != NULL )
        {
            lhs = tree->child[0]->nodetype;
            child0_sval = svalResolve(tree->child[0]);
        }
        if ( tree->child[1] != NULL )
        {
            rhs = tree->child[1]->nodetype;
            child1_sval = svalResolve(tree->child[1]);
        }

        const char * lhs_str;
        const char * rhs_str;
        const char * tree_type_str;
        tree_type_str = typeToStr(tree->nodetype);
        lhs_str = typeToStr(lhs);
        rhs_str = typeToStr(rhs);

        if(testing)
        {
            printf("Tree\n");
            fflush(stdout);
            printTreeNode(tree);
        }
        // Switch on NodeKind (Declaration, Statement, Expression), then Kind
        switch (tree->nodekind)
          {
            case DeclK: // *** Node is part of a declaration ***
              switch (tree->kind)
                {
                  case VarK:
                    if ( !symtable->insert(tree_svalue, tree) )
                    {
                        tmp = (TreeNode *) symtable->lookup(tree_svalue);
                        printf("ERROR(%d): Symbol '%s' is already defined at line %d.\n", line, tree_svalue.c_str(), tmp->lineno);
                        errors++;
                    }
                    else if ( tree->numChildren == 1 && tree->child[0] != NULL )
                    {
                        if ( tree->isArray )
                        {
                            if ( tree->nodetype != Character )
                            {
                                printf("ERROR(%d): Array '%s' must be of type char to be initialized, but is of type %s.\n",
                                       line, tree_svalue.c_str(), tree_type_str);
                                errors++;
                            }
                            else if ( lhs != Character )
                            {
                                printf("ERROR(%d): Initializer for array variable '%s' must be a string, but is of nonarray type %s.\n",
                                       line, tree_svalue.c_str(), lhs_str);
                                errors++;
                            }
                        } else if ( tree->child[0]->isIndex )
                        {
                            printf("ERROR(%d): Cannot index nonarray '%s'.\n", line, tree_svalue.c_str());
                            errors++;
                        } else if ( tree->child[0]->isArray && tree->child[0]->child[0] == NULL )
                        {
                            printf("ERROR(%d): Initializer for nonarray variable '%s' of type %s cannot be initialized with an array.\n",
                                   line, tree_svalue.c_str(), tree_type_str);
                            errors++;
                        } else if ( tree->child[0]->isConstant != true )
                        {
                            printf("ERROR(%d): Initializer for variable '%s' is not a constant expression.\n",
                                   line, tree_svalue.c_str());
                            errors++;
                        } else if ( lhs != tree->nodetype )
                        {
                            printf("ERROR(%d): Variable '%s' is of type %s but is being initialized with an expression of type %s.\n",
                                   line, tree_svalue.c_str(), tree_type_str, lhs_str);
                            errors++;
                        }
                    }
                    break;

                  case ParamK:
                    if ( !symtable->insert(tree_svalue, tree) )
                    {
                        tmp = (TreeNode *) symtable->lookup(tree_svalue);
                        printf("ERROR(%d): Symbol '%s' is already defined at line %d.\n", line, tree_svalue.c_str(), tmp->lineno);
                        errors++;
                    }
                    break;

                  case FunK:
                    if ( !symtable->insert(tree_svalue, tree) )
                    {
                        tmp = (TreeNode *) symtable->lookup(tree_svalue);
                        printf("ERROR(%d): Symbol '%s' is already defined at line %d.\n", line, tree_svalue.c_str(), tmp->lineno);
                        errors++;
                    }
                    symtable->enter("Function " + tree_svalue);
                    func = tree;
                    break;

                  default:
                    if ( testing )
                        std::cout << "Hit default case in DeclK switch" << std::endl;
                    break;

                } // end DeclK kind switch
              break;


              // *** Node is part of a statement ***
            case StmtK:
              switch (tree->kind)
                {
                  case IfK:
                    if ( tree->numChildren > 0 && tree->child[0] != NULL )
                    {
                        if ( tree->child[0]->isArray && tree->child[0]->child[0] == NULL )
                        {
                            printf("ERROR(%d): Cannot use array as test condition in if statement.\n", line);
                            errors++;
                        } else if ( isBoolean(lhs) == false )
                        {
                            printf("ERROR(%d): Expecting Boolean test condition in if statement but got type %s.\n", line, lhs_str);
                            errors++;
                        }
                    }
                    else
                    {
                        if(testing)
                        {
                            std::cout << "Less than 2 children in a IfK..." << std::endl;
                            printTreeNode(tree);
                        }
                    }
                    break;

                  case CompoundK:
                    if(tree->isFuncCompound == false)
                        symtable->enter("Compound" + line);
                    break;

                    // TODO: change all if statements to use error checking methods
                  case ForeachK:
                      loopDepth.push(true);
                    if ( tree->numChildren > 1 && tree->child[0] != NULL && tree->child[1] != NULL )
                    {
                        if ( tree->child[0]->isArray && tree->child[0]->child[0] == NULL )
                        {
                            printf("ERROR(%d): In foreach statement the variable to the left of 'in' must not be an array.\n", line);
                            errors++;
                        }
                        if ( tree->child[1]->isArray == false || 
                                ( tree->child[1]->isArray && tree->child[1]->child[0] != NULL ) )
                        {
                            if ( !isInteger(lhs) )
                            {
                                printf("ERROR(%d): If not an array, foreach requires lhs of 'in' be of type int but it is type %s.\n",
                                       line, lhs_str);
                                errors++;
                            }
                            if ( !isInteger(rhs) )
                            {
                                printf("ERROR(%d): If not an array, foreach requires rhs of 'in' be of type int but it is type %s.\n",
                                       line, rhs_str);
                                errors++;
                            }
                        } else if ( !typeCompare(lhs, rhs) )
                        {
                            printf("ERROR(%d): Foreach requires operands of 'in' be the same type but lhs is type %s and rhs array is type %s.\n",
                                   line, lhs_str, rhs_str);
                            errors++;
                        }
                    }
                    else if (testing)
                    {
                        std::cout << "Foreach didn't have 2 children!" << std::endl;
                        printTreeNode(tree);
                    }
                        
                    break;

                  case WhileK:
                      loopDepth.push(true);
                    if ( tree->numChildren > 0 && tree->child[0] != NULL )
                    {
                        if ( lhs != Undef &&  tree->child[0]->isArray && tree->child[0]->child[0] == NULL )
                        {
                            printf("ERROR(%d): Cannot use array as test condition in while statement.\n", line);
                            errors++;
                        } else if ( lhs != Undef && lhs != Boolean )
                        {
                            printf("ERROR(%d): Expecting Boolean test condition in while statement but got type %s.\n", line, lhs_str);
                            errors++;
                        }
                    }
                    else if (testing)
                    {
                        std::cout << "While didn't have 2 children!" << std::endl;
                        printTreeNode(tree);
                    }
                    break;

                  case ReturnK:
                    return_found = true;
                    if ( tree->numChildren == 1 && tree->child[0] != NULL )
                    {
                        if ( lhs != Undef && tree->child[0]->isArray && tree->child[0]->child[0] == NULL )
                        {
                            printf("ERROR(%d): Cannot return an array.\n", line);
                            errors++;
                        }
                        tree->nodetype = tree->child[0]->nodetype;
                    }
                    if ( func != NULL )
                    {
                        if ( func->nodetype == Void && tree->nodetype != Void && tree->nodetype != Undef )
                        {
                            printf("ERROR(%d): Function '%s' at line %d is expecting no return value, but return has return value.\n",
                                   line, svalResolve(func).c_str(), func->lineno);
                            errors++;
                        } else if ( func->nodetype != Void && tree->nodetype == Void && tree->nodetype != Undef )
                        {
                            printf("ERROR(%d): Function '%s' at line %d is expecting to return type %s but return has no return value.\n",
                                   line, svalResolve(func).c_str(), func->lineno, typeToStr(func->nodetype));
                            errors++;
                        } else if ( func->nodetype != Void && func->nodetype != Undef && tree->nodetype != Undef && func->nodetype != tree->nodetype )
                        {
                            printf("ERROR(%d): Function '%s' at line %d is expecting to return type %s but got %s.\n",
                                   line, svalResolve(func).c_str(), func->lineno, typeToStr(func->nodetype), typeToStr(tree->nodetype));
                            errors++;
                        }
                    }
                    break;

                  case BreakK:
                    if(loopDepth.top() == false)
                    {
                        printf("ERROR(%d): Cannot have a break statement outside of loop.\n", line);
                        errors++;
                    }
                    break;

                  default:
                    if ( testing )
                    {
                        std::cout << "Hit default case in StmtK switch" << std::endl;
                    }
                    break;
                } // end StmtK kind switch
              break;


              // *** Node is part of an expression ***
            case ExpK:
              switch (tree->kind)
                {
                  case AssignK:
                    if ( tree->token != NULL )
                    {
                        switch (tree->token->bval)
                          {
                            case ASSIGN:
                              if ( lhs != Undef && rhs != Undef && lhs != rhs )
                              {
                                  printf("ERROR(%d): '%s' requires operands of the same type but lhs is type %s and rhs is %s.\n",
                                         line, op.c_str(), lhs_str, rhs_str);
                                  errors++;
                              }
                              if ( lhs != Undef && rhs != Undef && (tree->child[0]->isArray && tree->child[0]->child[0] == NULL)
                                   != (tree->child[1]->isArray && tree->child[1]->child[0] == NULL) )
                              {
                                  printf("ERROR(%d): '%s' requires that if one operand is an array so must the other operand.\n",
                                         line, op.c_str());
                                  errors++;
                              }
                              break;

                            case ADDASS:
                            case SUBASS:
                            case MULASS:
                            case DIVASS:
                              if ( ( tree->child[0]->isArray && tree->child[0]->child[0] == NULL)
                                || ( tree->child[1]->isArray && tree->child[1]->child[0] == NULL) )
                              {
                                  printf("ERROR(%d): The operation '%s' does not work with arrays.\n",
                                         line, op.c_str());
                                  errors++;
                              }                                
                              if (  lhs != Void && lhs != Undef && lhs != Integer )
                              {
                                  printf("ERROR(%d): '%s' requires operands of type %s but lhs is of type %s.\n",
                                         line, op.c_str(), typeToStr(Integer), lhs_str);
                                  errors++;
                              }
                              if ( rhs != Void && rhs != Undef && rhs != Integer )
                              {
                                  printf("ERROR(%d): '%s' requires operands of type %s but rhs is of type %s.\n",
                                         line, op.c_str(), typeToStr(Integer), rhs_str);
                                  errors++;
                              }
                              break;
                              // TODO: lhs != rhs checking?

                            case INC:
                            case DEC:
                              if ( lhs != Undef && lhs != Integer )
                              {
                                  printf("ERROR(%d): Unary '%s' requires operands of type %s but lhs is of type %s.\n",
                                         line, op.c_str(), typeToStr(Integer), lhs_str);
                                  errors++;
                              }
                              break;
                          } // end assign switch
                    } else
                    {
                        if ( testing )
                        {
                            std::cout << "Tried to access NULL token in ExpK: AssignK" << std::endl;
                        }
                    }
                    break;

                  case CallK:
                    if ( tree_svalue != "" )
                    {
                        tmp = (TreeNode *) symtable->lookup(tree_svalue);
                        if ( tmp == NULL )
                        {
                            printf("ERROR(%d): Symbol '%s' is not defined.\n", line, tree_svalue.c_str());
                            errors++;
                            tree->nodetype = Undef;
                        } else
                        {
                            if ( tmp->kind != FunK )
                            {
                                printf("ERROR(%d): '%s' is a simple variable and cannot be called.\n",
                                       line, svalResolve(tmp).c_str());
                                errors++;
                            } else
                            {
                                int treesibs = countSiblings(tree->child[0]);
                                int tempsibs = countSiblings(tmp->child[0]);

                                if ( treesibs < tempsibs )
                                {
                                    printf("ERROR(%d): Too few parameters passed for function '%s' defined on line %d.\n",
                                           line, svalResolve(tmp).c_str(), tmp->lineno);
                                    errors++;
                                } else if ( treesibs > tempsibs )
                                {
                                    printf("ERROR(%d): Too many parameters passed for function '%s' defined on line %d.\n",
                                           line, svalResolve(tmp).c_str(), tmp->lineno);
                                    errors++;
                                }
                                checkArgTypes(tree, tmp);
                            }
    
                        }
                    }
                    break;

                  case FunK:
                    printf("ERROR(%d): Cannot use function '%s' as a simple variable.\n", line, tree_svalue.c_str());
                    errors++;
                    break;

                  case OpK:
                    if ( tree->numChildren == 2 )
                    {
                        if ( tree->token != NULL )
                        {
                            switch (tree->token->bval)
                              {
                                case NOT:
                                case QUESTION:
                                  if ( testing )
                                  {
                                      std::cout << "Shouldn't reach Unary in a OpK" << std::endl;
                                  }
                                  break;

                                case EQ:
                                case NOTEQ:
                                  if ( (tree->child[0]->isArray && tree->child[0]->child[0] == NULL)
                                    != (tree->child[1]->isArray && tree->child[1]->child[0] == NULL) )
                                  {
                                      printf("ERROR(%d): '%s' requires that if one operand is an array so must the other operand.\n", line, op.c_str());
                                      errors++;
                                  }                                  
                                  if( !typeCompare(lhs, rhs) )
                                  {
                                      printf("ERROR(%d): '%s' requires operands of the same type but lhs is type %s and rhs is %s.\n",
                                             line, op.c_str(), lhs_str, rhs_str);
                                      errors++;
                                  }                                   
                                  break;                                  

                                case LESSEQ:
                                case LTHAN:
                                case GRTEQ:
                                case GTHAN:
                                  if ( (!isInteger(lhs) && !isCharacter(lhs))
                                    || (!isInteger(rhs) && !isCharacter(rhs)) )
                                  {
                                      if ( !isInteger(lhs) && !isCharacter(lhs) )
                                      {
                                          printf("ERROR(%d): '%s' requires operands of type %s but lhs is of type %s.\n",
                                                 line, op.c_str(), "char or int", lhs_str);
                                          errors++;
                                      }
                                      if ( !isInteger(rhs) && !isCharacter(rhs) )
                                      {
                                          printf("ERROR(%d): '%s' requires operands of type %s but rhs is of type %s.\n",
                                                 line, op.c_str(), "char or int", rhs_str);
                                          errors++;
                                      }
                                  }
                                  if ( !typeCompare(lhs, rhs) )
                                  {
                                      printf("ERROR(%d): '%s' requires operands of the same type but lhs is type %s and rhs is %s.\n",
                                             line, op.c_str(), lhs_str, rhs_str);
                                      errors++;
                                  }
                                  if ( (tree->child[0]->isArray && tree->child[0]->child[0] == NULL)
                                       || (tree->child[1]->isArray && tree->child[1]->child[0] == NULL) )
                                  {
                                      printf("ERROR(%d): The operation '%s' does not work with arrays.\n",
                                             line, op.c_str());
                                      errors++;
                                  }
                                  break;

                                case MULTIPLY:
                                case PLUS:
                                case MINUS:
                                case DIVIDE:
                                case MODULUS:
                                  if ( !isInteger(lhs) || !isInteger(rhs) )
                                  {
                                      if ( !isInteger(lhs) )
                                      {
                                          printf("ERROR(%d): '%s' requires operands of type %s but lhs is of type %s.\n",
                                                 line, op.c_str(), typeToStr(Integer), lhs_str);
                                          errors++;
                                      }
                                      if ( !isInteger(rhs) )
                                      {
                                          printf("ERROR(%d): '%s' requires operands of type %s but rhs is of type %s.\n",
                                                 line, op.c_str(), typeToStr(Integer), rhs_str);
                                          errors++;
                                      }
                                  } else if ( !typeCompare(lhs, rhs) )
                                  {
                                      printf("ERROR(%d): '%s' requires operands of the same type but lhs is type %s and rhs is %s.\n",
                                             line, op.c_str(), lhs_str, rhs_str);
                                      errors++;
                                  }
                                  if ( (tree->child[0]->isArray && tree->child[0]->child[0] == NULL)
                                       || (tree->child[1]->isArray && tree->child[1]->child[0] == NULL) )
                                  {
                                      printf("ERROR(%d): The operation '%s' does not work with arrays.\n",
                                             line, op.c_str());
                                      errors++;
                                  }                                  
                                  break;
                                  
                                case AND:
                                case OR:
                                  if ( !isBoolean(lhs) )
                                  {
                                      printf("ERROR(%d): '%s' requires operands of type %s but lhs is of type %s.\n",
                                             line, op.c_str(), typeToStr(Integer), lhs_str);
                                      errors++;
                                  }
                                  if ( !isBoolean(rhs) )
                                  {
                                      printf("ERROR(%d): '%s' requires operands of type %s but rhs is of type %s.\n",
                                             line, op.c_str(), typeToStr(Integer), rhs_str);
                                      errors++;
                                  }
                                  if ( (tree->child[0]->isArray && tree->child[0]->child[0] == NULL)
                                       || (tree->child[1]->isArray && tree->child[1]->child[0] == NULL) )
                                  {
                                      printf("ERROR(%d): The operation '%s' does not work with arrays.\n",
                                             line, op.c_str());
                                      errors++;
                                  }                                      
                                    break;
                              }
                           
                        } else
                        {
                            if ( testing )
                            {
                                std::cout << "Tried to access NULL token in ExpK: OpK, or something like that" << std::endl;
                            }
                        }
                    }
                    break;

                  case UnaryK:
                    if ( tree->numChildren == 1 && tree->child[0] != NULL )
                    {
                        switch (tree->token->bval)
                          {
                            case MULTIPLY:
                              if ( tree->child[0]->isArray == false || 
                                 ( tree->child[0]->isArray && tree->child[0]->child[0] != NULL ) )
                              {
                                  printf("ERROR(%d): The operation '%s' only works with arrays.\n",
                                         line, tree_svalue.c_str());
                                  errors++;
                              }
                              break;

                            case MINUS:
                              if ( !isInteger(lhs) )
                              {
                                  printf("ERROR(%d): Unary '%s' requires an operand of type %s but was given %s.\n",
                                         line, tree_svalue.c_str(), typeToStr(Integer), lhs_str);
                                  errors++;
                              }
                              break;
                            case QUESTION:
                              if ( !isInteger(lhs) )
                              {
                                  printf("ERROR(%d): Unary '%s' requires an operand of type %s but was given %s.\n",
                                         line, tree_svalue.c_str(), typeToStr(Integer), lhs_str);
                                  errors++;
                              }
                              if ( tree->child[0]->isArray && tree->child[0]->child[0] == NULL )
                              {
                                  printf("ERROR(%d): The operation '%s' does not work with arrays.\n",
                                         line, op.c_str());
                                  errors++;
                              }
                              break;

                            case NOT:
                              if ( lhs != Undef && lhs != Boolean )
                              {
                                  printf("ERROR(%d): Unary '%s' requires an operand of type %s but was given %s.\n",
                                         line, tree_svalue.c_str(), typeToStr(Boolean), lhs_str);
                                  errors++;
                              }
                              if ( tree->child[0]->isArray && tree->child[0]->child[0] == NULL )
                              {
                                  printf("ERROR(%d): The operation '%s' does not work with arrays.\n",
                                         line, op.c_str());
                                  errors++;
                              }                              
                              break;
                          }
                    }
                    break;

                  case ParamK:
                      if (testing)
                      {
                          std::cout << "ExpK:ParamK...wat?" << std::endl;
                          printTreeNode(tree);
                      }
                    break;

                  case IdK:
                    tmp = (TreeNode *) symtable->lookup(tree_svalue);
                    if ( tmp != NULL )
                    {
                        if ( tree->child[0] != NULL && tree->child[0]->isIndex )
                        {
                            tree->isArray = false; // can we have arr[] be a thing?
                            if ( tmp->isArray ) // TODO: possible subtle error, if add "child[0] == null" check we have...issues. do everything05.c-
                            {
                                if ( child0_sval == tree_svalue && tree->child[0]->child[0] == NULL)
                                {
                                    printf("ERROR(%d): Array index is the unindexed array '%s'.\n", line, tree_svalue.c_str());
                                    errors++;
                                } else if ( lhs != Integer )
                                {
                                    printf("ERROR(%d): Array '%s' should be indexed by type int but got %s.\n",
                                           line, tree_svalue.c_str(), lhs_str);
                                    errors++;
                                }
                            } else
                            {
                                printf("ERROR(%d): Cannot index nonarray '%s'.\n",
                                       line, tree_svalue.c_str());
                                errors++;
                            }
                        }
                    } else
                    {
                        printf("ERROR(%d): Symbol '%s' is not defined.\n", line, tree_svalue.c_str());
                        errors++;
                        tree->nodetype = Undef;
                    }
                    break;

                  default:
                    break;
                } // end ExpK kind switch
              break;

            default:
              if ( testing )
              {
                  std::cout << "Hit default case in ExpK switch" << std::endl;
                  printTreeNode(tree);
              }
              break;
          } // end nodekind switch


        // Check if there are children
        if ( tree->numChildren > 0 )
        {
            for (int i = 0; i < tree->numChildren; i++)
            {
                if ( tree->child[i] != NULL )
                {
                    treeParse(tree, tree->child[i], symtable );
                }
            }
        }
        
        if ( (tree->kind == CompoundK && (tree->isFuncCompound == false)) || tree->kind == FunK )
        {
            symtable->leave();
        }
        else if(tree->kind == WhileK || tree->kind == ForeachK)
        {
            //iloop = false;
            loopDepth.pop();
        }
        
        if ( tree->kind == FunK && tree->lineno > -1 )
        {
            if ( tree->nodetype != Void && return_found == false )
            {
                printf("WARNING(%d): Expecting to return type %s but function '%s' has no return statement.\n",
                       line, tree_type_str, tree_svalue.c_str());
                warnings++;
            }
            return_found = false;
            func = NULL;
        } 

        tree = tree->sibling; // Jump to the next sibling
        sibling_count++;

    } // end while
}

void generateCode( std::string output_file, std::string infile )
{
    codegenTM cg( annotatedTree, global_offset, output_file, infile );
    cg.generateCode();
}

// Creates the tree of IO functions
TreeNode * buildIOLibrary ( )
{
    TreeNode * in, out, inputb, outputb, inputc, outputc, outnl, idummy, bdummy, cdummy;

    /* Integer Input */
    in = makeParent(DeclK, FunK, Integer, -1, "input");
    in->isIO = InputI;
    
    /* Integer Output */
    out = makeParent(DeclK, FunK, Void, -1, "output");
    out->isIO = OutputI;
    idummy = makeParent(DeclK, ParamK, Integer, -1, "*dummy*");
    addChild(out, idummy);
    linkSiblings(in, out);
    
    /* Boolean Input */
    inputb = makeParent(DeclK, FunK, Boolean, -1, "inputb");
    inputb->isIO = InputB;
    linkSiblings(in, inputb);
    
    /* Boolean Output */
    outputb = makeParent(DeclK, FunK, Void, -1, "outputb");
    outputb->isIO = OutputB;
    bdummy = makeParent(DeclK, ParamK, Boolean, -1, "*dummy*");
    addChild(outputb, bdummy);
    linkSiblings(in, outputb);
    
    /* Character Input */
    inputc = makeParent(DeclK, FunK, Character, -1, "inputc");
    inputc->isIO = InputC;
    linkSiblings(in, inputc);
    
    /* Character Output */
    outputc = makeParent(DeclK, FunK, Void, -1, "outputc");
    outputc->isIO = OutputC;
    cdummy = makeParent(DeclK, ParamK, Character, -1, "*dummy*");
    addChild(outputc, cdummy);
    linkSiblings(in, outputc);
    
    /* Newline */
    outnl = makeParent(DeclK, FunK, Void, -1, "outnl");
    outnl->isIO = OutNL;
    linkSiblings(in, outnl);

    // Return beginning of the linked sibling list
    return in;
}

void checkArgTypes( TreeNode * call, TreeNode * func)
{
    if(call == NULL || func == NULL )
    {
        if(testing)
        {
            std::cout << "NULL passed to checkArgTypes" << std::endl;
        }
        return;
    }
    
    if(testing)
    {
        std::cout << "Entering checkArgTypes..." << std::endl;
    }
    TreeNode * temp_call = call->child[0];
    TreeNode * temp_func = func->child[0];
    int sibling_count = 1;
    
    while( temp_call != NULL && temp_func != NULL)
    {
        if(testing)
        {
            std::cout << "checkArgTypes:sibling:" << sibling_count << std::endl;
        }
        if ( temp_func->nodetype != temp_call->nodetype )
        {
            printf("ERROR(%d): Expecting type %s in parameter %i of call to '%s' defined on line %d but got %s.\n",
                   call->lineno, typeToStr(temp_func->nodetype), sibling_count, svalResolve(func).c_str(), func->lineno, typeToStr(temp_call->nodetype));
            errors++;
        }
        if ( temp_func->isArray && (!temp_call->isArray || (temp_call->isArray && temp_call->child[0] != NULL)) )
        {
            printf("ERROR(%d): Expecting array in parameter %i of call to '%s' defined on line %d.\n",
                   call->lineno, sibling_count, svalResolve(func).c_str(), func->lineno);
            errors++;
        }
        if ( !temp_func->isArray && (temp_call->isArray && temp_call->child[0] == NULL) )
        {
            printf("ERROR(%d): Not expecting array in parameter %i of call to '%s' defined on line %d.\n",
                   call->lineno, sibling_count, svalResolve(func).c_str(), func->lineno);
            errors++;
        }
        
        sibling_count++;
        temp_call = temp_call->sibling;
        temp_func = temp_func->sibling;
    }
    
    
}

// TODO: rewrite into a class similiar to codegen
// TODO: SERIOUSLY, CLASS==BETTER
int memorySizing( TreeNode * node, SymbolTable * symtable, int parOff )
{
    TreeNode * tree = node;
    int tOff = 0;
    int childVal = 0;
    int line = -99;
    std::string tree_svalue;

    while (tree != NULL)
    {
        line = tree->lineno;                // Node's line number
        tree_svalue = svalResolve(tree);    // Node's string

        switch (tree->nodekind) {
        case DeclK:
            switch (tree->kind) {
            case VarK:
                if ( tree->isArray )
                    tree->size = tree->arraySize + 1; // +1 for pointer
                else
                    tree->size = 1; // We use the same size regardless of type

                if ( symtable->depth() > 1 && !tree->isStatic )
                {
                    tree->offsetReg = local;
                    tree->location = parOff + tOff;
                    tOff -= tree->size;
                } else
                {
                    if ( tree->isStatic )
                        tree->offsetReg = local;
                    else
                        tree->offsetReg = global;
                    tree->location = global_offset;
                    global_offset -= tree->size;
                }
                if ( tree->isArray )
                    tree->location--; // skip array size
                break;

            case ParamK:
                tree->size = 1; // It's just a pointer to location of passed variable
                tree->location = parOff + tOff;
                tOff -= tree->size;
                tree->offsetReg = local;
                tree->isParam = true;
                param_count++;
                break;

            case FunK:
                tOff = 0;
                tree->offsetReg = global;
                param_count = 0;
                tree->location = 0;
                tree->size = 2;
                tOff -= tree->size;
                symtable->enter("Function " + tree_svalue);
                
                tOff += memorySizing(tree->child[0], symtable, tOff);   // Params
                memorySizing(tree->child[1], symtable, tOff);           // Compound

                tree->size += param_count;
                symtable->leave();
                tOff = 0;
                break;
            default:
                for (int i = 0; i < tree->numChildren; i++)
                    memorySizing(tree->child[i], symtable, parOff);
                break;
            } // end DeclK switch
            break;

        case StmtK:
            switch (tree->kind) {
            case CompoundK:
                tree->location = tOff;
                if ( tree->isFuncCompound == false )
                {
                    symtable->enter("Compound" + line);
                    childVal = memorySizing(tree->child[0], symtable, tOff + parOff);
                    memorySizing(tree->child[1], symtable, childVal + tOff + parOff);
                    symtable->leave();
                } else
                {
                    childVal = memorySizing(tree->child[0], symtable, tOff + parOff);
                    memorySizing(tree->child[1], symtable, childVal + tOff + parOff);
                }
                tree->size = parOff + childVal;
                break;
                
            default:
                for(int i = 0; i < tree->numChildren; i++)
                    memorySizing(tree->child[i], symtable, parOff);
                break;
            } // end StmtK switch
            break;

        case ExpK:
            for (int i = 0; i < tree->numChildren; i++)
                memorySizing(tree->child[i], symtable, parOff);
            break;
        } // end nodekind switch
        tree = tree->sibling; // Jump to the next sibling
    } // end while    
    return tOff;
}