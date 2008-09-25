#ifndef uigmtmainwin_h
#define uigmtmainwin_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: uigmtmainwin.h,v 1.3 2008-09-25 12:01:13 cvsraman Exp $
________________________________________________________________________

-*/

#include "gmtpar.h"
#include "uibatchlaunch.h"

class CtxtIOObj;
class Timer;
class uiGMTBaseMapGrp;
class uiGMTOverlayGrp;
class uiFileInput;
class uiListBox;
class uiPushButton;
class uiToolButton;
class uiTabStack;

class uiGMTMainWin : public uiFullBatchDialog
{
public:
    			uiGMTMainWin(uiParent*);
			~uiGMTMainWin();

protected:

    CtxtIOObj&		ctio_;
    uiGMTBaseMapGrp*	basemapgrp_;
    uiGroup*		flowgrp_;
    uiListBox*		flowfld_;
    uiToolButton*	upbut_;
    uiToolButton*	downbut_;
    uiToolButton*	rmbut_;

    uiFileInput*	filefld_;
    uiPushButton*	createbut_;
    uiPushButton*	viewbut_;

    uiPushButton*	addbut_;
    uiPushButton*	editbut_;
    uiPushButton*	resetbut_;

    uiTabStack*		tabstack_;
    ObjectSet<uiGMTOverlayGrp> overlaygrps_;

    ObjectSet<GMTPar>	pars_;
    Timer*		tim_;
    bool		needsave_;

    void		createPush(CallBacker*);
    void		viewPush(CallBacker*);
    void		butPush(CallBacker*);
    void		setButStates(CallBacker*);
    void		selChg(CallBacker*);
    void		tabSel(CallBacker*);
    void		addCB(CallBacker*);
    void		editCB(CallBacker*);
    void		resetCB(CallBacker*);
    void		checkFileCB(CallBacker*);
    void		newFlow(CallBacker*);
    void		openFlow(CallBacker*);
    void		saveFlow(CallBacker*);

    bool		prepareProcessing()		{ return true; }
    bool		fillPar(IOPar&);
    bool		usePar( const IOPar&);
    void		makeLegendPar(IOPar&) const;
};

#endif
