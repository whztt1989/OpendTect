#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : Sept 2008
-*/
 
#include "gmtmod.h"
#include "transl.h"
class Conn;
namespace ODGMT { class ProcFlow; }


mExpClass(GMT) ODGMTProcFlowTranslatorGroup : public TranslatorGroup
{   isTranslatorGroup(ODGMTProcFlow);
    mODTextTranslationClass(ODGMTProcFlowTranslatorGroup);
public:
    			mDefEmptyTranslatorGroupConstructor(ODGMTProcFlow)

    const char*		defExtension() const		{ return "gmf"; }
};


mExpClass(GMT) ODGMTProcFlowTranslator : public Translator
{ mODTextTranslationClass(ODGMTProcFlowTranslator)
public:
    			mDefEmptyTranslatorBaseConstructor(ODGMTProcFlow)

    virtual const char*	read(ODGMT::ProcFlow&,Conn&)	= 0;
			//!< returns err msg or null on success
    virtual const char*	write(const ODGMT::ProcFlow&,Conn&) = 0;
			//!< returns err msg or null on success

    static bool		retrieve(ODGMT::ProcFlow&,const IOObj*,BufferString&);
    static bool		store(const ODGMT::ProcFlow&,const IOObj*,
	    		      BufferString&);
    static bool		retrieve(ODGMT::ProcFlow&,const IOObj*,uiString&);
    static bool		store(const ODGMT::ProcFlow&,const IOObj*,
	    		      uiString&);

};


mExpClass(GMT) dgbODGMTProcFlowTranslator : public ODGMTProcFlowTranslator
{			     isTranslator(dgb,ODGMTProcFlow)
public:

    			mDefEmptyTranslatorConstructor(dgb,ODGMTProcFlow)

    const char*		read(ODGMT::ProcFlow&,Conn&);
    const char*		write(const ODGMT::ProcFlow&,Conn&);

};
