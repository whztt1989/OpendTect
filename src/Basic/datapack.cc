/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "datapack.h"

#include "ascstream.h"
#include "atomic.h"
#include "debug.h"
#include "file.h"
#include "iopar.h"
#include "keystrs.h"
#include "od_ostream.h"

DataPackMgr::ID DataPackMgr::BufID()		{ return 1; }
DataPackMgr::ID DataPackMgr::PointID()		{ return 2; }
DataPackMgr::ID DataPackMgr::CubeID()		{ return 3; }
DataPackMgr::ID DataPackMgr::FlatID()		{ return 4; }
DataPackMgr::ID DataPackMgr::SurfID()		{ return 5; }
DataPackMgr::ID DataPackMgr::SeisID()		{ return 6; }
const char* DataPack::sKeyCategory()		{ return "Category"; }
float DataPack::sKb2MbFac()			{ return 0.0009765625; }

Threads::Lock DataPackMgr::mgrlistlock_;
ManagedObjectSet<DataPackMgr> DataPackMgr::mgrs_;

Threads::Atomic<int> curid( DataPack::cNoID() );


DataPack::ID DataPack::getNewID()
{
    return ++curid;
}


void DataPack::setManager( const DataPackMgr* mgr )
{
    Threads::Locker lock ( nruserslock_ );
    if ( manager_ && mgr )
    {
	if ( manager_!=mgr )
	    DBG::forceCrash( false );

	return;
    }

    manager_ = mgr;
}


void DataPack::release()
{
    const_cast<DataPackMgr*>(manager_)->release( this );
}


DataPack* DataPack::obtain()
{
    Threads::Locker lock( nruserslock_ );
    if ( !manager_ ) return 0;
    nrusers_++;

    return this;
}


DataPackMgr* DataPackMgr::gtDPM( DataPackMgr::ID dpid, bool crnew )
{
    Threads::Locker lock( mgrlistlock_ );

    for ( int idx=0; idx<mgrs_.size(); idx++ )
    {
	DataPackMgr* mgr = mgrs_[idx];
	if ( mgr->id() == dpid )
	    return mgr;
    }
    if ( !crnew )
	return 0;

    DataPackMgr* newmgr = new DataPackMgr( dpid );
    mgrs_ += newmgr;
    return newmgr;
}


DataPackMgr& DataPackMgr::DPM( DataPackMgr::ID dpid )
{
    return *gtDPM( dpid, true );
}


DataPackMgr& DPM( DataPackMgr::ID dpid )
{
    return DataPackMgr::DPM( dpid );
}


DataPackMgr& DPM( const DataPack::FullID& fid )
{
    const DataPackMgr::ID manid = fid.ID(0);
    DataPackMgr* dpm = DataPackMgr::gtDPM( manid, false );
    if ( dpm ) return *dpm;

    mDefineStaticLocalObject( PtrMan<DataPackMgr>, emptymgr, = 0 );
    emptymgr = new DataPackMgr( manid );
    return *emptymgr;
}



const char* DataPackMgr::nameOf( const DataPack::FullID& fid )
{
    return ::DPM(fid).nameOf( DataPack::getID(fid) );
}


const char* DataPackMgr::categoryOf( const DataPack::FullID& fid )
{
    return ::DPM(fid).categoryOf( DataPack::getID(fid) );
}


void DataPackMgr::dumpDPMs( od_ostream& strm )
{
    Threads::Locker lock( mgrlistlock_ );
    strm << "** Data Pack Manager dump **\n";
    if ( !mgrs_.size() )
	{ strm << "No Data pack managers (yet)" << od_newline; return; }

    for ( int imgr=0; imgr<mgrs_.size(); imgr++ )
    {
	strm << "\n\n";
	mgrs_[imgr]->dumpInfo(strm);
    }
}


DataPackMgr::DataPackMgr( DataPackMgr::ID dpid )
    : id_(dpid)
    , newPack(this)
    , packToBeRemoved(this)
{
}


DataPackMgr::~DataPackMgr()
{
    deepErase( packs_ );
}


bool DataPackMgr::haveID( DataPack::ID dpid ) const
{
    return indexOf( dpid ) >= 0;
}


float DataPackMgr::nrKBytes() const
{
    float res = 0;
    for ( int idx=0; idx<packs_.size(); idx++ )
	res += packs_[idx]->nrKBytes();
    return res;
}


void DataPackMgr::dumpInfo( od_ostream& strm ) const
{
    strm << "Manager.ID: " << id() << od_newline;
    const od_int64 nrkb = mCast(od_int64,nrKBytes());
    strm << "Total memory: " << File::getFileSizeString(nrkb)
			     << od_newline;
    ascostream astrm( strm );
    astrm.newParagraph();
    for ( int idx=0; idx<packs_.size(); idx++ )
    {
	const DataPack& pack = *packs_[idx];
	IOPar iop;
	pack.dumpInfo( iop );
	iop.putTo( astrm );
    }
}


#define mGetWriteLocker(lck,var) \
    Threads::Locker var( lck, Threads::Locker::WriteLock )

void DataPackMgr::add( DataPack* dp )
{
    if ( !dp ) return;

    Threads::Locker usrlock( dp->nruserslock_ );
    dp->setManager( this );
    usrlock.unlockNow();

    mGetWriteLocker( rwlock_, lckr );
    packs_ += dp;
    lckr.unlockNow();
    newPack.trigger( dp );
}


DataPack* DataPackMgr::addAndObtain( DataPack* dp )
{
    if ( !dp ) return 0;

    Threads::Locker lckr( dp->nruserslock_ );
    dp->nrusers_++;
    dp->setManager( this );
    lckr.unlockNow();

    mGetWriteLocker( rwlock_, rwlckr );
    const int idx = packs_.indexOf( dp );
    if ( idx==-1 )
	packs_ += dp;
    rwlckr.unlockNow();

    if ( idx==-1 )
	newPack.trigger( dp );

    return dp;
}


DataPack* DataPackMgr::doObtain( DataPack::ID dpid, bool obs ) const
{
    Threads::Locker lckr( rwlock_ );
    const int idx = indexOf( dpid );

    DataPack* res = 0;
    if ( idx!=-1 )
    {
	res = const_cast<DataPack*>( packs_[idx] );
	if ( !obs )
	{
	    Threads::Locker ulckr( res->nruserslock_ );
	    res->nrusers_++;
	}
    }

    return res;
}


int DataPackMgr::indexOf( DataPack::ID dpid ) const
{
    for ( int idx=0; idx<packs_.size(); idx++ )
    {
	if ( packs_[idx]->id()==dpid )
	    return idx;
    }

    return -1;
}


void DataPackMgr::release( DataPack::ID dpid )
{
    Threads::Locker lckr( rwlock_ );
    int idx = indexOf( dpid );
    if ( idx==-1 )
	return;

    DataPack* pack = const_cast<DataPack*>( packs_[idx] );
    Threads::Locker usrslckr( pack->nruserslock_ );
    pack->nrusers_--;

    if ( pack->nrusers_>0 )
	return;

    //We should be unlocked during callback
    //to avoid deadlocks
    usrslckr.unlockNow();
    lckr.unlockNow();

    packToBeRemoved.trigger( pack );

    mGetWriteLocker( rwlock_, wrlckr );
    usrslckr.reLock();

    if ( pack->nrusers_>0 )
	return;

    //We lost our lock, so idx may have changed.
    if ( !packs_.isPresent( pack ) )
	{ pErrMsg("Double delete detected"); }

    pack->setManager( 0 );
    usrslckr.unlockNow();

    packs_ -= pack;
    delete pack;
}


void DataPackMgr::releaseAll( bool notif )
{
    if ( !notif )
    {
	mGetWriteLocker( rwlock_, wrlckr );
	deepErase( packs_ );
    }
    else
    {
	for ( int idx=0; idx<packs_.size(); idx++ )
	    release( packs_[idx]->id() );
    }
}


#define mDefDPMDataPackFn(ret,fn) \
ret DataPackMgr::fn##Of( DataPack::ID dpid ) const \
{ \
    const int idx = indexOf( dpid ); if ( idx < 0 ) return 0; \
    return packs_[idx]->fn(); \
}

mDefDPMDataPackFn(const char*,name)
mDefDPMDataPackFn(const char*,category)
mDefDPMDataPackFn(float,nrKBytes)

void DataPackMgr::dumpInfoFor( DataPack::ID dpid, IOPar& iop ) const
{
    const int idx = indexOf( dpid ); if ( idx < 0 ) return;
    packs_[idx]->dumpInfo( iop );
}


void DataPack::dumpInfo( IOPar& iop ) const
{
    iop.set( sKeyCategory(), category() );
    iop.set( sKey::Name(), name() );
    iop.set( "Pack.ID", id_ );
    iop.set( "Nr users", nrusers_ );
    const od_int64 nrkb = mCast(od_int64,nrKBytes());
    iop.set( "Memory consumption", File::getFileSizeString(nrkb) );
}
