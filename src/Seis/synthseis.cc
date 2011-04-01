/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : Wavelet
-*/

static const char* rcsID = "$Id: synthseis.cc,v 1.14 2011-04-01 12:59:18 cvsbruno Exp $";

#include "arrayndimpl.h"
#include "fourier.h"
#include "genericnumer.h"
#include "raytrace1d.h"
#include "reflectivitymodel.h"
#include "reflectivitysampler.h"
#include "seistrc.h"
#include "synthseis.h"
#include "survinfo.h"
#include "velocitycalc.h"
#include "wavelet.h"

namespace Seis
{

#define mErrRet(msg) { errmsg_ = msg; return false; }
SynthGenerator::SynthGenerator()
    : wavelet_(0)
    , fft_(new Fourier::CC)
    , fftsz_(0)			   
    , freqwavelet_(0)
    , outtrc_(*new SeisTrc)
    , refmodel_(0)			   
    , needprepare_(true)
{}


SynthGenerator::~SynthGenerator()
{
    if ( waveletismine_ )
	delete wavelet_;

    delete fft_;
    delete [] freqwavelet_;
    delete &outtrc_;
}


bool SynthGenerator::setModel( const ReflectivityModel& refmodel )
{
    refmodel_ = &refmodel;
    return true;
}


bool SynthGenerator::setWavelet( const Wavelet* wvlt, OD::PtrPolicy pol )
{
    if ( waveletismine_ ) 
	{ delete wavelet_; wavelet_ = 0; }
    if ( !wvlt )
	mErrRet( "No valid wavelet given" );	
    if ( pol == OD::CopyPtr )
    {
	mDeclareAndTryAlloc( float*, wavelet_, float[wvlt->size()] );
	if ( !wavelet_ )
	    mErrRet( "Not enough memory" );	
	MemCopier<float> copier( wavelet_, wvlt->samples(), wvlt->size() );
	copier.execute();
    }
    else 
    {
	wavelet_ = wvlt;
    }
    waveletismine_ = pol != OD::UsePtr;
    needprepare_ = true;

    return true;
}


void SynthGenerator::setConvolDomain( bool fourier )
{
    if ( fourier == ((bool) fft_ ) )
	return;

    if ( fft_ )
    {
	delete fft_; fft_ = 0;
    }
    else
    {
	fft_ = new Fourier::CC;
    }
}


bool SynthGenerator::setOutSampling( const StepInterval<float>& si )
{
    outputsampling_ = si;
    outtrc_.reSize( si.nrSteps(), false );
    outtrc_.info().sampling = si;
    fftsz_ = fft_->getFastSize(  si.nrSteps()+1 );
    needprepare_ = true;
    return true;
}


#define mDoFFT( arr, dir )\
    fft_->setInputInfo( Array1DInfoImpl(fftsz_) );\
    fft_->setDir( dir );\
    fft_->setNormalization( !dir );\
    fft_->setInput( arr );\
    fft_->setOutput( arr );\
    fft_->run( true );

bool SynthGenerator::doPrepare()
{
    if ( !needprepare_ )
	return true;

    if ( freqwavelet_ )
	delete [] freqwavelet_;

    freqwavelet_ = new float_complex[ fftsz_];

    for ( int idx=0; idx<fftsz_; idx++ )
	freqwavelet_[idx] = idx<wavelet_->size() ? wavelet_->samples()[idx] : 0;

    mDoFFT( freqwavelet_, true )

    needprepare_ = false;
    return true;
}


bool SynthGenerator::doWork()
{
    if ( needprepare_ )
	doPrepare();

    if ( !refmodel_ || refmodel_->isEmpty() ) 
	mErrRet( "No reflectivity model found" );	

    if ( !wavelet_ ) 
	mErrRet( "No wavelet found" );	

    if ( outputsampling_.nrSteps() < 2 )
	mErrRet( "Output sampling is too small" );	

    const int wvltsz = wavelet_->size(); 
    if ( wvltsz < 2 )
	mErrRet( "Wavelet is too short" );	

    if ( wvltsz > outtrc_.size() )
	mErrRet( "Wavelet is longer than the output trace" );	

    return computeTrace( (float*)outtrc_.data().getComponent(0)->data() );
}


bool SynthGenerator::computeTrace( float* result ) 
{
    outtrc_.zero();
    cresamprefl_.erase();
    ReflectivitySampler sampler( *refmodel_, outputsampling_, cresamprefl_ );
    sampler.setTargetDomain( (bool)fft_ );
    sampler.execute();

    return fft_ ? doFFTConvolve( result) : doTimeConvolve( result );
}


bool SynthGenerator::doFFTConvolve( float* result )
{
    if ( !fft_ ) return false; 

    const int wvltsz = wavelet_->size();
    const int nrsamp = outtrc_.size();
    float_complex* cres = new float_complex[ fftsz_ ];

    for ( int idx=0; idx<cresamprefl_.size(); idx++ )
	cres[idx] = cresamprefl_[idx] * freqwavelet_[idx]; 

    mDoFFT( cres, false )

    for ( int idx=0; idx<wvltsz/2; idx++ )
	result[nrsamp-idx-1] = cres[idx].real();
    for ( int idx=0; idx<fftsz_-wvltsz/2; idx++ )
    {
	if ( idx >= nrsamp ) break;
	result[idx] = cres[idx+wvltsz/2].real();
    }

    delete [] cres;
    
    return true;
}


bool SynthGenerator::doTimeConvolve( float* result )
{
    const int ns = outtrc_.size();
    TypeSet<float> refs; getSampledReflectivities( refs );
    const int wvltcs = wavelet_->centerSample();
    const int wvltsz = wavelet_->size();
    GenericConvolve( wvltsz, -wvltcs-1, wavelet_->samples(),
		     refs.size(), 0, refs.arr(),
		     ns, 0, result );
    return true;
}


void SynthGenerator::getSampledReflectivities( TypeSet<float>& refs ) const
{
    for ( int idx=0; idx<cresamprefl_.size(); idx++ )
	refs += cresamprefl_[idx].real();
}



MultiTraceSynthGenerator::~MultiTraceSynthGenerator()
{
    deepErase( synthgens_ );
}


bool MultiTraceSynthGenerator::setOutSampling( const StepInterval<float>& si )
{
    outputsampling_ = si;
    return true;
}


void MultiTraceSynthGenerator::setModels( 
			const ObjectSet<const ReflectivityModel>& refmodels ) 
{
    for ( int idx=0; idx<refmodels.size(); idx++ )
    {
	SynthGenerator* synthgen = new SynthGenerator();
	synthgen->setModel( *refmodels[idx] );
	synthgens_ += synthgen;
    }
}


od_int64 MultiTraceSynthGenerator::nrIterations() const
{ return synthgens_.size(); }


bool MultiTraceSynthGenerator::doWork(od_int64 start, od_int64 stop, int thread)
{
    for ( int idx=start; idx<=stop; idx++ )
    {
	SynthGenerator& synthgen = *synthgens_[idx];
	if ( !synthgen.doPrepare() )
	    mErrRet( synthgen.errMsg() );	

	synthgen.setWavelet( wavelet_, OD::UsePtr );
	synthgen.setConvolDomain( isfourier_ );
	synthgen.setOutSampling( outputsampling_ );
	if ( !synthgen.doWork() )
	    mErrRet( synthgen.errMsg() );	
    }
    return true;
}


void MultiTraceSynthGenerator::result( ObjectSet<const SeisTrc>& trcs ) const
{
    for ( int idx=0; idx<synthgens_.size(); idx++ )
	trcs += new SeisTrc( synthgens_[idx]->result() );
}


void MultiTraceSynthGenerator::setWavelet( const Wavelet* wvlt )
{ wavelet_ = wvlt; }


void MultiTraceSynthGenerator::setConvolDomain( bool fourier )
{ isfourier_ = fourier; }



mImplFactory( RaySynthGenerator, RaySynthGenerator::factory );

RaySynthGenerator::RaySynthGenerator()
    : isfourier_(true)
    , wavelet_(0)
{
}


RaySynthGenerator::~RaySynthGenerator()
{
    deepErase( raymodels_ );
}


void RaySynthGenerator::setConvolDomain( bool fourier )
{
    isfourier_ = fourier;
}


bool RaySynthGenerator::setWavelet( const Wavelet* wvlt, OD::PtrPolicy ptr )
{
    wvltsamplingrate_ = wvlt ? wvlt->sampleRate() : 0;
    wavelet_ = wvlt;
    return true;
}


bool RaySynthGenerator::addModel( const AIModel& aim, 
				  const RayTracer1D::Setup& rs )
{
    RayTracer1D* rt1d = new RayTracer1D( rs );
    rt1d->setModel( true, aim );
    raytracers_ += rt1d;

    modelssz_ += aim.size()-2;

    return true;
}


bool RaySynthGenerator::setSampling( const CubeSampling& cs, 
				    const SamplingData<float>& offsetsd	)
{
    cs_ = cs;
    offsets_.erase();
    for ( int idx=0; idx<cs.nrCrl(); idx++ )
	offsets_ += offsetsd.atIndex( cs.hrg.crlRange().atIndex( idx ) );

    return true;
}


bool RaySynthGenerator::doWork( TaskRunner& tr )
{
    return doRayTracers( tr ) && doSynthetics( tr );
}


bool RaySynthGenerator::doRayTracers( TaskRunner& tr )
{
    if ( raytracers_.isEmpty() ) 
	mErrRet( "No AIModel set" );

    for ( int idx=0; idx<raytracers_.size(); idx++ )
    {
	raytracers_[idx]->setOffsets( offsets_ );
	if ( !raytracers_[idx]->execute() )
	    mErrRet( raytracers_[idx]->errMsg() )
    }

    for ( int idx=0; idx<raytracers_.size(); idx++ )
    {
	RayModel* rm = new RayModel( *raytracers_[idx], offsets_.size() );
	for ( int idoff=0; idoff<offsets_.size(); idoff++ )
	    cs_.zrg.include( rm->t2dmodels_[idoff]->getTime( modelssz_[idx] ) );
	raymodels_ += rm;
    }
    deepErase( raytracers_ );
    return true;
}


bool RaySynthGenerator::doSynthetics( TaskRunner& tr )
{
    cs_.zrg.step = wvltsamplingrate_;
    if ( cs_.zrg.nrSteps() < 1 )
	mErrRet( "no valid times generated" )

    ObjectSet<MultiTraceSynthGenerator> mtsgs;
    for ( int idx=0; idx<raymodels_.size(); idx++ )
    {
	RayModel& rm = *raymodels_[idx];
	MultiTraceSynthGenerator* mtsg = new MultiTraceSynthGenerator();
	mtsg->setModels( rm.refmodels_ );
	mtsg->setOutSampling( cs_.zrg );
	mtsg->setWavelet( wavelet_ );
	mtsg->setConvolDomain( isfourier_ );
	mtsgs += mtsg;
	if ( !mtsg->execute() )
	    { errmsg_ = mtsg->errMsg(); }
    }
    for ( int idx=0; idx<raymodels_.size(); idx++ )
    {
	mtsgs[idx]->result( raymodels_[idx]->outtrcs_ );
    }
    return true;
}


void RaySynthGenerator::fillPar( IOPar& par ) const
{}


bool RaySynthGenerator::usePar( const IOPar& par ) 
{
    return true;
}


void RaySynthGenerator::getTrcs( ObjectSet<const SeisTrc>& trcs ) const
{
    for ( int idtrc=0; idtrc<cs_.nrInl(); idtrc++ )
    {
	if ( !raymodels_.validIdx(idtrc) ) continue;
	const RayModel& rm = *raymodels_[idtrc];
	for ( int idoff=0; idoff<cs_.nrCrl(); idoff++ )
	{
	    if ( rm.outtrcs_.validIdx(idoff) )
		trcs += rm.outtrcs_[idoff];
	}
    }
}


void RaySynthGenerator::getTWTs( ObjectSet<const TimeDepthModel>& d2ts ) const
{
    for ( int idtrc=0; idtrc<cs_.nrInl(); idtrc++ )
    {
	const RayModel& rm = *raymodels_[idtrc];
	if ( !raymodels_.validIdx(idtrc) ) 
	for ( int idoff=0; idoff<cs_.nrCrl(); idoff++ )
	{
	    if ( rm.t2dmodels_.validIdx(idoff) )
		d2ts += rm.t2dmodels_[idoff];
	}
    }
}


void RaySynthGenerator::getReflectivities( 
				ObjectSet<const ReflectivityModel>& rfs ) const
{
    for ( int idtrc=0; idtrc<cs_.nrInl(); idtrc++ )
    {
	if ( !raymodels_.validIdx(idtrc) ) continue;
	const RayModel& rm = *raymodels_[idtrc];
	for ( int idoff=0; idoff<cs_.nrCrl(); idoff++ )
	{
		if ( rm.refmodels_.validIdx(idoff) )
		    rfs += rm.refmodels_[idoff];
	}
    }
}



RaySynthGenerator::RayModel::RayModel( const RayTracer1D& rt1d, int nroffsets )
{
    for ( int idx=0; idx<nroffsets; idx++ )
    {
	ReflectivityModel* refmodel = new ReflectivityModel(); 
	refmodels_ += refmodel;
	rt1d.getReflectivity( idx, *refmodel );
	TimeDepthModel* t2dm = new TimeDepthModel();
	rt1d.getTWT( idx, *t2dm );
	t2dmodels_ += t2dm;
    }
}


RaySynthGenerator::RayModel::~RayModel()
{
    deepErase( refmodels_ );
    deepErase( t2dmodels_ );
    deepErase( outtrcs_ );
}

}// namespace

