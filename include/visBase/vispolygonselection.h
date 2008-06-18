#ifndef vispolygonselection_h
#define vispolygonselection_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		June 2008
 RCS:		$Id: vispolygonselection.h,v 1.1 2008-06-18 21:53:08 cvskris Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "draw.h"
#include "thread.h"

class SoPolygonSelect;
class SoSeparator;
template <class T> class ODPolygon;

namespace visBase
{
class Material;
class DrawStyle;

/*!
Paints a polygon or a rectangle just in front of near-clipping plane driven
by mouse- movement. Once drawn, queries can be made whether points are
inside or outside the polygon.
*/

class PolygonSelection : public VisualObjectImpl
{
public:
    static PolygonSelection*	create()
				mCreateDataObj(PolygonSelection);

    enum			SelectionType { Off, Rectangle, Polygon };
    void			setSelectionType(SelectionType);
    SelectionType		getSelectionType() const;

    void			setLineStyle(const LineStyle&);
    const LineStyle&		getLineStyle() const;

    bool			isInside(const Coord3&,
	    				 bool displayspace=false) const;

    void			setDisplayTransformation( Transformation* );
    Transformation*		getDisplayTransformation();

protected:

    static void				polygonChangeCB(void*, SoPolygonSelect*);
					~PolygonSelection();
    Transformation*			transformation_;

    DrawStyle*				drawstyle_;
    mutable ODPolygon<double>*		polygon_;
    mutable Threads::ReadWriteLock	polygonlock_;

    SoPolygonSelect*			selector_;
};

};


#endif
