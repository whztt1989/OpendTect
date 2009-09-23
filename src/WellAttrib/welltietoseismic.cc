/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltietoseismic.cc,v 1.31 2009-09-23 13:27:47 cvsbruno Exp $";

#include "welltietoseismic.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "ioman.h"
#include "linear.h"
#include "mousecursor.h"
#include "posvecdataset.h"
#include "survinfo.h"
#include "task.h"
#include "wavelet.h"

#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "welltrack.h"

#include "welltiedata.h"
#include "welltieextractdata.h"
#include "welltiegeocalculator.h"
#include "welltiesetup.h"
#include "welltieunitfactors.h"


namespace WellTie
{

DataPlayer::DataPlayer( WellTie::DataHolder* dh, 
			const Attrib::DescSet& ads,
			TaskRunner* tr ) 
    	: wtsetup_(dh->setup())
	, dataholder_(dh)  
	, ads_(ads)
	, wd_(*dh->wd()) 
	, params_(*dh->dpms())		 
	, logsset_(*dh->logsset())	   
	, tr_(tr)		  
      	, d2tmgr_(dh->d2TMGR())
	, dps_(new DataPointSet(false, false))	   
	, wvltset_(dataholder_->wvltset())
{
    dps_->dataSet().add( new DataColDef( params_.attrnm_ ) );
    geocalc_ = new WellTie::GeoCalculator(*dh);
} 


DataPlayer::~DataPlayer()
{
    delete geocalc_;
    delete tr_;
    delete dps_;
}


bool DataPlayer::computeAll()
{
    logsset_.resetData( params_ );
  
    if ( !resampleLogs() ) 	   return false;
    if ( !computeReflectivity() )  return false;
    if ( !convolveWavelet() ) 	   return false;

    if ( !extractWellTrack() )     return false;
    if ( !extractSeismics() ) 	   return false;

    if ( !estimateWavelet() )	   return false;
    if ( !computeCrossCorrel() )   return false;
    
    return true;	
}


bool DataPlayer::extractWellTrack()
{
    dps_->bivSet().empty();
    dps_->dataChanged();

    MouseCursorManager::setOverride( MouseCursor::Wait );
    
    WellTie::TrackExtractor wtextr( *dps_, &wd_ );
    wtextr.timeintv_ = params_.getTimeIntv();
    wtextr.timeintv_.step = params_.timeintv_.step*params_.step_;
    if ( !tr_->execute( wtextr ) ) return false;

    MouseCursorManager::restoreOverride();
    dps_->dataChanged();

    return true;
}


bool DataPlayer::resampleLogs()
{
    MouseCursorManager::setOverride( MouseCursor::Wait );

    resLogExecutor( wtsetup_.corrvellognm_ );
    resLogExecutor( wtsetup_.vellognm_ );
    resLogExecutor( wtsetup_.denlognm_ );

    MouseCursorManager::restoreOverride();

    return true;
}


bool DataPlayer::resLogExecutor( const char* logname )
{
    const Well::Log* log =  wd_.logs().getLog( logname );
    if ( !log  ) return false;
    mDynamicCastGet( WellTie::Log*, tlog, logsset_.getLog(logname) );
    if ( !tlog  ) return false;

    WellTie::LogResampler logres( *tlog, *log, &wd_, *dataholder_ );
    logres.timenm_ = params_.timenm_; logres.dptnm_ = params_.dptnm_;
    logres.timeintv_ = params_.getTimeIntv();
    return tr_->execute( logres );
}


#define mSetData(newnm,dahnm,val)\
{\
    logsset_.setDah( newnm, logsset_.getDah(dahnm) );\
    logsset_.setVal( newnm, &val );\
}
bool DataPlayer::computeReflectivity()
{ 
    Array1DImpl<float> tmpai( params_.worksize_ ), tmpref( params_.dispsize_ );

    geocalc_->computeAI( *logsset_.getVal(params_.currvellognm_),
	      		 *logsset_.getVal(params_.denlognm_), tmpai );
    geocalc_->lowPassFilter( tmpai,  1/( 4*SI().zStep() ) );
    geocalc_->computeReflectivity( tmpai, tmpref, params_.step_ );

    mSetData( params_.ainm_, params_.denlognm_, tmpai );

    mDynamicCastGet(WellTie::Log*,velog,logsset_.getLog(params_.currvellognm_));
    mDynamicCastGet( WellTie::Log*, delog, logsset_.getLog(params_.denlognm_) );
    mDynamicCastGet( WellTie::Log*, ailog, logsset_.getLog(params_.ainm_) );
    if ( !velog || !delog || !ailog ) return false;

    velog->resample( params_.step_ );
    delog->resample( params_.step_ );
    ailog->resample( params_.step_ );

    mSetData( params_.refnm_, params_.ainm_, tmpref );

    return true;
}


bool DataPlayer::extractSeismics()
{
    Attrib::EngineMan aem; BufferString errmsg;
    PtrMan<Executor> tabextr = aem.getTableExtractor( *dps_, ads_, errmsg,
						       dps_->nrCols()-1 );
    if ( !tabextr ) return false;
    if (!tr_->execute( *tabextr )) return false;
    dps_->dataChanged();

    Array1DImpl<float> tmpseis ( params_.dispsize_ );
    getDPSZData( *dps_, tmpseis );

    mSetData( params_.attrnm_, params_.synthnm_, tmpseis );

    return true;
}


bool DataPlayer::convolveWavelet()
{
    bool isinitwvltactive = params_.isinitwvltactive_;
    Wavelet* wvlt = isinitwvltactive ? wvltset_[0] : wvltset_[1]; 
    const int wvltsz = wvlt->size();
    if ( !wvlt || wvltsz <= 0 || wvltsz > params_.dispsize_ ) return false;
    Array1DImpl<float> wvltvals( wvlt->size() );
    memcpy( wvltvals.getData(), wvlt->samples(), wvltsz*sizeof(float) );

    int wvltidx = wvlt->centerSample();

    Array1DImpl<float> tmpsynth ( params_.dispsize_ );
    geocalc_->convolveWavelet( wvltvals, *logsset_.getVal(params_.refnm_), 
			       tmpsynth, wvltidx );
    mSetData( params_.synthnm_, params_.refnm_, tmpsynth );

    return true;
}


bool DataPlayer::estimateWavelet()
{
    const int datasz = params_.corrsize_; 

    Wavelet* wvlt = wvltset_[1];
    if ( !wvlt ) return false;

    wvlt->setName( "Estimated Wavelet" );
    const int wvltsz = wvlt->size();
    if ( datasz < wvltsz +1 )
       return false;

    const bool iswvltodd = wvltsz%2;
    if ( iswvltodd ) wvlt->reSize( wvltsz+1 );
   
    Array1DImpl<float> wvltarr( datasz ), wvltvals( wvltsz );
    //performs deconvolution
    geocalc_->deconvolve( *logsset_.getVal(params_.attrnm_), 
	    		  *logsset_.getVal(params_.refnm_), 
			  wvltarr, wvltsz );

    //retrieve wvlt samples from the deconvolved vector
    for ( int idx=0; idx<wvltsz; idx++ )
	wvlt->samples()[idx] = wvltarr.get( datasz/2 + idx - wvltsz/2 + 1 );
    
    memcpy( wvltvals.getData(),wvlt->samples(), wvltsz*sizeof(float) );
    ArrayNDWindow window( Array1DInfoImpl(wvltsz), false, "CosTaper", .05 );
    window.apply( &wvltvals );
    memcpy( wvlt->samples(), wvltvals.getData(), wvltsz*sizeof(float) );
  
    return true;
}


bool DataPlayer::computeCrossCorrel()
{
    Array1DImpl<float> tmpcrosscorr( params_.corrsize_ );

    geocalc_->crosscorr( *logsset_.getVal(params_.synthnm_), 
	    		 *logsset_.getVal(params_.attrnm_), 
	    		 tmpcrosscorr );

    mSetData( params_.crosscorrnm_, params_.synthnm_, tmpcrosscorr );
    //computes cross-correl coeff
    LinStats2D ls2d;
    ls2d.use( logsset_.getLog(params_.attrnm_)->valArr(),
	      logsset_.getLog(params_.synthnm_)->valArr(),
	      params_.corrsize_ );
    dataholder_->corrcoeff() = ls2d.corrcoeff;

    return true;
}


void DataPlayer::getDPSZData( const DataPointSet& dps, Array1DImpl<float>& vals)
{
    TypeSet<float> zvals, tmpvals;
    for ( int idx=0; idx<dps.size(); idx++ )
	zvals += dps.z(idx);

    const int sz = zvals.size();
    mAllocVarLenArr( int, zidxs, sz );
    for ( int idx=0; idx<sz; idx++ )
	zidxs[idx] = idx;

    sort_coupled( zvals.arr(), mVarLenArr(zidxs), sz );

    for ( int colidx=0; colidx<dps.nrCols(); colidx++ )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    float val = dps.getValues(zidxs[idx])[colidx];
	    tmpvals += mIsUdf(val) ? 0 : val;
	}
    }

    memcpy(vals.getData(), tmpvals.arr(), vals.info().getSize(0)*sizeof(float));
}    


}; //namespace WellTie
