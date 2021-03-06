//
//  ParseStmt.cpp
//  uscc
//
//  Implements all of the recursive descent parsing
//  functions for statement grammar rules.
//
//---------------------------------------------------------
//  Copyright (c) 2014, Sanjay Madhav
//  All rights reserved.
//
//  This file is distributed under the BSD license.
//  See LICENSE.TXT for details.
//---------------------------------------------------------

#include "Parse.h"
#include "Symbols.h"

using namespace uscc::parse;
using namespace uscc::scan;

using std::shared_ptr;
using std::make_shared;

shared_ptr<ASTDecl> Parser::parseDecl()
{
	shared_ptr<ASTDecl> retVal;
	// A decl MUST start with int or char
	if (peekIsOneOf({Token::Key_int, Token::Key_char}))
	{
		Type declType = Type::Void;
		if (peekToken() == Token::Key_int)
		{
			declType = Type::Int;
		}
		else
		{
			declType = Type::Char;
		}
		
		consumeToken();
		
		// Set this to @@variable for now. We'll later change it
		// assuming we parse the identifier properly
		Identifier* ident = mSymbols.getIdentifier("@@variable");
		
		// Now we MUST get an identifier so go into a try
		try
		{
			if (peekToken() != Token::Identifier)
			{
				throw ParseExceptMsg("Type must be followed by identifier");
			}
			
			
			ident = mSymbols.createIdentifier(getTokenTxt());
			
			consumeToken();
			
			// Is this an array declaration?
			if (peekAndConsume(Token::LBracket))
			{
				shared_ptr<ASTConstantExpr> constExpr;
				if (declType == Type::Int)
				{
					declType = Type::IntArray;
					
					// int arrays must have a constant size defined,
					// because USC doesn't support initializer lists
					constExpr = parseConstantFactor();
					if (!constExpr)
					{
						reportSemantError("Int arrays must have a defined constant size");
					}
					
					if (constExpr)
					{
						int count = constExpr->getValue();
						if (count <= 0 || count > 65536)
						{
							reportSemantError("Arrays must have a min of 1 and a max of 65536 elements");
						}
						ident->setArrayCount(count);
					}
					else
					{
						ident->setArrayCount(0);
					}
				}
				else
				{
					declType = Type::CharArray;
					
					// For character, we support both constant size or
					// implict size if it's assigned to a constant string
					constExpr = parseConstantFactor();
					if (constExpr)
					{
						int count = constExpr->getValue();
						if (count <= 0 || count > 65536)
						{
							reportSemantError("Arrays must have a min of 1 and a max of 65536 elements");
						}
						ident->setArrayCount(count);
					}
					else
					{
						// We'll determine this later in the parse
						ident->setArrayCount(0);
					}
				}
				
				matchToken(Token::RBracket);
			}
			
			ident->setType(declType);
			
			shared_ptr<ASTExpr> assignExpr;
			
			// Optionally, this decl may have an assignment
			if (peekAndConsume(Token::Assign))
			{
				// We don't allow assignment for int arrays
				if (declType == Type::IntArray)
				{
					reportSemantError("USC does not allow assignment of int array declarations");
				}
				
				assignExpr = parseExpr();
				if (!assignExpr)
				{
					throw ParseExceptMsg("Invalid expression after = in declaration");
				}
				
				// PA2: Type checks
				
				// If this is a character array, we need to do extra checks
				if (ident->getType() == Type::CharArray)
				{
					ASTStringExpr* strExpr = dynamic_cast<ASTStringExpr*>(assignExpr.get());
					if (strExpr != nullptr)
					{
						// If we have a declared size, we need to make sure
						// there's enough room to fit the requested string.
						// Otherwise, we need to set our size
						if (ident->getArrayCount() == 0)
						{
							ident->setArrayCount(strExpr->getLength() + 1);
						}
						else if (ident->getArrayCount() < (strExpr->getLength() + 1))
						{
							reportSemantError("Declared array cannot fit string");
						}
					}
				}
			}
			else if (ident->getType() == Type::CharArray && ident->getArrayCount() == 0)
			{
				reportSemantError("char array must have declared size if there's no assignment");
			}
			
			matchToken(Token::SemiColon);
			
			retVal = make_shared<ASTDecl>(*ident, assignExpr);
		}
		catch (ParseExcept& e)
		{
			reportError(e);
			
			// Skip all the tokens until the next semi-colon
			consumeUntil(Token::SemiColon);
			
			if (peekToken() == Token::EndOfFile)
			{
				throw EOFExcept();
			}
			
			// Grab this semi-colon, also
			consumeToken();
			
			// Put in a decl here with the bogus identifier
			// "@@error". This is so the parse will continue to the
			// next decl, if there is one.
			retVal = make_shared<ASTDecl>(*(ident));
		}
	}
	
	return retVal;
}

shared_ptr<ASTStmt> Parser::parseStmt()
{
	shared_ptr<ASTStmt> retVal;
	try
	{
		// NOTE: AssignStmt HAS to go before ExprStmt!!
		// Read comments in AssignStmt for why.
		if ((retVal = parseCompoundStmt()))
			;
		else if ((retVal = parseAssignStmt()))
			;
		// PA1: Add additional cases

		
		
		else if ((retVal = parseExprStmt()))
			;
		else if ((retVal = parseReturnStmt()))
			;
		
		else if ((retVal = parseWhileStmt()))
			;

		else if ((retVal = parseIfStmt()))
			;
		else if ((retVal = parseNullStmt()))
			;
		

		// -------
		else if (peekIsOneOf({Token::Key_int, Token::Key_char}))
		{
			throw ParseExceptMsg("Declarations are only allowed at the beginning of a scope block");
		}
	}
	catch (ParseExcept& e)
	{
		reportError(e);
		
		// Skip all the tokens until the next semi-colon
		consumeUntil(Token::SemiColon);
		
		if (peekToken() == Token::EndOfFile)
		{
			throw EOFExcept();
		}
		
		// Grab this semi-colon, also
		consumeToken();
		
		// Put in a null statement here
		// so we can try to continue.
		retVal = make_shared<ASTNullStmt>();
	}
	
	return retVal;
}

// If the compound statement is a function body, then the symbol table scope
// change will happen at a higher level, so it shouldn't happen in
// parseCompoundStmt.
shared_ptr<ASTCompoundStmt> Parser::parseCompoundStmt(bool isFuncBody)
{
	shared_ptr<ASTCompoundStmt> retVal;
	
	// PA1: Implement

	if (peekAndConsume(Token::LBrace)) {

		retVal = make_shared<ASTCompoundStmt>();

		if (!isFuncBody) {
			mSymbols.enterScope();
		}
			
		while (peekIsOneOf({Token::Key_int, Token::Key_char})) {
			retVal->addDecl(parseDecl());
		}

		while (!peekIsOneOf({Token::RBrace, Token::EndOfFile})) {
			retVal->addStmt(parseStmt());
		}

		if (!isFuncBody) {
			mSymbols.exitScope();
		}

		if (isFuncBody) {
			
			shared_ptr<ASTStmt> lstStm = retVal->getLastStmt();
			ASTReturnStmt* lstPt = dynamic_cast<ASTReturnStmt*>(lstStm.get());

			if (!lstPt) {
				std::string erro("USC requires non-void functions to end with a return");
				reportSemantError(erro);
			}
			else if (!lstPt && mCurrReturnType == Type::Void) {
				
				shared_ptr<ASTReturnStmt> vidRtnStm = make_shared<ASTReturnStmt>(nullptr);
				retVal->addStmt(vidRtnStm);
			}

		}
		matchToken(Token::RBrace);
	}
	// -------
	return retVal;
}

shared_ptr<ASTStmt> Parser::parseAssignStmt()
{
	shared_ptr<ASTStmt> retVal;
	shared_ptr<ASTArraySub> arraySub;
	
	if (peekToken() == Token::Identifier)
	{
		Identifier* ident = getVariable(getTokenTxt());
		
		consumeToken();
		
		// Now let's see if this is an array subscript
		if (peekAndConsume(Token::LBracket))
		{
			try
			{
				shared_ptr<ASTExpr> expr = parseExpr();
				if (!expr)
				{
					throw ParseExceptMsg("Valid expression required inside [ ].");
				}
				
				arraySub = make_shared<ASTArraySub>(*ident, expr);
			}
			catch (ParseExcept& e)
			{
				// If this expr is bad, consume until RBracket
				reportError(e);
				consumeUntil(Token::RBracket);
				if (peekToken() == Token::EndOfFile)
				{
					throw EOFExcept();
				}
			}
			
			matchToken(Token::RBracket);
		}
		
		// Just because we got an identifier DOES NOT necessarily mean
		// this is an assign statement.
		// This is because there is a common left prefix between
		// AssignStmt and an ExprStmt with statements like:
		// id ;
		// id [ Expr ] ;
		// id ( FuncCallArgs ) ;
		
		// So... We see if the next token is a =. If it is, then this is
		// an AssignStmt. Otherwise, we set the "unused" variables
		// so parseFactor will later find it and be able to match
		int col = mColNumber;
		if (peekAndConsume(Token::Assign))
		{
			shared_ptr<ASTExpr> expr = parseExpr();
			
			if (!expr)
			{
				throw ParseExceptMsg("= must be followed by an expression");
			}
			
			// If we matched an array, we want to make an array assign stmt
			if (arraySub)
			{
				// Make sure the type of this expression matches the declared type
				Type subType;
				if (arraySub->getType() == Type::IntArray)
				{
					subType = Type::Int;
				}
				else
				{
					subType = Type::Char;
				}
				if (mCheckSemant && subType != expr->getType())
				{
					// We can do a conversion if it's from int to char
					if (subType == Type::Char &&
						expr->getType() == Type::Int)
					{
						expr = intToChar(expr);
					}
					else
					{
						std::string err("Cannot assign an expression of type ");
						err += getTypeText(expr->getType());
						err += " to ";
						err += getTypeText(subType);
						reportSemantError(err, col);
					}
				}
				retVal = make_shared<ASTAssignArrayStmt>(arraySub, expr);
			}
			else
			{
				// PA2: Check for semantic errors
				
				retVal = make_shared<ASTAssignStmt>(*ident, expr);
			}
			
			matchToken(Token::SemiColon);
		}
		else
		{
			// We either have an unused array, or an unused ident
			if (arraySub)
			{
				mUnusedArray = arraySub;
			}
			else
			{
				mUnusedIdent = ident;
			}
		}
	}
	
	return retVal;
}

shared_ptr<ASTIfStmt> Parser::parseIfStmt()
{
	shared_ptr<ASTIfStmt> retVal;
	
	// PA1: Implement
	
    shared_ptr<ASTStmt> ifStmt;
    shared_ptr<ASTStmt> elseStmt = nullptr;
	shared_ptr<ASTExpr> exp;
	shared_ptr<ASTExpr> stm;

	if (peekToken() == Token::Key_if) {

        consumeToken();
        
        if (peekToken() == Token::LParen) {

            consumeToken();
            exp = parseExpr();
            if (!exp) {

                throw ParseExceptMsg("Invalid condition for if statement");
            }
            
            matchToken(Token::RParen);

        }
        else {
            throw TokenMismatch(Token::LParen, peekToken(), getTokenTxt());
        }
        
        ifStmt = parseStmt();
        
        if (peekToken() == Token::Key_else) {
            consumeToken();
            
            elseStmt = parseStmt();
        }
        
        retVal = make_shared<ASTIfStmt>(exp, ifStmt, elseStmt);
    }

	// -------
	return retVal;
}

shared_ptr<ASTWhileStmt> Parser::parseWhileStmt()
{
	shared_ptr<ASTWhileStmt> retVal;
	
	// PA1: Implement



	shared_ptr<ASTStmt> stm;
	shared_ptr<ASTExpr> exp;

	if (peekToken() == Token::Key_while) {

        consumeToken();
        
        if (peekToken() == Token::LParen) {
			
            consumeToken();
            
            exp = parseExpr();
            
            if (exp) {
                
                if (peekToken() == Token::RParen) {
                    consumeToken();
                    
                    stm = parseStmt();
                    
                    if (stm) {
                        retVal = make_shared<ASTWhileStmt>(exp, stm);
                    }
                }
            }
            else {
                throw ParseExceptMsg("Invalid condition for while statement");
            }
            
        }
        
    }
	
	// -------
	
	return retVal;
}

shared_ptr<ASTReturnStmt> Parser::parseReturnStmt()
{
	shared_ptr<ASTReturnStmt> retVal;
	
	// PA1: Implement

	if (peekToken() == Token::Key_return) {
        consumeToken();
        shared_ptr<ASTExpr> exp = parseExpr();
        int coln = mColNumber;
        
    

        if (exp && mCurrReturnType != exp ->getType()) {
			std::string erro("Expected type ");
            erro += getTypeText(mCurrReturnType);
            erro += " in return statement";
            reportSemantError(erro, coln);

        }
        else if (exp && mCurrReturnType == Type::Char && exp->getType() == Type::Int) {
            exp = intToChar(exp);
        }
        else if (!exp && mCurrReturnType != Type::Void) {
            
            reportSemantError("Invalid empty return in non-void function", coln);
        }
        
        if (exp) {
            retVal = make_shared<ASTReturnStmt>(exp);
        }
        else {
            retVal = make_shared<ASTReturnStmt>(nullptr);
        }
        
        matchToken(Token::SemiColon);
    }

	
	// -------
	return retVal;
}

shared_ptr<ASTExprStmt> Parser::parseExprStmt()
{
	shared_ptr<ASTExprStmt> retVal;
	
	// PA1: Implement


	shared_ptr<ASTExpr> exp = parseExpr();

    if (exp) {

        if (peekToken() == Token::SemiColon) {
            consumeToken();

            retVal = make_shared<ASTExprStmt>(exp);
        }
    }
	
	// -------
	return retVal;
}

shared_ptr<ASTNullStmt> Parser::parseNullStmt()
{
	shared_ptr<ASTNullStmt> retVal;
	
	// PA1: Implement

    if (peekToken() == Token::SemiColon) {
		
		consumeToken();
		
        retVal = make_shared<ASTNullStmt>();

    }
		
	// -------
	return retVal;
}
