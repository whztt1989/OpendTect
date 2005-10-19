/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uiwizard.cc,v 1.10 2005-10-19 16:37:38 cvskris Exp $
________________________________________________________________________

-*/


#include "uiwizard.h"

#include "uibutton.h"
#include "uicursor.h"
#include "uigroup.h"

uiWizard::uiWizard( uiParent* p, uiDialog::Setup& s_ )
    : uiDialog(p,s_)
    , pageidx(0)
    , rotatemode(false)
{
    finaliseDone.notify( mCB(this,uiWizard,doFinalise) );
}


void uiWizard::doFinalise( CallBacker* )
{
    isStarting();
    setCurrentPage( firstPage() );
    updateButtonText();
}


int uiWizard::addPage( uiGroup* page, bool display )
{
    if ( !page ) return -1;

    pages += page;
    dodisplay += display;
    return pages.size()-1;
}


void uiWizard::displayPage( int idx, bool yn )
{
    if ( idx>=0 && idx<dodisplay.size() )
    {
	dodisplay[idx] = yn;
	updateButtonText();
    }
}


bool uiWizard::acceptOK( CallBacker* )
{
    const int prevpage = pageidx;
    uiCursorChanger cursorchanger( uiCursor::Wait );
    bool firstpage = pageidx == firstPage();

    if ( !leavePage(pageidx,false) )
	return false;

    if ( pageidx==firstPage() )
	return isClosing(true);

    pageidx--;
    while ( !dodisplay[pageidx] && !pageidx )
	pageidx--;

    if ( displayCurrentPage() )
	updateButtonText();
    else
	pageidx = prevpage;

    return false;
}


bool uiWizard::rejectOK( CallBacker* cb )
{
    if ( cb!=button(uiDialog::CANCEL) ) 
    {
	leavePage(pageidx,false);
	isClosing(true);
	return true;
    }

    uiCursorChanger cursorchanger( uiCursor::Wait );
    if ( !leavePage(pageidx,true) )
	return false;

    if ( lastPage()==-1 || pageidx==lastPage() )
    {
	if ( rotatemode )
	{
	    reset();
	    pageidx = 0;
	}
	else
	{
	    return isClosing(false);
	}
    }
    else
	pageidx++;

    while ( !dodisplay[pageidx] && pageidx<pages.size() )
	pageidx++;

    updateButtonText();
    displayCurrentPage();
    return false;
}


bool uiWizard::displayCurrentPage()
{
    if ( !preparePage(pageidx) )
	return false;

    for ( int idx=0; idx<pages.size(); idx++ )
	pages[idx]->display( idx==pageidx );

    return true;
}


void uiWizard::setCurrentPage( int idx )
{
    pageidx = idx;
    displayCurrentPage();
}


void uiWizard::updateButtonText()
{
    bool firstpage = pageidx == firstPage();
    bool lastpage = pageidx == lastPage();
    const char* oktxt = firstpage ? "Cancel" : "<< Back";
    const char* canceltxt = lastpage && !rotatemode ? "Finish" : "Next >>";

    setButtonText( uiDialog::OK, oktxt );
    setButtonText( uiDialog::CANCEL, canceltxt );
}


void uiWizard::setRotateMode( bool yn )
{
    rotatemode=yn;
    updateButtonText();
}


int uiWizard::firstPage() const
{
    int firstidx = -1;
    for ( int idx=0; idx<pages.size(); idx++ )
    {
	if ( dodisplay[idx] )
	{
	    firstidx = idx;
	    break;
	}
    }

    return firstidx;
}


int uiWizard::lastPage() const
{
    int lastidx=-1;
    for ( int idx=0; idx<pages.size(); idx++ )
    {
	if ( dodisplay[idx] )
	    lastidx = idx;
    }

    return lastidx;
}
