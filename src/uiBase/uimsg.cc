/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uimsg.h"

#include "bufstringset.h"
#include "mousecursor.h"
#include "oddirs.h"
#include "perthreadrepos.h"
#include "pixmap.h"
#include "separstr.h"

#include "uimain.h"
#include "uimainwin.h"
#include "uistatusbar.h"
#include "uiobj.h"
#include "uibody.h"
#include "uiparentbody.h"
#include "uistrings.h"
#include "uistring.h"

#undef Ok
#include <QMessageBox>
#include <QAbstractButton>

#include "odlogo128x128.xpm"
static const char** sODLogo = od_logo_128x128;

mUseQtnamespace

uiMsg* uiMsg::theinst_ = 0;
uiMsg& uiMSG()
{
    if ( !uiMsg::theinst_ )
	uiMsg::theinst_ = new uiMsg;
    return *uiMsg::theinst_;
}


uiMsg::uiMsg()
	: uimainwin_(0)
{
}


uiMainWin* uiMsg::setMainWin( uiMainWin* m )
{
    uiMainWin* old = uimainwin_;
    uimainwin_ = m;
    return old;
}


uiStatusBar* uiMsg::statusBar()
{
    uiMainWin* mw = uimainwin_;/* ? uimainwin_
			       : uiMainWin::gtUiWinIfIsBdy( parent_ );*/

    if ( !mw || !mw->statusBar() )
	mw = uiMainWin::activeWindow();

    if ( !mw || !mw->statusBar() )
	mw = uiMain::theMain().topLevel();

    return mw ? mw->statusBar() : 0;
}


QWidget* uiMsg::popParnt()
{
    uiMainWin* mw = uimainwin_; //Always respect user's setting first.
    if ( !mw ) mw = uiMainWin::activeWindow();
    if ( !mw ) mw = uiMain::theMain().topLevel();

    if ( !mw  )		return 0;
    return mw->body()->qwidget();
}


bool uiMsg::toStatusbar( const char* msg, int fldidx, int msec )
{
    if ( !statusBar() ) return false;

    statusBar()->message( msg, fldidx, msec );
    return true;
}


static uiString& gtCaptn()
{
    mDefineStaticLocalObject( PerThreadObjectRepository<uiString>, captn,  );
    return captn.getObject();
}


void uiMsg::setNextCaption( const uiString& s )
{
    gtCaptn() = s;
}


#define mPrepCursor() \
    MouseCursorChanger cc( MouseCursor::Arrow )

#define mCapt(s) \
BufferString addendum; \
const uiString wintitle = \
	getCaptn( uiMainWin::uniqueWinTitle((s),0,&addendum ) ); \
const BufferString utfwintitle( wintitle.getOriginalString(), addendum )

#define mTxt QString( msg.buf() )

uiString getCaptn( const uiString& s )
{
    if ( gtCaptn().isEmpty() )
	return s;

    uiString oldcaptn = gtCaptn();
    gtCaptn() = "";

    return oldcaptn;
}


int uiMsg::beginCmdRecEvent( const char* wintitle )
{
    uiMainWin* carrier = uiMain::theMain().topLevel();
    if ( !carrier )
	return -1;

    BufferString msg( "QMsgBoxBut " );
    msg += wintitle;

#ifdef __lux32__
    return carrier->beginCmdRecEvent( (od_uint32) this, msg );
#else
    return carrier->beginCmdRecEvent( (od_uint64) this, msg );
#endif

}


#define mEndCmdRecEvent( refnr, qmsgbox ) \
\
    QAbstractButton* abstrbut = qmsgbox.clickedButton(); \
    BufferString msg; if ( abstrbut ) msg = abstrbut->text(); \
    endCmdRecEvent( refnr, 0, msg );

void uiMsg::endCmdRecEvent( int refnr, int retval, const char* buttxt0,
			    const char* buttxt1, const char* buttxt2 )
{
    BufferString msg( "QMsgBoxBut " );

    msg += !retval ? buttxt0 : ( retval==1 ? buttxt1 : buttxt2 );

    uiMainWin* carrier = uiMain::theMain().topLevel();
    if ( carrier )
#ifdef __lux32__
	carrier->endCmdRecEvent( (od_uint32) this, refnr, msg );
#else
	carrier->endCmdRecEvent( (od_uint64) this, refnr, msg );
#endif
}


#define mImplSimpleMsg( odfunc, caption, qtfunc ) \
void uiMsg::odfunc( const char* text, const char* p2, const char* p3 ) \
{ \
    BufferString msg( text ); if ( p2 ) msg += p2; if ( p3 ) msg += p3; \
    odfunc( msg ); \
} \
void uiMsg::odfunc( const uiString& text ) \
{ \
    mPrepCursor(); \
    if ( text.isEmpty() ) \
	return; \
\
    uiString oktxt = uiStrings::sOk(); \
\
    mCapt(caption); \
    const int refnr = beginCmdRecEvent( utfwintitle.buf() ); \
    QMessageBox::qtfunc( popParnt(), wintitle.getQtString(), \
			    text.getQtString(), oktxt.getQtString() ); \
    endCmdRecEvent( refnr, 0, oktxt.getOriginalString() ); \
}


mImplSimpleMsg( message, tr("Information"), information )
mImplSimpleMsg( warning, tr("Warning"), warning )


void uiMsg::error( const char* p1, const char* p2, const char* p3 )
{
    BufferString msg( p1 ); if ( p2 ) msg += p2; if ( p3 ) msg += p3;
    errorWithDetails( FileMultiString(msg.buf()) );
}


static void addStayOnTopFlag( QMessageBox& mb )
{
    Qt::WindowFlags flags = mb.windowFlags();
    flags |= Qt::WindowStaysOnTopHint;
    mb.setWindowFlags( flags );
}


void uiMsg::error( const uiString& str )
{
    MouseCursorChanger cc( MouseCursor::Arrow );

    mCapt(tr("Error"));
    const int refnr = beginCmdRecEvent( utfwintitle.buf() );

    QMessageBox msgbox( QMessageBox::Critical, wintitle.getQtString(),
			str.getQtString(), QMessageBox::Ok, popParnt() );
    addStayOnTopFlag( msgbox );
    msgbox.exec();
    mEndCmdRecEvent( refnr, msgbox );
}


void uiMsg::errorWithDetails( const TypeSet<uiString>& bss,
			      const uiString& before )
{
    TypeSet<uiString> strings;

    if ( !before.isEmpty() )
	strings += before;

    strings.append( bss );

    errorWithDetails( strings );
}


void uiMsg::errorWithDetails( const BufferStringSet& bss )
{
    TypeSet<uiString> strings;
    bss.fill( strings );

    errorWithDetails( strings );
}


void uiMsg::errorWithDetails( const FileMultiString& fms )
{
    TypeSet<uiString> strings;

    for ( int idx=0; idx<fms.size(); idx++ )
	strings.add( fms[idx] );

    errorWithDetails( strings );
}


void uiMsg::errorWithDetails( const TypeSet<uiString>& strings )
{
    if ( !strings.size() )
	return;

    MouseCursorChanger cc( MouseCursor::Arrow );

    mCapt(tr("Error"));
    const int refnr = beginCmdRecEvent( utfwintitle.buf() );

    QMessageBox msgbox( QMessageBox::Critical, wintitle.getQtString(),
			strings[0].getQtString(), QMessageBox::Ok, popParnt() );
    if ( strings.size()>1 )
    {
	uiString detailed = strings[0];

	for ( int idx=1; idx<strings.size(); idx++ )
	{
	    uiString old = detailed;
	    detailed = uiString( "%1\n%2" ).arg( old ).arg( strings[idx] );
	}

	msgbox.setDetailedText( detailed.getQtString() );
    }

    addStayOnTopFlag( msgbox );
    msgbox.exec();
    mEndCmdRecEvent( refnr, msgbox );
}


int uiMsg::askSave( const uiString& text, bool wcancel )
{
    const uiString dontsavetxt = tr("Don't save");
    return question( text, uiStrings::sSave(true), dontsavetxt,
		     wcancel ? uiStrings::sCancel() : 0,
		     tr("Data not saved") );
}


int uiMsg::askRemove( const uiString& text, bool wcancel )
{
    const uiString notxt = wcancel ? tr("Don't remove") : uiStrings::sCancel();
    return question( text, uiStrings::sRemove(true), notxt,
		     wcancel ? uiStrings::sCancel() : uiString(0),
		     tr("Remove data") );
}


int uiMsg::askContinue( const uiString& text )
{
    return question( text, uiStrings::sContinue(), uiStrings::sAbort(), 0 );
}


int uiMsg::askOverwrite( const uiString& text )
{
    const uiString yestxt = uiStrings::sOverwrite();
    return question( text, yestxt, uiStrings::sCancel(), 0 );
}


int uiMsg::question( const uiString& text, const uiString& yestxtinp,
		     const uiString& notxtinp,
		     const uiString& cncltxtinp, const uiString& title )
{
    mPrepCursor();

    mCapt(title.isEmpty() ? tr("Please specify") : title);
    const int refnr = beginCmdRecEvent( utfwintitle );

    const uiString yestxt = yestxtinp.isEmpty() ? uiStrings::sYes() : yestxtinp;
    const uiString notxt = notxtinp.isEmpty() ? uiStrings::sNo() : notxtinp;

    const int res = QMessageBox::question( popParnt(), wintitle.getQtString(),
				    text.getQtString(), yestxt.getQtString(),
				    notxt.getQtString(),
				    !cncltxtinp.isEmpty()
					? cncltxtinp.getQtString()
					: QString::null,
				    0, 2 );

    endCmdRecEvent( refnr, res, yestxt.getOriginalString(),
		    notxt.getOriginalString(),
		    cncltxtinp.getOriginalString() );
    return res == 0 ? 1 : (res == 1 ? 0 : -1);
}


void uiMsg::about( const uiString& text )
{
    mPrepCursor();
    mCapt( tr("About") );
    const int refnr = beginCmdRecEvent( utfwintitle );
    QMessageBox::about( popParnt(), wintitle.getQtString(), text.getQtString());
    endCmdRecEvent( refnr, 0, uiStrings::sOk().getOriginalString() );
}


void uiMsg::aboutOpendTect( const uiString& text )
{
    mPrepCursor();
    mCapt( tr("About OpendTect") );
    const int refnr = beginCmdRecEvent( utfwintitle );
    QMessageBox msgbox( popParnt() );
    msgbox.addButton( QMessageBox::Close );
    ioPixmap pm( sODLogo );
    if ( pm.qpixmap() )
	msgbox.setIconPixmap( *pm.qpixmap() );
    msgbox.setWindowTitle( wintitle.getQtString() );
    msgbox.setText( text.getQtString() );
    msgbox.setBaseSize( 600, 300 );
    addStayOnTopFlag( msgbox );
    msgbox.exec();
    endCmdRecEvent( refnr, 0, uiStrings::sOk().getOriginalString() );
}


bool uiMsg::askGoOn( const uiString& text, bool yn )
{
    const uiString oktxt = yn ? uiStrings::sYes() : uiStrings::sOk();
    const uiString canceltxt = yn ? uiStrings::sNo() : uiStrings::sCancel();

    return askGoOn( text, oktxt, canceltxt );
}


bool uiMsg::askGoOn( const uiString& text, const uiString& textyes,
		     const uiString& textno )
{
    mPrepCursor();

    mCapt(tr("Please specify"));
    const int refnr = beginCmdRecEvent( utfwintitle );
    const int res = QMessageBox::warning( popParnt(), wintitle.getQtString(),
				      text.getQtString(), textyes.getQtString(),
				      textno.getQtString(),
				      QString::null, 0, 1);
    endCmdRecEvent( refnr, res, textyes.getOriginalString(),
		    textno.getOriginalString() );
    return !res;
}


int uiMsg::askGoOnAfter( const uiString& text, const uiString& cnclmsginp ,
			 const uiString& textyesinp, const uiString& textnoinp )
{
    mPrepCursor();

    const uiString yestxt = textyesinp.isEmpty()
	? uiStrings::sYes()
	: textyesinp;
    const uiString notxt = textnoinp.isEmpty()
	? uiStrings::sNo()
	: textnoinp;
    const uiString cncltxt = cnclmsginp.isEmpty()
	? uiStrings::sCancel()
	: cnclmsginp;

    mCapt(tr("Please specify"));
    const int refnr = beginCmdRecEvent( utfwintitle );
    const int res = QMessageBox::warning( popParnt(), wintitle.getQtString(),
					  text.getQtString(),
					  yestxt.getQtString(),
					  notxt.getQtString(),
					  cncltxt.getQtString(), 0, 2 );

    endCmdRecEvent( refnr, res, yestxt.getOriginalString(),
		    notxt.getOriginalString(), cncltxt.getOriginalString() );
    return res;
}


bool uiMsg::showMsgNextTime( const uiString& text, const uiString& ntmsginp )
{
    mPrepCursor();
    const uiString oktxt = uiStrings::sOk();
    const uiString notmsg = ntmsginp.isEmpty()
	? tr("Don't show this message again")
	: ntmsginp;

    mCapt(tr("Information"));
    const int refnr = beginCmdRecEvent( utfwintitle );

    const int res = QMessageBox::information( popParnt(),wintitle.getQtString(),
				      text.getQtString(), oktxt.getQtString(),
				      notmsg.getQtString(),
				      QString::null, 0, 1 );

    endCmdRecEvent( refnr, res, oktxt.getOriginalString(),
		    notmsg.getOriginalString() );
    return !res;
}
