#include <assert.h>
#include <map>
#include <vector>
#include "strconv.h"

enum Type {
	VALUE_NONE,
	VALUE_STRING,
	VALUE_INT,
	VALUE_MACRO,
	VALUE_FUNCTION,
	VALUE_UFUNCTION
};

typedef int MInt;

class MString : public std::string {
public:
	MString() : std::string() {}
	MString(std::string s) : std::string(s) {}
	MString(const char* s) : std::string(s) {}
	operator const char* () { return c_str(); }
	CUTF8_to_UTF16 utf16() { return CUTF8_to_UTF16(c_str()); }
};

class MacroDef;
class FunctionDef;
class Value;
class MacroFile;
struct FunctionData;

typedef struct {
	HWND hWnd;
	HWND hTab;
	MacroFile* origmf;
	MacroFile* mf;
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
		FunctionDef* uf;
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

	Value(FunctionDef* def) {
		t = VALUE_UFUNCTION;
		uf = def;
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

	MInt intval() const {
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
	MString strval() const {
		switch (t) {
			case VALUE_INT:{ char buf[34];_itoa(i, buf, 10);  return buf;}
			case VALUE_STRING: return *str;
			default : return "";
		}
	}

	MInt boolval() const {
		switch (t) {
			case VALUE_INT: return i;
			case VALUE_STRING:
				return str->compare("false") && str->compare("0") && str->compare("");
			default: return 0;
		}
	}

	bool isint() const { return t == VALUE_INT; }
	bool isstring() const{ return t == VALUE_STRING; }
	bool ismacro() const { return t == VALUE_MACRO; }
	bool isfunction() const { return t == VALUE_FUNCTION; }
	bool isufunction() const { return t == VALUE_UFUNCTION; }
	bool isvalid() const { return t != VALUE_NONE; }
	
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

	Value operator =(const Value& right ){
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
	
	Value operator =(const MString& right ) {
		if (t!=VALUE_STRING) {
			t = VALUE_STRING;
			str = new MString(right);
		}
		else
			*str = right;
		return *this;
	}

	Value operator =(const MInt right) {
		if (t == VALUE_STRING) delete str;
		t = VALUE_INT;
		i = right;
		return i;
	}

	MString concat(const Value& right) { 
		return strval() + right.strval();
	}
	
	MInt operator !(){ return !boolval(); }


//	inline std::string* operator-> () {return &str;}

};

class Statement;

typedef struct FunctionData
{
	Context* pc;
	Context c;
	unsigned short nparam;
	Value* params;
	Statement* stat;

	Value getarg(unsigned short i) {return i>0&&i<=nparam ? *(params+i-1) : Value();} 
	MString getstr(unsigned short i, const char* def = "") {return i>0&&i<=nparam ? (params+i-1)->strval() : def;} 
	int getint(unsigned short i, int def = 0) {return i>0&&i<=nparam ? (params+i-1)->intval() : def;} 
	int getbool(unsigned short i, int def = 0) {return i>0&&i<=nparam ? (params+i-1)->boolval() : def;}
	void setContext(Context* ac) {pc = ac; c = *ac;}
	void setWin(HWND h) {pc->hWnd = h;}

} FunctionData;

typedef Value ValueFunc;
typedef Value ValueMacro;
typedef Value ValueStr;
typedef Value ValueInt;
	
class TDS : public std::map < std::string, Value >
{
public:

	void emptyvar()
	{
		for (TDS::iterator it = begin(); it != end(); it++)
			it->second = Value();
	}

	Value* add(const std::string& name, const Value& v)
	{
		TDS::iterator it;
		it = insert(end(), TDS::value_type(name, v));
		return &it->second;
	}

	Value* find(const std::string& name)
	{
		TDS::iterator it = __super::find(name);
		if (it == end())
			return nullptr;
		return &it->second;
	}

	std::string find(const Value* val)
	{
		for (TDS::iterator it = begin(); it != end(); it++)
			if (val == &it->second)
				return it->first;
		return "";
	}
};

typedef enum {
	NODE_STATLIST,
	NODE_STAT,
	NODE_EXPR,
	NODE_MACRO,
	NODE_FUNCTION,
	NODE_VALUE
} NodeType;


typedef enum {
	STAT_WHILE,
	STAT_EXPR,
	STAT_IF,
	STAT_RETURN,
	STAT_BREAK
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
#define ISLIST(node) (node->t == NODE_STATLIST || node->t == NODE_MACRO || node->t == NODE_FUNCTION)

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
		next = nullptr;
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
		child = last = nullptr;
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

	MacroNode* GetLast() {
		return last;
	}
};

class Expression : public MacroNode {
public:
	ExprType et;
	
	Expression() : MacroNode() {
		t = NODE_EXPR;
		et = EXPR_NONE;
	}

	virtual ~Expression() {}
};

class ExprValue : public Expression {
protected:
	Value* v;
public:
	ExprValue() : Expression() {
		v = nullptr;
		et = EXPR_VALUE;
	}

	virtual ~ExprValue() {
	}

	const Value* Get() {
		return v;
	}

	void Set(Value* av) {
		v = av;
	}

	void Set(const Value& av) {
		assert(v);
		*v = av;
	}
};

class ExprCall : public Expression {

	Expression* lastParam;

public:
	Value* v;
	Expression* firstParam;

	ExprCall() : Expression() {
		v = nullptr;
		et = EXPR_CALL;
		firstParam = lastParam = nullptr;
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
		A = B = C = nullptr;
	}
	
	Expr(ExprType type, Expression* l) : Expression() {
		et = type;
		A = l;
		B = nullptr;
		C = nullptr;
	}

	Expr(ExprType type, Expression* l, Expression* r) : Expression() {
		et = type;
		A = l;
		B = r;
		C = nullptr;
	}
};

class FunctionDef : public StatList {
public:
	std::string name;
	std::vector<Value*> params;
	MacroFile* mf;
	TDS tds;

	FunctionDef(MacroFile* amf) : StatList() {
		t = NODE_FUNCTION;
		mf = amf;
	}

	virtual ~FunctionDef() {}
};

class MacroDef : public StatList {
public:
	std::string name;
	Expression* macroInfo;
	Expression* menuString;
	Expression* menuChecked;
	Expression* menuGrayed;
	Expression* btnChecked;
	MacroFile* mf;

	MacroDef(MacroFile* amf) : StatList() {
		t = NODE_MACRO;
		macroInfo = menuString = menuChecked = menuGrayed = btnChecked = NULL;
		mf = amf;
	}

	virtual ~MacroDef() {
		if (macroInfo) delete macroInfo;
		if (menuString) delete menuString;
		if (menuChecked) delete menuChecked;
		if (menuGrayed) delete menuGrayed;
		if (btnChecked) delete btnChecked;
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
		A = B = C = next = nullptr;
	}

	virtual int getLine() { return -1;}
	virtual MacroFile* getMFile() { return nullptr;}
	virtual const char* getFile() { return "";}
	
	virtual ~Statement() {
		if (A) delete A;
		if (B) delete B;
		if (C) delete C;
	}
};

class Mac {
public:
	TDS tds;
	StatList root;

	Value* AddSymbol(const std::string& name, const Value& v)
	{
		TDS::iterator it;
		it = tds.insert(tds.end(), TDS::value_type(name, v));
		return &it->second;
	}

	Value* FindSymbol(const std::string& name)
	{
		return tds.find(name);
	}

	std::string FindSymbol(const Value* val)
	{
		for (TDS::iterator it = tds.begin(); it != tds.end(); it++)
			if (val == &it->second)
				return it->first;
		return "";
	}
};

class MacroFile {
public:
	std::string name;
	std::string desc;
	std::string file;
	std::wstring wfile;
	bool user;
	bool loaded;
	bool trusted;
	bool denied;
	Mac m;

	MacroFile(wchar_t* afile) {
		wfile = afile;
		file = CUTF16_to_UTF8(afile);

		CharLowerBuff(afile, wcslen(afile));
		wchar_t* ext = wcsrchr(afile, L'.');
		if (ext) *ext = 0;
		wchar_t* pos = wcsrchr(afile, L'\\');
		name = (const char*)CT_to_UTF8(pos ? pos + 1 : afile);
		*ext = L'.';

		user = false;
		loaded = false;
		trusted = false;
		denied = false;
	}
};

class DebugStatement : public Statement {
	int mLine;
	MacroFile* mMf; 
public:
	DebugStatement(StatType type, MacroFile* mf, int line) : Statement(type) {
		mLine = line;
		mMf = mf;
	}

	virtual int getLine() { return mLine;}
	virtual MacroFile* getMFile() { return mMf;}
	virtual const char* getFile() { return mMf->file.c_str();}
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