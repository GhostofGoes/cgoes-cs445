#include "cminus.h"
#include "types.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <string.h>
#include <sstream>

// Recursively prints the abstract syntax tree
// Print spaces at end of strings, if necessary.
// TODO: output redirection
// TODO: null characters in char and string consts, store/print properly (check treeParse for solution, make function)
void printAbstractTree(TreeNode * og, int indent_count) {
	
	TreeNode * tree = og;
	int sibling_count = 0; // Keeps track of siblings

	// Output buffer (TODO: string stream better option?)
	std::string outstr;

	// Prints all nodes of the tree
	while( tree != NULL ) {

		if(sibling_count > 0) {
			outstr.append("|Sibling: ");
			//outstr += sibling_count;
			outstr.append(std::to_string(sibling_count));
			outstr.append("  ");
			std::cout << applyIndents(outstr, indent_count);
			std::cout.flush();
			outstr.clear();
		}

		switch(tree->kind) {
			case OpK:
				outstr += "Op: ";
				if( tree->token->svalue != NULL ) {
					outstr += tree->token->svalue;
				}
				else {
					outstr.push_back(tree->token->cvalue);
				}
				break;

			case UnaryK:
				outstr.append("Op: ");
				outstr.push_back(tree->token->cvalue);
				break;

			case ConstK:
				outstr.append("Const: ");
				if(tree->nodetype == Boolean) {
					outstr.append(iboolToString(tree->token->ivalue));
				}
				else if(tree->nodetype == Integer) {
					outstr += tree->token->ivalue;
				}
				else if(tree->nodetype == Character) {
					if(tree->token->svalue != NULL ) {
						outstr += '"';
						outstr += tree->token->svalue ? tree->token->svalue : "";
						outstr += '"';
					}
					else {
						outstr += '\'';
						outstr += (tree->token->cvalue);
						outstr += '\'';
					}
				}
				break;

			case IdK:
				outstr.append("Id: ");
				outstr.append(tree->token->svalue ? tree->token->svalue : "");
				break;

			case AssignK:
				outstr.append("Assign: ");

				if( tree->nodetype == Void ) {
					outstr.push_back(tree->token->cvalue);
				}
				if( tree->nodetype == Integer ) {
					outstr += tree->token->svalue ? tree->token->svalue : "";
				}

				break;

			case IfK:
				outstr.append("If");
				break;

			case CompoundK:
				outstr.append("Compound");
				break;

			case ForeachK:
				outstr.append("Foreach");
				break;

			case WhileK:
				outstr.append("While");
				break;

			case ReturnK:
				outstr.append("Return");
				break;

			case BreakK:
				outstr.append("Break");
				break;

			case VarK:
			case ParamK:
				if(tree->kind == VarK)
					{ outstr.append("Var "); }
				else
					{ outstr.append("Param "); }
				outstr.append(tree->svalue ? tree->svalue : "");
				if(tree->isArray)
					{ outstr.append(" is array"); }
				outstr.append(" of type ");
				outstr.append(typeToStr(tree->nodetype));
				break;

			case FunK:
				outstr.append("Func ");
				outstr.append(tree->svalue ? tree->svalue : "");
				outstr.append(" returns type ");
				outstr.append(typeToStr(tree->nodetype));
				break;

			case CallK:
				outstr.append("Call: ");
				outstr.append(tree->svalue ? tree->svalue : "");
				break;
			default:
				outstr.append("\nWe shouldn't get here\n");
				break;

		} // end switch

		std::cout << outstr << " [line: " << tree->lineno << "]" << std::endl;
		std::cout.flush();
		outstr.clear();
		
		// Check if there are children
		// TODO: check for NULL children
		if( tree->numChildren > 0 ) {
			for ( int i = 0; i < tree->numChildren; i++ ) {
				if(tree->child[i] != NULL ) {
					outstr.append("|   Child: ");
					//outstr += i;
					outstr.append(std::to_string(i));
					outstr.append("  ");
					std::cout << applyIndents(outstr, indent_count);
					std::cout.flush();
					outstr.clear();
					printAbstractTree(tree->child[i], indent_count + 1);
				}
			}			
		}
		
		tree = tree->sibling; // Jump to the next sibling
		sibling_count++;
	} // end while
	
}

// Prints the Annotated Syntax Tree 
void printAnnotatedTree( TreeNode * og, int indent_count ) {

	TreeNode * tree = og;
	int sibling_count = 0; // Keeps track of siblings
	std::string outstr; // Output buffer

	// Prints all nodes of the tree
	while( tree != NULL ) {

		if(sibling_count > 0) {
			outstr.append("|Sibling: ");
			outstr.append(std::to_string(sibling_count));
			outstr.append("  ");
			std::cout << applyIndents(outstr, indent_count);
			std::cout.flush();
			outstr.clear();
		}

		switch(tree->kind) {
			case OpK:
				outstr += "Op: ";
				outstr += opToStr(tree->token);
				outstr += (" Type: ");
				outstr += typeToStr(tree->nodetype);
				break;

			case UnaryK:
				outstr.append("Op: ");
				outstr.push_back(tree->token->cvalue);
				outstr += (" Type: ");
				outstr += typeToStr(tree->nodetype);
				break;

			case ConstK:
				outstr.append("Const: ");
				if(tree->nodetype == Boolean) {
					outstr.append(iboolToString(tree->token->ivalue));
				}
				else if(tree->nodetype == Integer) {
					outstr.append(std::to_string(tree->token->ivalue));
				}
				else if(tree->nodetype == Character) {
					if(tree->token->svalue != NULL ) {
						outstr += '"';
						outstr += tree->token->svalue ? tree->token->svalue : "";
						outstr += '"';
					}
					else {
						outstr += '\'';
						outstr += (tree->token->cvalue);
						outstr += '\'';
					}
				}

				outstr.append(" Type: ");
				if(tree->isArray)
					{ outstr.append("is array of "); }
				outstr.append(typeToStr(tree->nodetype));
				break;

			case IdK:
				outstr.append("Id: ");
				outstr.append(svalResolve(tree));
				outstr += (" Type: ");
				if(tree->isArray)
					{ outstr.append("is array of "); }
				outstr += typeToStr(tree->nodetype);
				break;

			case AssignK:
				outstr.append("Assign: ");
				outstr += opToStr(tree->token);
				outstr += (" Type: ");
				if(tree->isArray)
					{ outstr.append("is array of "); }
				outstr += typeToStr(tree->nodetype);
				break;

			case IfK:
				outstr.append("If");
				break;

			case CompoundK:
				outstr.append("Compound");
				break;

			case ForeachK:
				outstr.append("Foreach");
				break;

			case WhileK:
				outstr.append("While");
				break;

			case ReturnK:
				outstr.append("Return");
				break;

			case BreakK:
				outstr.append("Break");
				break;

				// TODO: combine var/param?
			case VarK:
				outstr.append("Var ");
				outstr.append(svalResolve(tree));
				if(tree->isArray)
					{ outstr.append(" is array of"); }
				outstr += " ";
				outstr.append(typeToStr(tree->nodetype));

				break;

			case ParamK:
				outstr.append("Param ");
				outstr.append(svalResolve(tree));
				if(tree->isArray)
					{ outstr.append(" is array of "); }
				outstr += " ";
				outstr.append(typeToStr(tree->nodetype));
				break;

			case FunK:
				outstr.append("Func ");
				outstr.append(svalResolve(tree));
				outstr.append(" returns type ");
				outstr.append(typeToStr(tree->nodetype));
				break;

			case CallK:
				outstr.append("Call: ");
				outstr.append(svalResolve(tree));
				outstr += (" Type: ");
				outstr += typeToStr(tree->nodetype);
				break;

			default:
				outstr.append("\nWe shouldn't get here\n");
				break;

		} // end switch

		std::cout << outstr << " [line: " << tree->lineno << "]" << std::endl;
		std::cout.flush();
		outstr.clear();

		// Check if there are children
		// TODO: check for NULL children
		if( tree->numChildren > 0 ) {
			for ( int i = 0; i < tree->numChildren; i++ ) {
				if(tree->child[i] != NULL ) {
					outstr.append("|   Child: ");
					outstr.append(std::to_string(i));
					outstr.append("  ");
					std::cout << applyIndents(outstr, indent_count);
					std::cout.flush();
					outstr.clear();
					printAnnotatedTree(tree->child[i], indent_count + 1);
				}
			}
		}

		tree = tree->sibling; // Jump to the next sibling
		sibling_count++;
	} // end while

}

// Performs semantic analysis, generating the Annotated Syntax Tree
void semanticAnalysis( TreeNode * og ) {
	// TODO: add return
	SymbolTable * symtable = new SymbolTable();
	TreeNode * tree = og;
	// TODO: initial node creations from Bison file

	treeParse( NULL, tree, symtable );

}


void treeParse( TreeNode * par, TreeNode * node, SymbolTable * symtable ) {

	TreeNode * tree;
	tree = node;

	TreeNode * parent;
	if( par == NULL ) {
		// TODO: do something (actually) special for initial case?
		parent = tree;
	}
	else {
		parent = par;
	}

	while( tree != NULL ) {

		TreeNode * tmp; // Temporary TreeNode

		int sibling_count = 0; // Keeps track of siblings
		int line = tree->lineno; // Node's line number

		Type lhs = Void; // Left hand side (child[0])'s type
		Type rhs = Void; // Right hand side (child[1])'s type

		// TODO: variable comments
		std::string child0_sval;
		std::string child1_sval;
		std::string tree_svalue = svalResolve(tree);
		std::string op = opToStr(tree->token);

		if(tree->child[0] != NULL ) {
			lhs = tree->child[0]->nodetype;
			child0_sval = svalResolve(tree->child[0]);
		}
		if(tree->child[1] != NULL ) {
			rhs = tree->child[1]->nodetype;
			child1_sval = svalResolve(tree->child[1]);
		}

		const char * lhs_str;
		const char * rhs_str;
		const char * tree_type_str;
		tree_type_str = typeToStr(tree->nodetype);
		lhs_str = typeToStr(lhs);
		rhs_str = typeToStr(rhs);


		// Switch on NodeKind (Declaration, Statement, Expression), then Kind
		switch(tree->nodekind) {

		// *** Node is part of a declaration ***
		case DeclK:
			switch(tree->kind) {
			case VarK:
				if( !symtable->insert(tree_svalue, tree) ) {
					TreeNode * tmp = (TreeNode *)symtable->lookup(tree_svalue);
					printf("ERROR(%d): Symbol '%s' is already defined at line %d.\n", line, tree_svalue.c_str(), tmp->lineno );
					errors++;
				}
				if( tree->numChildren == 1 ) {
					if( tree->isArray ) {
						if( tree->child[0] != NULL && tree->child[0]->isIndex ) {
							if( child0_sval == tree_svalue ) {
								printf("ERROR(%d): Array index is the unindexed array '%s'.\n", line, tree_svalue.c_str());
								errors++;
							}
							else if( lhs != Integer ) {
								printf("ERROR(%d): Array '%s' should be indexed by type int but got %s.\n",
										line, tree_svalue.c_str(), lhs_str);
								errors++;
							}
						}
					}
					else if( tree->child[0]->isIndex ){
						printf("ERROR(%d): Cannot index nonarray '%s'.\n", line, tree_svalue.c_str());
						errors++;
					}
				}

				break;

			case ParamK:
				if( !symtable->insert(tree_svalue, tree) ) {
					TreeNode * tmp = (TreeNode *)symtable->lookup(tree_svalue);
					printf("ERROR(%d): Symbol '%s' is already defined at line %d.\n", line, tree_svalue.c_str(), tmp->lineno );
					errors++;
				}
				break;

			case FunK:
				if( !symtable->insert(tree_svalue, tree) ) {
					TreeNode * tmp = (TreeNode *)symtable->lookup(tree_svalue);
					printf("ERROR(%d): Symbol '%s' is already defined at line %d.\n", line, tree_svalue.c_str(), tmp->lineno );
					errors++;
				}
				else if( tree->nodetype != Void ) {
					bool returnPresent = false;
					if(tree->child[0] != NULL && tree->child[0]->kind == CompoundK) {
						for(int i = 0; i < tree->child[0]->numChildren; i++) {
							if( tree->child[0]->child[i] != NULL && tree->child[0]->child[i]->kind == ReturnK ) {
								returnPresent = true;
								break;
							}
						}
					}
					if(!returnPresent) {
						printf("WARNING(%d): Expecting to return type %s but function '%s' has no return statement.\n",
								line, tree_type_str, tree_svalue.c_str() );
						warnings++;
					}
				}
				break;

			default:
				break;

			} // end DeclK kind switch
			break;



		// *** Node is part of a statement ***
		case StmtK:
			switch(tree->kind) {
			case IfK:
				if( tree->numChildren == 2 ) {
					if( tree->child[0]->isArray ) {
						printf("ERROR(%d): Cannot use array as test condition in if statement.\n", line);
						errors++;
					}
					else if( tree->child[0]->nodetype != Boolean ) {
						printf("ERROR(%d): Expecting Boolean test condition in if statement but got type %s.\n", line, lhs_str);
						errors++;
					}
				}
				break;

			case CompoundK:
				symtable->enter("Compound" + line);
				break;

			case ForeachK:
				if( tree->numChildren)  {
					if( tree->child[0]->isArray ) {
						printf("ERROR(%d): In foreach statement the variable to the left of 'in' must not be an array.\n", line);
						errors++;
					}
					else if ( lhs != Integer ) {
						printf("ERROR(%d): If not an array, foreach requires lhs of 'in' be of type int but it is type %s.\n",
								line, lhs_str);
						errors++;
					}
					if( rhs != Integer ) {
						printf("ERROR(%d): If not an array, foreach requires rhs of 'in' be of type int but it is type %s.\n",
								line, rhs_str);
						errors++;
					}
					if( lhs != rhs ) {
						printf("ERROR(%d): Foreach requires operands of 'in' be the same type but lhs is type %s and rhs array is type %s.\n",
								line, lhs_str, rhs_str);
						errors++;
					}
				}
				break;

			case WhileK:
				if( tree->numChildren == 2 ) {
					if( tree->child[0]->isArray ) {
						printf("ERROR(%d): Cannot use array as test condition in while statement.\n", line);
						errors++;
					}
					else if( tree->child[0]->nodetype != Boolean ) {
						printf("ERROR(%d): Expecting Boolean test condition in while statement but got type %s.\n", line, lhs_str);
						errors++;
					}
				}
				break;

			case ReturnK:
				if( tree->numChildren == 1 && tree->child[0] != NULL ) {
					if(symtable->lookup(child0_sval) == NULL ) {
						printf("ERROR(%d): Symbol '%s' is not defined.\n", line, child0_sval.c_str());
						errors++;
						tree->nodetype = Undef;
					}
					else if(tree->child[0]->isArray) {
						printf("ERROR(%d): Cannot return an array.\n", line);
						errors++;
					}

				}
				if( parent->kind == FunK ) {
					if( parent->nodetype == Void && tree->nodetype != Void ) {
						printf("ERROR(%d): Function '%s' at line %d is expecting no return value, but return has return value.\n",
								line, parent->svalue ? parent->svalue : "", parent->lineno);
						errors++;
					}
					else if( parent->nodetype != Void && tree->nodetype == Void ) {
						printf("ERROR(%d): Function '%s' at line %d is expecting to return type %s but return has no return value.\n",
								line, parent->svalue ? parent->svalue : "", parent->lineno, typeToStr(parent->nodetype));
						errors++;
					}
					else if( parent->nodetype != tree->nodetype ) {
						printf("ERROR(%d): Function '%s' at line %d is expecting to return type %s but got %s.\n",
								line, parent->svalue ? parent->svalue : "", parent->lineno, typeToStr(parent->nodetype), tree_type_str);
						errors++;
					}
				}
				break;

			case BreakK:
				if( parent->kind != ForeachK || parent->kind != WhileK ) {
					printf("ERROR(%d): Cannot have a break statement outside of loop.\n", line);
					errors++;
				}
				break;

			default:
				break;
			} // end StmtK kind switch
			break;


		// *** Node is part of an expression ***
		case ExpK:
			switch(tree->kind) {
			case AssignK:
				if( tree->numChildren == 1 ) {
					if( tree->child[0]->kind == VarK || tree->child[0]->kind == CallK ) {
						if(symtable->lookup(child0_sval) == NULL ) {
							printf("ERROR(%d): Symbol '%s' is not defined.\n", line, child0_sval.c_str());
							errors++;
							tree->child[0]->nodetype = Undef;
						}
					}
				}
				else if( tree->numChildren == 2 ) {
					if( tree->child[0]->kind == VarK || tree->child[0]->kind == CallK ) {
						if(symtable->lookup(child0_sval) == NULL ) {
							printf("ERROR(%d): Symbol '%s' is not defined.\n", line, child0_sval.c_str());
							errors++;
							tree->child[0]->nodetype = Undef;
						}
					}
					if( tree->child[1]->kind == VarK || tree->child[1]->kind == CallK ) {
						if(symtable->lookup(child1_sval) == NULL ) {
							printf("ERROR(%d): Symbol '%s' is not defined.\n", line, child1_sval.c_str());
							errors++;
						}
					}
				}
				break;

			case CallK:
				if( tree->svalue != NULL ) {
					TreeNode * temp = (TreeNode *)symtable->lookup(tree_svalue);
					if(temp == NULL ) {
						printf("ERROR(%d): Symbol '%s' is not defined.\n", line, tree_svalue.c_str());
						errors++;
						tree->nodetype = Undef;
					}
					else {
						if( temp->kind != FunK ) {
							printf("ERROR(%d): '%s' is a simple variable and cannot be called.\n", line, tree_svalue.c_str());
							errors++;
						}
						else {
							tree->nodetype = temp->nodetype;
						}
					}
				}
				break;

			case FunK:
				printf("ERROR(%d): Cannot use function '%s' as a simple variable.\n", line, tree_svalue.c_str());
				errors++;
				break;

			case OpK:
				if( tree->numChildren == 2 ) {
					if(tree->child[0]->kind == IdK && symtable->lookup(child0_sval) == NULL ) {
						printf("ERROR(%d): Symbol '%s' is not defined.\n", line, child0_sval.c_str());
						errors++;
						tree->child[0]->nodetype = Undef;
					}
					if(tree->child[1]->kind == IdK && symtable->lookup(child1_sval) == NULL ) {
						printf("ERROR(%d): Symbol '%s' is not defined.\n", line, child1_sval.c_str());
						errors++;
						tree->child[1]->nodetype = Undef;
					}

					if( tree->child[0]->isArray != tree->child[1]->isArray ) {
						printf("ERROR(%d): '%s' requires that if one operand is an array so must the other operand.\n", line, op.c_str());
						errors++;
					}

					// TODO: op table lookup
					// TODO: typing for operations, char/int
					if( rhs != tree->nodetype ) {
						printf("ERROR(%d): '%s' requires operands of type %s but rhs is of type %s.\n", line, op.c_str(), tree_type_str, rhs_str);
						errors++;
					}
					if( lhs != tree->nodetype ) {
						printf("ERROR(%d): '%s' requires operands of type %s but lhs is of type %s.\n",
								line, op.c_str(), tree_type_str, lhs_str);
						errors++;
					}
					if( lhs != rhs ) {
						printf("ERROR(%d): '%s' requires operands of the same type but lhs is type %s and rhs is %s.\n",
								line, op.c_str(), lhs_str, rhs_str);
						errors++;
					}
					if( tree->child[0]->isArray != tree->child[1]->isArray ) {
						printf("ERROR(%d): '%s' requires that if one operand is an array so must the other operand.\n", line, op.c_str());
						errors++;
					}
				}
				// TODO: works/doesn't work with arrays
				break;
			case UnaryK:
				if( tree->numChildren == 1 ) {
					if(tree->child[0]->kind == IdK && symtable->lookup(child0_sval) == NULL ) {
						printf("ERROR(%d): Symbol '%s' is not defined.\n", line, child0_sval.c_str());
						errors++;
						tree->child[0]->nodetype = Undef;
					}
					else if( lhs != tree->nodetype ) {
						printf("ERROR(%d): Unary '%s' requires an operand of type %s but was given %s.\n",
								line, tree_svalue.c_str(), tree_type_str, lhs_str );
						errors++;
					}
				}
				break;

			case ParamK:
				if( parent->kind == CallK ) { // TODO: this is wrong place
					tmp = (TreeNode *)symtable->lookup(tree_svalue);
					if( tmp != NULL ) {
						if( tmp->nodetype != tree->nodetype ) {
							printf("ERROR(%d): Expecting type %s in parameter %i of call to '%s' defined on line %d but got %s.\n",
								line, typeToStr(tmp->nodetype), sibling_count, svalResolve(parent).c_str(), parent->lineno, typeToStr(tree->nodetype) );
							errors++;
						}

						if( tmp->isArray && !tree->isArray ) {
							printf("ERROR(%d): Expecting array in parameter %i of call to '%s' defined on line %d.\n",
								line, sibling_count, svalResolve(parent).c_str(), parent->lineno);
							errors++;
						}
						else if( !tmp->isArray && tree->isArray ) {
							printf("ERROR(%d): Not expecting array in parameter %i of call to '%s' defined on line %d.\n",
								line, sibling_count, svalResolve(parent).c_str(), parent->lineno);
							errors++;
						}

					}
					else {
						printf("ERROR(%d): Symbol '%s' is not defined.\n", line, tree_svalue.c_str());
						errors++;
						tree->nodetype = Undef;
					}
				}
				break;

			case IdK:
				tmp = (TreeNode *)symtable->lookup(tree_svalue);
				if( tmp != NULL ) {
					tree->nodetype = tmp->nodetype;
				}
				else {
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
			break;
		} // end nodekind switch


		// Check if there are children
		if( tree->numChildren > 0 ) {
			for ( int i = 0; i < tree->numChildren; i++ ) {
				if(tree->child[i] != NULL ) {
					treeParse(tree, tree->child[i], symtable);
				}
			}
		}

		if( tree->kind == CompoundK ) {
			symtable->leave();
		}

		tree = tree->sibling; // Jump to the next sibling
		sibling_count++;

	} // end while
}

// TODO: placeholder Generates executable code 
void generateCode() {
	;
}

// Creates a new node for the syntax tree
// TODO: combine with make parent?
TreeNode * makeNode( NodeKind nk, Kind k, Type t, int line, char * svalue, TokenData * token ) {
	TreeNode * tempNode = allocNode();
	tempNode->nodekind = nk;
	tempNode->kind = k;
	tempNode->nodetype = t;
	tempNode->lineno = line;
	if( svalue != NULL ) {
		tempNode->svalue = strdup(svalue);
	}
	if(token != NULL) {
		tempNode->token = token;
	}

	return tempNode;
}

TreeNode * makeParent( NodeKind nk, Kind k, Type t, int l, char * svalue ) {
	TreeNode * tempNode = allocNode();
	tempNode->nodekind = nk;
	tempNode->kind = k;
	tempNode->nodetype = t;
	tempNode->lineno = l;
	if(svalue != NULL) {
		tempNode->svalue = strdup(svalue);
	}
	return tempNode;
}

// Adds a child to an existing syntax tree node
void addChild( TreeNode * parent, TreeNode * child ) {
	if( parent == NULL || child == NULL ) {
		return;
	}

	if(testing) {
		std::cout <<
				"Adding child " << typeToStr(child->nodetype) << " at line " << child->lineno
				<< " to parent " << typeToStr(parent->nodetype) << " of line " << parent->lineno << std::endl;
	}

	if( parent->numChildren >= 0 && parent->numChildren < 3 ) {
		parent->child[parent->numChildren] = child;
		parent->numChildren++;
	}
}

// Links siblings to each other, starting with the first
TreeNode * linkSiblings( TreeNode * sib1, TreeNode * sib2 ) {

	if(sib1 == NULL) {
		return sib2;
	}
	else if( sib2 == NULL ) {
		return sib1;
	}
	else {
		if(testing) {
			std::cout << "Linking sibling " << typeToStr(sib2->nodetype) << " of line " << sib1->lineno
					<< " to initial sibling " << typeToStr(sib1->nodetype) << " of line " << sib2->lineno << std::endl;
		}

		TreeNode * temp = sib1;

		while( temp->sibling != NULL ) {
			temp = temp->sibling;
		}

		temp->sibling = sib2;
	}

	return sib1;
}

// Applies Type t to all the siblings of TreeNode init
void applyTypeToSiblings( TreeNode * init, Type t ) {
	TreeNode * temp = init;

	while( temp != NULL) {
		temp->nodetype = t;
		temp = temp->sibling;
	}
}

// Applies Type t to all the children of TreeNode parent
void appplyTypeToChildren( TreeNode * parent, Type t ) {
	if(parent != NULL && parent->numChildren > 0) {
		for(int i = 0; i < parent->numChildren; i++) {
			parent->child[i]->nodetype = t;
		}
	}
}


// Allocates and zeros a new TreeNode
TreeNode * allocNode() {
	//TreeNode * tempNode = (TreeNode *)calloc(1, sizeof(TreeNode));
	TreeNode * tempNode = (TreeNode *)new TreeNode;
	tempNode->token = NULL;
	tempNode->lineno = 0;
	tempNode->svalue = NULL;
	tempNode->nodetype = Void;
	tempNode->nodekind = DefaultK;
	tempNode->numChildren = 0;
	tempNode->sibling = NULL;
	tempNode->isStatic = false;
	tempNode->isArray = false;
	tempNode->isIndex = false;
	
	return tempNode;
}

void freeTree( TreeNode * tree ) {
	TreeNode * prev;
	TreeNode * temp;

	prev = tree;

	while( prev != NULL ) {
		temp = prev;
		if(prev->token != NULL) {
			freeToken(prev->token);
		}

		if(prev->svalue != NULL) {
			free(prev->svalue);
		}
		// Check if there are children
		if( prev->numChildren > 0 ) {
			for ( int i = 0; i < prev->numChildren; i++ ) {
				if(prev->child[i] != NULL ) {
					freeTree(prev->child[i]);
				}
			}
		}

		prev = prev->sibling;
		delete(temp);
		//free(temp);
	} // end while
}

void freeToken( TokenData * token ) {
	if(token->svalue != NULL) {
		free(token->svalue);
	}

	if(token->input != NULL) {
		free(token->input);
	}

	free(token);
}

const char * iboolToString( int ib ) {
	if( ib ) {
		return "true";
	}
	else {
		return "false";
	}
}

// Returns the text string version of Type t
const char * typeToStr( Type t ) {
	switch(t) {
		case Void:
			return("void");
			break;
		case Integer:
			return("int");
			break;
		case Boolean:
			return("bool");
			break;
		case Character:
			return("char");
			break;
		case Undef:
			return("undefined type");
			break;
	}
	return "";
}

std::string opToStr( TokenData * tok ) {

	std::string temp;

	if( tok != NULL ) {
		if( tok->svalue != NULL ) {
			temp.assign(tok->svalue);
		}
		else {
			temp.push_back(tok->cvalue);
		}
	}

	return temp;
}

// TODO: ivalue handling!
std::string svalResolve( TreeNode * tree ) {

	std::string temp;
	if( tree != NULL ) {
		if( tree->svalue != NULL ) {
			temp.assign(tree->svalue);
		}
		else if( tree->token != NULL ) {
			temp = opToStr(tree->token);
		}
	}
	return temp;
}

// TODO: makes assumption about indentation with "|   "
std::string applyIndents( std::string s, int indent ) {
	std::string temp;
	for( int i = 0; i < indent; i++ ) {
		temp.append("|   ");
	}
	temp.append(s);
	return temp;
}

void printError( int line, std::string err ) {
	std::cout << "ERROR(" << line << "): " << err << "." << std::endl;
	std::cout.flush();
	err.clear();
	errors++;
}

void printWarning( int line, std::string warn ) {
	std::cout << "WARNING(" << line << "): " << warn << "." << std::endl;
	std::cout.flush();
	warn.clear();
	warnings++;
}
