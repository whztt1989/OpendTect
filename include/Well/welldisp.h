#ifndef welldisp_h
#define welldisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Dec 2008
 RCS:		$Id: welldisp.h,v 1.36 2011-06-10 07:34:31 cvsbruno Exp $
________________________________________________________________________

-*/

#include "fontdata.h"
#include "namedobj.h"
#include "color.h"
#include "ranges.h"

#include "bufstringset.h"

class IOPar;
class BufferStringSet;

namespace Well
{

/*!\brief Display properties of a well */

static const char* sKey2DDispProp = "2D Display";
static const char* sKey3DDispProp = "3D Display";

mClass DisplayProperties
{
public:

			DisplayProperties(const char* subj = sKey3DDispProp);
			DisplayProperties(const Well::DisplayProperties& dp)
			    : track_(dp.track_)
			    , markers_(dp.markers_)
			    , selmarkernms_(dp.selmarkernms_)
			    , displaystrat_(dp.displaystrat_)
			    {
				deepCopy( logs_, dp.logs_ );
			    }			    
			~DisplayProperties();

    mStruct BasicProps
    {
			BasicProps( int sz=1 )
			    : size_(sz)			
			    , color_(Color(0,0,255))
			    {}

	Color		color_;
	int		size_;

	void		usePar(const IOPar&);
	void		fillPar(IOPar&) const;
	void		useLeftPar(const IOPar&);
	void		useRightPar(const IOPar&);
	void		fillLeftPar(IOPar&) const;
	void		fillRightPar(IOPar&) const;
	virtual const char* subjectName() const		= 0;

    protected:

	virtual void	doUsePar(const IOPar&)		{}
	virtual void	doFillPar(IOPar&) const		{}
	virtual void	doUseLeftPar(const IOPar&) {}
	virtual void	doFillRightPar(IOPar&) const {}
	virtual void	doUseRightPar(const IOPar&){}
	virtual void	doFillLeftPar(IOPar&) const{}

    };

    mStruct Track : public BasicProps
    {
			Track()
			    : BasicProps(1)
			    , dispabove_(true)
			    , dispbelow_(true)	
			    , font_(10)
		       	    {}

	virtual const char* subjectName() const		{ return "Track"; }

	bool		dispabove_;
	bool		dispbelow_;
	FontData	font_;

    protected:

	virtual void	doUsePar(const IOPar&);
	virtual void	doFillPar(IOPar&) const;
    };

    mStruct Markers : public BasicProps
    {

			Markers()
			    : BasicProps(5)
			    , shapeint_(0)	
			    , cylinderheight_(1)			
			    , issinglecol_(false)
			    , font_(10)
			    , samenmcol_(true)	 
			    {}

	virtual const char* subjectName() const	{ return "Markers"; }
	int		shapeint_;
	int		cylinderheight_;
	bool 		issinglecol_;
	FontData 	font_;
	Color		nmcol_;
	bool		samenmcol_;

    protected:

	virtual void	doUsePar(const IOPar&);
	virtual void	doFillPar(IOPar&) const;
    };

    mStruct Log : public BasicProps
    {
			Log()
			    : cliprate_(0)
			    , fillname_("none")
			    , fillrange_(mUdf(float),mUdf(float))
			    , isleftfill_(false) 
			    , isrightfill_(false)
		            , isdatarange_(true)
			    , islogarithmic_(false) 
			    , islogreverted_(false) 
			    , issinglecol_(false)
			    , iswelllog_(true)	
			    , name_("none")
		            , logwidth_ (40)	
			    , range_(mUdf(float),mUdf(float))
			    , repeat_(5)
			    , repeatovlap_(50)
		            , seiscolor_(Color::White())
			    , seqname_("Rainbow")
			    , iscoltabflipped_(false)			 
			    {}		 

	virtual const char* subjectName() const 	{ return "Log"; }

	BufferString	name_;
	BufferString	fillname_;
	bool		iswelllog_;
	float           cliprate_;      
	Interval<float> range_;        
	Interval<float> fillrange_;       
	bool 		isleftfill_;				 
	bool 		isrightfill_;				 
	bool            islogarithmic_;
	bool 		islogreverted_; 
	bool            issinglecol_;
	bool            isdatarange_;
	bool 		iscoltabflipped_;
	int             repeat_;
	float           repeatovlap_;
	Color           linecolor_;
	Color 		seiscolor_;
	BufferString    seqname_;
	int 		logwidth_;

    protected:

	virtual void	doUseLeftPar(const IOPar&);
	virtual void	doFillRightPar(IOPar&) const;
	virtual void	doUseRightPar(const IOPar&);
	virtual void	doFillLeftPar(IOPar&) const;
    };

    Track		track_;
    Markers		markers_;
    BufferStringSet	selmarkernms_;
    bool		displaystrat_; //2d only

    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    static DisplayProperties&	defaults();
    static void		commitDefaults();

    mStruct LogCouple 	{ Log left_; Log right_; };
    ObjectSet<LogCouple> logs_;

    virtual const char* subjectName() const 	{ return subjectname_.buf(); }
protected:
    BufferString 	subjectname_;
};

} // namespace

#endif
