#ifndef uifreqfilterattrib_h
#define uifreqfilterattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id: uifreqfilterattrib.h,v 1.3 2005-08-04 14:37:29 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiImagAttrSel;
class uiGenInput;
class uiLabeledSpinBox;

/*! \brief ** Attribute description editor */

class uiFreqFilterAttrib : public uiAttrDescEd
{
public:

			uiFreqFilterAttrib(uiParent*);

protected:

    uiImagAttrSel*      inpfld;
    uiGenInput*         isfftfld;
    uiGenInput*		typefld;
    uiGenInput*		freqfld;
    uiLabeledSpinBox*	polesfld;
    uiGenInput*         winfld;

    void		finaliseCB(CallBacker*);
    void		typeSel(CallBacker*);
    void		isfftSel(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
};

#endif
