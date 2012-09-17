/*+

________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	J.C. Glas
 Date:		March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: cmdrecorder.cc,v 1.36 2012/09/11 14:36:37 cvsjaap Exp $";

#include "cmdrecorder.h"

#include "cmdcomposer.h"
#include "objectfinder.h"
#include "tablecommands.h"

#include <iostream>

#include "ascstream.h"
#include "oddirs.h"
#include "strmprov.h"
#include "timer.h"
#include "uiodapplmgr.h"

#include "uitoolbutton.h"
#include "uicanvas.h"
#include "uicombobox.h"
#include "uigraphicsview.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uilistview.h"
#include "uimainwin.h"
#include "uimdiarea.h"
#include "uimenu.h"
#include "uiobj.h"
#include "uislider.h"
#include "uispinbox.h"
#include "uitabbar.h"
#include "uitable.h"
#include "uilineedit.h"
#include "uiseparator.h"
#include "uithumbwheel.h"

namespace CmdDrive
{


CmdRecorder::CmdRecorder()
    	: rec_(*this)
	, outputfnm_(0)
	, outputsd_(*new StreamData)
	, recording_(false)
	, dynamicpopupmenu_(0)
	, openqdialog_(false)
	, currootwin_(0)
	, lastobjsearched_(0)
{}


CmdRecorder::~CmdRecorder()
{
    if ( recording_ ) stop();

    outputsd_.close();
    delete &outputsd_;
}


bool CmdRecorder::start()
{
    outputsd_ = StreamProvider(outputfnm_).makeOStream( false );
    if ( !outputfnm_ || !outputsd_.usable() )
	return false;

    ascostream astrm( *outputsd_.ostrm );
    if ( !astrm.putHeader("OpendTect commands") )
	return false;

    uiBaseObject::setCmdRecorder( mCB(this,CmdRecorder,handleEvent) );
    uiMenuItem::setCmdRecorder( mCB(this,CmdRecorder,handleEvent) );
    uiPopupMenu::setInterceptor( mCB(this,CmdRecorder,dynamicMenuInterceptor) );
    Timer::setUserWaitFlag( false );

    dynamicpopupmenu_ = 0;
    openqdialog_ = false;

    lastobjsearched_ = 0;
    lastobjfreewins_.erase();

    winassertion_.setEmpty();
    winassertcasedep_ = false;

    popuprefnrs_.erase();
    composers_.erase();
    winstack_.synchronize();

    currootwin_ = ODMainWin();
    recording_ = true;

    return true;
}


void CmdRecorder::stop()
{
    CmdRecEvent dummy;
    dummy.srcwin_ = winstack_.topWin();
    insertWinAssertion( dummy );

    uiBaseObject::unsetCmdRecorder();
    uiMenuItem::unsetCmdRecorder();
    uiPopupMenu::unsetInterceptor(); 
    recording_ = false;
}


void CmdRecorder::dynamicMenuInterceptor( CallBacker* cb )
{
    mDynamicCast( uiPopupMenu*, dynamicpopupmenu_, cb );
    if ( dynamicpopupmenu_ )
	dynamicpopupmenu_->doIntercept( false ); 
}


#define mMatchObjClassExtra( srcobj, newobj, objclass, extraclass, yn ) \
{ \
    mDynamicCastGet( const objclass*,   srcobj0, &srcobj ); \
    mDynamicCastGet( const objclass*,   newobj1, &newobj ); \
    mDynamicCastGet( const extraclass*, newobj2, &newobj ); \
    if ( srcobj0 && (newobj1 || newobj2) ) \
	yn = true; \
}

#define mMatchObjClass( srcobj, newobj, objclass, yn ) \
    mMatchObjClassExtra( srcobj, newobj, objclass, objclass, yn )

static void takeSimilarObjs( ObjectSet<const uiObject>& objects,
			     const uiObject& srcobj, bool tofront, bool toback )
{
    int nrobjstodo = objects.size();
    int offset = 0;
    int idx = 0;

    while ( nrobjstodo )
    {
	const uiObject& newobj = *objects[idx];
	bool yn = false;

	mMatchObjClass( srcobj, newobj, uiLabel, yn );
	mMatchObjClass( srcobj, newobj, uiGroupObj, yn );
	mMatchObjClass( srcobj, newobj, uiSeparator, yn );

	mMatchObjClass( srcobj, newobj, uiButton, yn );
	mMatchObjClass( srcobj, newobj, uiSpinBox, yn );
	mMatchObjClass( srcobj, newobj, uiSlider, yn );
	mMatchObjClass( srcobj, newobj, uiThumbWheel, yn );
	mMatchObjClass( srcobj, newobj, uiMdiArea, yn );
	mMatchObjClass( srcobj, newobj, uiTabBar, yn );
	mMatchObjClass( srcobj, newobj, uiComboBox, yn );
	mMatchObjClass( srcobj, newobj, uiListBox, yn );
	mMatchObjClass( srcobj, newobj, uiListView, yn );
	mMatchObjClass( srcobj, newobj, uiTable, yn );

	mMatchObjClassExtra( srcobj, newobj, uiLineEdit, uiSpinBox, yn );
	mMatchObjClassExtra( srcobj, newobj, uiCanvas, uiGraphicsView, yn );
	mMatchObjClassExtra( srcobj, newobj, uiGraphicsView, uiCanvas, yn );

	if ( tofront!=yn && toback!=yn )
	    objects.remove( idx );
	else if ( toback && yn )
	    objects += objects.remove( idx );
	else if ( tofront && yn )
	    objects.insertAt( objects.remove(idx++), offset++ );
	else
	    idx++;

	nrobjstodo--;
    }
}


static bool deepFindKeyStr( const uiMainWin&, CmdRecEvent&, const uiObject* );

#define mFindAllNodes( objfinder, localenv, objsfound ) \
\
    ObjectSet<const uiObject> objsfound; \
    if ( localenv ) \
	objfinder.findNodes( localenv, &objsfound ); \
    else \
	objfinder.findNodes( ObjectFinder::Everything, &objsfound ); \
    objfinder.deleteGreys( objsfound );

static bool doFindKeyStr( const uiMainWin& srcwin, CmdRecEvent& event,
			  const uiObject* localenv=0 )
{
    if ( !event.object_ )
	return false;

    bool allobjsrelative = false;
    ObjectFinder csobjfinder( srcwin, true );
    mFindAllNodes( csobjfinder, localenv, objsfound );
    takeSimilarObjs( objsfound, *event.object_, true, true );

    if ( objsfound.indexOf(event.object_) < 0 )
	return deepFindKeyStr( srcwin, event, localenv );

    ObjectSet<const uiObject> curobjset = objsfound;
    ObjectFinder::NodeTag curtag = ObjectFinder::UiObjNode;
    const uiObject* curnode = event.object_;
    FileMultiString curkeystr;

    do
    {
	ObjectSet<const uiObject> relatives;
	csobjfinder.findNodes( curtag, curnode, &relatives ); 

	for ( int idx=0; idx<curobjset.size(); idx++ )
	{
	    allobjsrelative = relatives.indexOf(curobjset[idx]) >= 0;
	    if ( !allobjsrelative )
		break;
	}

	static uiLabel lbldummy(0,0);
	static uiSeparator sepdummy(0);
	static uiGroup grpdummy(0);

	takeSimilarObjs( relatives, lbldummy, true, false );
	takeSimilarObjs( relatives, sepdummy, false, true );
	takeSimilarObjs( relatives, *event.object_, false, true );
	takeSimilarObjs( relatives, *grpdummy, false, true );

	ObjectSet<const uiObject> minobjset;
	BufferString minkey;

	BufferStringSet aliases;
	while ( !relatives.isEmpty() || !aliases.isEmpty() )
	{
	    if ( aliases.isEmpty() )
	    {
		csobjfinder.getAliases( *relatives[0], aliases );
		relatives.remove( 0 );
	    }

	    ObjectSet<const uiObject> newobjset = curobjset;
	    FileMultiString newkeystr;
	    newkeystr += aliases[0]->buf(); 
	    aliases.remove( 0 );

	    csobjfinder.selectNodes( newobjset, newkeystr );
	    if ( newobjset.indexOf(event.object_) < 0 )
		continue;

	    if ( minobjset.isEmpty() || newobjset.size()<minobjset.size() )
	    {
		minobjset = newobjset;
		minkey = newkeystr[0];
	    }
	    if ( minobjset.size() == 1 )
		break;
	}

	if ( minobjset.size()==1 || minobjset.size()<curobjset.size() )
	{
	    curobjset = minobjset;
	    curkeystr += minkey;
	}

	csobjfinder.getAncestor( curtag, curnode );
    }
    while ( curobjset.size()>1 && !allobjsrelative );

    for ( int idx=curkeystr.size()-2; idx>=0; idx-- )
    {
	ObjectSet<const uiObject> newobjset = objsfound;
	FileMultiString shorterkeystr;
	for ( int idy=0; idy<curkeystr.size(); idy++ )
	{
	    if ( idy != idx )
		shorterkeystr += curkeystr[idy];
	}
	csobjfinder.selectNodes( newobjset, shorterkeystr );
	if ( newobjset.indexOf(event.object_) < 0 )
	    continue;
	if ( newobjset.size() == curobjset.size() )
	    curkeystr = shorterkeystr;
    }

    event.keystr_ = curkeystr.unescapedStr();
    if ( curobjset.size() > 1 )
    {
	event.keystr_ += "#";
	event.keystr_ += 1 + curobjset.indexOf( event.object_ );
    }
    event.similarobjs_ = objsfound.size() > 1;
    event.srcwin_ = &srcwin;

    ObjectFinder ciobjfinder( srcwin, false );
    ciobjfinder.selectNodes( objsfound, curkeystr );
    if ( objsfound.size() > curobjset.size() )
	event.casedep_ = true;

    return true;
}


static bool deepFindKeyStr( const uiMainWin& srcwin, CmdRecEvent& event,
			    const uiObject* localenv )
{
    ObjectFinder objfinder( srcwin );
    mFindAllNodes( objfinder, localenv, objsfound );

    for ( int idx=0; idx<objsfound.size(); idx++ )
    {
	mDynamicCastGet( const uiTable*, uitable, objsfound[idx] );
	if ( uitable )
	{
	    RowCol rc;
	    for ( rc.row=0; rc.row<uitable->nrRows(); rc.row++ )
	    {
		for ( rc.col=0; rc.col<uitable->nrCols(); rc.col++ )
		{
		    uiObject* uiobj = uitable->getCellObject( rc );
		    if ( uiobj && doFindKeyStr(srcwin, event, uiobj) )
		    {
			CmdRecEvent execdummy;
			execdummy.object_ = const_cast<uiTable*>( uitable );
			doFindKeyStr( srcwin, execdummy, localenv );
			TableCmdComposer::getExecPrefix( execdummy, rc );

			event.casedep_ = event.casedep_ || execdummy.casedep_;
			BufferString execprefixtail = event.execprefix_;
			event.execprefix_ = execdummy.execprefix_;
			event.execprefix_ += execprefixtail;
			return true;
		    }
		}
	    }
	}
    }
    return false;
}


bool CmdRecorder::findKeyString( const uiMainWin& srcwin, CmdRecEvent& event )
{
    if ( event.object_ != lastobjsearched_ )
    {
	lastobjsearched_ = event.object_;
	lastobjfreewins_.erase();
    }
    else if ( lastobjfreewins_.indexOf( &srcwin ) >= 0 )
	return false;

    bool res = doFindKeyStr( srcwin, event );
    if ( !res && event.object_->visible() && event.object_->sensitive()  )
	lastobjfreewins_ += &srcwin;

    return res;
}


static bool findMenuPath( const uiMenuItemContainer& mnu,
			  const uiMenuItem& searchitem,
			  FileMultiString& menupath, bool& casedep )
{
    bool itemfound = false;
    const uiMenuItem* curitem;
    FileMultiString pathtail;

    for ( int idx=0; idx<mnu.items().size(); idx++ )
    {
	curitem = mnu.items()[idx];
	if ( !curitem->isEnabled() )
	    continue;

	if ( curitem == &searchitem )
	{
	    itemfound = true;
	    casedep = false;
	    break;
	}

	mDynamicCastGet( const uiPopupItem*, popupitm, curitem );
	if ( !popupitm )
	    continue;

	if ( findMenuPath(popupitm->menu(), searchitem, pathtail, casedep) )
	{
	    itemfound = true;
	    break;
	}
    }

    if ( !itemfound )
	return false;

    mGetAmpFilteredStr( curtxt, curitem->text() );
    mDressNameString( curtxt, sMenuPath );

    int nrmatches = 0;
    int selnr = 0;

    for ( int idx=0; idx<mnu.items().size(); idx++ )
    {
	const uiMenuItem* mnuitm = mnu.items()[idx];
	if ( !mnuitm->isEnabled() )
	    continue;

	mGetAmpFilteredStr( mnuitmtxt, mnuitm->text() );
	if ( SearchKey(curtxt,false).isMatching(mnuitmtxt) )
	{
	    if ( SearchKey(curtxt,true).isMatching(mnuitmtxt) )
	    {
		nrmatches++;
		if ( mnuitm == curitem )
		    selnr = nrmatches;
	    }
	    else
		casedep = true;
	}
    }

    if ( selnr && nrmatches>1 )
    {
	curtxt += "#"; curtxt += selnr;
    }

    menupath.setEmpty();
    menupath += curtxt;
    for ( int idx=0; idx<pathtail.size(); idx++ )
	menupath += pathtail[idx];

    return true;
}


static bool findMenuPath( const uiMainWin& srcwin, CmdRecEvent& ev )
{
    if ( !ev.mnuitm_ )
	return false;

    FileMultiString pathfms;
    const uiMenuBar* mnubar = const_cast<uiMainWin&>(srcwin).menuBar();
    if ( mnubar && mnubar->isSensitive() &&
	 findMenuPath(*mnubar, *ev.mnuitm_, pathfms, ev.casedep_) )
    {
	ev.srcwin_ = &srcwin;
	ev.menupath_ = pathfms.unescapedStr();
	return true;
    }

    ObjectSet<const uiObject> objsfound;
    ObjectFinder objfinder( srcwin );
    objfinder.findNodes( ObjectFinder::Everything, &objsfound );

    for ( int idx=objsfound.size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet( const uiToolButton*, toolbut, objsfound[idx] );
	const uiPopupMenu* menu = toolbut ? toolbut->menu() : 0;
	if ( menu && findMenuPath(*menu,*ev.mnuitm_,pathfms,ev.casedep_) )
	{
	    ev.srcwin_ = &srcwin;
	    ev.menupath_ = pathfms.unescapedStr();
	    ev.object_ = const_cast<uiToolButton*>( toolbut );
	    doFindKeyStr( srcwin, ev );
	    return true;
	}
    }
    return false;
}


void CmdRecorder::handleEvent( CallBacker* cb )
{
    CmdRecEvent ev;
    mCBCapsuleUnpackWithCaller( const char*, msg, caller, cb );

    const char* msgnext = getNextWord( msg, ev.idstr_.buf() );
    mSkipBlanks ( msgnext );

    const bool iscarrieronly = isNumberString( ev.idstr_, true );
    if ( !iscarrieronly )
    {
#ifdef __lux32__
	ev.idstr_ = toString((od_uint32) caller );
#else
	ev.idstr_ = toString((od_uint64) caller );
#endif
	msgnext = msg;
    }

    BufferString beginendword;
    const char* msgnexxt = getNextWord( msgnext, beginendword.buf() );

    if ( mMatchCI(beginendword,"Begin") )
	ev.begin_ = true;
    else if ( mMatchCI(beginendword,"End") )
	ev.begin_ = false;
    else 
	return;

    char* msgnexxxt;
    ev.refnr_ = strtol( msgnexxt, &msgnexxxt, 0 );
    mSkipBlanks ( msgnexxxt );
    ev.msg_ = msgnexxxt;

    BufferString keyword;
    const char* msgtail = getNextWord( msgnexxxt, keyword.buf() );
    mSkipBlanks ( msgtail );

    if ( !iscarrieronly )
    {
	if ( ev.begin_ || !mMatchCI(keyword,"Close") )
	{
	    mDynamicCast( const uiMainWin*, ev.srcwin_, caller );
	    mDynamicCast( uiObject*, ev.object_, caller );
	    mDynamicCast( uiMenuItem*, ev.mnuitm_, caller );
	}
    }

    if ( mMatchCI(keyword,"WinPopUp") )
    {
	if ( ev.begin_ )
	    popuprefnrs_ += ev.refnr_;
	else
	{
	    const int idx = popuprefnrs_.indexOf( ev.refnr_ );
	    if ( idx >= 0 )
		popuprefnrs_.remove( idx );
	}
	return;
    }

    if ( !ev.begin_ )
    {
	for ( int idx=0; idx<composers_.size(); idx++ )
	{
	    if ( composers_[idx]->traceSrcWin(ev) )
		break;
	}
    }
    else if ( ev.object_ || ev.mnuitm_ )
    {
	ObjectSet<uiMainWin> windowlist;
	uiMainWin::getTopLevelWindows( windowlist );

	for ( int idx=windowlist.size()-1; idx>=0; idx-- )
	{
	    uiMainWin* uimw = windowlist[idx];
	    if ( ev.object_ && findKeyString(*uimw, ev) )
		break;

	    if ( ev.mnuitm_ )
	    {
		if ( findMenuPath(*uimw, ev) )
		    break;

		FileMultiString pathfms;
		if ( !idx && dynamicpopupmenu_ &&
		     findMenuPath(*dynamicpopupmenu_,
				  *ev.mnuitm_, pathfms, ev.casedep_) )
		{
		    ev.dynamicpopup_ = true;
		    ev.srcwin_ = winstack_.topWin();
		    ev.menupath_ = pathfms.unescapedStr();
		}	
	    }
	}
    }
    else if ( iscarrieronly )
    {
	ev.srcwin_ = winstack_.topWin(); 
	ev.openqdlg_ = true;
	ev.qdlgtitle_ = msgtail;
	openqdialog_ = true;
    }

    ev.stolen_ = !popuprefnrs_.isEmpty() || (openqdialog_ && !ev.openqdlg_);

    dynamicpopupmenu_ = 0;

    if ( !ev.srcwin_ ) 
	return;

    if ( ev.begin_ || !mMatchCI(keyword,"Close") )
	ev.interrupt_ = isCmdDriverWindow( openqdialog_ ? 0 : ev.srcwin_ );
    if ( !ev.interrupt_ )
	winstack_.moveToTop( ev.srcwin_ );

    ObjectSet<CmdComposer> candidates = composers_;

    for ( int idx=candidates.size()-1; idx>=0; idx-- )
    {
	if ( candidates[idx]->greedy() )
	{
            if ( candidates[idx]->accept(ev) )
		ev.nraccepts_++;

	    candidates.remove( idx );
	}
    }

    for ( int idx=candidates.size()-1; idx>=0; idx-- )
    {
	if ( candidates[idx]->accept(ev) )
	    ev.nraccepts_++;
    }
		
    if ( ev.openqdlg_ && !ev.begin_ )
	openqdialog_ = false;

    if ( !ev.nraccepts_ && !ev.dynamicpopup_ && !ev.interrupt_ )
    {
	CmdComposer* newcomp = CmdComposer::factory( caller, keyword, rec_ );

	if ( newcomp )
	{
	    newcomp->accept( ev );
	    composers_.insertAt( newcomp, 0 );
	}
    }

    for ( int idx=composers_.size()-1; idx>=0; idx-- )
    {
	if ( composers_[idx]->done() )
	    delete composers_.remove( idx );
    }
}


void CmdRecorder::insertWinAssertion( const CmdRecEvent& ev )
{
    BufferString winstr;

    if ( ev.openqdlg_ )
	winstr = ev.qdlgtitle_;
    else if ( ev.srcwin_ )
	winstr = windowTitle( ev.srcwin_ );
    else
	return;

    mDressNameString( winstr, sWinAssert );
    
    ObjectSet<uiMainWin> ciwinlist, cswinlist;
    SearchKey(winstr, true).getMatchingWindows( cswinlist ); 
    SearchKey(winstr,false).getMatchingWindows( ciwinlist ); 

    if ( cswinlist.size() > (ev.openqdlg_ ? 0 : 1) )
    {
	winstr += "#";
	winstr += (ev.openqdlg_ ? 1 + cswinlist.size() :
				  1 + cswinlist.indexOf(ev.srcwin_) );
    }

    const bool winstrcasedep = ciwinlist.size() > cswinlist.size();
    const bool hasbecomecasedep = !winassertcasedep_ && winstrcasedep;

    if ( winstr!=winassertion_ || hasbecomecasedep )
    {
	mRecOutStrm << std::endl;
	if ( ciwinlist.size() > cswinlist.size() )
	    mRecOutStrm << "Case Sensitive" << std::endl;

	mRecOutStrm << "[" << winstr << "]" << std::endl;

	winassertion_ = winstr;
	winassertcasedep_ = winstrcasedep;
    }
}


}; // namespace CmdDrive
