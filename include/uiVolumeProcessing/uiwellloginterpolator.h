#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2007
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"

#include "uivolprocstepdlg.h"
#include "bufstringset.h"
#include "wellloginterpolator.h"

class uiGenInput;
class uiInterpolationLayerModel;
class uiMultiWellLogSel;

namespace VolProc
{

mExpClass(uiVolumeProcessing) uiWellLogInterpolator : public uiStepDialog
{ mODTextTranslationClass(uiWellLogInterpolator)
public:
    mDefaultFactoryInstanciationBase(
	WellLogInterpolator::sFactoryKeyword(),
	WellLogInterpolator::sFactoryDisplayName())
	mDefaultFactoryInitClassImpl( uiStepDialog, createInstance )

protected:
				uiWellLogInterpolator(uiParent*,
						WellLogInterpolator&,bool is2d);
				~uiWellLogInterpolator();

    static VolProc::uiStepDialog* createInstance(uiParent*,VolProc::Step*,bool);

    void			finaliseCB(CallBacker*);
    bool			acceptOK();
    void			algoChg(CallBacker*);
    void			initWellLogSel();

    WellLogInterpolator&	hwinterpolator_;

    uiInterpolationLayerModel*	layermodelfld_;
    uiGenInput*			algosel_;
    uiGenInput*			radiusfld_;
    uiGenInput*			extensfld_;
    uiGenInput*			logextenfld_;
    uiMultiWellLogSel*		welllogsel_;
};

} // namespace VolProc
