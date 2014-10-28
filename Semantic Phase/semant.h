#ifndef SEMANT_H_
#define SEMANT_H_

#include <assert.h>
#include <map>
#include <utility>
#include <set>
#include <iostream>  
#include "cool-tree.h"
#include "stringtab.h"
#include "symtab.h"
#include "list.h"

#define TRUE 1
#define FALSE 0

class ClassTable;
typedef ClassTable *ClassTableP;

// This is a structure that may be used to contain the semantic
// information such as the inheritance graph.  You may use it or not as
// you like: it is only here to provide a container for the supplied
// methods.

class ClassTable {
private:
  int semant_errors;
  void install_basic_classes();
  ostream& error_stream;

public:
  ClassTable(Classes);
  int errors() { return semant_errors; }
  ostream& semant_error();
  ostream& semant_error(Class_ c);
  ostream& semant_error(Symbol filename, tree_node *t);
};

ClassTable *classtable;

typedef std::map<Symbol, Class_> classMAP;		/* map to maintain name versus Class_ object */
typedef std::pair<classMAP::iterator, bool> cMAPit;	/* map iterator */
typedef std::map<Symbol, bool> classMAP_bool;

classMAP classGraph;					/* mapping from class name of type Symbol to Class_ object */

typedef SymbolTable<Symbol,Symbol> symTab;		/* symbol table structure */
symTab *methodTab, *attrTab;					/* mapping from method/attribute to class name */

/* added prototypes */

void build_hierarchy(Class_); 					/* populate symbol tables from outermost scope to innermost */ 
Symbol leastAncestorCheck(Symbol, Symbol);		/* find the closest ancestor of two names */
Feature getMethods(Class_, Symbol);				/* search for a method recursively in full hierarchy of class */
bool checkClassInheritance(Symbol, Symbol);		/* check whether target class is one of the sub classes of parent */
#endif

