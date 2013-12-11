/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "welld2tmodel.h"
#include "welltrack.h"
#include "idxable.h"
#include "iopar.h"
#include "stratlevel.h"
#include "survinfo.h"
#include "tabledef.h"

const char* Well::D2TModel::sKeyTimeWell()	{ return "=Time"; }
const char* Well::D2TModel::sKeyDataSrc()	{ return "Data source"; }


Well::D2TModel& Well::D2TModel::operator =( const Well::D2TModel& d2t )
{
    if ( &d2t != this )
    {
	setName( d2t.name() );
	desc = d2t.desc; datasource = d2t.datasource;
	dah_ = d2t.dah_; t_ = d2t.t_;
    }
    return *this;
}


bool Well::D2TModel::insertAtDah( float dh, float val )
{
    mWellDahObjInsertAtDah( dh, val, t_, true  );
    return true;
}


float Well::D2TModel::getTime( float dh, const Track& track ) const
{
    float twt = mUdf(float);
    Interval<double> depths( mUdf(double), mUdf(double) );
    Interval<float> times( mUdf(float), mUdf(float) );

    if ( !getVelocityBoundsForDah(dh,track,depths,times) )
	return twt;

    double reqz = track.getPos(dh).z;
    const double curvel = getVelocityForDah( dh, track );
    twt = times.start + 2.f* (float)( ( reqz - depths.start ) / curvel );

    return twt;
}


float Well::D2TModel::getDepth( float twt, const Track& track ) const
{
    Interval<float> times( mUdf(float), mUdf(float) );
    Interval<double> depths( mUdf(double), mUdf(double) );

    if ( !getVelocityBoundsForTwt(twt,track,depths,times) )
	return mUdf(float);

    const double curvel = getVelocityForTwt( twt, track );
    const double depth = depths.start +
	( ( mCast(double,twt) - mCast(double,times.start) ) * curvel ) / 2.f;

    return mCast( float, depth );
}


float Well::D2TModel::getDah( float twt, const Track& track ) const
{
    const float depth = getDepth( twt, track );
    if ( mIsUdf(depth) )
        return mUdf(float);

    return track.getDahForTVD( depth );
}


double Well::D2TModel::getVelocityForDah( float dh, const Track& track ) const
{
    double velocity = mUdf(double);
    Interval<double> depths( mUdf(double), mUdf(double) );
    Interval<float> times( mUdf(float), mUdf(float) );

    if ( !getVelocityBoundsForDah(dh,track,depths,times) )
	return velocity;

    velocity = 2.f * depths.width() / (double)times.width();

    return velocity;
}


double Well::D2TModel::getVelocityForDepth( float dpt,
					    const Track& track ) const
{
    const float dahval = track.getDahForTVD( dpt );
    return getVelocityForDah( dahval, track );
}


double Well::D2TModel::getVelocityForTwt( float twt, const Track& track ) const
{
    double velocity = mUdf(double);
    Interval<double> depths( mUdf(double), mUdf(double) );
    Interval<float> times( mUdf(float), mUdf(float) );

    if ( !getVelocityBoundsForTwt(twt,track,depths,times) )
	return velocity;

    velocity = 2.f * depths.width() / (double)times.width();

    return velocity;
}


bool Well::D2TModel::getVelocityBoundsForDah( float dh, const Track& track,
					Interval<double>& depths,
					Interval<float>& times ) const
{
    const int idah = getVelocityIdx( dh, track );
    if ( idah < 0 )
	return false;

    depths.start = track.getPos(dah_[idah-1]).z;
    depths.stop = track.getPos(dah_[idah]).z;
    times.start = t_[idah-1];
    times.stop = t_[idah];

    if ( depths.isUdf() )
        return false;

    bool reversedz = times.isRev() || depths.isRev();
    bool sametwt = mIsZero(times.width(),1e-6f) || mIsZero(depths.width(),1e-6);

    if ( reversedz || sametwt )
	return getOldVelocityBoundsForDah(dh,track,depths,times);

    return true;
}


bool Well::D2TModel::getVelocityBoundsForTwt( float twt, const Track& track,						      Interval<double>& depths,
					      Interval<float>& times ) const
{
    const int idah = getVelocityIdx( twt, track, false );
    if ( idah < 0 )
	return false;

    depths.start = track.getPos(dah_[idah-1]).z;
    depths.stop = track.getPos(dah_[idah]).z;
    if ( depths.isUdf() )
	return false;

    times.start = t_[idah-1];
    times.stop = t_[idah];

    bool reversedz = times.isRev() || depths.isRev();
    bool sametwt = mIsZero(times.width(),1e-6f) || mIsZero(depths.width(),1e-6);

    if ( reversedz || sametwt )
	return getOldVelocityBoundsForTwt(twt,track,depths,times);

    return true;
}


int Well::D2TModel::getVelocityIdx( float pos, const Track& track,
				    bool posisdah ) const
{
    const int dtsize = size();
    const int tsize = t_.size();
    if ( dtsize != tsize )
	return -1;

    int idah = posisdah
	     ? IdxAble::getUpperIdx( dah_, dtsize, pos )
	     : IdxAble::getUpperIdx( t_, dtsize, pos );

    if ( idah >= (dtsize-1) && posisdah )
    {
	int idx = 0;
	const int trcksz = track.size();
	const TypeSet<Coord3> allpos = track.getAllPos();
	TypeSet<double> trckposz;
	for ( int idz=0; idz<trcksz; idz++ )
	    trckposz += allpos[idz].z;

	const double reqz = track.getPos( pos ).z;
	idx = IdxAble::getUpperIdx( trckposz, trcksz, reqz );
	if ( idx >= trcksz ) idx--;
	const double dhtop = mCast( double, track.dah(idx-1) );
	const double dhbase = mCast( double, track.dah(idx) );
	const double fraction = ( reqz - track.value(idx-1) ) /
				( track.value(idx) - track.value(idx-1) );
	const float reqdh = mCast( float, dhtop + (fraction * (dhbase-dhtop)) );
	idah = IdxAble::getUpperIdx( dah_, dtsize, reqdh );
    }

    return idah >= dtsize ? dtsize-1 : idah;
}


int Well::D2TModel::getDahIndex( float dh, const Track& track) const
{
    return getVelocityIdx( dh, track );
}


bool Well::D2TModel::getOldVelocityBoundsForDah( float dh, const Track& track,
						 Interval<double>& depths,
						 Interval<float>& times ) const
{
    const int dtsize = size();
    const int idah = getVelocityIdx( dh, track );
    if ( idah < 0 )
	return false;

    depths.start = track.getPos( dah_[idah-1] ).z;
    times.start = t_[idah-1];
    depths.stop = mUdf(double);
    times.stop = mUdf(float);

    for ( int idx=idah; idx<dtsize; idx++ )
    {
	const double curz = track.getPos( dah_[idx] ).z;
	const float curtwt = t_[idx];
	if ( curz > depths.start && curtwt > times.start )
	{
	    depths.stop = curz;
	    times.stop = curtwt;
	    break;
	}
    }

    if ( mIsUdf(times.stop) ) // found nothing good below, search above
    {
	depths.stop = depths.start;
	times.stop = times.start;
	depths.start = mUdf(double);
	times.start = mUdf(float);
	for ( int idx=idah-2; idx>=0; idx-- )
	{
	    const double curz = track.getPos( dah_[idx] ).z;
	    const float curtwt = t_[idx];
	    if ( curz < depths.stop && curtwt < times.stop )
	    {
		depths.start = curz;
		times.start = curtwt;
		break;
	    }
	}
	if ( mIsUdf(times.start) )
	    return false; // could not get a single good velocity
    }

    return true;
}


bool Well::D2TModel::getOldVelocityBoundsForTwt( float twt, const Track& track,
						 Interval<double>& depths,
						 Interval<float>& times ) const
{
    const int dtsize = size();
    const int idah = getVelocityIdx( twt, track, false );
    if ( idah < 0 )
	return false;

    depths.start = track.getPos( dah_[idah-1] ).z;
    times.start = t_[idah-1];
    depths.stop = mUdf(double);
    times.stop = mUdf(float);

    for ( int idx=idah; idx<dtsize; idx++ )
    {
	const double curz = track.getPos( dah_[idx] ).z;
	const float curtwt = t_[idx];
	if ( curz > depths.start && curtwt > times.start )
	{
	    depths.stop = curz;
	    times.stop = curtwt;
	    break;
	}
    }

    if ( mIsUdf(times.stop) ) // found nothing good below, search above
    {
	depths.stop = depths.start;
	times.stop = times.start;
	depths.start = mUdf(double);
	times.start = mUdf(float);
	for ( int idx=idah-2; idx>=0; idx-- )
	{
	    const double curz = track.getPos( dah_[idx] ).z;
	    const float curtwt = t_[idx];
	    if ( curz < depths.stop && curtwt < times.stop )
	    {
		depths.start = curz;
		times.start = curtwt;
		break;
	    }
	}
	if ( mIsUdf(times.start) )
	    return false; // could not get a single good velocity
    }

    return true;
}


void Well::D2TModel::makeFromTrack( const Track& track, float vel,
				    float replvel )
{
    setEmpty();
    const float srd = mCast( float, SI().seismicReferenceDatum() );
    const float kb  = track.getKbElev();
    const float bulkshift = mIsUdf( replvel ) ? 0.f
			  : ( kb-srd )* ( (2.f / vel) - (2.f / replvel) );

    int idahofminz = 0;
    int idahofmaxz = track.size()-1;
    float tvdmin = 1e10;
    float tvdmax = -1;
    for ( int idx=0; idx<track.size(); idx++ )
    {
	const float zpostrack = (float)track.pos(idx).z;
	if ( zpostrack > tvdmax )
	{
	    tvdmax = zpostrack;
	    idahofmaxz = idx;
	}

	if ( zpostrack < tvdmin )
	{
	    tvdmin = zpostrack;
	    idahofminz = idx;
	}
    }

    if ( tvdmax < -1.f * srd ) // whole track above SRD !
	return;

    float firstdah = track.dah(idahofminz);
    if ( tvdmin < -1.f * srd ) // no write above SRD
    {
	tvdmin = -1.f * srd;
	firstdah = track.getDahForTVD( tvdmin );
    }

    add( firstdah, 2.f*( tvdmin+srd )/vel + bulkshift );
    add( track.dah(idahofmaxz), 2.f*( tvdmax+srd )/vel + bulkshift );
}


