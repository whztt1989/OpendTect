/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uipickpartserv.cc,v 1.46 2007-08-22 12:27:57 cvsraman Exp $
________________________________________________________________________

-*/

#include "uipickpartserv.h"

#include "uicursor.h"
#include "uicreatepicks.h"
#include "uiimppickset.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uipicksetman.h"

#include "ioman.h"
#include "pickset.h"
#include "picksettr.h"
#include "surfaceinfo.h"
#include "color.h"
#include "ioobj.h"
#include "survinfo.h"
#include "statrand.h"
#include "ptrman.h"

const int uiPickPartServer::evGetHorInfo2D = 0;
const int uiPickPartServer::evGetHorInfo3D = 1;
const int uiPickPartServer::evGetHorDef = 2;
const int uiPickPartServer::evFillPickSet = 3;
const int uiPickPartServer::evGet2DLineInfo = 4;
const int uiPickPartServer::evGet2DLineDef = 5;


uiPickPartServer::uiPickPartServer( uiApplService& a )
	: uiApplPartServer(a)
	, uiPickSetMgr(Pick::Mgr())
    	, gendef_(2,true)
{
}


uiPickPartServer::~uiPickPartServer()
{
    deepErase( selhorids_ );
    deepErase( linegeoms_ );
}


void uiPickPartServer::managePickSets()
{
    uiPickSetMan dlg( appserv().parent() );
    dlg.go();
}


void uiPickPartServer::impexpSet( bool import )
{
    uiImpExpPickSet dlg( this, import );
    dlg.go();
}


void uiPickPartServer::fetchHors( bool is2d )
{
    deepErase( hinfos_ );
    sendEvent( is2d ? evGetHorInfo2D : evGetHorInfo3D );
}

bool uiPickPartServer::loadSets()
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSet);
    ctio->ctxt.forread = true;
    uiIOObjSelDlg dlg( appserv().parent(), *ctio, 0, true );
    if ( !dlg.go() ) return false;

    bool retval = false;
    const int nrsel = dlg.nrSel();
    for ( int idx=0; idx<nrsel; idx++ )
    {
	const MultiID id = dlg.selected(idx);
	PtrMan<IOObj> ioobj = IOM().get( id );
	if ( setmgr_.indexOf(ioobj->key()) >= 0 )
	    continue;

	Pick::Set* newps = new Pick::Set;
	BufferString bs;
	if ( PickSetTranslator::retrieve(*newps,ioobj,bs) )
	{
	    setmgr_.set( ioobj->key(), newps );
	    retval = true;
	}
	else
	{
	    delete newps;
	    if ( idx == 0 )
	    {
		uiMSG().error( bs );
		return false;
	    }
	    else
	    {
		BufferString msg( ioobj->name() );
		msg += ": "; msg += bs;
		uiMSG().warning( msg );
	    }
	}
    }

    return retval;
}


#define mHandleDlg() \
    if ( !dlg.go() ) \
        return false; \
    Pick::Set* newps = new Pick::Set( dlg.getName() ); \
    newps->disp_.color_ = dlg.getPickColor(); 

bool uiPickPartServer::createSet( bool rand, bool is2d )
{
    if ( !rand )
    {
	uiCreatePicks dlg( parent() );
	mHandleDlg();
	if ( newps )
	    return storeNewSet( newps );
    }

    fetchHors( false );
    BufferStringSet hornms;
    for ( int idx=0; idx<hinfos_.size(); idx++ )
	hornms.add( hinfos_[idx]->name );

    if ( is2d )
    {
	deepErase( linesets_ );
	linenms_.erase();
	sendEvent( evGet2DLineInfo );
	uiGenRandPicks2D dlg( parent(), hornms, linesets_, linenms_ );
	mHandleDlg();
	if ( !mkRandLocs2D(*newps,dlg.randPars()) )
	{ delete newps; newps = 0; }
	if ( newps )
	    return storeNewSet( newps );
    }
    else
    {
	uiGenRandPicks3D dlg( parent(), hornms );
	mHandleDlg();
	if ( !mkRandLocs3D(*newps,dlg.randPars()) )
	{ delete newps; newps = 0; }
	if ( newps )
	    return storeNewSet( newps );
    }

    return false;
}


bool uiPickPartServer::mkRandLocs3D( Pick::Set& ps, const RandLocGenPars& rp )
{
    uiCursorChanger cursorlock( uiCursor::Wait );

    Stats::RandGen::init();
    selbr_ = &rp.bidrg_;
    const bool do2hors = rp.needhor_ && rp.horidx2_ >= 0 && 
			 rp.horidx2_ != rp.horidx_;
    gendef_.empty();
    deepErase( selhorids_ );
    if ( rp.needhor_ )
    {
	selhorids_ += new MultiID( hinfos_[rp.horidx_]->multiid );
	if ( do2hors )
	    selhorids_ += new MultiID( hinfos_[rp.horidx2_]->multiid );
	sendEvent( evGetHorDef );
    }
    else
    {
	const BinID stp = BinID( SI().inlStep(), SI().crlStep() );
	BinID bid;
	for ( bid.inl=selbr_->start.inl; bid.inl<=selbr_->stop.inl;
		bid.inl +=stp.inl )
	{
	    for ( bid.crl=selbr_->start.crl; bid.crl<=selbr_->stop.crl;
		    	bid.crl += stp.crl )
		gendef_.add( bid, rp.zrg_.start, rp.zrg_.stop );
	}
    }

    const int nrpts = gendef_.totalSize();
    if ( !nrpts ) return true;

    BinID bid; Interval<float> zrg;
    for ( int ipt=0; ipt<rp.nr_; ipt++ )
    {
	const int ptidx = Stats::RandGen::getIndex( nrpts );
	BinIDValueSet::Pos pos = gendef_.getPos( ptidx );
	gendef_.get( pos, bid, zrg.start, zrg.stop );
	float val = zrg.start + Stats::RandGen::get() * zrg.width();

	ps += Pick::Location( SI().transform(bid), val );
    }

    gendef_.empty();
    return true;
}


bool uiPickPartServer::mkRandLocs2D(Pick::Set& ps,const RandLocGenPars& rp)
{
    uiCursorChanger cursorlock( uiCursor::Wait );

    Stats::RandGen::init();
    setid_ = setids_[rp.lsetidx_];
    selectlines_ = rp.linenms_;
    deepErase( linegeoms_ );
    deepErase( selhorids_ );
    if ( rp.needhor_ )
	return false;
    else
    {
	sendEvent( evGet2DLineDef );
	if ( !linegeoms_.size() ) return false;

	TypeSet<Coord> coords;
	for ( int ldx=0; ldx<linegeoms_.size(); ldx++ )
	    for ( int tdx=0; tdx<linegeoms_[ldx]->posns.size(); tdx++ )
		coords += linegeoms_[ldx]->posns[tdx].coord_;

	const int nrpos = coords.size();
	if ( !nrpos ) return false;

	for ( int ipt=0; ipt<rp.nr_; ipt++ )
	{
	    const int posidx = Stats::RandGen::getIndex( nrpos );
	    float val = rp.zrg_.start 
				+ Stats::RandGen::get() * rp.zrg_.width(false);
	    ps += Pick::Location( coords[posidx], val );
	}
    }

    deepErase( linegeoms_ );
    return true;
}


void uiPickPartServer::setMisclassSet( const BinIDValueSet& bivs )
{
    static const char* sKeyMisClass = "Misclassified [NN]";
    int setidx = setmgr_.indexOf( sKeyMisClass );
    const bool isnew = setidx < 0;
    Pick::Set* ps = isnew ? 0 : &setmgr_.get( setidx );
    if ( ps )
	ps->erase();
    else
    {
	ps = new Pick::Set( sKeyMisClass );
	ps->disp_.color_.set( 240, 0, 0 );
    }

    BinIDValueSet::Pos pos; BinIDValue biv;
    while ( bivs.next(pos,false) )
    {
	bivs.get( pos, biv );
	Coord crd = SI().transform( biv.binid );
	*ps += Pick::Location( crd.x, crd.y, biv.value );
    }

    if ( isnew )
	storeNewSet( ps );
    else
    {
	PtrMan<IOObj> ioobj = getSetIOObj( *ps );
	if ( ioobj )
	    doStore( *ps, *ioobj );
    }
}


void uiPickPartServer::fillZValsFrmHor( Pick::Set* ps, int horidx )
{
    ps_ = ps;
    horid_ = hinfos_[horidx]->multiid;
    sendEvent( evFillPickSet );
}
