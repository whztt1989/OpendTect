/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2007
-*/


#include "envvars.h"
#include "filepath.h"
#include "iopar.h"
#include "keystrs.h"
#include "madprocexec.h"
#include "madprocflow.h"
#include "madstream.h"
#include "od_ostream.h"
#include "oddirs.h"
#include "strmprov.h"
#include "progressmeterimpl.h"
#include "perthreadrepos.h"
#include "uistrings.h"
#include <iostream>

const char* ODMad::ProcExec::sKeyFlowStage()	{ return "Flow Stage"; }
const char* ODMad::ProcExec::sKeyCurProc()	{ return "Current proc"; }

mDefineEnumUtils(ODMad::ProcExec,FlowStage,"Flow Stage")
{ "Start", "Intermediate", "Finish", 0 };


ODMad::ProcExec::ProcExec( const IOPar& iop, od_ostream& reportstrm )
    : Executor("Madagascar processing")
    , pars_(*new IOPar(iop))
    , strm_(reportstrm)
    , nrdone_(0)
    , stage_(Start)
    , madstream_(0)
    , procstream_(*new StreamData)
    , plotstream_(*new StreamData)
    , progmeter_(0)
    , trc_(0)
{
    FlowStageDef().parse( pars_.find( sKeyFlowStage() ), stage_ );
}


ODMad::ProcExec::~ProcExec()
{
    delete &pars_;
    delete &procstream_; delete &plotstream_;
    delete madstream_; delete progmeter_;
    delete [] trc_;
}


#define mErrRet(s) { errmsg_ = s; return false; }
bool ODMad::ProcExec::init()
{
    delete madstream_; madstream_ = 0;
    if ( stage_ == Finish )
    {
	pars_.setYN( "Write", true );
	madstream_ = new ODMad::MadStream( pars_ );
	if ( !madstream_->isOK() )
	    mErrRet( madstream_->errMsg() )

	if ( !madstream_->writeTraces() )
	    mErrRet( madstream_->errMsg() )
    }
    else
    {
	madstream_ = new ODMad::MadStream( pars_ );
	if ( !madstream_->isOK() )
	    mErrRet( madstream_->errMsg() )

	PtrMan<IOPar> inpar = pars_.subselect( ODMad::ProcFlow::sKeyInp() );
	if ( !inpar || !inpar->size() )
	    mErrRet(uiStrings::sInputParamsMissing())

	ODMad::ProcFlow::IOType inptyp = ODMad::ProcFlow::ioType( *inpar );
	const char* comm = getProcString();
	if ( !comm || !*comm )
	    return false;

	std::cerr << "About to execute: " << comm << std::endl;
	if ( stage_ == Start && ( inptyp == ODMad::ProcFlow::None
				|| inptyp == ODMad::ProcFlow::SU ) )
	{
	    BufferString cmd( comm + 1 );
	    if ( inptyp == ODMad::ProcFlow::SU )
	    {
		const BufferString rsfroot = GetEnvVar( "RSFROOT" );
		cmd = File::Path( rsfroot, "bin", "sfsu2rsf" ).fullPath();
		cmd += " "; cmd += "tape=";
		cmd += inpar->find( sKey::FileName() );
		cmd += " | "; cmd += comm + 1;
	    }

	    return !system( cmd.buf() );
	}

	if ( FixedString(comm) == StreamProvider::sStdIO() )
	    procstream_.ostrm = &std::cout;
	else
	    procstream_ = StreamProvider( comm ).makeOStream();

	if ( !procstream_.usable() )
	    mErrRet(tr("Failed to create output stream"))

        od_ostream procstrm(*procstream_.ostrm);
	if ( !madstream_->putHeader(procstrm) )
	    mErrRet(tr("Failed to get RSF header"))

	if ( stage_ == Intermediate )
	{
	    BufferString plotcomm = "@";
	    plotcomm += getPlotString();
	    std::cerr << "About to plot: " << plotcomm << std::endl;
	    plotstream_ = StreamProvider( plotcomm.buf() ).makeOStream();
            od_ostream plotstrm( *plotstream_.ostrm );
	    if ( !madstream_->putHeader(plotstrm) )
		mErrRet(tr("Failed to put RSF header in plot stream"))
	}

	const int trcsize = madstream_->getNrSamples();
	trc_ = new float[trcsize];
    }

    return true;
}


#ifdef __win__
    #define mAddNewExec \
        BufferString fname = File::Path::getTempName( "par" ); \
	pars_.write( fname, sKey::Pars() ); \
	ret += File::Path(rsfroot).add("bin").add("sfdd").fullPath(); \
	ret += " form=ascii_float | \""; \
	ret += File::Path(GetExecPlfDir()).add("od_madexec").fullPath(); \
	ret += "\" "; ret += fname
#else
    #define mAddNewExec \
	BufferString fname = File::Path::getTempName( "par" ); \
	pars_.write( fname, sKey::Pars() ); \
	ret += GetExecScript( false ); ret += " "; \
	ret += "od_madexec"; ret += " "; ret += fname
#endif

const char* ODMad::ProcExec::getProcString()
{
    const BufferString rsfroot = GetEnvVar( "RSFROOT" );
    mDeclStaticString( ret );
    ret = "@";
    ODMad::ProcFlow procflow;
    procflow.usePar( pars_ );
    ODMad::ProcFlow::IOType outtyp = procflow.ioType( false );
    int curprocidx = 0;
    if ( pars_.get(sKeyCurProc(),curprocidx) && curprocidx < 0 )
	return 0;

    if ( !curprocidx )
    {
	pars_.write( strm_, IOPar::sKeyDumpPretty() );
	progmeter_ = new TextStreamProgressMeter( strm_ );
	progmeter_->setName( "Madagascar Processing:" );
	progmeter_->setMessage( m3Dots(tr("Reading Traces")) );
    }

    const bool hasprocessing = procflow.size() && curprocidx < procflow.size();
    if ( !hasprocessing )
    {
	if ( outtyp == ODMad::ProcFlow::Madagascar )
	    ret = procflow.output().find( sKey::FileName() );
	else if ( outtyp == ODMad::ProcFlow::None )
	    return 0;
	else
	{
	    pars_.set( sKeyFlowStage(), toString(Finish) );
	    pars_.set( sKey::LogFile(), StreamProvider::sStdErr() );
	    mAddNewExec;
	}

	return ret.buf();
    }

    bool firstproc = true;
    for ( int pidx=curprocidx; pidx<procflow.size(); pidx++ )
    {
	if ( !firstproc )
	    ret += " | ";
	else
	{
#ifdef __win__
	    ret += File::Path(rsfroot).add("bin").add("sfdd").fullPath();
	    ret += " form=native_float | ";
#endif
	    firstproc = false;
	}

	if ( !procflow[pidx]->isValid() )
	    ret += procflow[pidx]->getCommand();
	else
	{
	    const File::Path fp( rsfroot, "bin", procflow[pidx]->getCommand() );
	    ret += fp.fullPath();
	}

	const char* plotcmd = procflow[pidx]->auxCommand();
	const bool hasplot = plotcmd && *plotcmd;
	const bool endproc = pidx == procflow.size()-1;
	const bool nooutput = outtyp == ODMad::ProcFlow::None;
	if ( hasplot )
	{
	    FlowStage newstage = endproc && nooutput ? Finish : Intermediate;
	    pars_.set( sKeyCurProc(), pidx + 1 );
	    pars_.set( sKey::LogFile(), StreamProvider::sStdErr() );
	    pars_.set( IOPar::compKey(ODMad::ProcFlow::sKeyInp(),sKey::Type() ),
		       "Madagascar" );
	    pars_.removeWithKey( IOPar::compKey(ODMad::ProcFlow::sKeyInp(),
						 sKey::FileName()) );
	    pars_.set( sKeyFlowStage(), toString(newstage) );
	    ret += " | ";
	    if ( endproc && nooutput )
	    {
		ret += getPlotString();
		break;
	    }

	    mAddNewExec;
	    pars_.set( sKeyCurProc(), curprocidx );
	    break;
	}

	if ( endproc )
	{
	    if ( outtyp == ODMad::ProcFlow::Madagascar )
	    {
		ret += " out=stdout > ";
		ret += procflow.output().find( sKey::FileName() );
	    }
	    else
	    {
		pars_.set( sKeyFlowStage(), toString(Finish) );
		pars_.set( sKey::LogFile(), StreamProvider::sStdErr() );
		ret += " | ";
		mAddNewExec;
	    }
	}
    }

    return ret.buf();
}


const char* ODMad::ProcExec::getPlotString() const
{
    const BufferString rsfroot = GetEnvVar( "RSFROOT" );
    mDeclStaticString( ret );
    int curprocidx = 0;
    if ( !pars_.get(sKeyCurProc(),curprocidx) || curprocidx < 1 )
	return 0;

    ODMad::ProcFlow procflow;
    procflow.usePar( pars_ );
    BufferString plotcmd = procflow[curprocidx-1]->auxCommand();
    if ( plotcmd.isEmpty() )
	return 0;

    File::Path fp( rsfroot, "bin" );
    char* pipechar = plotcmd.find( '|' );
    if ( pipechar )
    {
	BufferString tmp = pipechar + 2;
	*(pipechar+2) = '\0';
	fp.add( tmp );
	plotcmd += fp.fullPath();
	fp.setFileName( 0 );
    }

    fp.add( plotcmd );
    ret += fp.fullPath();
    ret += "&";
    return ret.buf();
}


uiString ODMad::ProcExec::uiMessage() const
{ return tr("Working"); }


uiString ODMad::ProcExec::uiNrDoneText() const
{ return tr("Traces handled"); }


od_int64 ODMad::ProcExec::totalNr() const
{
    return -1;
}

#define mWriteToStream(strm) \
    if ( madstream_->isBinary() ) \
        (*strm.ostrm).write( (const char*) &val, sizeof(val)); \
    else \
	*strm.ostrm << val << " ";

int ODMad::ProcExec::nextStep()
{
    if ( stage_ == Finish || !madstream_ || !madstream_->getNextTrace(trc_) )
    {
	procstream_.close();
	plotstream_.close();
	if ( progmeter_ ) progmeter_->setFinished();
	return Executor::Finished();
    }

    const int trcsize = madstream_->getNrSamples();
    for ( int idx=0; idx<trcsize; idx++ )
    {
	const float val = trc_[idx];
	mWriteToStream( procstream_ )
	if ( stage_ == Intermediate && plotstream_.usable() )
	{ mWriteToStream( plotstream_ ) }
    }

    if ( progmeter_ ) progmeter_->setNrDone( ++nrdone_ );
    return Executor::MoreToDo();
}
