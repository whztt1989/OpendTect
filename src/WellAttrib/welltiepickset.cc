/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiepickset.cc,v 1.8 2009-06-22 10:19:11 cvsbruno Exp $";

#include "welltiepickset.h"

#include "arrayndimpl.h"
#include "sorting.h"
#include "welldata.h"
#include "welld2tmodel.h"

WellTiePickSetMGR::WellTiePickSetMGR( const Well::Data* wd )
	: CallBacker(CallBacker::CallBacker())
	, mousemoving(this)
    	, wd_(wd)
{
}

WellTiePickSetMGR::~WellTiePickSetMGR()
{}


void WellTiePickSetMGR::addPick( float vwrszstart, float vwrszstop, 
				 float xpos, float zpos )
{
    const int seissz = seispickset_.getSize();
    const int synthsz = synthpickset_.getSize();
    if ( abs(seissz-synthsz)<2 )
    {
	if ( xpos<(vwrszstop-vwrszstart)/2 )
	{
	    if ( abs(synthsz+1-seissz) < 2 )
		synthpickset_.add( 0, xpos, wd_->d2TModel()->getDepth( zpos ) );
	}
	else
	{
	    if ( abs(seissz+1-synthsz) < 2 )
	    seispickset_.add( 0, xpos, wd_->d2TModel()->getDepth( zpos ) );
	}
    }
}


void WellTiePickSetMGR::updateShift( int vwridx, float curpos )
{
    logpickset_.setMousePos( curpos );
    mousemoving.trigger();
}


void WellTiePickSetMGR::clearAllPicks()
{
    logpickset_.clearAll();
    seispickset_.clearAll();
    synthpickset_.clearAll();
}


void WellTiePickSetMGR::clearLastPicks()
{
    seispickset_.clear( seispickset_.getSize()-1 );
    synthpickset_.clear( synthpickset_.getSize()-1 );
}


bool WellTiePickSetMGR::checkIfPick()
{
    if ( seispickset_.getSize() || synthpickset_.getSize())
	return true;
    else
	return false;
}


void WellTiePickSetMGR::sortByDah( WellTiePickSet& pickset )
{
    const int sz = pickset.getSize();
    TypeSet<float> zvals;
    for ( int idx=0; idx<sz; idx++ )
	zvals += pickset.getDah(idx);

    mAllocVarLenArr( int, zidxs, sz );
    for ( int idx=0; idx<sz; idx++ )
	zidxs[idx] = idx;

    sort_coupled( zvals.arr(), mVarLenArr(zidxs), sz );

    for ( int idx=0; idx<sz; idx++ )
	pickset.setDah( idx, zvals[idx]  );
}




WellTiePickSet::WellTiePickSet()
	: mousepos_(0)
	, nrpickstotal_(0)
   	, pickadded(this)
{
}


WellTiePickSet::~WellTiePickSet()
{
    for ( int idx=pickset_.size()-1; idx>=0; idx-- )
	delete ( pickset_.remove(idx) );
}


void WellTiePickSet::add( int vwridx, float xpos, float zpos )
{
    UserPick* pick = new UserPick();
    pick->vidx_ = vwridx; pick->color_ = Color::DgbColor();
    pick->xpos_ = xpos;   pick->dah_ = zpos;
    pickset_ += pick;
    nrpickstotal_++;
    pickadded.trigger();
}


void WellTiePickSet::clear( int idx )
{
    if ( pickset_.size() )
	delete ( pickset_.remove(idx) );
}


void WellTiePickSet::clearAll()
{
    for ( int idx=pickset_.size()-1; idx>=0; idx-- )
	delete ( pickset_.remove(idx) );
}


void WellTiePickSet::updateSupPickedPos( float& lastpos, float curpos,
       						int vwridx )
{
    for ( int idx=0; idx<pickset_.size(); idx++ )
    { 
	float pos = getDah( idx );
	if ( pos > curpos ) 
	{	
	    if ( (pos - curpos) < ( lastpos - curpos ) )
		lastpos = pos;
	}
    }
}


void WellTiePickSet::updateInfPickedPos( float& firstpos, float curpos,
						int vwridx )
{
    for ( int idx=0; idx<pickset_.size(); idx++ )
    { 
	float pos = getDah( idx );
	if ( pos < curpos )
	{
	    if ( (pos - curpos) > ( firstpos - curpos) )
		firstpos = pos;
	}
    }
}

