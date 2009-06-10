/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltied2tmodelmanager.cc,v 1.4 2009-06-10 08:07:46 cvsbruno Exp $";

#include "welltied2tmodelmanager.h"

#include "ioman.h"
#include "iostrm.h"
#include "filegen.h"
#include "filepath.h"
#include "strmprov.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welllog.h"
#include "wellman.h"
#include "welllogset.h"
#include "wellwriter.h"

#include "welltiegeocalculator.h"
#include "welltiesetup.h"
#include "welltieunitfactors.h"

WellTieD2TModelMGR::WellTieD2TModelMGR( Well::Data* d, 
					const WellTieParams* params)
	: wd_(d)
	, geocalc_(*new WellTieGeoCalculator(params,d))
	, prvd2t_(0)
	, emptyoninit_(false)
	, wtsetup_(params->getSetup())		     
{
    if ( !wd_ ) return;
    if ( !wd_->d2TModel() || wd_->d2TModel()->size()<=2 )
    {
	emptyoninit_ = true;
	wd_->setD2TModel( new Well::D2TModel );
    }
    orgd2t_ = new Well::D2TModel( *wd_->d2TModel() );

    if ( wd_->checkShotModel() || wd_->d2TModel()->size()<=2 )
	setFromVelLog();
} 


WellTieD2TModelMGR::~WellTieD2TModelMGR()
{
    delete prvd2t_;
    delete orgd2t_;
}


Well::D2TModel& WellTieD2TModelMGR::d2T()
{
    return *wd_->d2TModel();
}


void WellTieD2TModelMGR::setFromVelLog( bool docln )
{setAsCurrent( geocalc_.getModelFromVelLog(docln) );}


void WellTieD2TModelMGR::setFromData( const Array1DImpl<float>& time,
				      const Array1DImpl<float>& dpt )
{setAsCurrent( geocalc_.getModelFromVelLogData( time, dpt) );}


void WellTieD2TModelMGR::shiftModel( float shift)
{
    TypeSet<float> dah, time;

    Well::D2TModel* d2t =  new Well::D2TModel( d2T() );
    //copy old d2t
    for (int idx = 0; idx<d2t->size(); idx++)
    {
	time += d2t->value( idx );
	dah  += d2t->dah( idx );
    }

    //replace by shifted one
    d2t->erase();
    for ( int dahidx=0; dahidx<dah.size(); dahidx++ )
	d2t->add( dah[dahidx], time[dahidx] + shift );

    setAsCurrent( d2t );
}



void WellTieD2TModelMGR::setAsCurrent( Well::D2TModel* d2t )
{
    if ( !d2t || d2t->size() < 1 )
    { pErrMsg("Bad D2TMdl: ignoring"); delete d2t; return; }

    if ( prvd2t_ )
	delete prvd2t_;
    prvd2t_ =  new Well::D2TModel( d2T() );
    wd_->setD2TModel( d2t );
}


bool WellTieD2TModelMGR::undo()
{
    if ( !prvd2t_ ) return false; 
    Well::D2TModel* tmpd2t =  new Well::D2TModel( *prvd2t_ );
    setAsCurrent( tmpd2t );
    return true;
}


bool WellTieD2TModelMGR::cancel()
{
    if ( emptyoninit_ )
    {
	wd_->d2TModel()->erase();	
	wd_->d2tchanged.trigger();
    }
    else
	setAsCurrent( orgd2t_ );
    return true;
}


bool WellTieD2TModelMGR::updateFromWD()
{
    if ( !wd_->d2TModel() || wd_->d2TModel()->size()<1 )
       return false;	
    setAsCurrent( wd_->d2TModel() );
    return true;
}


bool WellTieD2TModelMGR::commitToWD()
{
    mDynamicCastGet(const IOStream*,iostrm,IOM().get(wtsetup_.wellid_))
    if ( !iostrm ) 
	return false;
    StreamProvider sp( iostrm->fileName() );
    sp.addPathIfNecessary( iostrm->dirName() );
    BufferString fname = sp.fileName();

    Well::Writer wtr( fname, *wd_ );
    if ( !wtr.putD2T() ) 
	return false;

    wd_->d2tchanged.trigger();

    return true;
}


bool WellTieD2TModelMGR::save( const char* filename )
{
    StreamData sdo = StreamProvider( filename ).makeOStream();
    if ( !sdo.usable() )
    {
	sdo.close();
	return false;
    }

    const Well::D2TModel& d2t = d2T();
    for ( int idx=0; idx< d2t.size(); idx++ )
    {
	*sdo.ostrm <<  d2t.dah(idx); 
	*sdo.ostrm << '\t';
       	*sdo.ostrm <<  d2t.value(idx);
	*sdo.ostrm << '\n';
    }
    sdo.close();

    return true;
}
