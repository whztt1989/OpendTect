/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : May 2004
-*/

static const char* rcsID = "$Id: wellextractdata.cc,v 1.11 2004-05-10 11:12:58 bert Exp $";

#include "wellextractdata.h"
#include "wellreader.h"
#include "wellmarker.h"
#include "welltrack.h"
#include "welld2tmodel.h"
#include "welllogset.h"
#include "welllog.h"
#include "welldata.h"
#include "welltransl.h"
#include "survinfo.h"
#include "iodirentry.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "ptrman.h"
#include "multiid.h"
#include "sorting.h"
#include "errh.h"

DefineEnumNames(Well::TrackSampler,SelPol,1,Well::TrackSampler::sKeySelPol)
	{ "Nearest trace only", "All corners", 0 };
const char* Well::TrackSampler::sKeyTopMrk = "Top marker";
const char* Well::TrackSampler::sKeyBotMrk = "Bottom marker";
const char* Well::TrackSampler::sKeyLimits = "Extraction extension";
const char* Well::TrackSampler::sKeySelPol = "Trace selection";
const char* Well::TrackSampler::sKeyDataStart = "<Start of data>";
const char* Well::TrackSampler::sKeyDataEnd = "<End of data>";
const char* Well::TrackSampler::sKeyLogNm = "Log name";
const char* Well::LogDataExtracter::sKeyLogNm = Well::TrackSampler::sKeyLogNm;

DefineEnumNames(Well::LogDataExtracter,SamplePol,2,
		Well::LogDataExtracter::sKeySamplePol)
	{ "Median", "Average", "Most frequent", "Nearest sample", 0 };
const char* Well::LogDataExtracter::sKeySamplePol = "Data sampling";


Well::InfoCollector::InfoCollector( bool dologs, bool domarkers )
    : Executor("Well information extraction")
    , domrkrs_(domarkers)
    , dologs_(dologs)
    , curidx_(0)
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    IOM().to( ctio->ctxt.stdSelKey() );
    direntries_ = new IODirEntryList( IOM().dirPtr(), ctio->ctxt );
    totalnr_ = direntries_->size();
    curmsg_ = totalnr_ ? "Gathering information" : "No wells";
}


Well::InfoCollector::~InfoCollector()
{
    deepErase( infos_ );
    deepErase( markers_ );
    deepErase( logs_ );
}


int Well::InfoCollector::nextStep()
{
    if ( curidx_ >= totalnr_ )
	return ErrorOccurred;

    IOObj* ioobj = (*direntries_)[curidx_]->ioobj;
    Well::Data wd;
    Well::Reader wr( ioobj->fullUserExpr(true), wd );
    if ( wr.getInfo() )
    {
	ids_ += new MultiID( ioobj->key() );
	infos_ += new Well::Info( wd.info() );
	if ( dologs_ )
	{
	    BufferStringSet* newlognms = new BufferStringSet;
	    wr.getLogInfo( *newlognms );
	    logs_ += newlognms;
	}
	if ( domrkrs_ )
	{
	    ObjectSet<Well::Marker>* newset = new ObjectSet<Well::Marker>;
	    markers_ += newset;
	    if ( wr.getMarkers() )
		deepCopy( *newset, wd.markers() );
	}
    }

    return ++curidx_ >= totalnr_ ? Finished : MoreToDo;
}


Well::TrackSampler::TrackSampler( const BufferStringSet& i,
				  ObjectSet<BinIDValueSet>& b )
	: Executor("Well data extraction")
	, above(0)
    	, below(0)
    	, selpol(Corners)
    	, ids(i)
    	, bivsets(b)
    	, curidx(0)
    	, timesurv(SI().zIsTime())
{
}


void Well::TrackSampler::usePar( const IOPar& pars )
{
    pars.get( sKeyTopMrk, topmrkr );
    pars.get( sKeyBotMrk, botmrkr );
    pars.get( sKeyLogNm, lognm );
    pars.get( sKeyLimits, above, below );
    const char* res = pars.find( sKeySelPol );
    if ( res && *res ) selpol = eEnum(SelPol,res);
}


#define mRetNext() { \
    delete ioobj; \
    curidx++; \
    return curidx >= ids.size() ? Finished : MoreToDo; }

int Well::TrackSampler::nextStep()
{
    if ( curidx >= ids.size() )
	return 0;

    BinIDValueSet* bivset = new BinIDValueSet;
    bivsets += bivset;

    IOObj* ioobj = IOM().get( MultiID(ids.get(curidx)) );
    if ( !ioobj ) mRetNext()
    Well::Data wd;
    Well::Reader wr( ioobj->fullUserExpr(true), wd );
    if ( !wr.getInfo() ) mRetNext()
    if ( timesurv && !wr.getD2T() ) mRetNext()
    fulldahrg = wr.getLogZRange( lognm );
    if ( mIsUndefined(fulldahrg.start) ) mRetNext()
    wr.getMarkers();

    getData( wd, *bivset );
    mRetNext();
}


void Well::TrackSampler::getData( const Well::Data& wd, BinIDValueSet& bivset )
{
    Interval<float> dahrg;
    getLimitPos(wd.markers(),true,dahrg.start);
    	if ( mIsUndefined(dahrg.start) ) return;
    getLimitPos(wd.markers(),false,dahrg.stop);
    	if ( mIsUndefined(dahrg.stop) ) return;
    if ( dahrg.start > dahrg.stop  ) return;

    float dahincr = SI().zRange().step * .5;
    if ( SI().zIsTime() )
	dahincr = 1000 * dahincr; // As dx = v * dt , Using v = 1000 m/s

    BinIDValue biv; float dah = dahrg.start;
    int trackidx = 0; Coord3 precisepos;
    if ( !getSnapPos(wd,dah,biv,trackidx,precisepos) )
	return;

    addBivs( bivset, biv, precisepos );
    BinIDValue prevbiv = biv;

    while ( true )
    {
	dah += dahincr;
	if ( dah > dahrg.stop || !getSnapPos(wd,dah,biv,trackidx,precisepos) )
	    return;

	if ( biv.binid != prevbiv.binid || !mIS_ZERO(biv.value-prevbiv.value) )
	{
	    addBivs( bivset, biv, precisepos );
	    prevbiv = biv;
	}
    }
}


void Well::TrackSampler::getLimitPos( const ObjectSet<Marker>& markers,
				      bool isstart, float& val ) const
{
    const BufferString& mrknm = isstart ? topmrkr : botmrkr;
    if ( mrknm == sKeyDataStart )
	val = fulldahrg.start;
    else if ( mrknm == sKeyDataEnd )
	val = fulldahrg.stop;
    else
    {
	val = mUndefValue;
	for ( int idx=0; idx<markers.size(); idx++ )
	{
	    if ( markers[idx]->name() == mrknm )
	    {
		val = markers[idx]->dah;
		break;
	    }
	}
    }

    float shft = isstart ? (mIsUndefined(above) ? above : -above) : below;
    if ( mIsUndefined(val) )
	return;

    if ( !mIsUndefined(shft) )
	val += shft;
}


bool Well::TrackSampler::getSnapPos( const Well::Data& wd, float dah,
				     BinIDValue& biv, int& trackidx,
				     Coord3& pos ) const
{
    const int tracksz = wd.track().size();
    while ( trackidx < tracksz && dah > wd.track().dah(trackidx) )
	trackidx++;
    if ( trackidx < 1 || trackidx >= tracksz )
	return false;

    // Position is between trackidx and trackidx-1
    pos = wd.track().coordAfterIdx( dah, trackidx-1 );
    biv.binid = SI().transform( pos );
    if ( SI().zIsTime() && wd.d2TModel() )
    {
	pos.z = wd.d2TModel()->getTime( dah );
	if ( mIsUndefined(pos.z) )
	    return false;
    }
    biv.value = SI().zRange().snap( pos.z );
    return true;
}


void Well::TrackSampler::addBivs( BinIDValueSet& bivset, const BinIDValue& biv,
				  const Coord3& precisepos ) const
{
    bivset += biv;
    if ( selpol == Corners )
    {
	BinID stp( SI().inlStep(), SI().crlStep() );
	BinID bid( biv.binid.inl+stp.inl, biv.binid.crl+stp.crl );
	Coord crd = SI().transform( bid );
	double dist = crd.distance( precisepos );
	BinID nearest = bid; double lodist = dist;

#define mTestNext(op1,op2) \
	bid = BinID( biv.binid.inl op1 stp.inl, biv.binid.crl op2 stp.crl ); \
	crd = SI().transform( bid ); \
	dist = crd.distance( precisepos ); \
	if ( dist < lodist ) \
	    { lodist = dist; nearest = bid; }

	mTestNext(+,-)
	mTestNext(-,+)
	mTestNext(-,-)

	BinIDValue newbiv( biv );
	newbiv.binid.inl = nearest.inl; bivset += newbiv;
	newbiv.binid.crl = nearest.crl; bivset += newbiv;
	newbiv.binid.inl = biv.binid.inl; bivset += newbiv;
    }
}


Well::LogDataExtracter::LogDataExtracter( const BufferStringSet& i,
					  const ObjectSet<BinIDValueSet>& b )
	: Executor("Well log data extraction")
	, ids(i)
	, bivsets(b)
    	, samppol(Med)
	, curidx(0)
    	, timesurv(SI().zIsTime())
{
}


void Well::LogDataExtracter::usePar( const IOPar& pars )
{
    pars.get( sKeyLogNm, lognm );
    const char* res = pars.find( sKeySamplePol );
    if ( res && *res ) samppol = eEnum(SamplePol,res);
}


#define mRetNext() { \
    delete ioobj; \
    curidx++; \
    return curidx >= ids.size() ? Finished : MoreToDo; }

int Well::LogDataExtracter::nextStep()
{
    if ( curidx >= ids.size() )
	return 0;
    IOObj* ioobj = 0;
    const BinIDValueSet& bivset = *bivsets[curidx];
    if ( !bivset.size() ) mRetNext()

    ioobj = IOM().get( MultiID(ids.get(curidx)) );
    if ( !ioobj ) mRetNext()
    Well::Data wd;
    Well::Reader wr( ioobj->fullUserExpr(true), wd );
    if ( !wr.getInfo() ) mRetNext()

    PtrMan<Well::Track> timetrack = 0;
    if ( timesurv )
    {
	if ( !wr.getD2T() ) mRetNext()
	timetrack = new Well::Track( wd.track() );
	timetrack->toTime( *wd.d2TModel() );
    }
    const Well::Track& track = timesurv ? *timetrack : wd.track();
    if ( track.size() < 2 ) mRetNext()
    if ( !wr.getLogs() ) mRetNext()
    
    TypeSet<float>* newres = new TypeSet<float>;
    ress += newres;
    getData( bivset, wd, track, *newres );

    mRetNext();
}


void Well::LogDataExtracter::getData( const BinIDValueSet& bivset,
				      const Well::Data& wd,
				      const Well::Track& track,
				      TypeSet<float>& res ) const
{
    int wlidx = wd.logs().indexOf( lognm );
    if ( wlidx < 0 )
	return;
    const Well::Log& wl = wd.logs().getLog( wlidx );

    if ( !track.alwaysDownward() )
    {
	// Slower, less precise
	getGenTrackData( bivset, wd, track, wl, res );
	return;
    }

    int bividx = 0; int trackidx = 0;
    const float tol = 0.001;
    float z1 = track.pos(trackidx).z;
    while ( bividx < bivset.size() && bivset[bividx].value < z1 - tol )
	bividx++;
    if ( bividx >= bivset.size() ) // Duh. All data below track.
	return;

    BinIDValue biv( bivset[bividx] );
    for ( trackidx=1; trackidx<track.size(); trackidx++ )
    {
	if ( track.pos(trackidx).z > biv.value - tol )
	    break;
    }
    if ( trackidx >= track.size() ) // Duh. Entire track below data.
	return;

    for ( ; bividx<bivset.size(); bividx++ )
    {
	biv = bivset[bividx];
	float z2 = track.pos( trackidx ).z;
	while ( biv.value > z2 )
	{
	    trackidx++;
	    if ( trackidx >= track.size() )
		return;
	}
	z1 = track.pos( trackidx - 1 ).z;
	float dah = ( (biv.value-z2) * track.dah(trackidx-1)
		    + (z1-biv.value) * track.dah(trackidx) )
		  / (z2 - z1);
	float vel = timesurv ? wd.d2TModel()->getVelocity(dah) : 1;
	addValAtDah( dah, wl, vel, res );
    }
}


void Well::LogDataExtracter::getGenTrackData( const BinIDValueSet& bivset,
					      const Well::Data& wd,
					      const Well::Track& track,
					      const Well::Log& wl,
					      TypeSet<float>& res ) const
{
    int bividx = 0; int trackidx = 0;
    const float tol = 0.001;
    while ( bividx<bivset.size() && mIsUndefined(bivset[bividx].value) )
	bividx++;
    if ( bividx >= bivset.size() || !track.size() )
	return;

    BinIDValue biv( bivset[bividx] );
    const float dahstep = SI().zRange().step / 2;
    const float extratol = 1.01; // Allow 1% extra tolerance
    const float ztolbase = extratol * dahstep;
    BinID b( biv.binid.inl+SI().inlStep(),  biv.binid.crl+SI().crlStep() );
    const float dtol = SI().transform(biv.binid).distance( SI().transform(b) )
		     * extratol;

    float dah = track.dah(0);
    const float lastdah = track.dah( track.size() - 1 );
    for ( ; bividx<bivset.size(); bividx++ )
    {
	biv = bivset[bividx];
	Coord coord = SI().transform( biv.binid );
	float vel = 1;
	while ( dah <= lastdah )
	{
	    Coord3 pos = track.getPos( dah );
	    if ( timesurv ) vel = wd.d2TModel()->getVelocity(dah);
	    float ztol = ztolbase * vel;
	    if ( coord.distance(pos) < dtol && fabs(pos.z-biv.value) < ztol )
		break;
	    dah += dahstep;
	}
	if ( dah > lastdah ) return;
	addValAtDah( dah, wl, vel, res );
    }
}


void Well::LogDataExtracter::addValAtDah( float dah, const Well::Log& wl,
					  float vel, TypeSet<float>& res ) const
{
    float val;
    if ( samppol == Nearest )
	val = wl.getValue( dah );
    else
	val = calcVal( wl, dah, SI().zRange().step * vel );

    if ( !mIsUndefined(val) )
	res += val;
}


float Well::LogDataExtracter::calcVal( const Well::Log& wl, float dah,
				       float winsz ) const
{
    Interval<float> rg( dah-winsz, dah+winsz );
    TypeSet<float> vals;
    for ( int idx=0; idx<wl.size(); idx++ )
    {
	if ( rg.includes(wl.dah(idx)) )
	{
	    float val = wl.value(idx);
	    if ( !mIsUndefined(val) )
		vals += wl.value(idx);
	}
    }
    if ( vals.size() < 1 ) return mUndefValue;
    if ( vals.size() == 1 ) return vals[0];
    if ( vals.size() == 2 ) return samppol == Avg ? (vals[0]+vals[1])/2
						  : vals[0];

    const int sz = vals.size();
    if ( samppol == Med )
    {
	sort_array( vals.arr(), sz );
	return vals[sz/2];
    }
    else if ( samppol == Avg )
    {
	float val = 0;
	for ( int idx=0; idx<sz; idx++ )
	    val += vals[idx];
	return val / sz;
    }
    else if ( samppol == MostFreq )
    {
	TypeSet<float> valsseen;
	TypeSet<int> valsseencount;
	valsseen += vals[0]; valsseencount += 0;
	for ( int idx=1; idx<sz; idx++ )
	{
	    float val = vals[idx];
	    for ( int ivs=0; ivs<valsseen.size(); ivs++ )
	    {
		float vs = valsseen[ivs];
		if ( mIS_ZERO(vs-val) )
		    { valsseencount[ivs]++; break; }
		if ( ivs == valsseen.size()-1 )
		    { valsseen += val; valsseencount += 0; }
	    }
	}

	int maxvsidx = 0;
	for ( int idx=1; idx<valsseencount.size(); idx++ )
	{
	    if ( valsseencount[idx] > valsseencount[maxvsidx] )
		maxvsidx = idx;
	}
	return valsseen[ maxvsidx ];
    }
    else
    {
	pErrMsg( "SamplePol not supported" );
	return vals[0];
    }
}
