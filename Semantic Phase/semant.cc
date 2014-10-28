#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "semant.h"
#include "utilities.h"


extern int semant_debug;
extern char *curr_filename;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
static Symbol 
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;
//
// Initializing the predefined symbols.
//
static void initialize_constants(void)
{
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy        = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any 
    //   user-defined class.
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}

ClassTable::ClassTable(Classes classes) : semant_errors(0) , error_stream(cerr) {
    /* Fill this in */

    install_basic_classes();		/* add basic classes Str,Int,Bool,IO,Object */
    
    bool foundMain = false;         /* flag for Main class */
    cMAPit it;
    for(int i=classes->first(); classes->more(i); i = classes->next(i)){
    	Class_ cur = classes->nth(i);
    	Symbol name = cur->class_getName();
    	Symbol parent = cur->class_getParent();
        /* a class cannot inherit itself */
    	if(parent == name){
    		semant_error(cur) << "Class "<<name<<", or an ancestor of "<<name<<", is involved in an inheritance cycle.\n";
    		return;
    	}
        /* SELF_TYPE cannot be reimplemented */
        else if(name == SELF_TYPE){
            semant_error(cur) << "Redefinition of basic class SELF_TYPE.\n";
            return;
        }
        /* class cannot inherit from basic classes or self */
    	else if(parent == Int || parent == Bool || parent == Str || parent == SELF_TYPE){
    		semant_error(cur) << "Class "<<name<<" cannot inherit from class "<<parent<<".\n";
    		return;
    	}
        /* redefinition of classes is illegal */
    	it = classGraph.insert(std::make_pair(name,cur));
    	if(!it.second){
    		semant_error(cur) << "Class "<<name<<" was previously defined.\n";
    		return;
    	}
        /* verify if main found */
        if(name == Main){
            foundMain = true;
        }
    }
    /* program is illegal without a Main class */
    if(!foundMain){
        semant_error() << "Class Main is not defined.\n";
        return;      
    }

    /*
     *  check for cycle in the dependency graph above using 
     *  two pointer method (mainly used in linked lists)
     */
    classMAP::iterator start = classGraph.begin();
    Symbol pn,pnn,temp;

    while(start != classGraph.end()){
        pn = start->first;
        pnn = start->first;
        
        while(1){
            /* already reached end of graph */
            if(pn == Object || pnn == Object)
                break;
            temp = classGraph.find(pn)->second->class_getParent();
            /* check if class exists */
            if(classGraph.find(temp) == classGraph.end()){
                semant_error(start->second) << "Class "<<pn<<" inherits from an undefined class "<<temp<<".\n";
                return;                
            }
            pn = temp;
            if(pn == Object)
                break;

            temp = classGraph.find(pnn)->second->class_getParent();
            /* check if class exists */
            if(classGraph.find(temp) == classGraph.end()){
                semant_error(start->second) << "Class "<<pnn<<" inherits from an undefined class "<<temp<<".\n";
                return;                
            }
            pnn = temp;
            if(pnn == Object)
                break;
            temp = classGraph.find(pnn)->second->class_getParent();
            /* check if class exists */
            if(classGraph.find(temp) == classGraph.end()){
                semant_error(start->second) << "Class "<<pnn<<" inherits from an undefined class "<<temp<<".\n";
                return;                
            }
            pnn = temp;

            /* cycle found when both equal at any instance */
            if(pn == pnn){
                semant_error(start->second) << "Class "<<start->first<<", or an ancestor of "<<start->first<<", is involved in an inheritance cycle.\n";
                return;
            }
        }
        ++start;
    }
}

void ClassTable::install_basic_classes() {

    // The tree package uses these globals to annotate the classes built below.
   // curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");
    
    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.
    
    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    Class_ Object_class =
	class_(Object, 
	       No_class,
	       append_Features(
			       append_Features(
					       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
					       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
			       single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	       filename);

    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ IO_class = 
	class_(IO, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       single_Features(method(out_string, single_Formals(formal(arg, Str)),
										      SELF_TYPE, no_expr())),
							       single_Features(method(out_int, single_Formals(formal(arg, Int)),
										      SELF_TYPE, no_expr()))),
					       single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
			       single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
	       filename);  

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
	class_(Int, 
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       filename);

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);

    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //       
    Class_ Str_class =
	class_(Str, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       append_Features(
									       single_Features(attr(val, Int, no_expr())),
									       single_Features(attr(str_field, prim_slot, no_expr()))),
							       single_Features(method(length, nil_Formals(), Int, no_expr()))),
					       single_Features(method(concat, 
								      single_Formals(formal(arg, Str)),
								      Str, 
								      no_expr()))),
			       single_Features(method(substr, 
						      append_Formals(single_Formals(formal(arg, Int)), 
								     single_Formals(formal(arg2, Int))),
						      Str, 
						      no_expr()))),
	       filename);

    classGraph.insert(std::make_pair(Object_class->class_getName(),Object_class));
    classGraph.insert(std::make_pair(IO_class->class_getName(),IO_class));
    classGraph.insert(std::make_pair(Int_class->class_getName(),Int_class));
    classGraph.insert(std::make_pair(Bool_class->class_getName(),Bool_class));
    classGraph.insert(std::make_pair(Str_class->class_getName(),Str_class));
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()                
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)  
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

ostream& ClassTable::semant_error(Class_ c)
{                                                             
    return semant_error(c->get_filename(),c);
}    

ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error()                  
{                                                 
    semant_errors++;                            
    return error_stream;
} 

bool checkClassInheritance(Symbol parent, Symbol target){
    if(parent == target)
        return true;
    while(target != No_class){
        target = classGraph.find(target)->second->class_getParent();
        if(parent == target)
            return true;
    }
    return false;
}

Symbol leastAncestorCheck(Symbol then_cond, Symbol else_cond){
    Symbol else_copy = else_cond, then_copy = then_cond;

    while(then_copy != No_class){
        while(else_copy != No_class){
            if(else_copy == then_copy){
                return then_copy;
            }
            else_copy = classGraph.find(else_copy)->second->class_getParent();
        }
        else_copy = else_cond;
        then_copy = classGraph.find(then_copy)->second->class_getParent();
    }
    return NULL;   
}

Feature getMethods(Class_ cur_class , Symbol method_name){
    Feature feature = NULL;
    Symbol feature_name;
    Features features = cur_class->class_getFeatures();
    for(int i=features->first();features->more(i);i=features->next(i)){
        feature = features->nth(i);
        feature_name = feature->feature_getName();
        if(feature_name == method_name)
            return feature;
    }
    Symbol parent = cur_class->class_getParent();
    if(classGraph.find(parent) == classGraph.end())
        return NULL;
    cur_class = classGraph.find(parent)->second;
    return getMethods(cur_class,method_name);
}

void build_hierarchy(Class_ base){
    Symbol parent = base->class_getParent();
    /* reach top of the hierarchy */
    if(parent != No_class)
        build_hierarchy(classGraph.find(parent)->second);

    /* enter scopes for each class (methods and attributes separate) */
    methodTab->enterscope();
    attrTab->enterscope();
    Features featureList = base->class_getFeatures();
    /* evaluate each feature of the class */
    for(int i = featureList->first(); featureList->more(i); i = featureList->next(i))
        featureList->nth(i)->toSymTab(base);
}

/* member methods to add to the respective symbol table */
void method_class::toSymTab(Class_ cur){
    /* check if method in current scope */
    if(methodTab->probe(name) != NULL){
        classtable->semant_error(cur) << "Method "<<name<<" is multiply defined.\n";
    }
    /* check in scope of any ancestors */
    else if(methodTab->lookup(name) != NULL){
        Symbol lookForName = *(methodTab->lookup(name));
        /* get class features to verify overriding of method */
        Features featureList = classGraph.find(lookForName)->second->class_getFeatures();

        Formals inheritedFormals;
        Symbol inheritedReturnType;
        for(int i=featureList->first();featureList->more(i); i = featureList->next(i)){
            /* check the feature which matches */
            if(featureList->nth(i)->feature_getName() == name){
                inheritedFormals = featureList->nth(i)->feature_getFormals();
                inheritedReturnType = featureList->nth(i)->feature_getType();
                break;
            }
        }

        int curFormalLen = formals->len();
        int inheritedFormalLen = inheritedFormals->len();
        /* check if the overriding method in child class has same number of formals */
        if(curFormalLen != inheritedFormalLen){
            classtable->semant_error(cur) << "Incompatible number of formal parameters in redefined method "<<name<<".\n";
            return;
        }
        else{
            bool errorFlag = false;
            /* verify each formal type matches in order */
            for(int i=inheritedFormals->first(); inheritedFormals->more(i); i = inheritedFormals->next(i)){
                Symbol curType = formals->nth(i)->formal_getType();
                Symbol inheritedType = inheritedFormals->nth(i)->formal_getType();
                if(curType != inheritedType){
                    classtable->semant_error(cur)<<"In redefined method "<<name<<", parameter type "<<curType<<" is different from original type "<<inheritedType<<".\n";
                    errorFlag = true;
                }
            }
            /* check if return type is matching */
            if(return_type != inheritedReturnType){
                classtable->semant_error(cur)<<"In redefined method "<<name<<" return type "<<return_type<<" is different from original return type "<<inheritedReturnType<<".\n";
                errorFlag = true;
            }
            
            /* no collisions, add to method symbol table */
            if(!errorFlag)
                methodTab->addid(name, new Symbol(cur->class_getName()));
        }
    }
    /* no collisions, add to method symbol table */
    else{
        methodTab->addid(name, new Symbol(cur->class_getName()));
    }
}

/* member methods to add to the respective attribute table */
void attr_class::toSymTab(Class_ cur){
    /* name of attribute cannot be self */
    if(name == self){
        classtable->semant_error(cur) << "'self' cannot be the name of an attribute.\n";
    }
    /* attribute cannot be redefined in same scope */
    else if(attrTab->probe(name) != NULL){
        classtable->semant_error(cur) << "Attribute "<<name<<" is multiply defined in class.\n";
    }
    /* inherited attribute cannot be overridden */
    else if(attrTab->lookup(name) != NULL){
        classtable->semant_error(cur) << "Attribute "<<name<<" is an attribute of an inherited class.\n";
    }
    /* if self type then type of class */
    else if(type_decl == SELF_TYPE){
        attrTab->addid(name, new Symbol(cur->class_getName()));
    }
    /* no conflicts */
    else{
        attrTab->addid(name, new Symbol(type_decl));
    }
}
/********************************************************/

/* Member methods to validate feature types, methods and attributes */
void method_class::validate(Class_ cur){
    /* main method should be valid in class Main */
    if(cur->class_getName() == Main && name == main_meth && formals->len() != 0){
        classtable->semant_error(cur) << "'main' method in class Main should have no arguments.\n";
        return;
    }

    /* verify whether all formals are legal */
    for(int i=formals->first(); formals->more(i); i=formals->next(i)) {
        Formal curForm = formals->nth(i);
        /* name cannot be self */
        if(curForm->formal_getName() == self){
            classtable->semant_error(cur) << "'self' cannot be formal name\n";
        }
        /* type of formals cannot be SELF_TYPE */
        else if(curForm->formal_getType() == SELF_TYPE){
            classtable->semant_error(cur) << "Formal parameter cannot be SELF_TYPE.\n";
        }
        else if(attrTab->probe(curForm->formal_getName()) != NULL){
            classtable->semant_error(cur) << "Formal "<<curForm->formal_getName()<<" is multiply defined.\n";
        }
        /* no conflicts */
        else{
            attrTab->addid(curForm->formal_getName(), new Symbol(curForm->formal_getType()));
        }
    }

    /* get type of the expression */
    Symbol expreval = expr->validate(cur->class_getName());
    Symbol current = return_type;

    if(return_type == SELF_TYPE && expreval!=return_type){
        classtable->semant_error(cur)<<"Inferred return type "<<expreval<<" of method "<<name<<" does not conform to declared return type "<<return_type<<".\n";     
    }

    if(return_type == SELF_TYPE){
        current = cur->class_getName();
    }
    if(expreval == SELF_TYPE){
        expreval = cur->class_getName();
    }

    /* return type should be same or inherited class of the formal return type */
    if(!checkClassInheritance(current, expreval)){
        classtable->semant_error(cur)<<"Inferred return type "<<expreval<<" of method "<<name<<" does not conform to declared return type "<<return_type<<".\n";
    }
}

void attr_class::validate(Class_ cur){
    /* validate the attribute initialization expression */
    Symbol expreval = init->validate(cur->class_getName());
    Symbol searchType = type_decl;
    if(type_decl == SELF_TYPE)
        searchType = cur->class_getName();

    /* attribute type must be a valid existing class */
    if(classGraph.find(searchType) == classGraph.end()){
        classtable->semant_error(cur)<<"Class "<<searchType<<" of attribute "<<name<<" is undefined.\n";
    }

    if(expreval == SELF_TYPE){
        expreval = cur->class_getName();
    }

    if(expreval != No_type && !checkClassInheritance(searchType, expreval)){
        classtable->semant_error(cur)<<"Inferred type "<<expreval<<" of initialization of attribute "<<name<<" does not conform to declared type "<<type_decl<<".\n";
        return;       
    }
}

Symbol assign_class::validate(Symbol sym){
    Symbol* leftFind = attrTab->lookup(name);
    /* check if the variable assigning exists in scope */
    if(leftFind == NULL){
        classtable->semant_error(classGraph.find(sym)->second) << "Assignment to undeclared variable "<<name<<".\n";
        type = Object;
        return Object;
    }
    else{
        /* check validity of assigning expression */
        Symbol rightExpr = expr->validate(sym);

        /* the assignment type should be a subclass of the assignee type */
        if(!checkClassInheritance(*leftFind, rightExpr)){
            classtable->semant_error(classGraph.find(sym)->second) << "Type "<<rightExpr<<" of assigned expression does not conform to declared type "<<*leftFind<<" of identifier "<<name<<".\n";
            type = Object;   
        }else{
            type = rightExpr;
        }
        return type;
    }
}

Symbol static_dispatch_class::validate(Symbol sym){
    classMAP::iterator it = classGraph.find(type_name);
    Feature feature;
    /* find if class exists */
    if(it == classGraph.end()){
        classtable->semant_error(classGraph.find(sym)->second)<<"Static dispatch to undefined class "<<type_name<<"\n";
        type = Object;
        return type;
    }else{
        /* get the method from the classes hierarchy */
        feature = getMethods(it->second, name);
        /* throw error if method not found anywhere in hierarchy */
        if(feature == NULL){
            classtable->semant_error(classGraph.find(sym)->second)<<"Static dispatch to undefined method "<<name<<".\n";
            type = Object;
            return type;            
        }
    }
    
    int num_actuals = actual->len();
    int num_formals = feature->feature_getFormals()->len();
    /* verify correct number of arguments */
    if(num_actuals!=num_formals){
        classtable->semant_error(classGraph.find(sym)->second)<<"Method "<<name<<" invoked with wrong number of arguments.\n";
        type = Object;
        return type;
    }

    Formals formals = feature->feature_getFormals();
    /* verify validity of each of the arguments passed */
    for(int i=formals->first();formals->more(i);i=formals->next(i)){
        Symbol actual_type = actual->nth(i)->validate(sym);
        Symbol formal_type = formals->nth(i)->formal_getType();
        if(!checkClassInheritance(formal_type, actual_type)){
            classtable->semant_error(classGraph.find(sym)->second)<<"In call of method "<<name<<", type "<<actual_type<<" of parameter "<<formals->nth(i)->formal_getName()<<" does not conform to declared type "<<formal_type<<".\n";
            type = Object;
            return type;
        }
    }

    Symbol expreval = expr->validate(sym);
    if(expreval == SELF_TYPE)
        expreval = classGraph.find(sym)->second->class_getName();

    /* check whether the expression type is an inherited type of the class name */
    if(!checkClassInheritance(type_name, expreval)){
        classtable->semant_error(classGraph.find(sym)->second)<<"Expression type "<<expreval<<" does not conform to declared static dispatch type "<<type_name<<".\n";
        type = Object;
        return type;
    }

    /* validate the feature expression */
    type = feature->feature_getExpr()->validate(sym);
    if(type==SELF_TYPE)
        type = expreval;
    return type;
}

Symbol dispatch_class::validate(Symbol sym){
    /* get type of the expression */
    Symbol expreval = expr->validate(sym);
    Feature feature;
    if(expreval == SELF_TYPE)
        feature = getMethods(classGraph.find(sym)->second, name);

    else{
        classMAP::iterator it = classGraph.find(expreval);
        /* check if expression type exists */
        if(it == classGraph.end()){
            classtable->semant_error(classGraph.find(sym)->second)<<"Return type "<<expreval<<" is undefined.\n";
            type = Object;
            return type;
        }
        else
            feature = getMethods(it->second, name);
    }
    if(feature == NULL){
        classtable->semant_error(classGraph.find(sym)->second)<<"Dispath to undefined method "<<name<<".\n"; 
        type = Object;
        return type;       
    }
    
    int num_actuals = actual->len();
    int num_formals = feature->feature_getFormals()->len();
    /* verify if calling function parameters list match */
    if(num_actuals!=num_formals)
        classtable->semant_error(classGraph.find(sym)->second)<<"Method "<<name<<" called with wrong number of arguments.\n";

    Formals def_formals = feature->feature_getFormals();
    /* verify each parameter for proper types */
    for(int i=actual->first();actual->more(i);i=actual->next(i)){
        Symbol actual_type = actual->nth(i)->validate(sym);
        Symbol formal_type = def_formals->nth(i)->formal_getType();
        if(actual_type == SELF_TYPE)
            actual_type = classGraph.find(sym)->second->class_getName();

        /* check if return type is a sub class of the declared type */
        if(!checkClassInheritance(formal_type, actual_type)){
            classtable->semant_error(classGraph.find(sym)->second)<<"In call of method "<<name<<", type "<<actual_type<<" of parameter a does not conform to declared type "<<formal_type<<".\n";
            type = Object;
            return type;
        }
    }

    /* return feature type */
    type = feature->feature_getType();
    if(type == SELF_TYPE)
        type = expreval;
    return type;
}

Symbol cond_class::validate(Symbol sym){
    Symbol predRes = pred->validate(sym);
    /* check predicate type, should be Bool */
    if(predRes != Bool){
        classtable->semant_error(classGraph.find(sym)->second) << "Predicate of 'if' does not have type Bool.\n";
        type = Object;
        return type;    
    }

    /* get the action types of the then and else conditions */
    Symbol thenRes = then_exp->validate(sym);
    if(thenRes == SELF_TYPE)
        thenRes = sym;
    Symbol elseRes = else_exp->validate(sym);
    if(elseRes == SELF_TYPE)
        elseRes = sym;

    /* return type of if statement is the closest common ancestor of both blocks */
    type = leastAncestorCheck(thenRes, elseRes);
    return type;
}

Symbol loop_class::validate(Symbol sym){
    Symbol predRes = pred->validate(sym);
    /* predicate must have type Bool */
    if(predRes != Bool){
        classtable->semant_error(classGraph.find(sym)->second) << "Loop condition does not have type Bool.\n";        
    }
    body->validate(sym);

    /* while loop return Object */
    type = Object;
    return type;
}  

Symbol typcase_class::validate(Symbol sym){
    /* validate the expression */
    expr->validate(sym);

    std::set<Symbol> used;          /* check whether branch has been used or not */
    std::pair<std::set<Symbol>::iterator, bool> it;

    /* initialize 'type' to NULL */
    type = NULL;
    for(int i=cases->first(); cases->more(i); i = cases->next(i)){
        Case c = cases->nth(i);
        Symbol branchtype = c->case_getType();
        /* a legal case type must be checked for */
        if(classGraph.find(branchtype) == classGraph.end()){
            classtable->semant_error(classGraph.find(sym)->second) << "Class "<<branchtype<<" of case branch is undefined.\n";
            type = Object;
            return type;
        }

        it = used.insert(branchtype);
        /*  case type can be used only once in a case statement */
        if(!(it.second)){
            classtable->semant_error(classGraph.find(sym)->second) << "Duplicate branch "<<branchtype<<" in case statement.\n";
            type = Object;
            return type;             
        }
        
        /* enter scope of case statement expression */
        attrTab->enterscope();
        attrTab->addid(c->case_getName(), new Symbol(branchtype));
        
        /* evaluate case expression */
        Symbol expreval = c->case_getExpr()->validate(sym);
        /* case expression type must be inherited of type of the branch */
        if(!checkClassInheritance(branchtype, expreval)){
            classtable->semant_error(classGraph.find(sym)->second) << "Inferred return type "<<expreval<<" of branch "<<c->case_getName()<<" does not conform to declared return type "<<branchtype<<".\n";
            type = Object;
            return type;              
        }
        
        /* return type of case is least ancestor of all case types */
        if(type != NULL){
            type = leastAncestorCheck(expreval, type);
        }else{
            type = expreval;
        }
        attrTab->exitscope();
    }

    return type;
}

Symbol block_class::validate(Symbol sym){
    Symbol lastret;
    /* verify all statements in block */
    for(int stmt = body->first(); body->more(stmt); stmt = body->next(stmt)){
        lastret = body->nth(stmt)->validate(sym);
    }
    /* return type of block is type of last statement */
    type = lastret;
    return type;
}

Symbol let_class::validate(Symbol sym){
    /* let cannot have self identifier */
    if(identifier == self){
        classtable->semant_error(classGraph.find(sym)->second) << "'self' cannot be bound in a 'let' expression.\n";
        type = Object;
        return type;
    }

    /* evaluate initialization expression */
    Symbol initexpreval = init->validate(sym);
    
    /* enter scope of let statement */
    attrTab->enterscope();
    attrTab->addid(identifier, new Symbol(type_decl));

    /* verify if the type of initializer is an inherited type or not */
    if(initexpreval != No_type && !checkClassInheritance(type_decl, initexpreval)){
        classtable->semant_error(classGraph.find(sym)->second) << "type mismatch in let.\n";
        type = Object;
        return type;
    }

    /* type of let expression is the type of the body */
    type = body->validate(sym);
    attrTab->exitscope();
    return type;
}

Symbol plus_class::validate(Symbol sym){
    Symbol left = e1->validate(sym);
    Symbol right = e2->validate(sym);
    /* not type other than Int */
    if(left != Int || right != Int){
        classtable->semant_error(classGraph.find(sym)->second) << "non-Int arguments: "<<left<<" + "<<right<<"\n";
        type = Object;
    }else{
        type = Int;
    }
    return type;
}

Symbol sub_class::validate(Symbol sym){
    Symbol left = e1->validate(sym);
    Symbol right = e2->validate(sym);
    /* not type other than Int */
    if(left != Int || right != Int){
        classtable->semant_error(classGraph.find(sym)->second) << "non-Int arguments: "<<left<<" - "<<right<<"\n";
        type = Object;
    }else{
        type = Int;
    }
    return type;   
}

Symbol mul_class::validate(Symbol sym){
    Symbol left = e1->validate(sym);
    Symbol right = e2->validate(sym);
    /* not type other than Int */
    if(left != Int || right != Int){
        classtable->semant_error(classGraph.find(sym)->second) << "non-Int arguments: "<<left<<" * "<<right<<"\n";
        type = Object;
    }else{
        type = Int;
    }
    return type;  
}

Symbol divide_class::validate(Symbol sym){
    Symbol left = e1->validate(sym);
    Symbol right = e2->validate(sym);
    /* not type other than Int */
    if(left != Int || right != Int){
        classtable->semant_error(classGraph.find(sym)->second) << "non-Int arguments: "<<left<<" / "<<right<<"\n";
        type = Object;
    }else{
        type = Int;
    }
    return type;  
}

Symbol neg_class::validate(Symbol sym){
    Symbol right = e1->validate(sym);
    /* not type other than Int */
    if(right != Int){
        classtable->semant_error(classGraph.find(sym)->second) << "Argument of '~' has type "<<right<<" instead of Int.\n";
        type = Object;
    }else{
        type = Int;
    }
    return type;     
}

Symbol lt_class::validate(Symbol sym){
    Symbol left = e1->validate(sym);
    Symbol right = e2->validate(sym);
    /* not type other than Int */
    if(left != Int || right != Int){
        classtable->semant_error(classGraph.find(sym)->second) << "non-Int arguments: "<<left<<" < "<<right<<"\n";
        type = Object;
    }else{
        type = Bool;
    }
    return type;
}

Symbol eq_class::validate(Symbol sym){
    Symbol left = e1->validate(sym);
    Symbol right = e2->validate(sym);
    /* not type other than Int */
    if(((left==Int || right==Int) || (left==Bool || right==Bool) || (left==Str || right==Str)) && left != right){
        classtable->semant_error(classGraph.find(sym)->second) << "Illegal comparison\n";
        type = Object;
    }else{
        type = Bool;
    }
    return type;    
}

Symbol leq_class::validate(Symbol sym){
    Symbol left = e1->validate(sym);
    Symbol right = e2->validate(sym);
    /* not type other than Int */
    if(left != Int || right != Int){
        classtable->semant_error(classGraph.find(sym)->second) << "non-Int arguments: "<<left<<" <= "<<right<<"\n";
        type = Object;
    }else{
        type = Bool;
    }
    return type;   
}

Symbol comp_class::validate(Symbol sym){
    Symbol right = e1->validate(sym);
    /* not type other than Int */
    if(right != Bool){
        classtable->semant_error(classGraph.find(sym)->second) << "Argument of 'not' has type "<<right<<" instead of Bool.\n";
        type = Object;
    }else{
        type = Bool;
    }
    return type;
}

Symbol int_const_class::validate(Symbol sym){
    type = Int;
    return type;
}

Symbol bool_const_class::validate(Symbol sym){
    type = Bool;
    return type;
}

Symbol string_const_class::validate(Symbol sym){
    type = Str;
    return type;
}

Symbol new__class::validate(Symbol sym){
    if(type_name == SELF_TYPE){
        type = SELF_TYPE;
    }
    /* check if class exists */
    else{
        if(classGraph.find(type_name) == classGraph.end()){
            classtable->semant_error(classGraph.find(sym)->second) << "'new' used with undefined class "<<type_name<<".\n";
            type = Object;
        }else{
            type = type_name;
        }
    }
    return type;
}

Symbol isvoid_class::validate(Symbol sym){
    e1->validate(sym);
    type = Bool;
    return type;
}

Symbol no_expr_class::validate(Symbol sym){
    type = No_type;
    return type;
}

Symbol object_class::validate(Symbol sym){
    if(name == self){
        type = SELF_TYPE;
        return type;
    }
    Symbol* res = attrTab->lookup(name);
    if(res == NULL){
        classtable->semant_error(classGraph.find(sym)->second) << "Object "<<name<<" not found.\n";
        type = Object;      
    }else{
        type = *res;
    }
    return type;
}
/********************************************************************/

/*   This is the entry point to the semantic checker.

     Your checker should do the following two things:

     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')

     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */
void program_class::semant()
{
    initialize_constants();

    /* ClassTable constructor may do some semantic analysis */
    classtable = new ClassTable(classes);

    /* some semantic analysis code may go here */
    if (classtable->errors()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }

    /* check semantic validity for every class */
    for(int i=classes->first(); classes->more(i); i = classes->next(i)){
        methodTab = new symTab();
        attrTab = new symTab();
        Class_ cur = classes->nth(i);
        Features featureList = cur->class_getFeatures();
        /* populate full hierarchy of classes for current (with class checking) */
        build_hierarchy(cur);

        /* check all features validity */
        for(int i = featureList->first(); featureList->more(i); i = featureList->next(i)){
            methodTab->enterscope();
            attrTab->enterscope();
            /* check features validity here */
            featureList->nth(i)->validate(cur);
            attrTab->exitscope();
            methodTab->exitscope();
        }
    }

    if (classtable->errors()) {
    	cerr << "Compilation halted due to static semantic errors." << endl;
    	exit(1);
    }
}
