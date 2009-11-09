#ifndef welltieextractdata_h
#define welltieextractdata_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltieextractdata.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "executor.h"
#include "position.h"
#include "arrayndimpl.h"
#include "geometry.h"

class CubeSampling;
class SeisTrcReader;
class SeisTrc;
class MultiID;
class IOObj;
namespace Well 
{
    class Data;
    class Log;
    class Track;
    class D2TModel;
};

namespace WellTie
{

class DataHolder;

mClass TrackExtractor : public Executor
{
public:
			TrackExtractor(const Well::Data*);

    StepInterval<float> timeintv_;

    int                 nextStep();
    od_int64            totalNr() const		{ return timeintv_.nrSteps(); }
    od_int64            nrDone() const          { return nrdone_; }
    const char*         message() const         { return "Computing..."; }
    const char*         nrDoneText() const      { return "Points done"; }
    const BinID*	getBIDs() const		{ return bidset_.arr(); }

protected:

    BinID		prevbid_;
    TypeSet<BinID>	bidset_;
    const Well::Data& 	wd_;	 
    const Well::Track& track_;
    const Well::D2TModel& d2t_;
    int                 nrdone_;
};



mClass SeismicExtractor : public Executor
{
public:
			SeismicExtractor(const IOObj&);
			~SeismicExtractor();

    StepInterval<float> timeintv_;

    int                 nextStep();
    od_int64            totalNr() const		{ return timeintv_.nrSteps(); }
    od_int64            nrDone() const          { return nrdone_; }
    const char*         message() const         { return "Computing..."; }
    const char*         nrDoneText() const      { return "Points done"; }
    void		setBIDValues(const TypeSet<BinID>&); 
    void		setTimeIntv(const StepInterval<float>&);
    
    Array1DImpl<float>* vals_;
    Array1DImpl<float>* dahs_;


protected:

    CubeSampling* 	cs_;
    TypeSet<BinID>	bidset_;
    ObjectSet<SeisTrc>	trcset_;
    SeisTrcReader* 	rdr_;
    int                 nrdone_;
    int			radius_;
  
    void		collectTracesAroundPath();
};



mClass LogResampler : public Executor
{
public:
			LogResampler(Well::Log*, const Well::Log&,
				const Well::Data*, WellTie::DataHolder* d=0);
			~LogResampler();



    int                 nextStep();
    int           	colnr_;
    od_int64            totalNr() const		{ return timeintv_.nrSteps(); }
    od_int64            nrDone() const          { return nrdone_; }
    const char*         message() const         { return "Computing..."; }
    const char*         nrDoneText() const      { return "Points done"; }

    bool 		isavg_;
    Array1DImpl<float>* vals_;
    Array1DImpl<float>* dahs_;

    void		setTimeIntv(const StepInterval<float>&);

protected:

    Well::Log*   	newlog_;		
    const Well::Log& 	orglog_;
    const Well::Data& 	wd_;


    TypeSet<float> 	val_;
    TypeSet<float> 	dah_;

    int                 nrdone_;
    int                 curidx_;

    void        	fillProcLog(const Well::Log&);
    StepInterval<float> timeintv_;
};

};//namespace WellTie

#endif
