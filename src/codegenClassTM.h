/* 
 * File:   codegenClassTM.h
 * Author: tehlizard
 *
 * Created on December 4, 2015, 12:40 PM
 */

#ifndef CODEGENCLASSTM_H
#define	CODEGENCLASSTM_H

#include "cminus.h"
#include "symbolTable.h"
#include "types.h"
#include <string>
#include <fstream>

class codegenTM {
public:
    void generateCode(); // This actually generates code. wow.
    codegenTM( TreeNode * t, int g, std::string of, std::string inf );
    ~codegenTM(); // destructor: closes file, few misc cleanup tasks
    
private:
    /* Init */
    void initSetup();
    void initGlobals();
    
    /* Generate by nodekind */
    void generateDeclaration( TreeNode * node );    // generic decleration 
    void generateStatement( TreeNode * node );      // generic statement
    void generateExpression( TreeNode * node );     // generic expression
    void loopSiblings( NodeKind nk, TreeNode * node );
    
    /* General generation */
    void treeTraversal( TreeNode * tree ); // loopsiblings without nk selection
    void loadParams( TreeNode * node, int off ); // loads params before function call
    
    /* SymbolTable creation + copying node values */
    void buildTable();
    void tableRecurse(TreeNode * node);
    
    /* ID resolution functions */
    void storeVar( TreeNode * var, int reg ); // ST reg->var
    void storeArrayVar( TreeNode * arr, int reg, int index ); // USES: ac3
    // LD val<-var
    // If array, calcuate index, LD reg<-array[index]
    void loadVar( TreeNode * var, int reg );
    void loadArrayVar( TreeNode * arr, int reg, int index ); // USES: ac3
    void loadArrayAddr( TreeNode * arr, int reg ); // LDA reg<-arr
    
    /* Typing saving functions  */
    TreeNode * idResolve(TreeNode * node);
    TreeNode * lookup(std::string s);
    void standardRet(); // zero out return first
    void funRet();      // Load ret addr, adjust FP, return 
    void IOroutines( IO io ); // generates IO routines based on io
    
    /* Helper functions */
    std::string timestamp(); // returns a timestamp for current local time
    
    /* Parsed food for generation */
    SymbolTable * symtable;
    TreeNode * aTree;
    
    /* IO stuff */
    std::string infilename;
    std::string outfilename;
    std::ofstream outfile;
    bool emitToFile;
    
    
    /*** Emit stuff ***/
    
    /* Emit locations */
    int emitLoc; // Location for current instruction emission  
    int highEmitLoc; // Highest location emitted thus far. Used with: emitSkip, emitBackup, emitRestore
    int mainLoc;
    
    /* Offsets */
    int gOffset;
    int fOffset;
    int tOffset;
    
    /* Emit methods */
    void emitComment( std::string s );
    void emitRO( const char *op, int r, int s, int t, std::string c);
    void emitRM( const char * op, int r, int d, int s, std::string c);
    int emitSkip( int howMany);
    void emitBackup( int loc);
    void emitRestore();
    void emitRMAbs( const char *op, int r, int a, std::string c);
    
    
    /* REGISTERS  */
    const int gp = 0; // Global pointer 
    const int fp = 1; // Frame pointer
    const int ret = 2; // Value being returned
    const int val = 3; // Return pointer, calculation result, etc...
    const int ac1 = 4; // Accumulator 1
    const int ac2 = 5; // Accumulator 2
    const int ac3 = 6; // Accumulator 3
    const int pc = 7; // Program counter
    
};

#endif	/* CODEGENCLASSTM_H */

