#ifndef mathexpression_h
#define mathexpression_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id: mathexpression.h,v 1.4 2001-01-19 11:34:40 bert Exp $
________________________________________________________________________

-*/

#include <sets.h>
#include <gendefs.h>


/*!\brief parses a string with a mathematical expression.

The expression can consist of constants, variables and operators.
A constant can be any number like 3, -5, 3e-5, or pi. Everything that does
not start with a digit is treated as a variable. An operator can be either:

+, -, *, /, ^, >, <, <=, >=, ==, !=, &&, ||, cond ? true stat : false stat,
or |abs|

If the parser returns null, it couldn't parse the expression.

A MathExpression can be queried about its variables with getNrVariables(), and
each variable's name can be queried with getVariableStr( int ).

When a calculations should be done, all variables must be set with
setVariable( int, float ). Then, the calculation can be done with getValue().

-*/

class MathExpression
{
public:

    static MathExpression* 	parse( const char* );
    virtual float		getValue() const		= 0;

    virtual const char*		getVariableStr( int ) const;
    virtual int			getNrVariables() const;
    virtual void		setVariable( int, float );

    virtual MathExpression*	clone() const = 0;

				MathExpression(int);
				~MathExpression();

protected:

    int				getNrInputs() const { return inputs.size(); }
    bool			setInput( int, MathExpression* );
    void			copyInput( MathExpression* target ) const;


    ObjectSet<TypeSet<int> >	variableobj;
    ObjectSet<TypeSet<int> >	variablenr;
    ObjectSet<MathExpression>	inputs;

};


#endif
