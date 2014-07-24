
/*

macros   --> stmtlist | macrodef
macrodef --> name { stmtlist }
stmtlist --> stmt; {| stmlist}
stmt     --> var = expr | expr
var      --> $name
call     --> &name | name(exprlist)
exprlist --> expr {, exprlist}
expr     --> number | literal | var | call | unop expr |
             expr binop expr | expr ? stmt : stmt;

cond     --> expr ? stmt : stmt;

macros   --> stmtlist
stmtlist --> stmt;{stmtlist}
stmt     --> var = expr | macrodef | 

macros   --> tata {tata}
tata     --> stmtlist | macrodef
stmtlist --> stmt; {stmtlist}
				 */

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <assert.h>
#include <sys/stat.h>

typedef enum {
	OP_MOVE,
	OP_ADD,
	OP_SUB,
	OP_MULT,
	OP_DIV,
	OP_REM,
	OP_CONCAT,
	OP_EQ,
	OP_NEQ,
	OP_GT,
	OP_LT,
	OP_GE,
	OP_LE,
	OP_NOT,
	OP_JMP,
	OP_TEST,
	OP_CALL,
	OP_PUSH,
	OP_POP
} OpCode;


typedef struct Instruction {
	OpCode opcode;
	long A;
	long B;
	long C;
} Instruction;

typedef struct Program {
	Instruction* inst;
	int size;
	int nbinstr;

	Program() {
		inst = new Instruction[5];
		size = 5;
		nbinstr = 0;
	}

	~Program() {
		if (inst) delete inst;
	}

	Instruction* AddInstr() {
		if (!inst) return NULL;
		if (nbinstr == size) {
			Instruction* newset = new Instruction[size+5];
			if (newset) {
				memcpy(newset, inst, sizeof(Instruction)*size);
				size += 5;
				Instruction* oldinst = inst;
				inst = newset;
				delete oldinst;
			}
		}

		return &inst[nbinstr++];
	}
} Program;

static char* reservedwords[] = { "while", "if", "else", "and", "or", "not", "menu", "menuchecked", "menugrayed", "macroinfo" };
#define MAXRESERVED sizeof(reservedwords) / sizeof(char*)

static char* windowvars[] = { "URL", "URLBAR", "SelectedText", "FrameURL", "LinkURL", "ImageURL", "CHARSET", "TextZoom", "TITLE", "WindowNumber", "TabNumber", "CommandLine", "SEARCHURL" };
#define MAXWINDOWVARS sizeof(windowvars) / sizeof(char*)

static const int OpPriority[] = {0, 5, 5, 6, 6, 6, 4, 4, 4, 4, 4, 4, 2, 1, 3, 3};

enum TOKEN {
	TK_WHILE = 0,
	TK_IF,
	TK_ELSE,
	TK_AND,
	TK_OR,
	TK_NOT,
	TK_MENU,
	TK_MENUCHECK,
	TK_MENUGRAY,
	TK_MACROINFO,


	TK_EQ,
	TK_GT,
	TK_LT,
	TK_GE,
	TK_LE,
	TK_ADD,
	TK_SUB,
	TK_MULT,
	TK_DIV,
	TK_REM,
	TK_CONCAT,
	TK_NE,

	TK_ASSIGN,
	TK_NUMBER,
	TK_STRING,
	TK_NAME,
	TK_SEP,

	TK_MACRO,
	TK_VAR,
	TK_WVAR,
	TK_BEGIN,
	TK_END,
	TK_OPEN,
	TK_CLOSE,
	TK_COMMA,


	TK_COND,
	TK_COLON,
	TK_NONE,
	TK_ERR
};


static const char* TokenList[] = {
   		"while",
		"if",
		"else",
		"and",
		"or",
		"not",
		"<menu definition>",
		"<menu checked>",
		"<menu grayed>",
		"<macro info>",
		"=",
		">",
		"<",
		">=",
		"<=",
		"+",
		"-",
		"*",
		"/",
		"%",
		".",
		"!=",
		"=",
		"<number>",
		"<string>",
		"<name>",
		";",
		"&",
		"$",
		"@",
		"{",
		"}",
		"(",
		")",
		",",
		"?",
		":",
		"",
		""
};

class Lexer {

public:
	char* current;
	char* tokenstr;
	TOKEN token;
	TOKEN tkahead;
	TOKEN tkahead2;
	ValueStr data;
	MString errormsg;
	MString file;
	unsigned line;

	

	Lexer() : data(""), current(0), tkahead(TK_NONE), tkahead2(TK_NONE), line(1) {
	}

	~Lexer() {
	}

	void setinput(char* in) {
		current = in;
	}
	
	void setfile(const char* in) {
		file = in;
	}

	void next() {
		++current;
	}

	void incline(){
		++line;
	}

	void skipspace() {
		while (1) {
			while (isspace(*current) || *current == '\r')
				next();
			if (*current != '\n') break;
			incline();
		}
	}

	void skipline() {
		while (*current && *current != '\n') next();
		if (*current) next();
		incline();
	}

	bool isspace(char c) {
		return ((c & 0x80) != 0 || ::isspace(c));
	}

	bool isnum(char c) {
		return (c>='0' && c<='9');
	}

	bool isalpha(char c) {
		return ((c & 0x80) != 0 || ::isalpha(c));
	}

	bool isalphanum(char c) {
		return ((c & 0x80) != 0 || isalnum(c));
	}

	bool readnumber() {
		std::string v;;
		while (isnum(*current)) {
			v += *current;
			next();
		}
		data = atoi(v.c_str());
		return true;
	}

	bool readname() {
		MString v;
		v += *current;
		next();
		while (isalnum(*current) || *current=='_') {
			v += *current;
			next();
		}
		data = v;
		return true;
	}

	bool readstring() {
		MString v;
		next();
		char *begin = current;
		while (*current != '"') {

			switch (*current) {
				case 0: error("Unfinished string."); return false;
				case '\n': error("Unfinished string."); return false;
				case '\\':
					next();
					switch (*current) {
						case 'n': v += "\n";break;
						case 'r': v += "\r";break;
						case 't': v += "\t";break;
						case '\\': v += "\\";break;
						default : v += *current; 
					}
					next(); if (*current == '\r') next();
					break;

				default:
					v += *current;
					next();
			}
		}

		next();
		data = v;
		return true;
	}

	void error(char* msg) {
		DoError(msg, file.c_str(), line);
	}

	TOKEN lookahead2() {
		if (tkahead2 != TK_NONE)
			return tkahead2;
		if (tkahead == TK_NONE)
			lookahead();
		tkahead2 = _nexttoken();
		return tkahead2;
	}

	TOKEN lookahead() {
		if (tkahead != TK_NONE)
			return tkahead;
		tkahead = _nexttoken();
		return tkahead;
	}

	TOKEN nexttoken() {

		if (tkahead2 != TK_NONE) {
         token = tkahead;
			tkahead = tkahead2;
			tkahead2 = TK_NONE;
			return token;
		}

		if (tkahead != TK_NONE) {
			token = tkahead;
			tkahead = TK_NONE;
			return token;
		}

		token = _nexttoken();
		return token;
	}

	TOKEN _nexttoken()
	{
		while (1) {

			tokenstr = current;

			switch (*current)
			{
			case '#':
				skipline();
				continue;

			case '\n':
				incline();
			case '\r':
				next();
				continue;

			case '<':
				next();
				if (*current != '=') return TK_LT;
				next();
				return TK_LE;

			case '>':
				next();
				if (*current != '=') return TK_GT;
				next();
				return TK_GE;

			case '=':
				next();
				if (*current != '=') return TK_ASSIGN;
				next();
				return TK_EQ;

			case '!':
				next();
				if (*current != '=') return TK_NOT;
				next();
				return TK_NE;

			case '.':
				next();
				return TK_CONCAT;

			case '+':
				next();
				return TK_ADD;

			case '-':
				next();
				return TK_SUB;

			case '*':
				next();
				return TK_MULT;

			case '/':
				next();
				return TK_DIV;

			case '"':
				readstring();
				return TK_STRING;

			case '&':
				next();
				return TK_MACRO;
			
			case '@':
				next();
				return TK_WVAR;

			case '$':
				next();
				return TK_VAR;

			case '{':
				next();
				return TK_BEGIN;

			case '}':
				next();
				return TK_END;

			case '(':
				next();
				return TK_OPEN;

			case ')':
				next();
				return TK_CLOSE;

			case ';':
				next();
				return TK_SEP;
			
			case ',':
				next();
				return TK_COMMA;

			case '?':
				next();
				return TK_COND;

			case ':':
				next();
				return TK_COLON;

			case '%':
				next();
				return TK_REM;

			case 0:
				return TK_NONE;

			default :
				if (isspace(*current)) {
					next();
					continue;
				}

				if (isnum(*current)) {
					readnumber();
					return TK_NUMBER;
				}

				if (isalpha(*current) || *current == '_') {
					readname();
					for (int i=0; i<MAXRESERVED; i++)
						if (strcmp(data.strval(), reservedwords[i]) == 0)
							return (TOKEN)i;
					return TK_NAME;
				}

				//char buf[128];
				//sprintf(buf, "Invalid character %c.", *current);
				//error(buf);
				next();
				return TK_ERR;
			}
		}
	}
};


/*

enum BINOP {
	OP_ADD,
	OP_SUB,
	OP_MULT,
	OP_DIV,
	OP_REM,
	OP_CONCAT,
	OP_GT,
	OP_LT,
	OP_GE,
	OP_LE,
	OP_EQ,
	OP_NE,
};

enum UNOP {
	OP_NOT
};
*/

class Parser  {
public:
	MString file;
	Lexer lex;
	Mac*  m;
	char *input;
	MacroDef* currentMd; //XXX
	bool debug;

	Parser() { input = NULL; currentMd = NULL;}
	~Parser() { if (input) delete input;}

	bool init(Mac* mac, const TCHAR* srcFile, bool enableDebug = false) {
		struct _stat st;
		if (_tstat(srcFile, &st) == -1)
			return false;

		FILE* f = _tfopen(srcFile, _T("r"));
		if (!f) return false;

		input = new char[st.st_size+1];
		size_t r = fread(input, sizeof(char), st.st_size, f);
		input[r] = 0;
		lex.setinput(input);
		file = (const char*)CT_to_UTF8(srcFile);
		lex.setfile(file);
		debug = enableDebug;
		fclose(f);		
		m = mac;
		return true;
	}

	void error(const char* msg, TOKEN tk) {
		MString errmsg = msg;
		if (tk != TK_NONE)  {
			errmsg += TokenList[tk];
			errmsg += "\n";
			errmsg.append(lex.tokenstr, 25);
		}
		DoError(errmsg.c_str(), file.c_str(), lex.line);
	}

	void error(const char* msg) {
		error(msg, TK_NONE);
	}

	Statement* newStatement(StatType type)
	{
		if (!debug)
			return new Statement(type);
		return new DebugStatement(type, file.c_str(), lex.line);
	}

	bool isunop(TOKEN tk) {
		return (tk == TK_NOT || tk == TK_SUB);
	}

	bool isbinop(TOKEN tk) {
		return (tk >= TK_EQ && tk <= TK_NE);
	}

	Expression* unop(TOKEN tk, Expression* expr)
	{
		ExprType type;
		switch (tk) {
			case TK_NOT: type = EXPR_NOT; break;
			case TK_SUB: type = EXPR_MINUS; break; 
			default: return NULL;
		}
		return new Expr(type, expr);
	}

	ExprType getbinop(TOKEN tk) {
		ExprType type;
		switch (tk) {
			case TK_EQ: type = EXPR_EQ; break;
			case TK_NE: type = EXPR_NE; break; 
			case TK_GT: type = EXPR_GT; break; 
			case TK_LT: type = EXPR_LT; break; 
			case TK_GE: type = EXPR_GE; break; 
			case TK_LE: type = EXPR_LE; break; 
			case TK_AND: type = EXPR_AND; break;
			case TK_OR: type = EXPR_OR; break;
			case TK_NOT: type = EXPR_NOT; break;
			case TK_ADD: type = EXPR_ADD; break; 
			case TK_SUB: type = EXPR_SUB; break; 
			case TK_DIV: type = EXPR_DIV; break; 
			case TK_MULT: type = EXPR_MUL; break; 
			case TK_REM: type = EXPR_MOD; break; 
			case TK_CONCAT: type = EXPR_CONCAT; break; 
			case TK_COND: type = EXPR_COND; break; 
			default: type = EXPR_NONE;
		}
		return type;
	}

	bool skip(TOKEN tk) {
		if (tk != lex.token) {
			char buf[128];
			sprintf(buf, "%s expected but found %s.", TokenList[tk], TokenList[lex.token]);
			error(buf);
			return false;
		}
		lex.nexttoken();
		return true;
	}

	/*void skipstmt() {
		short cond = 0;

		lex.nexttoken();
		while ( !(lex.token == TK_NONE ||
			     lex.token == TK_SEP ||
				 (lex.token == TK_COLON && !cond)))
		{
			if (lex.token == TK_COND)
				cond++;
			if (lex.token == TK_COLON)
				cond--;
			lex.nexttoken();
		}
	}*/

	Expression* basicexpr() {
		Expression* e;
		switch (lex.token) {
			case TK_NAME:
				if (lex.lookahead() != TK_OPEN) {
					e = (Expression*)new ExprValue();
					((ExprValue*)e)->v = m->AddSymbol(std::string("##") + lex.data.strval(), Value(lex.data));
				}
				else {
					 return callfunc();
				}
				break;

			case TK_VAR: {
				 
				if (lex.lookahead2() == TK_ASSIGN) {
					return assignment();
				}

				for (int i=0; i<MAXWINDOWVARS; i++)
					if (strcmp(lex.data.strval(), windowvars[i]) == 0) {
						ExprCall* ec = new ExprCall();
						ec->v = m->FindSymbol("getwinvar");
						assert(ec->v);
				
						ExprValue* ename = new ExprValue();
						ename->v = m->AddSymbol(std::string("##") + lex.data.strval(), lex.data.strval());
						ec->AddParam(ename);
						
						lex.nexttoken();
						lex.nexttoken();
						return ec;
					}

					e = (Expression*)new ExprValue();
					Value* v = m->FindSymbol(std::string("$") + lex.data.strval());
					if (!v) v = m->AddSymbol(std::string("$") + lex.data.strval(), Value());
					((ExprValue*)e)->v = v; lex.nexttoken();
					break;
			}

			case TK_NUMBER:
				e = (Expression*)new ExprValue();
				((ExprValue*)e)->v = m->AddSymbol(std::string("%%") + lex.data.strval(), lex.data.intval());
				break;

			case TK_STRING:
				e = (Expression*)new ExprValue();
				((ExprValue*)e)->v = m->AddSymbol(std::string("##") + lex.data.strval(), lex.data.strval());
				break;

			case TK_MACRO:
				return macrocall();

			default :
				error("Invalid expression.", lex.token);
				while (lex.token != TK_NONE && lex.token != TK_SEP)
					lex.nexttoken();
				return NULL;
		}
		lex.nexttoken();
		return e;
	}

	

	Expression* firstexpr(int priority) {
		Expression* e1;
		TOKEN tk = lex.token;
		if (isunop(tk)) {
			lex.nexttoken();
			e1 = unop(tk, firstexpr(5));
		}
		else if (lex.token == TK_OPEN) {
			lex.nexttoken();
			e1 = firstexpr(0);
			skip(TK_CLOSE);
		}
		else 	e1 = basicexpr();

		tk = lex.token;
		ExprType op;
		while ( ((op = getbinop(tk)) != EXPR_NONE) && (OpPriority[op] > priority) ) {
		//while (tk == TK_COND || ((op = getbinop(tk)) != EXPR_NONE)) {
		//while (isbinop(tk) || tk == TK_COND || (op=binop() {
			//if (OpPriority[op] < priority) break;
			//if (priority>0) break;
			lex.nexttoken();

			if (tk == TK_COND) {
				Expr* e = new Expr(EXPR_COND);
				e->A = e1;
				e->B = evalexpr();
				skip(TK_COLON);
				e->C = evalexpr();
				e1 = e;
			}
			else
				e1 = new Expr(op, e1, firstexpr(OpPriority[op]));
			tk = lex.token;
		}

		return e1;
	}

	Expression* evalexpr() {
		return firstexpr(0);
	}

	Expression* assignment() {
		
		TOKEN tk = lex.token;
		
		if (lex.nexttoken() != TK_NAME) {
			error("Invalid variable name.");
			return NULL;
		}
		if (lex.nexttoken() != TK_ASSIGN) {
			error(" '=' expected.");
			return NULL;
		}

		MString varname = lex.data.strval();
		lex.nexttoken();
		
		for (int i=0; i<MAXWINDOWVARS; i++)
			if (stricmp(varname.c_str(), windowvars[i]) == 0) {
				ExprCall* e = new ExprCall();
				e->v = m->FindSymbol("setwinvar");
				assert(e->v);
				
				ExprValue* ename = new ExprValue();
				ename->v = m->AddSymbol(std::string("##") + varname, varname);
				e->AddParam(ename);
				e->AddParam(evalexpr());
				return e;
			}

		varname = std::string("$") + varname;

		Value* v = m->FindSymbol(varname);
		if (v && (v->ismacro() || v->isfunction())) {
			error(("Symbol '"+ varname +"' already defined as a macro or function.").c_str());
			return NULL;
		}

		Expr* e = new Expr(EXPR_ASSIGN);
		e->B = evalexpr();
		
		e->A = new ExprValue();

		if (!v) v = m->AddSymbol(varname, Value());
		((ExprValue*)e->A)->v = v;
		
		return e;
	}

	Expression* macrocall() {
		if (lex.nexttoken() != TK_NAME) {
			error("Invalid macro name.");
			return NULL;
		}
		ExprCall* e = new ExprCall();
		e->v = m->FindSymbol(lex.data.strval());
		if (!e->v) e->v = m->AddSymbol(lex.data.strval(), ValueMacro((MacroDef*)NULL));
		lex.nexttoken();
		return (Expression*)e;
	}

	Expression* callfunc() {
		std::string funcname = lex.data.strval();
		std::string exp, args;
		
		skip(TK_NAME);
		if (!skip(TK_OPEN))
			return NULL;

		ExprCall* e = new ExprCall();
		e->v = m->FindSymbol(funcname);
		if (!e->v) error(("Function " + funcname + " is not defined.").c_str());
		
		while (lex.token != TK_NONE && lex.token != TK_CLOSE) {
			e->AddParam(evalexpr());
         if (lex.token != TK_COMMA )
				break;
			lex.nexttoken();
		} 

		skip(TK_CLOSE);
		return (Expression*)e;
	}
/*
	std::string stmt() {
		std::string s;
		TOKEN tk;

		do { 
			char c = *(lex.current);
			*(lex.current) = 0;
			s += lex.tokenstr;
			*(lex.current) = c;
			tk = lex.nexttoken();
		} while ( tk!= TK_SEP && tk!=TK_NONE);

		return s;
	}*/

	MacroDef* macrodef() {
		std::string macroname = lex.data.strval();
		skip(TK_NAME);
		skip(TK_BEGIN);

		MacroDef* md = new MacroDef();
		currentMd = md;
		md->name = macroname;
		
		Value* v = m->FindSymbol(macroname);
		if (v && !v->ismacro()) {
			error(("Symbol '" + macroname + "' already defined.").c_str());
			return NULL;
		}

		m->AddSymbol(macroname, ValueMacro(md));
		stmtlist(static_cast<StatList*>(md));
		currentMd = NULL;
		skip(TK_END);
		return md;
	}

	Statement* whilestat() {
		Statement* s = newStatement(STAT_WHILE);
		skip(TK_WHILE);
		skip(TK_OPEN);
		s->A = evalexpr();
		skip(TK_CLOSE);

		s->B = new StatList();
		bool begin = false;
		if (lex.token == TK_BEGIN)
			stmtblock(static_cast<StatList*>(s->B));
		else 
			static_cast<StatList*>(s->B)->AddNode(evalstmt());
		
		return s;
	}

	Statement* ifstat() {
		Statement* s = newStatement(STAT_IF);
		skip(TK_IF);
		skip(TK_OPEN);
		s->A = evalexpr();
		skip(TK_CLOSE);

		s->B = new StatList();
		bool begin = false;
		if (lex.token == TK_BEGIN)
			stmtblock(static_cast<StatList*>(s->B));
		else 
			static_cast<StatList*>(s->B)->AddNode(evalstmt());

		if (lex.token == TK_ELSE) {
//		if (lex.lookahead() == TK_ELSE) {
//			skip(TK_SEP);
			lex.nexttoken();
			s->C = new StatList();
			if (lex.token == TK_BEGIN)
				stmtblock(static_cast<StatList*>(s->C));
			else
				static_cast<StatList*>(s->C)->AddNode(evalstmt());
		}
		return s;
	}

	void skipstmt() {
		while (lex.token != TK_NONE && lex.token != TK_SEP)
			lex.nexttoken();
	}

	Statement* evalstmt() {

		Statement* stat;

		switch (lex.token) {
		case TK_WHILE:
			return whilestat();
		case TK_IF:
			return ifstat();

			/*case TK_VAR:
			if (lex.lookahead2() == TK_ASSIGN) {
				return assignment();
			}
			else return evalexpr();


		case TK_NAME:
			ahead = lex.lookahead();

			if ( ahead == TK_BEGIN ) {
				macrodef();
				return "";
			}

			return evalexpr();
*/
		case TK_SEP:
			lex.nexttoken();
			return NULL;

		case TK_VAR:
		case TK_WVAR:
		case TK_NAME:
		case TK_MACRO:
		case TK_STRING:
		case TK_NUMBER:
		case TK_OPEN:
		case TK_NOT:
			stat = newStatement(STAT_EXPR);
			stat->A = evalexpr();
			skip(TK_SEP);
			return stat;

		case TK_MACROINFO:
			lex.nexttoken();
			skip(TK_ASSIGN);
			//skip(TK_STRING);
			assert(currentMd);
			if (!currentMd) skipstmt();
			else currentMd->macroInfo = evalexpr();//lex.data.strval();
			//skip(TK_SEP);
			return NULL;

		case TK_MENUCHECK:
			lex.nexttoken();
			skip(TK_ASSIGN);
			assert(currentMd);
			if (!currentMd) skipstmt();
			else currentMd->menuChecked = evalexpr();
			return NULL;

		case TK_MENUGRAY:
			lex.nexttoken();
			skip(TK_ASSIGN);
			assert(currentMd);
			if (!currentMd) skipstmt();
			else currentMd->menuGrayed = evalexpr();
			return NULL;

		case TK_MENU:
			lex.nexttoken();
			skip(TK_ASSIGN);
			assert(currentMd);
			if (!currentMd) skipstmt();
			else currentMd->menuString = evalexpr();
			return NULL;

		default:
			error("Invalid statement.");
			skipstmt();
			skip(TK_SEP);
			return NULL;
		}
	}

	void stmtlist(StatList* node) {

		while (lex.token != TK_NONE) {
			if (lex.token == TK_END) {
				return; // bad
			}

			node->AddNode(evalstmt());
		//	skip(TK_SEP);
		
			if (lex.token == TK_NAME && lex.lookahead() == TK_BEGIN)
				return; // macrodef
		}
	}

	void stmtblock(StatList* node) {
		skip (TK_BEGIN);		
		while (lex.token != TK_NONE && lex.token != TK_END) {
			node->AddNode(evalstmt());
			//skip(TK_SEP);
		}
		skip(TK_END);
	}

	bool parse() {
		if (!input) return false;

		lex.nexttoken();
		do {
			switch (lex.token)
			{
			case TK_NAME: {
				
				TOKEN ahead = lex.lookahead();

				if ( ahead == TK_BEGIN) {
					m->root.AddNode(macrodef());
					break;
				}

				if ( ahead == TK_OPEN ) {
					stmtlist(&m->root);
					break;
				}

				error("{ or ( expected.");
				lex.nexttoken();
				break;
			}

			case TK_END:
				error("} unexpected.");
				lex.nexttoken();
				break;

			default:
				stmtlist(&m->root);
				break;
			}
		} while (lex.token != TK_NONE && lex.token != TK_ERR);

		return lex.errormsg.length()>0;

	}
};
