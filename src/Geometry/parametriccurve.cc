/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2004
-*/

static const char* rcsID = "$Id: parametriccurve.cc,v 1.9 2007-01-29 20:36:34 cvskris Exp $";

#include "parametriccurve.h"

#include "extremefinder.h"
#include "mathfunc.h"
#include "sets.h"
#include "trigonometry.h"
#include "undefval.h"

namespace Geometry
{


class CurveSqDistanceFunction : public FloatMathFunction
{
public:
    			CurveSqDistanceFunction( const ParametricCurve& pc,
						 const Coord3& ppos )
			   : curve( pc ) 
			   , pos( ppos )
		       {}

    float		getValue( float p ) const
			{ return curve.computePosition(p).sqDistTo(pos); }

protected:
    const ParametricCurve&	curve;
    Coord3			pos;
};



bool ParametricCurve::findClosestPosition( float& p, const Coord3& pos,
					   float eps ) const
{
//    pErrMsg("This function is not tested, quality not assured (yet)");
    CurveSqDistanceFunction mfunc( *this, pos );
    const StepInterval<int> prange = parameterRange();
    if ( mIsUdf(p) || !prange.includes(p,false) )
    {
	float closestsqdist = mUdf(float);
	for ( int idx=prange.start; idx<=prange.stop; idx+=prange.step )
	{
	    const float sqdist = getPosition(idx).sqDistTo(pos);
	    if ( sqdist<closestsqdist )
	    {
		closestsqdist = sqdist;
		p = idx;
	    }
	}
    }

    const Interval<float> limits( prange.start, prange.stop );
    ExtremeFinder1D finder( mfunc, false, 20, eps,
	    		    Interval<float>(mMAX(p-prange.step,prange.start),
					    mMIN(p+prange.step,prange.stop) ),
			    &limits );

    int res;
    while ( (res=finder.nextStep())==1 ) ;

    if ( !res ) p = finder.extremePos();
    p = finder.extremePos();

    return res!=-1;
}


bool ParametricCurve::findClosestIntersection( float& p, const Plane3& plane,
					       float eps ) const
{
    pErrMsg("This function is not tested, quality not assured (yet)");
    const StepInterval<int> prange = parameterRange();
    if ( mIsUdf(p) || !prange.includes(p,false) )
    {
	float closestdist = mUdf(float);
	for ( int idx=prange.start; idx<=prange.stop; idx+=prange.step )
	{
	    const Coord3 pos = getPosition(idx);
	    const float dist =
		plane.A*pos.x+plane.B*pos.y+plane.C*pos.z+plane.D;
	    if ( fabs(dist)<closestdist )
	    {
		closestdist = dist;
		p = idx;
	    }
	}
    }

    const Interval<float> limits( prange.start, prange.stop );
    for ( int idx=0; idx<20; idx++ )
    {
	const Coord3 pos = computePosition(p);
	float fp = plane.A*pos.x+plane.B*pos.y+plane.C*pos.z+plane.D;

	const Coord3 dir = computeTangent(p);
	float dp = plane.A*dir.x+plane.B*dir.y+plane.C*dir.z+plane.D;

	const float diff = dp/fp;
	p = p - diff;
	if ( fabs(diff)<eps )
	    return true;

	if ( !prange.includes(p) )
	    return false;
    }

    return false;
}


void ParametricCurve::getPosIDs( TypeSet<GeomPosID>& ids, bool remudf ) const
{
    ids.erase();
    const StepInterval<int> range = parameterRange();

    for ( int param=range.start; param<=range.stop; param += range.step )
    {
	if ( remudf && !isDefined(param) ) continue;
	ids += param;
    }
}


}; //Namespace

