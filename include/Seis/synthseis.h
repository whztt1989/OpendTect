#ifndef synthseis_h
#define synthseis_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		24-3-1996
 RCS:		$Id: synthseis.h,v 1.13 2011-04-01 12:59:18 cvsbruno Exp $
________________________________________________________________________

-*/

#include "ailayer.h"
#include "cubesampling.h"
#include "factory.h"
#include "odmemory.h"
#include "reflectivitymodel.h"
#include "samplingdata.h"
#include "task.h"

#include "complex"

class RayTracer1D;
class SeisTrc;
class TimeDepthModel;
class Wavelet;

typedef std::complex<float> float_complex;
namespace Fourier { class CC; };

namespace Seis
{

/* brief generates synthetic traces.The SynthGenerator performs the basic 
   convolution with a reflectivity series and a wavelet. If you have AI layers 
   and want directly some synthetics out of them, then you should use the 
   RayTraceSynthGenerator.

   The different constructors and generate() functions will optimize for
   different situations. For example, if your Reflectivity/AI Model is fixed 
   and you need to generate for multiple wavelets, then you benefit from only 
   one anti-alias being done.
*/


mClass SynthGenerator
{
public:
    				SynthGenerator();
    				~SynthGenerator();

    bool			setModel(const ReflectivityModel&);
    bool			setWavelet(const Wavelet*,OD::PtrPolicy);
    bool			setOutSampling(const StepInterval<float>&);
    void 			setConvolDomain(bool fourier);
    				/*!<Default is fourier-domain */

    bool                        doPrepare();
    bool			doWork();
    const char*			errMsg() const		{ return errmsg_.buf();}
    const SeisTrc&		result() const		{ return outtrc_; }

    void 			getSampledReflectivities(TypeSet<float>&) const;

protected:

    bool 			computeTrace(float* result); 
    bool 			doTimeConvolve(float* result); 
    bool 			doFFTConvolve(float* result);

    const Wavelet*		wavelet_;
    const ReflectivityModel*	refmodel_;
    StepInterval<float>		outputsampling_;

    BufferString		errmsg_;

    Fourier::CC*                fft_;
    int				fftsz_;
    float_complex*		freqwavelet_;
    bool			needprepare_;	
    bool			waveletismine_;
    TypeSet<float_complex>	cresamprefl_;
    SeisTrc&			outtrc_;
};


mClass MultiTraceSynthGenerator : public ParallelTask
{
public:
    				~MultiTraceSynthGenerator();

    void 			setModels(
				    const ObjectSet<const ReflectivityModel>&);
    void			setWavelet(const Wavelet*);
    bool			setOutSampling(const StepInterval<float>&);
    void 			setConvolDomain(bool fourier);
    				/*!<Default is fourier-domain */

    void 			result(ObjectSet<const SeisTrc>&) const;
    const char*			errMsg() const		{ return errmsg_.buf();}

protected:

    od_int64            	nrIterations() const;
    virtual bool        	doWork(od_int64,od_int64,int);

    bool			isfourier_;
    BufferString		errmsg_;
    const Wavelet*		wavelet_;
    StepInterval<float>		outputsampling_;

    ObjectSet<SynthGenerator>	synthgens_;
};


mClass RaySynthGenerator 
{
public:
    mDefineFactoryInClass( RaySynthGenerator, factory );

    virtual bool		addModel(const AIModel&,
	    				 const RayTracer1D::Setup&);
    				/*!<you can have more than one model!*/
    virtual bool		setSampling(const CubeSampling& cs,
					const SamplingData<float>& offsetsd);
    				/*!<crl dir is offset index, inl dir is model!*/

    virtual bool		setWavelet(const Wavelet*,OD::PtrPolicy);
    void 			setConvolDomain(bool fourier);
    				/*!<Default is fourier-domain!*/

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    bool			doWork(TaskRunner& tr);

    const char*			errMsg() const 	{ return errmsg_.buf(); }

    //available after execution
    void			getTrcs(ObjectSet<const SeisTrc>& trcs) const;
    void			getTWTs(ObjectSet<const TimeDepthModel>&) const;
    void			getReflectivities(
				    ObjectSet<const ReflectivityModel>&) const;

protected:
    				RaySynthGenerator();
    virtual 			~RaySynthGenerator();

    bool			doRayTracers(TaskRunner& tr);
    bool			doSynthetics(TaskRunner& tr);

    BufferString		errmsg_;
    bool 			isfourier_;
    const Wavelet*		wavelet_;
    float			wvltsamplingrate_;

    TypeSet<int>		modelssz_;
    TypeSet<float>		offsets_;
    CubeSampling		cs_;

    mStruct RayModel
    {
					RayModel(const RayTracer1D& rt1d,
						 int nroffsets);	
					~RayModel();	

	ObjectSet<const SeisTrc>		outtrcs_; //this is a gather
	ObjectSet<const ReflectivityModel> 	refmodels_;
	ObjectSet<const TimeDepthModel> 	t2dmodels_;
    };
    ObjectSet<RayModel>		raymodels_;
    ObjectSet<RayTracer1D>	raytracers_;
};



mClass ODRaySynthGenerator : public RaySynthGenerator
{
public:

     bool        		setPar(const IOPar&) { return true; }
     void        		fillPar(IOPar&) const {}

protected:

    static void         	initClass() 
				{factory().addCreator(create,"Fast Generator");}
    static RaySynthGenerator* 	create() { return new ODRaySynthGenerator; }
};


} //namespace


#endif

