#include <assert.h>
#include <map>


enum Type {
	VALUE_NONE,
	VALUE_STRING,
	VALUE_INT,
	VALUE_MACRO,
	VALUE_FUNCTION
};

typedef int MInt;

class MString : public std::string {
public:
	MString() : std::string() {}
	MString(std::string s) : std::string(s) {}
	MString(const char* s) : std::string(s) {}
	operator const char* () { return c_str(); }
};

class MacroDef;
class Value;
struct FunctionData;

typedef struct {
	HWND hWnd;
} Context;

typedef Value (*MacroFunction)(FunctionData*);


#define MAX_PARAMS 6
#include <map>

struct WindowVar {
	short type;
	std::map<HWND, Value> val;
} WindowVar;

class Value {
public:
	Type t;
	
	union {
		MInt i;
		MacroDef* md;
		MacroFunction mf;
		MString* str; // XXX	
	};

	Value() {
		t = VALUE_NONE;
	}

	virtual ~Value() {
		if (t == VALUE_STRING) 
			delete str;
	}

	Value(const Value& v) {
		if (v.t == VALUE_STRING) {
			t = VALUE_STRING;
			str = new MString(v.str->c_str());
		}
		else {
			memcpy(this, &v, sizeof(Value));
		}
	}

	Value(MacroDef* def) {
		t = VALUE_MACRO;
		md = def;
	}

	Value(MacroFunction func) {
		t = VALUE_FUNCTION;
		mf = func;
	}

	Value(const char*c) {
		str = new MString();
		t = VALUE_STRING;
		str->assign(c);
	}
	
	Value(const std::string& s) {
		str = new MString();
		t = VALUE_STRING;
		str->assign(s);
	}
	
/*	Value(bool b) {
		t = VALUE_STRING;
		b ? str.assign("1") : str.assign("0");
	}*/

	Value(const int n) {
		t = VALUE_INT;
		i = n;
		/*char buf[34];
		itoa(i, buf, 10);
		str.assign(buf);*/
	}

	MInt intval() {
		switch (t) {
			case VALUE_INT: return i;
			case VALUE_STRING: return str->compare("true")==0?1:atoi(str->c_str()); 
			default : return 0;
		}
	}
/*
	std::string strval() {
		switch (t) {
			case VALUE_INT: char buf[34];_itoa(i, buf, 10); return buf;
			case VALUE_STRING: return str;
			default : return "";
		}
	}
*/
	MString strval() {
		switch (t) {
			case VALUE_INT:{ char buf[34];_itoa(i, buf, 10);  return buf;}
			case VALUE_STRING: return *str;
			default : return "";
		}
	}

	MInt boolval() {
		switch (t) {
			case VALUE_INT: return i;
			case VALUE_STRING:
				return str->compare("false") && str->compare("0") && str->compare("");
			default: return 0;
		}
	}

	bool isint() {return t == VALUE_INT;}
	bool isstring() {return t == VALUE_STRING;}
	bool ismacro() {return t == VALUE_MACRO;}
	bool isfunction() {return t == VALUE_FUNCTION;}
	bool isvalid() {return t!= VALUE_NONE;}
	
	MInt operator +(Value& right ){ return intval() + right.intval(); }
	MInt operator -(Value& right ){ return intval() - right.intval(); }
	MInt operator *(Value& right ){ return intval() * right.intval(); }
	MInt operator /(Value& right ){ MInt d = right.intval(); return d ? intval() / d : 0; }
	MInt operator %(Value& right ){ MInt d = right.intval(); return d ? intval() % d : 0; }
	MInt operator &&(Value& right ){ return boolval() && right.boolval(); }
	MInt operator ||(Value& right ){ return boolval() || right.boolval(); }
	MInt operator -( ){ return -intval(); }
	MInt operator ==(Value& right ){
		switch (t) {
			case VALUE_INT: return intval() == right.intval();
			case VALUE_STRING: return *str == right.strval();
			default : assert(true); return false;
		}
	}
	MInt operator !=(Value& right ){
		switch (t) {
			case VALUE_INT: return intval() != right.intval();
			case VALUE_STRING: return *str != right.strval();
			default : assert(true); return false;
		}
	}
	MInt operator <=(Value& right ){
		switch (t) {
			case VALUE_INT: return intval() <= right.intval();
			case VALUE_STRING: return *str <= right.strval();
			default : assert(true); return false;
		}
	}
	MInt operator >=(Value& right ){
		switch (t) {
			case VALUE_INT: return intval() >= right.intval();
			case VALUE_STRING: return *str >= right.strval();
			default : assert(true); return false;
		}
	}
	MInt operator <(Value& right ){
		switch (t) {
			case VALUE_INT: return intval() < right.intval();
			case VALUE_STRING: return *str < right.strval();
			default : assert(true); return false;
		}
	}
	MInt operator >(Value& right ){
		switch (t) {
			case VALUE_INT: return intval() > right.intval();
			case VALUE_STRING: return *str > right.strval();
			default : assert(false); return false;
		}
	}

	Value operator =(Value& right ){
		if (right.t == VALUE_STRING) {
			if (t == VALUE_STRING)
				*str = *right.str;
			else {
				t = VALUE_STRING;
				str = new MString(right.str->c_str());
			}
		}
		else {
			if (t == VALUE_STRING) delete str;
			memcpy(this, &right, sizeof(Value));
		}
		return *this;
	}/*
		switch (t) {
			case VALUE_INT: i = right.intval(); return i;
			case VALUE_STRING: str = right.strval(); return str;
			default : 
				 return *this;
		}
	}*/
	
	Value operator =(MString& right ) {
		if (t!=VALUE_STRING) {
			t = VALUE_STRING;
			str = new MString(right);
		}
		else
			*str = right;
		return *this;
	}

	Value operator =(MInt right ) {
		if (t == VALUE_STRING) delete str;
		t = VALUE_INT;
		i = right;
		return i;
	}

	MString concat(Value& right ) { 
		return strval() + right.strval();
	}
	
	MInt operator !(){ return !boolval(); }


//	inline std::string* operator-> () {return &str;}

};

class Statement;

typedef struct FunctionData
{
	Context c;
	unsigned short nparam;
	Value* params;
	Statement* stat;

	Value getarg(unsigned short i) {return i>0&&i<=nparam ? *(params+i-1) : Value();} 
	MString getstr(unsigned short i) {return i>0&&i<=nparam ? (params+i-1)->strval() : "";} 
	int getint(unsigned short i) {return i>0&&i<=nparam ? (params+i-1)->intval() : 0;} 
	int getbool(unsigned short i) {return i>0&&i<=nparam ? (params+i-1)->boolval() : 0;} 


} FunctionData;

typedef Value ValueFunc;
typedef Value ValueMacro;
typedef Value ValueStr;
typedef Value ValueInt;
	
typedef std::map<std::string, Value> TDS;

typedef enum {
	NODE_STATLIST,
	NODE_STAT,
	NODE_EXPR,
	NODE_MACRO,
	NODE_VALUE
} NodeType;


typedef enum {
	STAT_WHILE,
	STAT_EXPR,
	STAT_IF,

} StatType;

typedef enum {
	EXPR_NONE = 0,
	EXPR_ADD,
	EXPR_SUB,
	EXPR_MUL,
	EXPR_DIV,
	EXPR_MOD,
	EXPR_GT,
	EXPR_LT,
	EXPR_GE,
	EXPR_LE,
	EXPR_NE,
	EXPR_EQ,
	EXPR_AND,
	EXPR_OR,
	EXPR_CONCAT,
	EXPR_COND,
	EXPR_NOT,
	EXPR_MINUS,
	EXPR_CALL,
	EXPR_ASSIGN,
	EXPR_VALUE
} ExprType;

#define ISEXPR(node) (node->t == NODE_EXPR)
#define ISSTAT(node) (node->t == NODE_STAT)
#define ISLIST(node) (node->t == NODE_STATLIST ||node->t == NODE_MACRO)

/*
typedef struct Statement {
	StatType t;
	MacroNode* A;
	MacroNode* B;
	MacroNode* C;
}

typedef struct Expression {
	ExprType t;
	MacroNode* A;
	MacroNode* B;
	MacroNode* C;
}

typdef struct MacroDef {
	Value name;
	MacroNode* stats;
}

typedef struct MacroNode {
	NodeType t;
	union {
		Statement stat;
		Expression expr;
		MacroDef macro;
		Value value;
	};
	MacroNode* next;
} MacroNode;
*/
class MacroNode {
public:
	NodeType t;
	//MacroNode* child;
	MacroNode* next;
	//MacroNode* last;

	MacroNode() {
		t = NODE_STATLIST;
		next = NULL;
	}

	virtual ~MacroNode() {
		//if (child) delete child;
		if (next) delete next;
	}

	/*void AddNode(MacroNode* node) {
		if (last) {
			last->next = node;
		}
		else
			child = node;
		last = node;
	}*/
};

class StatList : public MacroNode {
	MacroNode* last;
public:
	MacroNode* child;

	StatList() {
		child = last = NULL;
		t = NODE_STATLIST;
	}

	virtual ~StatList() {
		if (child) delete child;
	}

	void AddNode(MacroNode* node) {
		if (last) {
			last->next = node;
		}
		else
			child = node;
		last = node;
	}

};

class Expression : public MacroNode {
public:
	ExprType et;
	
	Expression() : MacroNode() {
		t = NODE_EXPR;
	}

	virtual ~Expression() {}
};

class ExprValue : public Expression {
public:
	Value* v;

	ExprValue() : Expression() {
		v = NULL;
		et = EXPR_VALUE;
	}

	virtual ~ExprValue() {
	}
};

class ExprCall : public Expression {

	Expression* lastParam;

public:
	Value* v;
	Expression* firstParam;

	ExprCall() : Expression() {
		v = NULL;
		et = EXPR_CALL;
		firstParam = lastParam = NULL;
	}

	void AddParam(Expression* p) {
		if (lastParam) lastParam->next = p;
		else firstParam = p;
		lastParam = p;
	}

	virtual ~ExprCall() {
		if (firstParam) delete firstParam;/*
		while (firstParam) {
			MacroNode* tmp = firstParam->next;
			delete firstParam;
			firstParam = (Expression*)tmp;
		}*/
	}
};

class Expr : public Expression {
public:
	MacroNode* A;
	MacroNode* B;
	MacroNode* C;

	virtual ~Expr() {
		if (A) delete A;
		if (B) delete B;
		if (C) delete C;
	}

	Expr(ExprType type) : Expression() {
		et = type;
		A = B = C = NULL;
	}
	
	Expr(ExprType type, Expression* l) : Expression() {
		et = type;
		A = l;
		B = NULL;
		C = NULL;
	}

	Expr(ExprType type, Expression* l, Expression* r) : Expression() {
		et = type;
		A = l;
		B = r;
		C = NULL;
	}
};

class MacroDef : public StatList {
public:
	std::string name;
	Expression* macroInfo;
	Expression* menuString;
	Expression* menuChecked;
	Expression* menuGrayed;

	MacroDef() : StatList() {
		t = NODE_MACRO;
		macroInfo = menuString = menuChecked = menuGrayed = NULL;
	}

	virtual ~MacroDef() {
		if (macroInfo) delete macroInfo;
		if (menuString) delete menuString;
		if (menuChecked) delete menuChecked;
		if (menuGrayed) delete menuGrayed;
	}
};

class Statement : public MacroNode {
public:
	StatType st;
	MacroNode* A;
	MacroNode* B;
	MacroNode* C;


	Statement(StatType type) : MacroNode() {
		st = type;
		t = NODE_STAT;
		A = B = C = next = NULL;
	}

	virtual int getLine() { return -1;}
	virtual const char* getFile() { return "";}
	
	virtual ~Statement() {
		if (A) delete A;
		if (B) delete B;
		if (C) delete C;
	}
};

class DebugStatement : public Statement {
	int mLine;
	MString mFile; 
public:
	DebugStatement(StatType type, const char* file, int line) : Statement(type) {
		mLine = line;
		mFile = file;
	}

	virtual int getLine() { return mLine;}
	virtual const char* getFile() { return mFile.c_str();}
};


/*
class ValueDef : MacroNode {
	Value* v;

	ValueDef() : MacroNode(Value *v) {
		t = NODE_VALUE;
	}
};
*/

/*
typedef struct MacroDef {
	Program code;
	int nbparam;
} MacroDef;
*/