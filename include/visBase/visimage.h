#ifndef visimage_h
#define visimage_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		Feb 2007
 RCS:		$Id: visimage.h,v 1.2 2007-02-08 22:45:14 cvskris Exp $
________________________________________________________________________


-*/

#include "visdata.h"

template <class T> class Array2D;

class SoTexture2;
class Color;

namespace visBase
{

/*!Displays an image that either is read from disk or in memory. */

class Image : public DataObject
{
public:
    static Image*	create()
			mCreateDataObj( Image );

    bool		replacesMaterial() const;
    void		replaceMaterial(bool yn);

    void		setData(const Array2D<Color>&,bool trans);
    void		setFileName(const char*);
    const char*		getFileName() const;
    SoNode*		getInventorNode();

protected:
    			~Image();
    SoTexture2*		texture_;
};

};

#endif

