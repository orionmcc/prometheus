//============================================================================
// Name        : Prometheus.cpp
// Author      :
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <algorithm>
#ifdef _MSC_VER
#include <unordered_map>
#include <io.h>

size_t filelength(FILE* f)
{
	return _filelength(_fileno(f));
}


#else
#include <tr1/unordered_map>

void fopen_s(FILE **f, const char *name, const char *mode) {
    *f = fopen(name, mode);
    /* Can't be sure about 1-to-1 mapping of errno and MS' errno_t */
    return;
}

char* strcpy_s(char* dst, size_t len, const char* src)
{
	size_t cpy = strlen(src) > len - 1 ? len - 1 : strlen(src);
	strncpy(dst, src, len);
	dst[cpy] = 0;
	return dst;
}

#define sprintf_s snprintf

#endif
#include <set>
#include <string>
#include <vector>
using namespace std;

#include "lexer.h"

typedef int int32;
typedef unsigned int uint32;
typedef short int16;
typedef unsigned short uint16;
typedef char int8;
typedef unsigned char uint8;

typedef unsigned char 	byte;
typedef unsigned short 	word;
typedef unsigned int 	dword;

enum SymType{
	type_NoOp		= 0,
	type_Unknown	= 0x0001,
	type_Num		= 0x0002,
	type_String		= 0x0004,
	type_Bool		= 0x0008,
	type_Typeless	= 0x0010,
	type_Void		= 0x0020,
	type_Obj		= 0x0040,
	type_FuncReturn = 0x0080,

	//modifiers
	type_Array		= 0x80000000,
	type_Const		= 0x40000000,
	type_Func		= 0x20000000,
	type_Prototype	= 0x10000000,
	type_Interface	= 0x08000000,
	type_Constructor = 0x04000000,
	type_Public		= 0x02000000,
	type_VArg		= 0x01000000,
	type_Extern		= 0x00800000,

	type_ExternFunction		= type_Func | type_Public | type_Extern,
	type_PublicFunction		= type_Func | type_Public,
	type_Modifiers			= type_Array | type_Const | type_Func | type_Prototype | type_Interface | type_Constructor | type_Public | type_VArg | type_Extern,
	type_LanguageModifiers	= type_Prototype | type_Interface | type_Constructor | type_Public | type_VArg | type_Extern,
	type_ValueBits			= 0xC00000FF
};

struct Symbol
{
	string identifier;
	string prefix;
	SymType type;
	union{
		Symbol* prototype;
		Symbol* interface;
	};
	Symbol* parent;
	vector<Symbol*> arguments;
	vector<Symbol*> interfaces;
	string defaultValue;

	Symbol()
	{
		type = type_Unknown;
		prototype = NULL;
		parent = NULL;
		string defaultValue = "";
	}
};

bool isConst(SymType t) { return (t & type_Const) != 0; }
bool isArray(SymType t) { return (t & type_Array) != 0; }
bool isFunc(SymType t) { return (t & type_Func) != 0; }
bool isPublic(SymType t) { return (t & type_Public) != 0; }
bool isConstructor(SymType t) { return (t & type_Constructor) != 0; }
bool isVArg(SymType t) { return (t & type_VArg) != 0; }
bool isInterface(SymType t) { return (t & type_Interface) != 0; }
bool isPrototype(SymType t) { return (t & type_Prototype) != 0; }
SymType applyModifier(SymType t, SymType modifier) { return static_cast<SymType>(t | modifier); }
SymType stripModifiers(SymType t) { return static_cast<SymType>(t & ~type_Modifiers); }
SymType getReturnType(SymType t) { return static_cast<SymType>(t & type_ValueBits); }
SymType stripLanguageModifiers(SymType t) { return static_cast<SymType>(t & ~type_LanguageModifiers); }
SymType stripValue(SymType t) { return static_cast<SymType>(t & ~type_ValueBits); }
SymType changeValue(SymType t, SymType newValue) { return static_cast<SymType>(stripValue(t) | newValue);  }


struct SymbolBlock
{
	Symbol *symbols;
	uint32 numBlocks;
	uint32 curBlock;
	SymbolBlock *nextBlock;

	void alloc(uint32 count)
	{
		symbols = new Symbol[count];
		numBlocks = count;
		curBlock = 0;
		nextBlock = NULL;
	}

	SymbolBlock* grow(uint32 count)
	{
		nextBlock = new SymbolBlock;
		nextBlock->alloc(count);
		return nextBlock;
	}

	void dealloc()
	{
		if (nextBlock) {
			nextBlock->dealloc();
			delete nextBlock;
		}
		delete[] symbols;
		symbols = NULL;
		delete nextBlock;
		nextBlock = NULL;
	}

	Symbol* nextSymbol()
	{
		if(!symbols) return NULL;
		if(numBlocks == curBlock) return NULL;
		return &symbols[curBlock++];
	}
};
SymbolBlock SymbolPool = {0, };
SymbolBlock* currentHead = &SymbolPool;

Symbol* getNextSymbol()
{
	Symbol* s = NULL;
	s = currentHead->nextSymbol();
	if (!s)
	{
		currentHead = currentHead->grow(1000);
		s = currentHead->nextSymbol();
	}
	return s;
}

struct Error
{
	Error()
	{
		_fatal = false;
	}

	void logError(string error, int lineno)
	{
		char buffer[32];
		sprintf_s(buffer, sizeof(buffer), "%d", lineno);
		string err;
		if (_fatal)	err = "FATAL ERROR: " + error;
		else err ="ERROR: " + error;
		err += " on line ";
		err += buffer;
		err += "\n";
		errors.push_back(err);
	}

	void logWarnings(string error, int lineno)
	{
		char buffer[32];
		sprintf_s(buffer, sizeof(buffer), "%d", lineno);
		string err = "Warning: " + error;
		err += " on line ";
		err += buffer;
		err += "\n";
		warnings.push_back(err);
	}

	void fatal() //Set the error to fatal;
	{
		_fatal = true;
	}

	bool isFatal() { return _fatal; }

	vector<string> errors;
	vector<string> warnings;
	bool _fatal;
};

tr1::unordered_map<string, Symbol*> symbolTable;
tr1::unordered_map<string, vector<string>> macroTable;
vector<Symbol*> symbolStack;
vector<size_t> symbolScope;
//vector<Symbol*> returnStack;
set<string> keywordTable;

char delims[] = " \t";

//Static symbols for the generic return types
//static Symbol s_TypeUnknownSym;


Symbol* findSymbol(string& symbol, vector<Symbol*>& stack = symbolStack)
{
	for (size_t i = stack.size(); i > 0;)
	{
		i--;
		Symbol * s = stack[i];
		if( s->identifier == symbol ) return s;
	}
	return NULL;
}

bool symbolsEqual(Symbol* s1, Symbol* s2)
{
	if (
		s1->type != s2->type ||
		s1->prototype != s2->prototype ||
		s1->arguments.size() != s2->arguments.size() ||
		s1->identifier != s2->identifier
		) return false;

	return true;
}


bool addSymbol(Symbol* symbol)
{
	size_t start = 0;
	if (!symbolScope.empty())
	{
		start = symbolScope.back();
	}
	for (size_t i = start; i < symbolStack.size(); i++)
	{
		if (symbolStack[i]->identifier == symbol->identifier)
		{
			if (symbolsEqual(symbolStack[i], symbol))
			{
				//update the symbol, and return true
				symbolStack[i] = symbol;
				return true;
			}
			return false;
		}
	}
	symbolStack.push_back(symbol);
	return true;
}
void ouputAndIgnore(NRVLexToken<int>& token, string& output)
{
	output += token.Token;
}

string GetStringFromType(SymType type)
{
	switch (stripModifiers(type))
	{
	case type_Bool:		return string("boolean");
	case type_Num:		return string("number");
	case type_String:	return string("string");
	case type_Typeless:	return string("typeless");
	case type_Void:		return string("void");
	case type_FuncReturn: return string("function");
	case type_Obj: return string("object");
	case type_Unknown:	return string("unknown");
	}
	return string("unknown");
}

SymType GetTypeFromString(string &Token)
{
	SymType type = type_Unknown;

	//you're going to judge me, I know.  I didn't feel like writing another parser
	//so I went the old school tabling route.  It's my compiler, I do what I want!
	if (Token == "func")				type = type_FuncReturn;
	else if (Token == "func[]")			type = static_cast<SymType>(type_FuncReturn | type_Array);
	else if (Token == "() func")		type = static_cast<SymType>(type_FuncReturn | type_Func);
	else if (Token == "() func[]")		type = static_cast<SymType>(type_FuncReturn | type_Array | type_Func);
	if(Token == "obj")					type = type_Obj;
	else if (Token == "obj[]")			type = static_cast<SymType>(type_Obj | type_Array);
	else if (Token == "() obj")			type = static_cast<SymType>(type_Obj | type_Func);
	else if (Token == "() obj[]")		type = static_cast<SymType>(type_Obj | type_Array | type_Func);
	else if(Token == "num")				type = type_Num;
	else if (Token == "num[]")			type = static_cast<SymType>(type_Num | type_Array);
	else if (Token == "() num")			type = static_cast<SymType>(type_Num | type_Func);
	else if (Token == "() num[]")		type = static_cast<SymType>(type_Num | type_Array | type_Func);
	else if(Token == "string")			type = type_String;
	else if (Token == "string[]")		type = static_cast<SymType>(type_String | type_Array);
	else if (Token == "() string")		type = static_cast<SymType>(type_String | type_Func);
	else if (Token == "() string[]")	type = static_cast<SymType>(type_String | type_Array | type_Func);
	else if (Token == "bool")			type = type_Bool;
	else if (Token == "bool[]")			type = static_cast<SymType>(type_Bool | type_Array);
	else if (Token == "() bool")		type = static_cast<SymType>(type_Bool | type_Func);
	else if (Token == "() bool[]")		type = static_cast<SymType>(type_Bool | type_Array | type_Func);
	else if (Token == "void")		type = static_cast<SymType>(type_Void);
	else if (Token == "() void")		type = static_cast<SymType>(type_Void | type_Func);
	else if(Token == "typeless")		type = type_Typeless;
	else if (Token == "typeless[]")		type = static_cast<SymType>(type_Typeless | type_Array);
	else if (Token == "() typeless")	type = static_cast<SymType>(type_Typeless | type_Func);
	else if (Token == "() typeless[]")	type = static_cast<SymType>(type_Typeless | type_Array | type_Func);

	return type;
}

void pushScope()
{
	symbolScope.push_back(symbolStack.size());
}

void popScope()
{
	for (size_t i = symbolStack.size() - 1; !symbolStack.empty() && i >= symbolScope.back() && i > 0; i--)
	{
		symbolStack[i]->prefix = "";
	}
	symbolStack.erase(symbolStack.begin() + symbolScope.back(), symbolStack.end()); //Pop the symbol stack
	symbolScope.pop_back();
}

bool findAncestor(Symbol* findPrototype, Symbol* PrototypeTree)
{
	Symbol* current = PrototypeTree;
	while (current)
	{
		if (findPrototype == current) return true;
		for(size_t i = 0; i < current->interfaces.size(); i++) if (findPrototype == current->interfaces[i]) return true;
		current = current->parent;
	}
	return false;
}

void pushAncestorArgs(Symbol *parent, bool inClassDeclaration, Error &error)
{
	if (parent)
	{
		if (parent->parent)
		{
			pushAncestorArgs(parent->parent, inClassDeclaration, error);
		}

		for (uint32 i = 0; i < parent->arguments.size(); i++)
		{
			Symbol *s = parent->arguments[i];
			if (isPublic(s->type) && inClassDeclaration) s->prefix = "self.";
			symbolStack.push_back(s);
		}
	}
}

void cleanAncestorArgs(Symbol *parent)
{
	if (parent)
	{
		if (parent->parent)
		{
			cleanAncestorArgs(parent->parent);
		}

		for (uint32 i = 0; i < parent->arguments.size(); i++)
		{
			parent->arguments[i]->prefix = "";
		}
	}
}

void pushArguments(vector<Symbol*>& arguments, Error& error)
{
	for (uint32 i = 0; i < arguments.size(); i++)
	{
		if (!addSymbol(arguments[i]))
		{
			//Error
			//error.logError("Symbol redifinition in scope: " + arguments[i]->identifier, lexer.GetLineNumber());
		}
	}
}



SymType parseType(NRVLexer<int>& lexer, NRVLexToken<int>& token, Error& error)
{
	int ln = lexer.GetLineNumber();
	SymType type = type_Unknown;
	if (token.Type == lexmetype_Keyword)
	{
		type = GetTypeFromString(token.Token);
	}
	else
	{
		//function returns a specific class type
		//Symbol* s = findSymbol(token.Token);
		Symbol* s = symbolTable[token.Token];
		if (s && isPrototype(s->type))
		{
			type = type_Obj;
		}
	}

	lexer.GetNextToken(token);
	if (token.Type == lexmetype_Operator && token.Token == "[")
	{
		lexer.GetNextToken(token);
		if (token.Type == lexmetype_Operator && token.Token == "]")
		{
			type = applyModifier(type, type_Array);
		}
		else
		{
			//Error
			error.logError("Unexpected token '" + token.Token + "'", ln);
			return type_Unknown;
		}
	}
	else
	{
		lexer.RewindStream();
	}

	if (type == type_Unknown)
	{
		//Error
		error.logError("Unexpected token for type '" + token.Token + "'", ln);
	}
	return type;
}

SymType parseType(NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	return parseType(lexer, token, error );
}

string parseDeclType(SymType t)
{
	string dt = "";
	if (t & type_Prototype) dt += "class ";
	if (t & type_Interface) dt += "interface ";
	if (t & type_Func) dt += "() ";
	if (t & type_Const) dt += "const ";
	dt += GetStringFromType(t);
	if (t & type_Array) dt += "[]";
	return dt;
}

string parseDeclType(NRVLexToken<int>& token)
{
	Symbol *s = findSymbol(token.Token);
	return parseDeclType(s->type);
}

void parseBlock(bool isObj, Symbol* functionSymbol, NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error);
void parseStatement(Symbol* functionSymbol, NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error);
SymType parseExpression(NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error);
void parseParameter(Symbol *symbol, NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error);
SymType evalEq(SymType type, NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error);
SymType parseExpression(NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error);

int evalParams(Symbol *func, NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	int ln = lexer.GetLineNumber();

	uint32 c = 0;
	Symbol *param = NULL;
	if (func == NULL) param = NULL;
	else if(!func->arguments.empty()) param = func->arguments[c++];

	if(param && isVArg(param->type))
	{
		output += "[";
	}

	while (lexer.GetNextToken(token) && token.Token != ")")
	{
		if(error.isFatal()) return true;

		if (!func)
		{
			lexer.RewindStream();
			evalEq(type_Typeless, lexer, token, output, error);
		}
		if (param)
		{
			lexer.RewindStream();
			Symbol* s = findSymbol(token.Token);
			evalEq(isVArg(param->type) ? type_Typeless : param->type, lexer, token, output, error);

			//check to see if we are evaluating a prototype or interface
			if (stripModifiers(param->type)  == type_Obj && param->prototype && s && s->prototype) 
			{
				if (!findAncestor(param->prototype, s->prototype))
				{
					error.logError("Symbol "+s->identifier+" does not satisfy expected type " + param->prototype->identifier, ln);
				}
			}
		}
		else error.logError("Unexpected param " + token.Token, ln);

		if(token.Token == ",")
		{
			output += ",";
			if (param && !isVArg(param->type))
			{
				if(c < func->arguments.size())
				{
					param = func->arguments[c++];
					if(param && isVArg(param->type))
					{
						output += "[";
					}
				}
				else
				{
					//Error
					error.logError("Unexpected argument '"+token.Token+"'", ln);
				}
			}
			lexer.GetNextToken(token);
		}
		else if(token.Token == ")")
		{
			break;
		}
		else
		{
			//Error
			error.logError("Syntax error.  Expected , between arguments at "+token.Token, ln);
		}
	}

	if(param && isVArg(param->type))
	{
		output += "]";
	}

	if (func && c < func->arguments.size())
	{
		//look for default params
		while (c < func->arguments.size() && !func->arguments[c]->defaultValue.empty())
		{
			if (c < func->arguments.size()) output += ",";
			output += func->arguments[c]->defaultValue;
			c++;
		}
	}

	if (func && c < func->arguments.size())
	{

		if (isVArg(func->arguments[c]->type)) {
			if (c > 0) output += ",";
			output += "[]";
			return 0;
		}
		return int(func->arguments.size() - c);
	}
	return 0;
}

void parseObjectLiteralTypes(vector<Symbol> symbolData, NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	lexer.GetNextToken(token);

	if (token.Token != "{")
	{
		error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
		return;
	}

	output += token.Token;
	while (lexer.GetNextToken(token) && token.Token != "}")
	{
		if (error.isFatal()) return;

		switch (token.Type)
		{
			case lexmetype_NewLine:
				break;
			case lexmetype_Identifier:
			{
				string name = token.Token;
				output += name;
				lexer.GetNextToken(token);

				if (token.Token == ":")
				{
					output += token.Token;
					SymType t = parseExpression(lexer, token, output, error);

					//add it to our object data
					Symbol s;
					s.identifier = name;
					s.type = t;
					symbolData.push_back(s);

					lexer.GetNextToken(token);
					if (token.Token != ",")
					{
						lexer.RewindStream();
					}
					else
					{
						output += token.Token;
					}
					break;
				}
				else
				{
					error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
					break;
				}
			}
			case lexmetype_Operator:
			case lexmetype_String:
			case lexmetype_Int:
			case lexmetype_Float:
			case lexmetype_HexNumber:
			case lexmetype_Unknown:
				//Error
				error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
				return;
		}
	}
	output += token.Token;
}

void parseObjectLiteral(NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	vector<Symbol> temp;
	parseObjectLiteralTypes(temp, lexer, token, output, error);
}


SymType evalEq(SymType type, NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	int ln = lexer.GetLineNumber();
	SymType typeRight = parseExpression(lexer, token, output, error);

	if (stripLanguageModifiers(type) != type_Typeless 
		&& stripLanguageModifiers(type) != typeRight
		&& ( (type & type_FuncReturn) == 0 || (typeRight & type_Func) == 0))
	{
		error.logError("Type mismatch. Expected type "+ parseDeclType(type)+" but found "+ parseDeclType(typeRight), ln);
	}
	return typeRight;
}

SymType evalEq(Symbol* s, NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	return evalEq(s->type, lexer, token, output, error);
}




Symbol* parseDeclaration(NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	string name;
	SymType type = type_Unknown;
	Symbol* prototype = NULL;

	while (!lexer.EOS() && token.Type != lexmetype_NewLine)
	{
		if (error.isFatal()) return NULL;

		switch (token.Type)
		{
		case lexmetype_Identifier:
			{
				Symbol *s = symbolTable[token.Token];
				if (s == NULL)
				{
					name = token.Token;
					output += "var " + name;
					lexer.GetNextToken(token);
					prototype = symbolTable[token.Token];
					type = parseType(lexer, token, error);
				}
			}
			break;

		break;
		case lexmetype_Operator:
		case lexmetype_String:
		case lexmetype_Int:
		case lexmetype_Float:
		case lexmetype_HexNumber:
		case lexmetype_Unknown:
			//Error
			error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
			return NULL;
		}

		lexer.GetNextToken(token);
		Symbol *symbol = getNextSymbol();
		symbol->identifier = name;
		symbol->type = type;
		symbol->prototype = prototype;

		if (!addSymbol(symbol))
		{
			//Error
			error.logError("Symbol redefinition in scope: " + name, lexer.GetLineNumber());
			return NULL;
		}

		return symbol;
	}
	return NULL;
}

Symbol* parseDeclAssign(NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	string name;

	while (!lexer.EOS() && token.Type != lexmetype_NewLine)
	{
		if (error.isFatal()) return NULL;

		switch (token.Type)
		{
		case lexmetype_Identifier:
		{
			Symbol *s = symbolTable[token.Token];
			if (s == NULL)
			{
				name = token.Token;
			}
		}
		break;

		break;
		case lexmetype_Operator:
		case lexmetype_String:
		case lexmetype_Int:
		case lexmetype_Float:
		case lexmetype_HexNumber:
		case lexmetype_Unknown:
			//Error
			error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
			return NULL;
		}

		lexer.GetNextToken(token);
		Symbol *symbol = getNextSymbol();
		symbol->identifier = name;
		symbol->type = type_Unknown;
		symbol->prototype = NULL;

		if (!addSymbol(symbol))
		{
			//Error
			error.logError("Symbol redefinition in scope: " + name, lexer.GetLineNumber());
			return NULL;
		}

		return symbol;
	}
	return NULL;
}

int parseCommaListAssignment(NRVLexer<int>& lexer, NRVLexToken<int>& token)
{
	if (token.Token == ",") {
		lexer.GetNextToken(token);

		//account for comma dangle
		if (token.Token == "=" && token.Type == lexmetype_NewLine) return -1;
		return 1;
	}

	return 0;
}

void parseLetStatement(NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	bool isConst = token.Token == "const" ? true : false;
	int ln = lexer.GetLineNumber();

	vector<Symbol *> decldSymbols;
	lexer.GetNextToken(token);
	string temp;
	while (token.Token != "="
		&& token.Type != lexmetype_NewLine)
	{
		if (int comma = parseCommaListAssignment(lexer, token))
		{
			if (comma == -1) break;
			if (token.Token == "=") break;
			if (decldSymbols.empty()) error.logError("Unexpected token ','", ln);
		}

		Symbol* s = parseDeclaration(lexer, token, temp, error);
		if (s) {
			decldSymbols.push_back(s);
		}
	}

	//infer the types from the last type
	SymType type = type_Unknown;
	for (size_t i = decldSymbols.size() - 1; ; i--)
	{
		if (decldSymbols[i]->type != type_Unknown) {
			type = decldSymbols[i]->type;
		}
		else
		{
			decldSymbols[i]->type = type;
		}

		if (decldSymbols[i]->type == type_Unknown)
		{
			error.logError("Typing error, type unknown for symbol " + decldSymbols[i]->identifier, ln);
		}

		if (i == 0) break;
	}

	if (decldSymbols.empty()) return;

	if (token.Token == "=")
	{

		int count = 0;
		while (token.Type != lexmetype_NewLine)
		{
			int ln = lexer.GetLineNumber();
			if (int comma = parseCommaListAssignment(lexer, token))
			{
				if (comma == -1) break;
				if (decldSymbols.empty()) error.logError("Unexpected token ','", ln);
			}

			if (count < decldSymbols.size())
			{
				std::string expression;
				SymType t = parseExpression(lexer, token, expression, error);

				if (decldSymbols[count]->type != t)
					error.logError("Type mismatch. Expected type " + GetStringFromType(decldSymbols[count]->type) + " but found " + GetStringFromType(t), ln);
				output += "var " + decldSymbols[count]->identifier + " = " + expression + ";\n";
				count++;
			}
		}

		if (count != decldSymbols.size())
		{
			error.logError("Declaration list and assignment list lengths differ.", ln);
		}
	}
	else
	{
		for (uint32 i = 0; i < decldSymbols.size(); i++)
		{
			if (i > 0) output += ",";
			output += decldSymbols[i]->identifier;
		}
	}
}

void updatePrototype(Symbol* objSymbol, NRVLexer<int>& lexer, NRVLexToken<int>& token, Error& error)
{
	//we need arguments and types
	if (token.Type == lexmetype_Identifier)
	{
		Symbol* s = symbolTable[token.Token];
		if (s && isPrototype(s->type)) objSymbol->prototype = s;
	}

	while (token.Token != ")") lexer.GetNextToken(token);
	lexer.GetNextToken(token);
}

void updateCurrySymbol(Symbol* currySymbol, NRVLexer<int>& lexer, NRVLexToken<int>& token, Error& error)
{
	//we need arguments and types
	if (token.Type == lexmetype_Identifier) 
	{
		Symbol *s = findSymbol(token.Token);
		if (s && isFunc(s->type))
		{
			lexer.GetNextToken(token);
			if (token.Token != "(") return;

			string temp;
			int curry = evalParams(s, lexer, token, temp, error);
			if (token.Token != ")") return;
			lexer.GetNextToken(token);

			//define the curry function
			currySymbol->arguments.resize(curry);
			copy(s->arguments.begin() + (s->arguments.size() - curry), s->arguments.end(), currySymbol->arguments.begin());
			currySymbol->type = applyModifier(type_FuncReturn, type_PublicFunction);
		}
	}
}

bool parseDeclAssignStatement(NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	vector<Symbol *> decldSymbols;
	lexer.SaveState();
	lexer.GetNextToken(token);
	while (token.Token != ":="
		&& token.Token != "^="
		&& token.Type == lexmetype_Identifier)
	{
		int ln = lexer.GetLineNumber();
		if (token.Token == ",") {
			lexer.GetNextToken(token);
			if (decldSymbols.empty())
			{
				error.logError("Unexpected token ','", ln);
			}
		}

		Symbol* s = parseDeclAssign(lexer, token, output, error);
		if (s) {
			decldSymbols.push_back(s);
		}
	}

	if (decldSymbols.empty())
	{
		lexer.RestoreState();
		return false;
	}

	//infer the types from assignment
	if (token.Token == ":="
		|| token.Token == "^=")
	{
		int count = 0;
		bool isConst = token.Token == "^=";
		while (token.Type != lexmetype_NewLine)
		{
			if (token.Token == ",") {
				lexer.GetNextToken(token);
				if (decldSymbols.empty())
				{
					error.logError("Unexpected token ','", lexer.GetLineNumber());
				}
			}

			if (count < decldSymbols.size())
			{
				std::string expression;
				lexer.SaveState();
				SymType t = parseExpression(lexer, token, expression, error);

				if (isFunc(t)) {
					lexer.RestoreState();
					lexer.GetNextToken(token);
					updateCurrySymbol(decldSymbols[count], lexer, token, error);
				}

				if (t & type_Obj) {
					lexer.RestoreState();
					lexer.GetNextToken(token);
					updatePrototype(decldSymbols[count], lexer, token, error);
				}

				// consider allowing for destruction on decl assign
				//if (t == type_Obj && decldSymbols.size() > 1)
				//{
				//	output += "_obj = " + expression + ";\n";

				//	for (uint32 i = 0; i < decldSymbols.size(); i++)
				//	{
				//		output += "var " + decldSymbols[i]->identifier + " = _obj." + decldSymbols[i]->identifier + ";\n";
				//	}
				//}

				decldSymbols[count]->type = applyModifier(t, isConst ? type_Const : type_NoOp);
				output += "var " + decldSymbols[count]->identifier + " = " + expression + ";\n";
				count++;
			}
			else
			{
				//ERROR
			}
		}

		if (count != decldSymbols.size())
		{
			//ERROR
		}
	}
	else
	{
		lexer.RestoreState();
		return false;
	}

	return true;
}

void parseFunctionDeclaration(Symbol* functionSymbol, NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	bool hasVArg = false;
	bool hasDefaultValue = false;

	lexer.GetNextToken(token);
	while (token.Token != ")")
	{
		Symbol *argSymbol = getNextSymbol();
		parseParameter(argSymbol, lexer, token, output, error);

		if (hasVArg)
		{
			error.logError("Variable argument must be the last argument in the argument list.", lexer.GetLineNumber());
		}
		if (isVArg(argSymbol->type))
		{
			hasVArg = true;
		}
		if (!argSymbol->defaultValue.empty())
		{
			hasDefaultValue = true;
		}
		if (hasDefaultValue && argSymbol->defaultValue.empty() && !isVArg(argSymbol->type))
		{
			error.logError("Missing default value for identifier " + argSymbol->identifier, lexer.GetLineNumber());
		}
		if (!argSymbol->identifier.empty())
		{
			functionSymbol->arguments.push_back(argSymbol);
		}
		if (token.Token == ",")
		{
			output += ",";
			lexer.GetNextToken(token);
		};
	}
}

SymType parseFunction(NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	Symbol *symbol = getNextSymbol();
	Symbol *interface = NULL;
	symbol->type = applyModifier(type_Typeless, type_PublicFunction);

	int ln = lexer.GetLineNumber();

	output += "function ";
	lexer.GetNextToken(token);
	if (symbolTable.find(token.Token) != symbolTable.end())
	{
		interface = symbolTable[token.Token];
		if (isInterface(interface->type)) symbol->interface = interface;
		else error.logError("Type '" + token.Token + "' is not an interface", ln);

		lexer.GetNextToken(token);
		if (token.Token == "=>") lexer.GetNextToken(token);
		else error.logError("Unexpected token '" + token.Token + "'", ln);
	}

	if (token.Type == lexmetype_Identifier)
	{
		symbol->identifier = token.Token;
		output += symbol->identifier;

		lexer.GetNextToken(token);
		if (token.Token == "(")
		{


			output += "(";
			parseFunctionDeclaration(symbol, lexer, token, output, error);
			if (token.Token == ")") output += ")";

			lexer.GetNextToken(token);
			if (token.Token == ":") {
				lexer.GetNextToken(token);
				symbol->type = changeValue(symbol->type, parseType(lexer, token, output, error));
				lexer.GetNextToken(token);
			}


			while (token.Token != "{" && token.Type == lexmetype_NewLine && lexer.GetNextToken(token));

			if (token.Token == "{")
			{
				lexer.RewindStream();
				parseBlock(false, symbol, lexer, token, output, error);
			}
			else
			{
				output += "{";
				lexer.RewindStream();
				parseStatement(symbol, lexer, token, output, error);
				output += "}";
			}
		}
	}

	if (!addSymbol(symbol))  error.logError("Symbol redifinition in scope: " + symbol->identifier, ln);
	return symbol->type;
}


void parseMethod(bool isConstructor, Symbol *symbol, NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	string name;
	SymType type = (isConstructor) ? applyModifier(type_Obj, type_Func) : applyModifier(type_Typeless, type_Func);

	if (isConstructor)
	{
		output += "function ";
	}

	while (lexer.GetNextToken(token) && token.Type != lexmetype_NewLine && token.Token != "{" && (!isConstructor || token.Token != ":"))
	{
		if (error.isFatal()) return;

		switch (token.Type)
		{
		case lexmetype_Comment:
		case lexmetype_NewLine:
			break;
		case lexmetype_Keyword:
			//Error
			error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
			return;

			break;
		case lexmetype_Identifier:
		{
			if (isConstructor && symbolTable.find(token.Token) != symbolTable.end() && name.empty())
			{
				name = token.Token;
				output += name;
			}
			//This should be the func name
			else if (name.empty())
			{
				name = token.Token;
				output += name;
			}
			else
			{
				//Error
				error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
				return;
			}
		}

		break;
		case lexmetype_Operator:
			if (token.Token == "(")
			{

				if (!isConstructor) output += "=function(";
				else output += "(";

				parseFunctionDeclaration(symbol, lexer, token, output, error);\
				if (token.Token == ")") output += ")";
			}
			else if (token.Token == ":")
			{
				if (isConstructor) break; //constructor initializer list
				lexer.GetNextToken(token);
				if (token.Type == lexmetype_Keyword) type = changeValue(type, parseType(lexer, token, output, error));
				else
				{
					//Error
					error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
					return;
				}
			}
			break;
		case lexmetype_String:
		case lexmetype_Int:
		case lexmetype_Float:
		case lexmetype_HexNumber:
		case lexmetype_Unknown:
			//Error
			error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
			return;
			break;
		}
	}

	//TODO: check out this code, it looks suspect
	while ((!isConstructor || token.Token != ":") && (token.Type == lexmetype_NewLine || token.Type == lexmetype_Comment))
	{
		if (!lexer.GetNextToken(token)) break;
	}
	lexer.RewindStream();

	//push this symbol onto the symbol stack
	symbol->identifier = name;
	symbol->type = applyModifier(type, type_Func);
	symbol->type = applyModifier(symbol->type, isConstructor ? type_Constructor : type_NoOp);
}

void parseInitializerList(Symbol *con, Symbol *parent, NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, string& constructor_params, Error& error)
{
	Symbol* member = NULL;
	int ln = lexer.GetLineNumber();
	while (lexer.GetNextToken(token) && token.Token != "{" && token.Type != lexmetype_NewLine)
	{
		if (error.isFatal()) return;

		switch (token.Type)
		{
		case lexmetype_Comment:
		case lexmetype_NewLine:
			break;
		case lexmetype_Identifier:
			if (member == NULL)
			{
				(member = findSymbol(token.Token)) || ((parent != NULL) && (member = findSymbol(token.Token, parent->arguments)));
				lexer.GetNextToken(token);
				if (token.Token != "(")
				{
					error.logError("Unexpected token '" + token.Token + "'", ln);
				}
			}
			else
			{
				if (isConstructor(member->type))
				{
					lexer.RewindStream();
					pushScope();
					pushArguments(con->arguments, error);
					evalParams(member, lexer, token, constructor_params, error);
					popScope();
					//output = member->identifier + "().constructor.call(this," + constructor_params + ");";
					member = NULL;
				}
				else
				{
					Symbol *value = findSymbol(token.Token, con->arguments);

					if (stripLanguageModifiers(member->type) != type_Typeless && stripLanguageModifiers(member->type) != stripLanguageModifiers(value->type))
					{
						error.logError("Type mismatch. Expected type " + GetStringFromType(member->type) + " for member " + member->identifier + " but found " + GetStringFromType(value->type), ln);
					}
					output += member->prefix + member->identifier + "=" + value->identifier + ";";
				}
			}


			break;
		case lexmetype_Operator:
			if (token.Token == ")")
			{
				member = NULL; //Shouldn't hit this
			}
			break;
		case lexmetype_String:
			if (member == NULL)
			{
				error.logError("Unexpected token '" + token.Token + "'", ln);
				break;
			}
			else
			{
				if (member->type != type_Typeless && stripLanguageModifiers(member->type) != type_String)
				{
					error.logError("Type mismatch. Expected type " + GetStringFromType(member->type) + " for member " + member->identifier, ln);
				}
			}
			output += member->prefix + member->identifier + "=" + token.Token + ";";
			break;
		case lexmetype_Int:
		case lexmetype_Float:
		case lexmetype_HexNumber:
			if (member == NULL)
			{
				error.logError("Unexpected token '" + token.Token + "'", ln);
			}
			else
			{
				if (member->type != type_Typeless && stripLanguageModifiers(member->type) != type_Num)
				{
					error.logError("Type mismatch. Expected type " + GetStringFromType(member->type) + " for member " + member->identifier, ln);
				}
			}
			output += member->prefix + member->identifier + "=" + token.Token + ";";
			break;
		case lexmetype_Keyword:
		case lexmetype_Unknown:
			//Error
			error.logError("Unexpected token '" + token.Token + "'", ln);
			return;
		}
	}
}

void ouputClass(Symbol *obj, string& class_internal, string& constructor, string& parent_constructor_params, string& output)
{
	string constructor_list;
	string inheitance_block;

	vector<Symbol *>& definition = obj->arguments;

	for (uint32 i = 0; i < definition.size(); i++)
	{
		Symbol* s = definition[i];

		if (s->type & type_Constructor)
		{
			for (uint32 k = 0; k < s->arguments.size(); k++)
			{
				constructor_list += s->arguments[k]->identifier;
				if (k + 1 != s->arguments.size()) constructor_list += ",";
			}

		}
	}

	if (obj->parent)
	{
		inheitance_block = obj->identifier + ".prototype=" + obj->parent->identifier + "(" + parent_constructor_params + ");";
		inheitance_block += obj->identifier + ".prototype.constructor=" + obj->identifier + ";";
		inheitance_block += obj->identifier + ".prototype.parent=" + obj->parent->identifier + ".prototype;";
	}

	output += "var " + obj->identifier + "=function(" + constructor_list + "){";
	output += constructor;
	output += inheitance_block;
	output += class_internal;
	output += "return new " + obj->identifier + "(" + constructor_list + ");};\n";
}

void parseClassBody(Symbol* classSymbol, string& class_internal, string& constructor, string& parent_constructor_params, NRVLexer<int>& lexer, NRVLexToken<int>& token, Error& error)
{
	//Parse Members
	vector<Symbol *>& methods = classSymbol->arguments;
	int isPublic = -1;
	pushScope();

	//push the parent's arguments
	if (classSymbol->parent)
	{
		pushAncestorArgs(classSymbol->parent, true, error);
	}

	//The default constructor
	constructor = "function " + classSymbol->identifier + "(){}";

	while (lexer.GetNextToken(token) && token.Token != "}")
	{
		if (error.isFatal()) return;

		switch (token.Type)
		{
		case lexmetype_Comment:
		case lexmetype_NewLine:
			break;
		case lexmetype_Keyword:
			if (token.Token == "public")
			{
				if (isPublic != -1) error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
				isPublic = 1;
			}
			else if (token.Token == "private") 
			{
				if (isPublic != -1) error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
				isPublic = 0;
			}
			else if (token.Token == "func")
			{
				if (isPublic == -1) isPublic = 0;

				string function_body;
				Symbol *method = getNextSymbol(); //default constructor

				if (isPublic == 1) function_body += classSymbol->identifier + ".prototype.";

				parseMethod(false, method, lexer, token, function_body, error);
				method->type = applyModifier(method->type, isPublic ? type_Public : type_NoOp);

				if (isPublic == 1)
				{
					method->prefix = "self.";
					classSymbol->arguments.push_back(method);
				}
				else
				{
					//insert a self var
					if (method->arguments.empty()) function_body.insert(function_body.find("(") + 1, "self");
					else function_body.insert(function_body.find("(") + 1, "self,");
				}

				lexer.GetNextToken(token);
				if (token.Token == "{")
				{
					function_body += "{";
					if (isPublic == 1) function_body += "var self=this;";
					pushScope();

					//get the function and push it's params into scope
					pushArguments(method->arguments, error);
					//returnStack.push_back(method);


					parseBlock(false, method, lexer, token, function_body, error);

					lexer.GetNextToken(token);
					if (token.Token == "}") {
						//exiting scope
						popScope();
						//returnStack.pop_back();
						function_body += "};";
					}
					class_internal += function_body + "\n";
				}
				else
				{
					error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
				}

				addSymbol(method);//add to the scope of the class declaration
				isPublic = -1;
			}

			break;
		case lexmetype_Identifier:
			//This should be the func name
			if (token.Token == classSymbol->identifier) //parse the constructor
			{
				string initializer;
				constructor = "";
				Symbol* conFunc = methods[0];
				lexer.RewindStream();
				parseMethod(true, conFunc, lexer, token, constructor, error);

				lexer.GetNextToken(token);
				if (token.Token == ":")
				{
					//parse the initializer list
					parseInitializerList(conFunc, classSymbol->parent, lexer, token, initializer, parent_constructor_params, error);
				}

				while (token.Type == lexmetype_NewLine && lexer.GetNextToken(token));
				if (token.Token == "{")
				{
					constructor += "{";
					constructor += "var self=this;";
					constructor += initializer;
					pushScope();

					//get the function and push it's params into scope
					pushArguments(conFunc->arguments, error);
					//returnStack.push_back(conFunc);


					parseBlock(false, conFunc, lexer, token, constructor, error);

					lexer.GetNextToken(token);
					if (token.Token == "}") {
						//exiting scope
						popScope();
						//returnStack.pop_back();
						constructor += "}";
					}
				}
				else
				{
					constructor += "{";
					constructor += "var self=this;";
					constructor += initializer;
					constructor += "}";
					lexer.RewindStream();
				}
			}
			else
			{
				if (isPublic == -1) isPublic = 0;
				string decl;
				parseDeclaration(lexer, token, decl, error);
				Symbol * var = symbolStack.back();
				var->type = applyModifier(var->type, isPublic ? type_Public : type_NoOp);
				if (isPublic == 0)
				{
					decl += ";";
					class_internal += decl + "\n";
				}
				else
				{
					var->prefix = "self.";
					classSymbol->arguments.push_back(var);
				}
				isPublic = -1;
			}
			isPublic = -1;

			break;
		case lexmetype_Operator:
			break;
		case lexmetype_String:
		case lexmetype_Int:
		case lexmetype_Float:
		case lexmetype_HexNumber:
		case lexmetype_Unknown:
			//Error
			error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
			//return;
			break;
		}

	}

	popScope();
}

void parseClass(NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	Symbol *symbol = getNextSymbol();
	string name;
	vector<Symbol *> interfaces;
	Symbol* parent = NULL;

	int ln = lexer.GetLineNumber();

	//Parse Decl
	while (lexer.GetNextToken(token) && token.Token != "{")
	{
		if (error.isFatal()) return;

		switch (token.Type)
		{
		case lexmetype_Comment:
		case lexmetype_NewLine:
			break;
		case lexmetype_Keyword:

			if (token.Token == "implements")
			{
				while (lexer.GetNextToken(token) && token.Type == lexmetype_Identifier)
				{
					if (symbolTable.find(token.Token) == symbolTable.end())
					{
						//Error
						error.logError("Undeclared interface: " + token.Token, lexer.GetLineNumber());
					}
					else
					{
						Symbol* inter = symbolTable[token.Token];
						if (isInterface(inter->type)) interfaces.push_back(inter);
						else
						{
							error.logError(token.Token + " is not defined as an interface.", lexer.GetLineNumber());
						}
					}

					lexer.GetNextToken(token);
					if (token.Token != ",")
					{
						lexer.RewindStream();
					}
				}
				lexer.RewindStream();
			}
			else if (token.Token == "extends")
			{
				while (lexer.GetNextToken(token) && token.Type == lexmetype_Identifier)
				{
					if (symbolTable.find(token.Token) == symbolTable.end())
					{
						//Error
						error.logError("Undeclared class: " + token.Token, lexer.GetLineNumber());
					}
					else
					{
						if (!parent)
						{
							parent = symbolTable[token.Token];
							if (!isPrototype(parent->type))
							{
								error.logError(token.Token + " is not defined as a class", lexer.GetLineNumber());
								parent = NULL;
							}
						}
						else
						{
							error.logError("Unexpected identifier " + token.Token + ". " + name + " already has a parent class defined", lexer.GetLineNumber());
						}
					}
				}
				lexer.RewindStream();
			}
			else
			{
				//Error
				error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
				//return;
			}

			break;
		case lexmetype_Identifier:
			//This should be the func name
			if (name.empty())
			{
				name = token.Token;

				//build out the constructor
				Symbol *con = getNextSymbol(); //default constructor
				con->identifier = name;
				con->type = applyModifier(type_Obj, type_PublicFunction);
				con->type = applyModifier(type_Obj, type_Constructor);
				symbol->arguments.push_back(con);
			}
			else
			{
				//Error
				error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
				return;
			}

			break;
		case lexmetype_Operator:
			break;
		case lexmetype_String:
		case lexmetype_Int:
		case lexmetype_Float:
		case lexmetype_HexNumber:
		case lexmetype_Unknown:
			//Error
			error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
			return;
			break;
		}
	}



	//push this symbol onto the symbol stack
	symbol->identifier = name;
	symbol->parent = parent;
	symbol->type = applyModifier(type_Obj, type_Prototype);
	symbol->interfaces = interfaces;

	//if (!addSymbol(symbol))
	if (symbolTable.find(name) != symbolTable.end())
	{
		//Error
		error.logError("Symbol redefinition in scope: " + name, lexer.GetLineNumber());
	}
	else
	{
		symbolTable[name] = symbol;
	}

	//Parse the body
	string class_internal, constructor, parent_constructor_params;
	parseClassBody(symbol, class_internal, constructor, parent_constructor_params, lexer, token, error);


	tr1::unordered_map<string, Symbol*> classSymbols;
	for (size_t i = 1; i < symbol->arguments.size(); i++)
	{
		classSymbols[symbol->arguments[i]->identifier] = symbol->arguments[i];
	}
	for (size_t i = 0; i < symbol->interfaces.size(); i++)
	{
		Symbol* interface = symbol->interfaces[i];
		for (size_t j = 0; j < interface->arguments.size(); j++)
		{
			Symbol* argument = interface->arguments[i];
			if (classSymbols.find(argument->identifier) == classSymbols.end())
			{
				error.logError("Failed to satisify interface member " + interface->identifier + "." + argument->identifier + " for class " + symbol->identifier, ln);
			}
		}
	}

	//output the class
	ouputClass(symbol, class_internal, constructor, parent_constructor_params, output);

	//clean the prefixes
	for (uint32 i = 0; i < symbol->arguments.size(); i++)
	{
		symbol->arguments[i]->prefix = "";
	}
	cleanAncestorArgs(parent);
}

void parseInterface(NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	Symbol *symbol = getNextSymbol();
	string name;

	vector<Symbol *>& methods = symbol->arguments;

	//Parse Decl
	while (lexer.GetNextToken(token) && token.Token != "{")
	{
		if (error.isFatal()) return;

		switch (token.Type)
		{
		case lexmetype_NewLine:
			break;
		case lexmetype_Keyword:
			//Error
			error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
			return;

			break;
		case lexmetype_Identifier:
			//This should be the func name
			if (name.empty())
			{
				name = token.Token;
			}
			else
			{
				//Error
				error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
				return;
			}

			break;
		case lexmetype_Operator:
			break;
		case lexmetype_String:
		case lexmetype_Int:
		case lexmetype_Float:
		case lexmetype_HexNumber:
		case lexmetype_Unknown:
			//Error
			error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
			return;
			break;
		}
	}

	//Parse Members

	string class_internal, identifier;
	int isPublic = -1;
	pushScope();


	while (lexer.GetNextToken(token) && token.Token != "}")
	{
		if (error.isFatal()) return;

		switch (token.Type)
		{
		case lexmetype_NewLine:
			break;
		case lexmetype_Keyword:
			if (token.Token == "public")
			{
				if (isPublic != -1) error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
				isPublic = 1;
			}
			else if (token.Token == "private")
			{
				if (isPublic != -1) error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
				isPublic = 0;
			}
			else if (token.Token == "func")
			{
				if (isPublic == -1) isPublic = 0;

				string function_body;
				Symbol *method = getNextSymbol(); //default constructor

				parseMethod(false, method, lexer, token, function_body, error);
				method->type = applyModifier(method->type, isPublic ? type_Public : type_NoOp);
				if (isPublic == 0)
				{
					error.logError("Private symbols are not permitted in interfaces.", lexer.GetLineNumber());
				}
				else if (isPublic == -1)
				{
					error.logError("Access needs to be specified for interface member.", lexer.GetLineNumber());
				}
				else
				{
					symbol->arguments.push_back(method);
				}
				isPublic = -1;
			}
			else if (token.Token == "obj" ||
				token.Token == "num" ||
				token.Token == "string" ||
				token.Token == "bool" ||
				token.Token == "typeless")
			{
				lexer.RewindStream();
				string decl;
				parseDeclaration(lexer, token, decl, error);
				Symbol * var = symbolStack.back();
				var->type = applyModifier(var->type, isPublic ? type_Public : type_NoOp);
				if (isPublic == 0)
				{
					error.logError("Private symbols are not permitted in interfaces.", lexer.GetLineNumber());
				}
				else if (isPublic == -1)
				{
					error.logError("Access needs to be specified for interface member.", lexer.GetLineNumber());
				}
				else
				{
					symbol->arguments.push_back(var);
				}
				isPublic = -1;
			}
			else 
			{
				error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
			}

			break;
		case lexmetype_Identifier:
			//This should be the func name
			if (identifier.empty())
			{
				string function_body;
				Symbol *method = getNextSymbol(); //default constructor
				lexer.RewindStream();
				parseMethod(false, method, lexer, token, function_body, error);
				method->type = applyModifier(method->type, isPublic == 1 ? type_Public : type_NoOp);
				if (isPublic == 1)
				{
					symbol->arguments.push_back(method);
				}
				else
				{
					error.logError("Private symbols are not permitted in interfaces.", lexer.GetLineNumber());
				}

				addSymbol(method);//add to the scope of the class declaration
			}
			else
			{
				//Error
				error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
				return;
			}
			isPublic = -1;

			break;
		case lexmetype_Operator:
			break;
		case lexmetype_String:
		case lexmetype_Int:
		case lexmetype_Float:
		case lexmetype_HexNumber:
		case lexmetype_Unknown:
			//Error
			error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
			return;
			break;
		}
	}

	popScope();

	//push this symbol onto the symbol stack
	symbol->identifier = name;
	symbol->type = applyModifier(type_Obj, type_Interface);

	//if (!addSymbol(symbol))
	if (symbolTable.find(name) != symbolTable.end())
	{
		//Error
		error.logError("Symbol redefinition in scope: " + name, lexer.GetLineNumber());
	}
	else
	{
		symbolTable[name] = symbol;
	}
}

void outputParameterList(NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	while (token.Token != ")")
	{
		parseExpression(lexer, token, output, error);

		if (token.Token == ",")
		{
			output += ",";
			lexer.GetNextToken(token);
		}
	}
}

SymType executeFunction(Symbol* funcSymbol, NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	SymType type = type_Unknown;
	int ln = lexer.GetLineNumber();
	output += funcSymbol->prefix + funcSymbol->identifier;
	if (token.Type == lexmetype_NewLine ||
		token.Token == ")" ||
		token.Token == "}" ||
		token.Token == "]")
	{
		lexer.RewindStream();
		return type;
	}
	else if (token.Token != "(")
	{
		//Error
		error.logError("Expected token '(' at '" + token.Token + "'", ln);
		return type;
	}
	output += "(";

	//provide a self reference for private functions
	if (!isPublic(funcSymbol->type))
	{
		output += "self";
		if (!funcSymbol->arguments.empty())
		{
			output += ",";
		}
	}

	int curry = 0;
	if (funcSymbol->type & type_Extern)
	{
		outputParameterList(lexer, token, output, error);
	}
	else
	{
		curry = evalParams(funcSymbol, lexer, token, output, error);
	}


	if (token.Token != ")")
	{
		//Error
		error.logError("Expected token ')' at '" + token.Token + "'", ln);
		return type;
	}

	if (curry && curry == funcSymbol->arguments.size())
	{
		error.logError(funcSymbol->identifier + " must be called with at least one parameter", ln);
	}
	else if (curry)
	{
		string curryOutput;

		curryOutput += "function(";

		//loop the param list
		for (int i = 0; i < curry; i++)
		{
			if (i > 0) curryOutput += ",";
			curryOutput += "_" + to_string(i);
		}

		curryOutput += "){ return " + output;

		//loop the param list
		for (int i = 0; i < curry; i++)
		{
			curryOutput += ",_" + to_string(i);
		}

		output = curryOutput;
		type = applyModifier(type_FuncReturn, type_PublicFunction);
	}
	else
	{
		type = getReturnType(funcSymbol->type);
		if (type == type_FuncReturn)
		{
			type = applyModifier(type_Typeless, type_PublicFunction);
		}
	}
	output += ")";
	if (curry) output += "}";
	return type;
}

SymType parseExpression(NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	int ln = lexer.GetLineNumber();
	SymType type = type_Unknown;
	Symbol* s = NULL;
	Symbol* parent = NULL; //used when parsing a typeless object
	string name;

	//figure out what the type on the right is
	//lexer.GetNextToken(token);
	while (lexer.GetNextToken(token) && token.Type != lexmetype_NewLine && token.Token != "," && token.Token != ")" && token.Token != "}" && token.Token != "]")
	{
		if (error.isFatal()) return type_Unknown;

		switch (token.Type)
		{
		case lexmetype_NewLine:
			break;
		case lexmetype_Identifier:
		{
			//Find the type in the symbol table
			s = findSymbol(token.Token);
			if (s)
			{
				name = s->prefix + s->identifier;
				if (type == type_Unknown) type = stripLanguageModifiers(s->type);
				else
				{
					//Error
					error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
				}
			}
			else if (symbolTable.find(token.Token) != symbolTable.end())
			{
				s = symbolTable[token.Token];
				if (isPrototype(s->type))
				{
					output += s->prefix + s->identifier;
					Symbol* constructor = s->arguments[0];
					type = type_Obj;
					lexer.GetNextToken(token);
					if (token.Token != "(")
					{
						//Error
						error.logError("Expected token '(' at '" + token.Token + "'", lexer.GetLineNumber());
						break;
					}
					output += "(";
					evalParams(constructor, lexer, token, output, error);
					if (token.Token != ")")
					{
						//Error
						error.logError("Expected token ')' at '" + token.Token + "'", lexer.GetLineNumber());
						break;
					}
					output += ")";
				}
				else if (isInterface(s->type))
				{
					error.logError("Can not instantiate an interface '" + token.Token + "'", lexer.GetLineNumber());
				}
				else
				{
					//Error
					error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
				}
			}
			else
			{
				s = getNextSymbol();
				s->identifier = token.Token;
				s->type = applyModifier(type_Typeless, type_Extern);
				name = s->identifier;
				if (parent)
				{
					parent->arguments.push_back(s);

				}
				else if (!addSymbol(s))
				{
					//Error
					error.logError("Symbol redifinition in scope: " + token.Token, lexer.GetLineNumber());
				}

				type = s->type;
				parent = s;
				//output += s->identifier;
			}


			//is this an "is" statement
			lexer.GetNextToken(token);

			if (token.Type == lexmetype_Keyword)
			{
				if (token.Token == "is")
				{
					lexer.GetNextToken(token);
					type = GetTypeFromString(token.Token);
					if (type != type_Unknown || token.Token == "!0" || token.Token == "null")
					{
						output += "typeof " + name;
						if (type != type_Unknown)
						{
							string typeName = GetStringFromType(type);
							output += "=== \"" + typeName + "\"";
							if (type != s->type && s->type != type_Typeless)
							{
								error.logError("Static type check failed. " + name + " is type " + GetStringFromType(s->type) + ". Expected type " + typeName, lexer.GetLineNumber());
							}
						}
						else if (token.Token == "!0") output += "!== \"undefined\" && " + name + " !== null";
						else if (token.Token == "null") output += "=== \"undefined\" && " + name + " === null";
						type = type_Bool;
					}
					else
					{
						lexer.RewindStream();
						error.logError("Unexpected token '" + token.Token + "'. Expected a type after is", lexer.GetLineNumber());
					}
				}

				break;
			}
			else if (isFunc(s->type))
			{
				string funcOutput;
				SymType t = executeFunction(s, lexer, token, funcOutput, error);
				if (t != type_Unknown) type = t;
				output += funcOutput;
			}
			else if (token.Token == ".")
			{
				output += name + ".";
				lexer.GetNextToken(token);
				Symbol * m = NULL;
				pushScope();
				if (s->prototype)
				{
					pushAncestorArgs(s->prototype, false, error);
					if (!(m = findSymbol(token.Token))) error.logError("Unrecognized member  '" + token.Token + "'", lexer.GetLineNumber());
					//return type_Unknown;
				}
				else if (s->type == type_Obj)
				{
					pushAncestorArgs(s, false, error);
					if (!(m = findSymbol(token.Token))) error.logError("Unrecognized member  '" + token.Token + "'", lexer.GetLineNumber());
				}
				else if (m && !isPublic(m->type))
				{
					error.logError("Inaccessible memeber  '" + m->identifier + "' referenced on type " + s->prototype->identifier, lexer.GetLineNumber());
				}
				else if (stripLanguageModifiers(type) == type_Typeless)
				{
					popScope();
					lexer.RewindStream();
					type =  type_Typeless;
					s->type = changeValue(type, type_Obj);
					break;
				}

				{
					lexer.RewindStream();
					type = parseExpression(lexer, token, output, error);
					popScope();
					lexer.RewindStream();
				}
			}
			else if (token.Token == "=>")
			{
				lexer.GetNextToken(token);
				Symbol * m = findSymbol(token.Token);

				output += m->identifier + ".call(" + name;
				lexer.GetNextToken(token);
				if (token.Token != "(")
				{
					//Error
					error.logError("Expected token '(' at '" + token.Token + "'", lexer.GetLineNumber());
					break;
				}
				//Push the other params
				evalParams(m, lexer, token, output, error);
				if (token.Token != ")")
				{
					//Error
					error.logError("Expected token ')' at '" + token.Token + "'", lexer.GetLineNumber());
					break;
				}
				output += ")";
			}
			else
			{
				lexer.RewindStream();
				output += name;
			}

		}
		break;
		case lexmetype_Operator:

			if (token.Token == "(")
			{
				output += "(";
				if (type & type_Extern) {
					type = type_Typeless;
					s->type = applyModifier(type, type_ExternFunction);
					outputParameterList(lexer, token, output, error);
				}
				else
				{
					//Error
					error.logError("Unexpected token '" + token.Token + "'", ln);
				}

				if (token.Token == ")")
				{
					output += ")";
				}
				else
				{
					//Error
					error.logError("Expected token ')' at '" + token.Token + "'", lexer.GetLineNumber());
				}
			}
			else if (token.Token == "{")
			{
				lexer.RewindStream();
				parseObjectLiteral(lexer, token, output, error);
				type = type_Obj;
			}
			else if (token.Token == "=")
			{
				output += "=";
				evalEq(type, lexer, token, output, error);
				lexer.RewindStream();
				break;
			}
			else if (token.Token == "+")
			{
				if (type != type_Num && type != type_String)
				{
					error.logError("Syntax error, + with type ", lexer.GetLineNumber());
				}
				else
				{
					output += "+";
					parseExpression(lexer, token, output, error); //Change the type to that of the right hand
					lexer.RewindStream();
					//TODO: Follow operator precedence or something
				}
			}
			else if (token.Token == "*"
				|| token.Token == "-"
				|| token.Token == "/")
			{
				if (type != type_Num && type != type_String)
				{
					error.logError("Syntax error, + with type ", lexer.GetLineNumber());
				}
				else
				{
					output += token.Token;
					parseExpression(lexer, token, output, error); //Change the type to that of the right hand
					lexer.RewindStream();
					//TODO: Follow operator precedence or something
				}
			}
			else if (token.Token == ":")
			{
				//type declaration operator, changes the value of the left hand operator
				//this declaration will persist from here out
				if (type == type_Unknown)
				{
					error.logError("Syntax error, missing identifier", lexer.GetLineNumber());
				}
				else
				{
					lexer.GetNextToken(token);
					if (token.Type == lexmetype_Keyword)
					{
						Symbol *s = findSymbol(name);
						type = parseType(lexer, token, output, error);
						if (s->type != type_Typeless && type != s->type)
						{
							error.logError("Syntax error, redeclaration of type " + GetStringFromType(s->type) + " to type " + GetStringFromType(type), lexer.GetLineNumber());
						}
						else
						{
							s->type = type;
						}
					}
					else
					{
						error.logError("Syntax error, type expected following : ", lexer.GetLineNumber());
					}
				}

			}
			else if (token.Token == "::")
			{
				//static cast operator, changes the value of the left hand operator
				if (type == type_Unknown)
				{
					error.logError("Syntax error, missing identifier", lexer.GetLineNumber());
				}
				else
				{
					lexer.GetNextToken(token);
					if (token.Type == lexmetype_Keyword)
					{
						type = parseType(lexer, token, output, error);
					}
					else
					{
						error.logError("Syntax error, type expected following :: ", lexer.GetLineNumber());
					}
				}

			}
			else if (token.Token == "[")
			{
				output += token.Token;
				//we're parsing an array
				while (token.Token != "]")
				{
					int ln = lexer.GetLineNumber();

					if (token.Token == ",")
					{
						if (type == type_Unknown) error.logError("Unexpected token '" + token.Token + "'", ln);
						else output += token.Token;

						lexer.GetNextToken(token);
						if (token.Token == "]") break; //dangling comma
					}

					SymType t = parseExpression(lexer, token, output, error);
					if (type == type_Unknown) type = t;
					if (t != type) type = type_Typeless;
				}
				output += token.Token;
				type = applyModifier(type, type_Array);
			}
			else
			{
				error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
			}
			break;
		case lexmetype_Keyword:
			if (token.Token == "true"
				|| token.Token == "false")
			{
				type = type_Bool;
				output += token.Token;
			}
			else if (token.Token == "this")
			{
				type = type_Obj;
			}
			else if (token.Token == "is")
			{
				//TODO:
				type = type_Bool;
			}
			else if (token.Token == "jsonify")
			{
				output += "JSON.parse(";
				type = parseExpression(lexer, token, output, error);
				output += ")";
				if (type != type_String)
				{
					error.logError("Type mismatch, expected type string for jsonify operation", lexer.GetLineNumber());
				}
				type = type_Obj;
			}
			else if (token.Token == "stringify")
			{
				output += "JSON.stringify(";
				type = parseExpression(lexer, token, output, error);
				output += ")";
				if (type != type_Obj)
				{
					error.logError("Type mismatch, expected type object for striingify operation", lexer.GetLineNumber());
				}
				type = type_String;
			}
			else if (token.Token == "decltype")
			{
				type = type_String;
				int ln = lexer.GetLineNumber();

				//expect an identifier next
				string temp;
				lexer.GetNextToken(token);
				Symbol* s = findSymbol(token.Token);
				if (s && s->prototype)
				{
					output += "\"" + s->prototype->identifier + "\"";
					lexer.GetNextToken(token);
				}
				else
				{
					lexer.RewindStream();
					SymType t = parseExpression(lexer, token, temp, error);
					output += "\"" + parseDeclType(t) + "\"";
				}
				lexer.RewindStream();
			}
			else if (token.Token == "ext")
			{
			}
			else if (token.Token == "function"
				|| token.Token == "func") {
				type = parseFunction(lexer, token, output, error);
			}
			else if (token.Token == "obj" ||
				token.Token == "num" ||
				token.Token == "string" ||
				token.Token == "bool" ||
				token.Token == "typeless")
			{
				type = GetTypeFromString(token.Token);
				break;
			}
			else
			{
				//Error
				error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
				//ouputAndIgnore(token, output);
			}
			break;
		case lexmetype_String:
			output += token.Token;
			if (type == type_Unknown) type = type_String;
			else
			{
				//Error
				error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
			}
			break;
		case lexmetype_Int:
		case lexmetype_Float:
		case lexmetype_HexNumber:
			output += token.Token;
			if (type == type_Unknown) type = type_Num;
			else
			{
				//Error
				error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
			}
			break;
		case lexmetype_Unknown:
			//Error
			error.logError("Unexpected token '" + token.Token + "'", lexer.GetLineNumber());
			break;
		}

		//lexer.GetNextToken(token);
	}

	//if (token.Type != lexmetype_NewLine && token.Type != lexmetype_Comment)
	//{
	//	printf("Maybe you need to rewind after an expression?");
	//	//lexer.RewindStream();

	//}
	if (token.Token == ",")
	{
		lexer.RewindStream();
	}

	if (type == type_Unknown) {
		error.logError("Type resolution error", lexer.GetLineNumber());
	}
	return type;
}

void parseMacro(NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	//get macro from macro table
	//for each statement either
		// parse the next literal and add to putput
		//or parse the lexer as type
			//parse statment
			//or parse expression
			//or parse block
			//or parse paren
}

void parseMacroDef(NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	int ln = lexer.GetLineNumber();

	//get macro type
	if (token.Type == lexmetype_Identifier)
	{
		//look to see if this macro is defined already?.
		Symbol* s = findSymbol(token.Token);
		if (!s) s = symbolTable[token.Token];
		if (s)
		{
			error.logError("Macro " + token.Token + " is already defined as "+parseDeclType(s->type), ln);

		}
		else if (macroTable.find(token.Token) == macroTable.end())
		{
			vector<string> & macroDef = macroTable[token.Token];

			while (lexer.GetNextToken(token) && token.Token != "{")
			{
				//this will either be a literal or a macro var
			}
		}
		else
		{
			error.logError("Macro " + token.Token + " is already defined.", ln);
		}
	}
	else
	{
		error.logError("Unexpected symbol "+ token.Token, ln);
	}
}

void parseStatement(Symbol* functionSymbol, NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{

	while (lexer.GetNextToken(token))
	{
		if (error.isFatal()) return;

		switch (token.Type)
		{
		case lexmetype_Comment:
		case lexmetype_NewLine:
			break;
		case lexmetype_Keyword:
			if (token.Token == "class") {
				parseClass(lexer, token, output, error);
				break;
			}
			else if (token.Token == "interface") {
				parseInterface(lexer, token, output, error);
				break;
			}
			else if (token.Token == "if") 
			{
				output += token.Token;

				//if (token.Token == "(")
				//{
					output += "(";
					parseExpression(lexer, token, output, error);
					lexer.RewindStream();

					lexer.GetNextToken(token);
					//if (token.Token == ")")
					//{
						output += ")";
						lexer.GetNextToken(token);

						while (token.Type == lexmetype_NewLine) lexer.GetNextToken(token);
						if (token.Token == "{") {
							lexer.RewindStream();
							parseBlock(false, NULL, lexer, token, output, error);
						}
					//}

				//}
				break;
			}
			else if (token.Token == "while") 
			{
				parseExpression(lexer, token, output, error);
				break;
			}
			else if (token.Token == "switch") 
			{
				parseExpression(lexer, token, output, error);
				break;
			}
			else if (token.Token == "return") 
			{
				output += "return ";
				int ln = lexer.GetLineNumber();
				if (!functionSymbol)
				{
					error.logError("Unexpected return statement outside of function scope", ln);
					break;
				}
				SymType t = evalEq(getReturnType(functionSymbol->type), lexer, token, output, error);

				//infer the function type from the return
				if (stripLanguageModifiers(functionSymbol->type) & type_Typeless)
				{ 
					functionSymbol->type = changeValue(functionSymbol->type, t);
				}
				output += ";\n";
				lexer.RewindStream();
				break;
			}
			else if (token.Token == "let"
				|| token.Token == "const")
			{
				parseLetStatement(lexer, token, output, error);
				break;
			}
			else if (token.Token == "macro")
			{
				parseMacroDef(lexer, token, output, error);
				break;
			}
			else
			{
				lexer.RewindStream();
				parseExpression(lexer, token, output, error);
				output += ";\n";
				break;
			}
			break;
		case lexmetype_Identifier:
		{
			if (macroTable.find(token.Token) != macroTable.end())
			{
				vector<string>& macroDef = macroTable[token.Token];

				//parse the main lexer with a macroOuput
				string macroOutput;
				parseMacro(lexer, token, macroOutput, error);

				//now parse the macroOutput into the original output

				NRVLexer<int> lexer(&keywordTable, 0, delims);
				//NRVLexToken<int> token;
				lexer.SetSource(macroOutput.c_str());
				parseStatement(NULL, lexer, token, output, error);

				break;
			}

			//is this a symbol we know?
			Symbol *s = findSymbol(token.Token);

			//valid statements following an identifier are
			//declaration: foo := v6
			//assignment: foo = bar
			//member access: foo.bar
			//function call: foo(bar)
			//nothing: foo
			//is: foo is bar
			//operator: foo + bar
			if (s)
			{
				lexer.RewindStream();
				parseExpression(lexer, token, output, error);
				output += ";\n";
			}
			else
			{
				//try to declare a new var
				lexer.RewindStream();
				if (!parseDeclAssignStatement(lexer, token, output, error))
				{
					//otherwise, parse it as an expression
					parseExpression(lexer, token, output, error);
					output += ";\n";
				}
			}
			break;
		}

		break;
		case lexmetype_Operator:
			if (token.Token == "(")
			{
				output += "(";
				parseExpression(lexer, token, output, error);
				lexer.RewindStream();
				break;
			}
			else if (token.Token == ")")
			{
				output += ");\n";
				break;
			}
			else if (token.Token == "}")
			{
				lexer.RewindStream();
				return;
			}
			//else
			//{
			//	ouputAndIgnore(token, output);
			//}
			break;

		default:
			lexer.RewindStream();
			parseExpression(lexer, token, output, error);
			output += ";\n";
			break;
		}

		//lexer.GetNextToken(token);
	}
}

void parseParameter(Symbol *symbol, NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	string name;
	string defaultValue;
	SymType type = type_Unknown;
	Symbol* protoype = NULL;
	bool isArray = false;
	bool isvArg = false;

	int ln = lexer.GetLineNumber();

	//lexer.GetNextToken(token);
	while( !lexer.EOS() && token.Token != ")" && token.Token != ",")
	{
		if(error.isFatal()) return;

		switch(token.Type)
		{
			case lexmetype_Comment:
			case lexmetype_NewLine:
				break;
			case lexmetype_Keyword:
				if(type != type_Unknown && name.empty())
				{
					//Error
					error.logError("Unexpected token '"+token.Token+"'", ln);
					return;
				}

				type = parseType(lexer, token, output, error);
				break;
			case lexmetype_Identifier:
				//This should be the var name
				if (type == type_Unknown && symbolTable.find(token.Token) != symbolTable.end())
				{
					Symbol*s = symbolTable[token.Token];
					if (s && (isPrototype(s->type) || isInterface(s->type)))
					{
						protoype = s;
						type = type_Obj;
					}

					break;
				}
				else if(!name.empty())
				{
					//Error
					error.logError("Unexpected token '"+token.Token+"'", ln);
					return;
				}

				name = token.Token;
				output += name;



				break;
			case lexmetype_Operator:
				if (token.Token == "..."  && name.empty())
				{
					isvArg = true;
					isArray = true;
					type = type_Typeless;
					break;
				}
				else if (token.Token == "=")
				{
					SymType t = parseExpression(lexer, token, defaultValue, error);
					if (type == type_Unknown)
					{
						type = t;
					}
					else if (stripLanguageModifiers(type) != type_Typeless && stripLanguageModifiers(t) != stripLanguageModifiers(type))
					{
						defaultValue = "";
						error.logError("Default value of type " + GetStringFromType(t) + " differs from declaration type " + GetStringFromType(type), ln);
					}
					lexer.RewindStream();
					break;
				}
				else
				{
					error.logError("Unexpected spread operator", ln);
				}
			case lexmetype_String:
			case lexmetype_Int:
			case lexmetype_Float:
			case lexmetype_HexNumber:
			case lexmetype_Unknown:
				//Error
				error.logError("Unexpected token '"+token.Token+"'", ln);
				return;
		}

		lexer.GetNextToken(token);
	}

	if(name.empty())
	{
		//Error
		error.logError("Unexpected token '"+token.Token+"'", ln);
		return;
	}

	symbol->identifier = name;
	symbol->defaultValue = defaultValue;
	symbol->type = type;
	symbol->type = applyModifier(symbol->type, isArray ? type_Array : type_NoOp);
	symbol->type = applyModifier(symbol->type, isvArg ? type_VArg : type_NoOp);
	symbol->prototype = protoype;
}


void parseBlock(bool isObj, Symbol* functionSymbol, NRVLexer<int>& lexer, NRVLexToken<int>& token, string& output, Error& error)
{
	bool funcParse = false;

	while (lexer.GetNextToken(token))
	{
		if (error.isFatal()) return;

		switch (token.Type)
		{
		case lexmetype_NewLine:
			break;
		case lexmetype_Keyword:
		case lexmetype_Identifier:
			lexer.RewindStream();
			parseStatement(functionSymbol, lexer, token, output, error);
			break;
		case lexmetype_Operator:
			if(token.Token == "{")
			{
				output += "{";

				//entering scope
				pushScope();
				if(functionSymbol)
				{
					//get the function and push it's params into scope
					if (functionSymbol->interface)
					{
						for (uint32 i = 0; i < functionSymbol->interface->arguments.size(); i++)
						{
							addSymbol(functionSymbol->interface->arguments[i]);
							functionSymbol->interface->arguments[i]->prefix = "this.";
						}
					}
					pushArguments(functionSymbol->arguments, error);

					//returnStack.push_back(functionSymbol);
				}
				else
				{
					//returnStack.push_back(&s_TypeUnknownSym);
				}
			
				string k = output;
				parseBlock(isObj, functionSymbol, lexer, token, output, error);
			
			
				lexer.GetNextToken(token);
				if (token.Token == "}"){
					//exiting scope
					if(functionSymbol) cleanAncestorArgs(functionSymbol->interface);
					popScope();
					//returnStack.pop_back();
					output += "}\n";
				}

				return;
			
			}
			else if (token.Token == "}") {
				lexer.RewindStream();
				return;
			}
			break;
		}
	}
}

int main()
{
	//Setup the keywords
	keywordTable.insert("ext");
	keywordTable.insert("declext");
    keywordTable.insert("namespace");
	keywordTable.insert("class");
	keywordTable.insert("interface");
	keywordTable.insert("extends");
	keywordTable.insert("implements");
	keywordTable.insert("public");
	keywordTable.insert("private");
	keywordTable.insert("this");
	keywordTable.insert("let");
	keywordTable.insert("const");
    keywordTable.insert("function");
	keywordTable.insert("func");
	keywordTable.insert("var");
    keywordTable.insert("return");
	keywordTable.insert("void");
    keywordTable.insert("string");
    keywordTable.insert("num");
    keywordTable.insert("obj");
    keywordTable.insert("bool");
    keywordTable.insert("typeless");
	keywordTable.insert("is");
	keywordTable.insert("decltype");
    keywordTable.insert("jsonify");
    keywordTable.insert("stringify");
	keywordTable.insert("true");
	keywordTable.insert("false");
	keywordTable.insert("if");
	keywordTable.insert("while");
	keywordTable.insert("switch");
	keywordTable.insert("do");
	keywordTable.insert("macro");


	//s_TypeUnknownSym.type = type_Unknown;


    //Setup the symbol pool
    SymbolPool.alloc(1000);

	FILE* f = NULL;
	fopen_s(&f, "test.prom", "rt");
	//fopen_s(&f, "promise.prom", "rt");

	size_t len = filelength(f);

	if(len)
	{
		char* input = new char[len];
		string output;
		output.reserve(len);

		len = fread(input, 1, len, f);
		input[len] = 0;

		NRVLexer<int> lexer(&keywordTable, 0, delims);
		NRVLexToken<int> token;

		lexer.SetSource(input);

		//state vars
		Error error;
		parseBlock(false, NULL, lexer, token, output, error);

		FILE *out = NULL;
		fopen_s(&out, "test.js", "wt");
		if (!error.warnings.empty())
		{
			fwrite("/*\n", 1, 3, out);
			for (uint32 i = 0; i < error.warnings.size(); i++)
			{
				fwrite(error.warnings[i].c_str(), 1, error.warnings[i].size(), out);
			}
			fwrite("*/\n", 1, 3, out);
		}
		if(!error.errors.empty())
		{
			fwrite( "/*\n", 1, 3, out );
			for(uint32 i = 0; i < error.errors.size(); i++)
			{
				fwrite( error.errors[i].c_str(), 1, error.errors[i].size(), out );
			}
			fwrite( "*/\n", 1, 3, out );
		}
		fwrite( output.c_str(), 1, output.size(), out );
		fclose(f);
	}
	fclose(f);





	//start parsing


	//cleanup
	SymbolPool.dealloc();

}
