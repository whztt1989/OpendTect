/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisegyexamine.cc,v 1.26 2011-03-09 13:01:18 cvsbert Exp $";

#include "uisegyexamine.h"
#include "uitextedit.h"
#include "uitable.h"
#include "uisplitter.h"
#include "uifunctiondisplay.h"
#include "uilabel.h"
#include "uitoolbutton.h"
#include "uigroup.h"
#include "uiflatviewer.h"
#include "uifiledlg.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseistrcbufviewer.h"
#include "filepath.h"
#include "ioobj.h"
#include "ptrman.h"
#include "msgh.h"
#include "seistrc.h"
#include "seisread.h"
#include "segytr.h"
#include "segyhdr.h"
#include "iopar.h"
#include "timer.h"
#include "envvars.h"
#include "statruncalc.h"
#include "separstr.h"
#include "strmprov.h"
#include "oddirs.h"
#include <sstream>

const char* uiSEGYExamine::Setup::sKeyNrTrcs = "Examine.Number of traces";


uiSEGYExamine::Setup::Setup( int nrtraces )
    : uiDialog::Setup("SEG-Y Examiner",mNoDlgTitle,"103.0.5")
    , nrtrcs_(nrtraces)
    , fp_(true)
{
    nrstatusflds( 2 ).modal( false );
}


void uiSEGYExamine::Setup::usePar( const IOPar& iop )
{
    fp_.usePar( iop ); fs_.usePar( iop );
    iop.get( sKeyNrTrcs, nrtrcs_ );
}


uiSEGYExamine::uiSEGYExamine( uiParent* p, const uiSEGYExamine::Setup& su )
	: uiDialog(p,su)
	, setup_(su)
	, timer_(*new Timer("Startup timer"))
	, tbuf_(*new SeisTrcBuf(true))
	, rdr_(0)
{
    setCtrlStyle( LeaveOnly );

    uiGroup* txtgrp = new uiGroup( this, "Txt fld group" );
    uiLabel* lbl = new uiLabel( txtgrp, "File header information" );
    uiToolButton* tb = new uiToolButton( txtgrp, "saveset.png",
	    				 "Save text header to file",
				         mCB(this,uiSEGYExamine,saveHdr) );
    tb->attach( rightBorder );
    txtfld_ = new uiTextEdit( txtgrp, "", true );
    txtfld_->setPrefHeightInChar( 14 );
    txtfld_->setPrefWidthInChar( 80 );
    txtfld_->attach( ensureBelow, lbl );

    uiGroup* logrp = new uiGroup( this, "Low group" );
    uiGroup* tblgrp = new uiGroup( logrp, "Table group" );
    lbl = new uiLabel( tblgrp, "Trace header information" );
    tb = new uiToolButton( tblgrp, "viewflat.png", "Display traces",
			     mCB(this,uiSEGYExamine,dispSeis) );

    uiTable::Setup tblsu( SEGY::TrcHeader::hdrDef().size(), setup_.nrtrcs_ );
    tblsu.rowdesc("Header field").coldesc("Trace").selmode(uiTable::SingleRow);
    tbl_ = new uiTable( tblgrp, tblsu, "Trace info" );
    tbl_->setPrefHeightInChar( 14 );
    tbl_->setPrefWidthInChar( 40 );
    tbl_->attach( ensureBelow, lbl );
    tb->attach( rightAlignedAbove, tbl_ );
    for ( int icol=0; icol<setup_.nrtrcs_; icol++ )
    {
	const int tidx = icol + 1;
	BufferString tt( "Trace header info from ", tidx );
	tt.add( getRankPostFix(tidx) ).add( " trace" );
	tbl_->setColumnLabel( icol, toString(tidx) );
	tbl_->setColumnToolTip( icol, tt );
	tbl_->setColumnReadOnly( icol, true );
    }
    tbl_->selectionChanged.notify( mCB(this,uiSEGYExamine,rowClck) );

    uiGroup* fdgrp = new uiGroup( logrp, "FuncDisp group" );
    fdtitle_ = new uiLabel( fdgrp, "" );
    fdtitle_->setStretch( 2, 0 );
    fdtitle_->setAlignment( Alignment::HCenter );
    uiFunctionDisplay::Setup fdsu;
    funcdisp_ = new uiFunctionDisplay( fdgrp, fdsu );
    funcdisp_->attach( centeredBelow, fdtitle_ );
    fdtitle_->attach( widthSameAs, funcdisp_ );
    uiGroup* valgrp = new uiGroup( fdgrp, "Value fields group" );
    rgfld_ = new uiGenInput( valgrp, "Range", IntInpIntervalSpec() );
    rgfld_->setElemSzPol( uiObject::Small );
    avgfld_ = new uiGenInput( valgrp, " Avg", FloatInpSpec() );
    avgfld_->setElemSzPol( uiObject::Small );
    avgfld_->attach( rightOf, rgfld_ );
    valgrp->attach( rightAlignedBelow, funcdisp_ );

    uiSplitter* vsplit = new uiSplitter( logrp, "VSplitter", true );
    vsplit->addGroup( tblgrp );
    vsplit->addGroup( fdgrp );

    uiSplitter* hsplit = new uiSplitter( this, "HSplitter", false );
    hsplit->addGroup( txtgrp );
    hsplit->addGroup( logrp );

    toStatusBar( setup_.fs_.fname_, 1 );
    outInfo( "Opening input" );
    rdr_ = getReader( setup_, txtinfo_ );
    txtfld_->setText( txtinfo_ );

    BufferString str( "Reading first " );
    str += su.nrtrcs_; str += " traces ...";
    outInfo( str );

    finaliseDone.notify( mCB(this,uiSEGYExamine,onStartUp) );
}


uiSEGYExamine::~uiSEGYExamine()
{
    delete rdr_;
    delete &timer_;
    delete &tbuf_;
}


void uiSEGYExamine::onStartUp( CallBacker* )
{
    timer_.tick.notify( mCB(this,uiSEGYExamine,updateInput) );
    timer_.start( 100, true );
}


void uiSEGYExamine::rowClck( CallBacker* )
{
    setRow( tbl_->currentRow() );
}


void uiSEGYExamine::saveHdr( CallBacker* )
{
    if ( !rdr_ ) return;
    FilePath fp( GetDataDir() ); fp.add( "Seismics" );
    uiFileDialog dlg( this, false, fp.fullPath() );
    if ( !dlg.go() ) return;

    StreamData sd = StreamProvider(dlg.fileName()).makeOStream();
    if ( !sd.usable() )
	{ uiMSG().error("Cannot open file for writing"); return; }

    mDynamicCastGet(SEGYSeisTrcTranslator*,tr,rdr_->translator())
    const SEGY::TxtHeader& th = *tr->txtHeader();
    BufferString buf; th.getText( buf );
    *sd.ostrm << buf << std::endl;
    sd.close();
}


void uiSEGYExamine::dispSeis( CallBacker* )
{
    uiSeisTrcBufViewer::Setup su( "Trace display", 1 );
    uiSeisTrcBufViewer* vwr = new uiSeisTrcBufViewer( this, su );
    SeisTrcBufDataPack* dp = vwr->setTrcBuf( tbuf_, Seis::Line, "Examine",
				      FilePath(setup_.fs_.fname_).fileName() );
    vwr->getViewer()->usePack( true, dp->id(), true );
    vwr->getViewer()->usePack( false, dp->id(), true );
    vwr->getViewer()->appearance().ddpars_.show( true, true );
    vwr->start();
    vwr->handleBufChange();
    vwr->show();
}


void uiSEGYExamine::updateInput( CallBacker* )
{
    display( true );
    updateInp();
    setName( setup_.fs_.fname_ );
}


void uiSEGYExamine::setRow( int irow )
{
    if ( irow < 0 ) return;

    const SEGY::HdrEntry& he = *SEGY::TrcHeader::hdrDef()[irow];
    const int nrcols = tbl_->nrCols();
    TypeSet<float> data;
    for ( int icol=0; icol<nrcols; icol++ )
	data += tbl_->getfValue( RowCol(irow,icol) );
    Stats::RunCalcSetup rcsu( false );
    rcsu.require( Stats::Average ).require( Stats::Min ).require( Stats::Max );
    Stats::RunCalc<float> rc( rcsu );
    rc.addValues( nrcols, data.arr() );
    const Interval<int> rg( mNINT(rc.min()), mNINT(rc.max()) );
    rgfld_->setValue( rg );
    if ( rg.start == rg.stop )
	avgfld_->setValue( rg.start );
    else
	avgfld_->setValue( rc.average() );
    funcdisp_->setVals( Interval<float>(1,nrcols), data.arr(), nrcols );
    fdtitle_->setText(
	    BufferString(he.name()," (",he.description()).add(")") );
}


SeisTrcReader* uiSEGYExamine::getReader( const uiSEGYExamine::Setup& su,
       					 BufferString& emsg )
{
    IOObj* ioobj = su.fs_.getIOObj( true );
    su.fp_.fillPar( ioobj->pars() );

    SeisTrcReader* rdr = new SeisTrcReader( ioobj );
    delete ioobj;
    if ( rdr->errMsg() || !rdr->prepareWork(Seis::PreScan) )
	{ emsg = rdr->errMsg(); delete rdr; return 0; }

    mDynamicCastGet(SEGYSeisTrcTranslator*,tr,rdr->translator())
    if ( !tr )
	{ emsg = "Internal: cannot obtain SEG-Y Translator";
	    	delete rdr; return 0; }

    return rdr;
}


int uiSEGYExamine::getRev() const
{
    return rdr_ ? getRev( *rdr_ ) : -1;
}


int uiSEGYExamine::getRev( const uiSEGYExamine::Setup& su, BufferString& emsg )
{
    PtrMan<SeisTrcReader> rdr = getReader( su, emsg );
    return rdr ? getRev( *rdr ) : -1;
}


int uiSEGYExamine::getRev( const SeisTrcReader& rdr )
{
    mDynamicCastGet(SEGYSeisTrcTranslator*,tr,rdr.translator())
    if ( !tr ) return -1;
    return tr->isRev1() ? 1 : 0;
}


bool uiSEGYExamine::launch( const uiSEGYExamine::Setup& su )
{
    BufferString cmd( "od_SEGYExaminer --nrtrcs " );
    cmd += su.nrtrcs_;
    if ( su.fp_.ns_ > 0 )
	{ cmd += " --ns "; cmd += su.fp_.ns_; }
    if ( su.fp_.fmt_ > 0 )
	{ cmd += " --fmt "; cmd += su.fp_.fmt_; }
    if ( su.fp_.byteswap_ )
	{ cmd += " --swapbytes "; cmd += su.fp_.byteswap_; }
    if ( su.fs_.isMultiFile() )
    {
	FileMultiString fms;
	fms += su.fs_.nrs_.start;
	fms += su.fs_.nrs_.stop;
	fms += su.fs_.nrs_.step;
	if ( su.fs_.zeropad_ > 1 )
	    fms += su.fs_.zeropad_;
	cmd += " --filenrs '"; cmd += fms; cmd += "'";
    }

    BufferString fnm( su.fs_.fname_ );
    replaceString( fnm.buf(), "*", "+x+" );
    return ExecuteScriptCommand( cmd, fnm );
}


void uiSEGYExamine::updateInp()
{
    if ( !rdr_ ) return;

    const SEGY::HdrDef& hdef = SEGY::TrcHeader::hdrDef();
    const int nrvals = hdef.size();
    mDynamicCastGet(SEGYSeisTrcTranslator*,tr,rdr_->translator())
    const SEGY::TrcHeader& trhead = tr->trcHeader();

    SeisTrc trc; int nrdone = 0;
    bool stoppedatend = false;
    for ( int itrc=0; itrc<setup_.nrtrcs_; itrc++ )
    {
	if ( !rdr_->get(trc) )
	    { stoppedatend = true; break; }

	if ( nrdone == 0 )
	    handleFirstTrace( trc, *tr );

	for ( int ival=0; ival<nrvals; ival++ )
	{
	    int val = trhead.entryVal( ival );
	    RowCol rc( ival, itrc );
	    tbl_->setValue( rc, val );
	}

	nrdone++;
	trc.info().nr = nrdone;
	tbuf_.add( new SeisTrc(trc) );
    }

    if ( stoppedatend || nrdone < 1 )
    {
	BufferString str( "\n\n" );
	const bool ismulti = !mIsUdf(setup_.fs_.nrs_.start);
	if ( nrdone < 1 )
	    str += "No traces found";
	else
	{
	    str += "Total number of traces present in file";
	    if ( ismulti ) str += "s";
	    str += ":"; str += nrdone;
	}
	txtinfo_ += str;
    }
    outInfo( "" );
    txtfld_->setText( txtinfo_ );
}


void uiSEGYExamine::handleFirstTrace( const SeisTrc& trc,
				      const SEGYSeisTrcTranslator& tr )
{
    const SEGY::TxtHeader& txthead = *tr.txtHeader();
    const SEGY::BinHeader& binhead = tr.binHeader();
    std::ostringstream thstrm, bhstrm;
    txthead.dump( thstrm );
    binhead.dump( bhstrm );

    txtinfo_ = thstrm.str().c_str();
    txtinfo_ += "\n------\n\n"
		"Binary header info (non-zero values displayed only):\n\n";
    txtinfo_ += bhstrm.str().c_str();

    const SEGY::HdrDef& hdef = SEGY::TrcHeader::hdrDef();
    const int nrvals = hdef.size();
    const SEGY::TrcHeader& trhead = tr.trcHeader();
    for ( int ival=0; ival<nrvals; ival++ )
    {
	const SEGY::HdrEntry& he( *hdef[ival] );
	BufferString rownm( "", ((int)he.bytepos_)+1 );
	rownm.add( " [" ).add( he.name() ).add( "]" );
	tbl_->setRowLabel( ival, rownm );
	tbl_->setRowToolTip( ival, he.description() );
    }

    tbl_->resizeRowsToContents();
    tbl_->resizeHeaderToContents( false );
}


bool uiSEGYExamine::rejectOK()
{
    return true;
}


void uiSEGYExamine::outInfo( const char* txt )
{
    toStatusBar( txt, 0 );
}
