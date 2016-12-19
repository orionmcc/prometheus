////////////////////////////////////////////////////////////////////////////////////////////
//
// File: NRVLexer.h
//
// Author: Orion McClelland (OM)
//
// Creation: February 17, 2004
//
//
// Purpose: to create a recepticle for all of the particle effects
//			
//
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef _NRV_LEXER_H__
#define _NRV_LEXER_H__

//#include "library.h"
#include <ctype.h>
#include <set>


enum NRVLexState{
	lexstate_Start,
	lexstate_Int,
	lexstate_Float,
	lexstate_HexNumber,
	lexstate_String,
	lexstate_Escape_String,
	lexstate_Identifier,
	lexstate_Operator,
	lexstate_Unknown,
	lexstate_End,
};


enum NRVLexmeType{
	lexmetype_Keyword,
	lexmetype_Int,
	lexmetype_Float,
	lexmetype_HexNumber,
	lexmetype_String,
	lexmetype_Identifier,
	lexmetype_Operator,
	lexmetype_NewLine,
	lexmetype_Comment,
	lexmetype_Unknown,
};



template <typename T>
struct NRVLexToken{
	NRVLexmeType	Type; //The type of the token
	std::string		Token; //The actual token
	T				ID; //If an opcode was found, which is it?
};


//Ths size of the buffer allocated when using file mode
//#define FILE_BUFFER_SIZE	2048


template <typename T>
class NRVLexer
{
public:
	NRVLexer( set<std::string>*  keyword_defs, T error_t, const char * delimiters );
	virtual ~NRVLexer(void);

	//
	//////////////////////////////////////////////////////////////////////////////////////
	//
	// Function: "SetSource"
	//
	// Last Modified: Jan 14, 2004
	//
	// Input: line - pointer to a char buffer
	// 
	// Return: void
	//
	// Purpose:  Read a string into the lexer.
	//
	//////////////////////////////////////////////////////////////////////////////////////
	void	SetSource( const char * line );
	//
	//////////////////////////////////////////////////////////////////////////////////////
	//NRVLibrary<T>
	// Function: "SetSource"
	//
	// Last Modified: Feb 4, 2004
	//
	// Input: f - pointer to a file
	// 
	// Return: void
	//
	// Purpose:  Set the source as a file pointer.  
	//
	//////////////////////////////////////////////////////////////////////////////////////
	void	SetSource( FILE * f );

	//
	//////////////////////////////////////////////////////////////////////////////////////
	//
	// Function: "GetNextToken"
	//
	// Last Modified: Feb 4, 2004
	//
	// Input: token - refrence to a token to fill
	// 
	// Return: bool - is the stream empty?
	//
	// Purpose:  Get The Next Token in the stream
	//returns false if the stream has been read through  
	//
	//////////////////////////////////////////////////////////////////////////////////////
	bool GetNextToken( NRVLexToken<T> & token );

	//
	//////////////////////////////////////////////////////////////////////////////////////
	//
	// Function: "GetLookAheadChar"
	//
	// Last Modified: Feb 4, 2004
	//
	// Input: NONE
	// 
	// Return: char - the next char
	//
	// Purpose:  Get the look ahead character.  
	//
	//////////////////////////////////////////////////////////////////////////////////////
	char GetLookAheadChar();

	//
	//////////////////////////////////////////////////////////////////////////////////////
	//
	// Function: "ReadToken"
	//
	// Last Modified: Feb 4, 2004
	//
	// Input: token - the token to compare the next token to
	// 
	// Return: bool - is the next token equal to the input
	//
	// Purpose:  Is the next token this token? 
	//
	//////////////////////////////////////////////////////////////////////////////////////
	bool ReadToken( const char * token );


	//
	//////////////////////////////////////////////////////////////////////////////////////
	//
	// Function: "RewindStream"
	//
	// Last Modified: Feb 4, 2004
	//
	// Input: NONE
	// 
	// Return: void
	//
	// Purpose:  Rewind the token stream by one token.  
	//
	//////////////////////////////////////////////////////////////////////////////////////
    void RewindStream();

	//
	//////////////////////////////////////////////////////////////////////////////////////
	//
	// Function: "EatCharacters"
	//
	// Last Modified: Feb 4, 2004
	//
	// Input: delim - the character to signify to stop at
	// 
	// Return: void
	//
	// Purpose:  Continually eats characters from the stream until a delimiter character is found
	//or it reaches the end of the stream 
	//
	//////////////////////////////////////////////////////////////////////////////////////
	void EatCharacters( const char * delim );

	//
	//////////////////////////////////////////////////////////////////////////////////////
	//
	// Function: "EOS"
	//
	// Last Modified: Feb 4, 2004
	//
	// Input: NONE
	// 
	// Return: bool - is it the end of file
	//
	// Purpose:  Has the stream run short?
	//Call before Rewind 
	//
	//////////////////////////////////////////////////////////////////////////////////////
	bool EOS(){
		return CurChar == '\0';
	}

	//
	//////////////////////////////////////////////////////////////////////////////////////
	//
	// Function: "GetLineNumber"
	//
	// Last Modified: Feb 4, 2004
	//
	// Input: NONE
	// 
	// Return: int - the current line number
	//
	// Purpose:  line# info.  
	//
	//////////////////////////////////////////////////////////////////////////////////////
	int GetLineNumber(){ return LineNo; }
	int GetPreviousLineNumber() { return NumNewLines; }

	//////////////////////////////////////////////////////////////////////////////////////
	//
	// Function: "SetLineNumber"
	//
	// Last Modified: Feb 4, 2004
	//
	// Input: lineno - the line to set to
	// 
	// Return: void
	//
	// Purpose:  Set the Line number.  
	//
	//////////////////////////////////////////////////////////////////////////////////////
	void SetLineNumber( int lineno ) { LineNo = lineno; }

	void SaveState(){
		SaveStatePointer = CurPointer;
		SaveStateLineNo = LineNo;
	}
	void RestoreState(){
		CurPointer = SaveStatePointer;
		LineNo = SaveStateLineNo;
		CurChar = *CurPointer;
	}


private:
	//read the next character in the list
	bool	GetNextChar();


	//////////////////////////////////////////////////////////////////////////////////////
	//
	// Function: "SaveStreamPosition"
	//
	// Last Modified: Feb 4, 2004
	//
	// Input: NONE
	// NRVLibrary<T>
	// Return: void
	//
	// Purpose:  Save the current position in the stream.  
	//
	//////////////////////////////////////////////////////////////////////////////////////
	void SaveStreamPosition();

	//private members
	const char * LexString;
	FILE * FileSource;

	enum STREAM_SOURCE{ sFILE, sSTRING, sNONE } Source;

	const char * CurPointer;
	const char * LastTokenStart;
	const char * SaveStatePointer;
	char	CurChar;
	//char	NextChar;//Used only in File mode
	unsigned int BytesRead;//Used to roll back in file mode
	//int		LastFileToken;
	char Delimiters[16];
	std::string	Token;

	//the current stsate the lexer is in
	NRVLexState	LexState;

	//Pointer to a NRVLibrary defining the opcodes
	std::set<std::string>		*OpLib;

	//The error code returned by OpLib on failure to find;
	T					ErrorT;

	//the line that the lexer is currently on
	int					LineNo;
	int					SaveStateLineNo;

	//number of new lines we've come across
	int					NumNewLines;

	//Is the current char a digit?
	bool	IsNumeric();

	//Is the current char a delimiter?
	bool	IsDelimiter();

	//Is the current char an operator?
	bool	IsOperator();

	//Is the current char an escape sequence?
	bool	IsEscapeChar();

	//Does the current char make a double operator
	bool	IsDoubleOperator( char c );

	//Generates an escape sequence
	char	GenerateEscapeSequence( char c );

};


template <typename T>
NRVLexer<T>::NRVLexer( set<std::string> * keyword_defs, T error_t, const char * delimiters ) : Source( sNONE ), LexString(NULL), CurPointer( NULL ), CurChar('\0'),LastTokenStart(NULL), LexState(lexstate_Start), OpLib(NULL), BytesRead(0), LineNo(1){
	ErrorT = error_t;
	if( keyword_defs != NULL )
		OpLib = keyword_defs;

	strcpy_s(Delimiters, sizeof(Delimiters), delimiters);
}

template <typename T>
NRVLexer<T>::~NRVLexer(void){
	//delete the lib
	if( OpLib ){
		//delete OpLib;
	}

	//delete the lexstring<ctype.h>
	if( Source == sFILE ){
		delete[] LexString;
	}
}

//Read a string into the lexer
template <typename T>
void NRVLexer<T>::SetSource( const char * line ){

	//if the source was previously a file, 
	//and is being changed, delete the buffer
	if( Source == sFILE )
		delete[] LexString;

	LexString = CurPointer = SaveStatePointer = LastTokenStart = line;
	CurChar = *CurPointer;

	//CurPointer++;
	Source = sSTRING;

	//reset the LineNo
	LineNo = 1;
	if (CurChar == '\n') LineNo++;
	GetNextChar();
}

//Set the source as a file pointer
template <typename T>
void NRVLexer<T>::SetSource( FILE * f ){

	unsigned int uSize;
	fseek( f, SEEK_END, 0 );
	uSize = ftell( f );
	fseek( f, SEEK_SET, 0 );

	//create a buffer with the LexString
	if( Source != sFILE )
		LexString = new char[ uSize ];

	//fill the LexString with a portion of the file
	BytesRead = fread( LexString, 1, uSize, f );

	//If less than FILE_BUFFER_SIZE was read, cut the LexString off
	if( BytesRead )
		LexString[BytesRead] = '\0';

	FileSource = f;

	CurPointer = LastTokenStart = LexString;
	CurChar = *CurPointer;
	Source = sFILE;

	//reset the LineNo
	LineNo = 1;
	//CurPointer = &NextChar;
	////get the first char
	//if( ( CurChar = fgetc(FileSource) ) != EOF ){
	//	if( (NextChar = fgetc(FileSource)) == EOF ){
	//		NextChar = '\0';
	//		BytesRead = 1;
	//	}
	//	BytesRead = 2;
	//}else{
	//	BytesRead = 0;
	//	CurChar = '\0';
	//	NextChar = '\0';
	//}

}

//Get The Next Token in the stream
//returns false if the stream has been read through
template <typename T>
bool NRVLexer<T>::GetNextToken( NRVLexToken<T> & token ){

	if( !LexString ) return false;
	if( CurChar == '\0' ) return false;

	SaveStreamPosition();

	while( LexState != lexstate_End ){

		switch( LexState ){
			case lexstate_End:
				break;
			case lexstate_Start:

				while( IsDelimiter() ){ if(!GetNextChar()) return false; }

				if(  CurChar == '\n' ){
					Token += CurChar;

					LexState = lexstate_End;
					token.Type = lexmetype_NewLine;
					token.Token = Token;
					GetNextChar();
					break;
				}

				//Comment blocks
				if(  CurChar == '/' && GetLookAheadChar() == '*' ){
					GetNextChar(); //skip the comment
					GetNextChar();
					int nest = 1;
					while( CurChar && nest ){
						if (CurChar == '/' && GetLookAheadChar() == '*')
						{
							GetNextChar();
							nest++;
						} 
						else if (CurChar == '*' && GetLookAheadChar() == '/')
						{
							GetNextChar();
							nest--;
						}
						//Token += CurChar;
						GetNextChar();
					}

					LexState = lexstate_Start;
					//token.Type = lexmetype_Comment;
					//token.Token = Token;
					//GetNextChar();
					break;
				}

				//Comment line
				if(  CurChar == '/' && GetLookAheadChar() == '/' ){
					GetNextChar(); //skip the comment
					GetNextChar();
					while (CurChar != '\n' && CurChar != '\0'){
						Token += CurChar;
						GetNextChar();
					}

					LexState = lexstate_Start;
					//token.Type = lexmetype_Comment;
					//token.Token = Token;
					//Don't eat the newline
					break;
				}

				//start of an identifier
				if( isalpha(CurChar) || CurChar == '_' ){
					Token += CurChar;
					LexState = lexstate_Identifier;
					GetNextChar();
					break;
				}

				//start of a hex number
				if( CurChar == '0' && GetLookAheadChar() == 'x' ){
					Token += CurChar;
					GetNextChar();
					Token += CurChar;
					LexState = lexstate_HexNumber;
					GetNextChar();
					break;
				}

				//start of an integer
				if( isdigit( CurChar ) || CurChar == '-' || CurChar == '+' ){
					Token += CurChar;
					LexState = lexstate_Int;
					GetNextChar();
					break;
				}

				//start of an operator
				if( IsOperator() ){
					Token += CurChar;
					LexState = lexstate_Operator;
					GetNextChar();
					break;
				}

				//start of a float
				if( CurChar == '.' ){
					Token += CurChar;
					LexState = lexstate_Float;
					GetNextChar();
					break;
				}

				//start of a string
				if( CurChar == '"' ){
					Token += CurChar;
					LexState = lexstate_String;
					GetNextChar();
					break;
				}

				//end of the stream
				if( CurChar == '\0' )
					return false;

				//no idea what this is
				Token += CurChar;
				LexState = lexstate_Unknown;
				GetNextChar();

				break;
			case lexstate_Int:

				//if the next char is a digit, add it and continue
				if( isdigit( CurChar ) ){
					Token += CurChar;
					GetNextChar();
					break;
				}

				//if the next char is an int, add it and continue
				if( CurChar == '.' ){
					Token += CurChar;
					LexState = lexstate_Float;
					GetNextChar();
					break;
				}


				//if this character is an 'f' or and 'F', it is a finalized float
				if( CurChar == 'f' || CurChar == 'F' ){
					Token += CurChar;
					LexState = lexstate_End;
					token.Type = lexmetype_Float;
					token.Token = Token;
					token.ID = ErrorT;
					GetNextChar();
					break;
				}

				//if the current char is a delimiter, operator, or the end of the stream
				//finalize the token
				if (IsDelimiter() || IsOperator() || CurChar == '\0' || CurChar == '\n'){
					LexState = lexstate_End;

					if(Token == "+" || Token == "-") token.Type = lexmetype_Operator;
					else token.Type = lexmetype_Int;
					token.Token = Token;
					token.ID = ErrorT;
					break;
				}

				//anything else creates an unkown type
				Token += CurChar;
				LexState = lexstate_Unknown;
				GetNextChar();
				break;
			case lexstate_Float:

				//if the next char is a digit, add it and continue
				if( isdigit( CurChar ) ){
					Token += CurChar;
					GetNextChar();
					break;
				}

				//if this character is an 'f' or and 'F' or it is a delimiter, operator, or the end of the stream
				//finalize the token
				if (CurChar == 'f' || CurChar == 'F' || IsDelimiter() || IsOperator() || CurChar == '\0' || CurChar == '\n'){
					Token += CurChar;
					LexState = lexstate_End;
					token.Type = lexmetype_Float;
					token.Token = Token;
					token.ID = ErrorT;
					if( CurChar == 'f' || CurChar == 'F' )
						GetNextChar();
					break;
				}

				//anything else creates an unkown type
				Token += CurChar;
				LexState = lexstate_Unknown;
				GetNextChar();
				break;

			case lexstate_String:

				//If we have a backslash (\), it is an escape sequence
				if( CurChar == '\\' ){
					LexState = lexstate_Escape_String;
					GetNextChar();
					break;
				}

				//If we have a closing quote, end the string and finalize
				if( CurChar == '\"' ){
					Token += CurChar;
					LexState = lexstate_End;
					token.Type = lexmetype_String;
					token.Token = Token;
					token.ID = ErrorT;
					GetNextChar();
					break;
				}

				//if there are no other tokens in the stream, there is an issue
				if( CurChar == '\0' ){
					LexState = lexstate_End;
					token.Type = lexmetype_Unknown;
					token.Token = Token;
					token.ID = ErrorT;
					break;
				}

				//anything else gets added to the token
				Token += CurChar;
				LexState = lexstate_String;
				GetNextChar();
				break;
			case lexstate_Escape_String:
				//if it is an escape sequence
				if( IsEscapeChar() ){
					LexState = lexstate_String;
					//Token += GenerateEscapeSequence( CurChar );
					Token += '\\';
					Token += CurChar;
					GetNextChar();
					break;
				}

				//not an escape sequence, but let it through
				if( isalpha(CurChar) ){
					LexState = lexstate_String;
					Token += CurChar;
					GetNextChar();
					break;
				}

				//if there are no other tokens in the stream, there is an issue
				if( CurChar == '\0' ){
					LexState = lexstate_End;
					token.Type = lexmetype_Unknown;
					token.Token = Token;
					token.ID = ErrorT;
					break;
				}

				//if there is a token, generate an escape
				//anything else creates an unkown type
				Token += CurChar;
				LexState = lexstate_Unknown;
				GetNextChar();
				break;
			case lexstate_Identifier:

				//any alpha numeric or underscore can be added to the identifier
				if( isalpha(CurChar) || CurChar == '_' || IsNumeric() ){
					Token += CurChar;
					GetNextChar();
					break;
				}

				//if the current char is a delimiter, operator, or the end of the stream
				//finalize the token
				if( IsDelimiter() || IsOperator() || CurChar == '\0' || CurChar == '\n' ){

					//check to see if what we have is really a keyword
					if( OpLib ){
						if( OpLib->find(Token) != OpLib->end() ){
							//success, a keyword!
							LexState = lexstate_End;
							token.Type = lexmetype_Keyword;
							token.Token = Token;
							//token.ID = res;
							break;
						}
					}

					//otherwise, it's just an identifier
					LexState = lexstate_End;
					token.Type = lexmetype_Identifier;
					token.Token = Token;
					break;
				}

				//anything else creates an unkown type
				Token += CurChar;
				LexState = lexstate_Unknown;
				GetNextChar();
				break;
			case lexstate_Operator:

				//if the next token is a character, see if it is a valid double
				//character...
				if (IsOperator() || CurChar == '0'){
					if (IsDoubleOperator(*Token.c_str())){
						Token += CurChar;
						GetNextChar();
						if (Token == ".." && CurChar == '.') //triple operator
						{
							Token += CurChar;
							GetNextChar();
						}
						LexState = lexstate_End;
						token.Type = lexmetype_Operator;
						token.Token = Token;
						break;
					}
				}

				//otherwise, finalize as a single operator
				LexState = lexstate_End;
				token.Type = lexmetype_Operator;
				token.Token = Token;
				break;
			case lexstate_Unknown:
				//if the current char is a delimiter, operator, or the end of the stream
				//finalize the token
				if (IsDelimiter() || IsOperator() || CurChar == '\0' || CurChar == '\n'){
					//check to see if what we have is really a keyword
					if( OpLib ){
						if( OpLib->find(Token) != OpLib->end() ){
							//success, a keyword!
							LexState = lexstate_End;
							token.Type = lexmetype_Keyword;
							token.Token = Token;
							//token.ID = res;
							break;
						}
					}

					LexState = lexstate_End;
					token.Type = lexmetype_Unknown;
					token.Token = Token;
					token.ID = ErrorT;
					GetNextChar();
					break;
				}

				//We don't know what this is, so just keep adding chars, and maybe it will
				//be a keyword or something
				Token += CurChar;
				GetNextChar();
				break;
			case lexstate_HexNumber:
				//if it is a hexdigit, add it
				if( isxdigit(CurChar) ){
					Token += CurChar;
					GetNextChar();
					break;
				}

				//if the current char is a delimiter, operator, or the end of the stream
				//finalize the token
				if (IsDelimiter() || IsOperator() || CurChar == '\0' || CurChar == '\n'){
					LexState = lexstate_End;
					token.Type = lexmetype_HexNumber;
					token.Token = Token;
					token.ID = ErrorT;
					break;
				}

				//anything else creates an unkown type
				Token += CurChar;
				LexState = lexstate_Unknown;
				GetNextChar();
				break;
		}

	}

	//reset the LexState
	LexState = lexstate_Start;
	//clear the token
	Token.clear();

	return true;
}

//Get the next character
template <typename T>
bool NRVLexer<T>::GetNextChar(){
	/*if( Source == sSTRING ){
		if( *CurPointer == '\0' )
			return false;

		CurChar = *CurPointer;
		CurPointer++;
		return true;
	}else{
		CurChar = NextChar;
		if( NextChar != '\0' ){
			if( (NextChar = fgetc(FileSource)) == EOF ){
				NextChar = '\0';
			}
			BytesRead++;
			return true;
		}else
			return false;
	}*/

	//if we have run past the buffer, and need to read more bytes, do that now
	//if( CurPointer - LexString == FILE_BUFFER_SIZE ){
	//	//move back one byte, this is done to keep the look ahead 
	//	//character accurate
	//	fseek( FileSource, -1, SEEK_CUR );
	//	BytesRead = fread( LexString, 1, FILE_BUFFER_SIZE, FileSource );
	//	//if we read less than the size of FILE_BUFFER_SIZE, we reached the end
	//	//of the file stream somewhere, so mark the end of the string
	//	LexString[BytesRead] = '\0';
	//	//reset the various pointers
	//	CurPointer = LexString;
	//	LastTokenStart = LexString;
	//}

	if( *CurPointer == '\0' ){
		CurChar = '\0';
		return false;
	}

	CurChar = *(++CurPointer);
	if( CurChar == '\n' )
	{
		LineNo++;
		NumNewLines;
	}
	//CurPointer++;
	return true;
}

//Get the look ahead character
template <typename T>
char NRVLexer<T>:: GetLookAheadChar(){
	if( CurChar == '\0' )
		return '\0';
	return *(CurPointer+1);
}

//Is the next token this token?
template <typename T>
bool NRVLexer<T>::ReadToken( const char * token ){
	NRVLexToken<T> Tok;
	GetNextToken( Tok );
	if( Tok.Token.compare( token ) == 0 )
		return true;
	return false;
}

//Rewind the token stream by one token
template <typename T>
void NRVLexer<T>::RewindStream(){

	CurPointer = LastTokenStart;
	CurChar = *CurPointer;
	LineNo = NumNewLines;

	//if( Source == sSTRING ){
	//	CurPointer = LastTokenStart;
	//}else{
 //       fseek( FileSource, -(BytesRead - LastFileToken), SEEK_CUR );
	//	BytesRead = LastFileToken;
	//	//refill the current and next tokens
	//	if( ( CurChar = fgetc(FileSource) ) != EOF ){
	//		if( (NextChar = fgetc(FileSource)) == EOF ){
	//			NextChar = '\0';
	//			//BytesRead += 1;
	//		}
	//		//BytesRead += 2;
	//	}else{
	//		BytesRead = 0;
	//		CurChar = '\0';
	//		NextChar = '\0';
	//	}
	//}
}

//Continually eats characters from the stream until a delimiter character is found
//or it reaches the end of the stream
template <typename T>
void NRVLexer<T>::EatCharacters( const char *delim ){
	while( !strchr(delim, CurChar ) || CurChar == '\0' ){ GetNextChar(); }
}

//Is the current char a digit?
template <typename T>
bool NRVLexer<T>::IsNumeric() {
	return CurChar >= '0' && CurChar <= '9';
}

//Is the current char a delimeter?
template <typename T>
bool NRVLexer<T>::IsDelimiter(){
	return strchr( Delimiters, CurChar ) != NULL;
}

//Is the current char an operator?
template <typename T>
bool NRVLexer<T>::IsOperator(){
	return strchr( "+-/*=><&%!(){}[]:;',.^", CurChar ) != NULL;
}

//Save the current position in the stream
template <typename T>
void NRVLexer<T>::SaveStreamPosition(){
	LastTokenStart = CurPointer;
	NumNewLines = LineNo;
	//LastFileToken = BytesRead;
}

//Is the current char an escape sequence?
template <typename T>
bool NRVLexer<T>::IsEscapeChar(){
	return strchr( "btn\\vfa\'\"0", CurChar ) != NULL;
}

//Does the current char make a double operator
template <typename T>
bool NRVLexer<T>::IsDoubleOperator( char c ){
	switch( c ){
		case '+':
			if( CurChar == '+' ) return true;
			if( CurChar == '=' ) return true;
			break;
		case '-':
			if( CurChar == '-' ) return true;
			if( CurChar == '=' ) return true;
			break;
		case '/':
			if( CurChar == '=' ) return true;
			break;
		case '*':
			if( CurChar == '=' ) return true;
			break;
		case '>':
			if( CurChar == '<' ) return true;
			if( CurChar == '=' ) return true;
			break;
		case '<':
			if( CurChar == '>' ) return true;
			if( CurChar == '=' ) return true;
			break;
		case '=':
			if( CurChar == '=' ) return true;
			if( CurChar == '>' ) return true;
			break;
		case '&':
			if (CurChar == '=') return true;
			break;
		case '!':
			if (CurChar == '=') return true;
			if (CurChar == '0') return true;
			break;
		case '.':
			if (CurChar == '.') return true;
			break;
		case ':':
			if (CurChar == ':') return true;
			if (CurChar == '=') return true;
			break;
		case '^':
			if (CurChar == '=') return true;
			break;
	}
	return false;
}

template <typename T>
char NRVLexer<T>::GenerateEscapeSequence( char c ){
	switch( c ){
		case 'b':
			return '\b';
		case 't':
			return '\t';
		case 'n':
			return '\n';
		case '\\':
			return '\\';
		case 'v':
			return '\v';
		case 'r':
			return '\r';
		case 'f':
			return '\f';
		case 'a':
			return '\a';
		case '\'':
			return '\'';
		case '\"':
			return '\"';
		case '0':
			return '\0';
		default :
				return 'c';
	}
}


#endif//_NRV_LEXER_H__
