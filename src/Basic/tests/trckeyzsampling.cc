/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman K Singh
 * DATE     : April 2013
 * FUNCTION : Test various functions of TrcKeySampling and TrcKeyZSampling.
-*/


#include "testprog.h"
#include "trckeyzsampling.h"
#include "survinfo.h"

#define mDeclTrcKeyZSampling( cs, istart, istop, istep, \
			       cstart, cstop, cstep, \
			       zstart, zstop, zstep) \
    TrcKeyZSampling cs( false ); \
    cs.hsamp_.set( StepInterval<int>(istart,istop,istep), \
	        StepInterval<int>(cstart,cstop,cstep) ); \
    cs.zsamp_.set( zstart, zstop, zstep );


#define mRetResult( funcname ) \
    { \
	od_cout() << funcname << " failed" << od_endl; \
	return false; \
    } \
    else if ( !quiet ) \
	od_cout() << funcname << " succeeded" << od_endl; \
    return true;


static bool testEmpty()
{
    TrcKeyZSampling cs0( false );
    cs0.setEmpty();
    if ( cs0.nrInl() || cs0.nrCrl() || !cs0.isEmpty() )
	mRetResult( "testEmpty" );
}


static bool testInclude()
{
    mDeclTrcKeyZSampling( cs1, 2, 50, 6,
			    10, 100, 9,
			    1.0, 3.0, 0.004 );
    mDeclTrcKeyZSampling( cs2, 1, 101, 4,
			    4, 100, 3,
			    -1, 4.0, 0.005 );
    mDeclTrcKeyZSampling( expcs, 1, 101, 1,
			      4, 100, 3,
			      -1, 4, 0.004 );
    cs1.include ( cs2 );
    if ( cs1 != expcs )
	mRetResult( "testInclude" );
}


static bool testIncludes()
{
    mDeclTrcKeyZSampling( cs1, 2, 50, 6,
			    10, 100, 9,
			    1.0, 3.0, 0.004 );
    mDeclTrcKeyZSampling( cs2, 1, 101, 4,
			    4, 100, 3,
			    -1, 4.0, 0.005 );
    mDeclTrcKeyZSampling( cs3, 1, 101, 1,
			      4, 100, 3,
			      -1, 4, 0.004 );
    if ( cs2.includes(cs1) || !cs3.includes(cs1) )
	mRetResult( "testIncludes" );
}


static bool testLimitTo()
{
    mDeclTrcKeyZSampling( cs1, 3, 63, 6,
			    10, 100, 1,
			    1.0, 3.0, 0.004 );
    mDeclTrcKeyZSampling( cs2, 13, 69, 4,
			    4, 100, 1,
			    -1, 2.0, 0.005 );
    mDeclTrcKeyZSampling( csexp, 21, 57, 12,
			    10, 100, 1,
			    1, 2.0, 0.005 );
    mDeclTrcKeyZSampling( cs3, 2, 56, 2,
			    10, 100, 1,
			    1, 2.0, 0.004 );
    cs1.limitTo( cs2 );
    cs2.limitTo( cs3 );
    if ( cs1 != csexp || !cs2.isEmpty() )
	mRetResult( "testLimitTo" );
}


bool testIterator()
{
    TrcKeySampling hrg;
    hrg.survid_ = TrcKey::std3DSurvID();
    hrg.set( StepInterval<int>( 100, 102, 2 ),
	     StepInterval<int>( 300, 306, 3 ) );

    TrcKeySamplingIterator iter( hrg );
    TypeSet<BinID> bids;
    bids += BinID(100,300);
    bids += BinID(100,303);
    bids += BinID(100,306);
    bids += BinID(102,300);
    bids += BinID(102,303);
    bids += BinID(102,306);

    int idx=0;
    do
    {
	const BinID curbid( iter.curBinID() );
	mRunStandardTest( curbid == bids[idx], "do-While loop calls" )
	idx++;
    } while ( iter.next() );

    mRunStandardTest( idx == bids.size(),
		      "All positions processed once using iterator" )

    iter.reset();
    for ( int idy=0; idy<bids.size(); idy++, iter.next() )
    {
	const BinID curbid( iter.curBinID() );
	mRunStandardTest( curbid == bids[idy], "For loop calls" )
    }

    iter.reset();
    BinID curbid = iter.curBinID();
    mRunStandardTest( curbid==bids[0], "Reset");

    iter.setCurrentPos( hrg.globalIdx(bids[4]) );
    curbid = iter.curBinID();
    mRunStandardTest( curbid==bids[4], "setCurrentPos");

    return true;
}


int testMain( int argc, char** argv )
{
    mInitTestProg();

    mDeclTrcKeyZSampling( survcs, 1, 501, 2,
			    10, 100, 2,
			    1.0, 10.0, 0.004 );
    SurveyInfo& si = const_cast<SurveyInfo&>( SI() );
    si.setRange( survcs );
    si.setWorkRange( survcs ); //For the sanity of SI().

    if ( !testInclude()
      || !testIncludes()
      || !testEmpty()
      || !testLimitTo()
      || !testIterator() )
	return 1;

    return 0;
}
