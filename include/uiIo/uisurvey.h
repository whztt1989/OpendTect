#ifndef uisurvey_h
#define uisurvey_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvey.h,v 1.3 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class DirList;
class SurveyInfo;
class uiCanvas;
class uiListBox;
class uiPushButton;
class uiSurveyMap;
class uiTextEdit;
class uiLabel;

class uiSurvey : public uiDialog
{

public:
			uiSurvey(uiParent*);
			~uiSurvey();
    void		mkInfo();
    void 		drawMap();

protected:

    SurveyInfo*		survinfo;
    DirList*		dirlist;
    uiListBox*		listbox;
    uiCanvas*		mapcanvas;
    uiSurveyMap*	survmap;
    uiPushButton*	newbut;
    uiPushButton*	editbut;
    uiPushButton*	rmbut;
    uiPushButton*	convbut;
    uiLabel*		irange2;
    uiLabel*		xrange2; 
    uiLabel*		zrange2;
    uiLabel*		binsize2;
    uiTextEdit*		notes;

    bool		acceptOK(CallBacker*);  
    void		newButPushed(CallBacker*);
    void		editButPushed(CallBacker*);
    void		rmButPushed(CallBacker*);
    void		convButPushed(CallBacker*);
    void 		getSurvInfo();
    bool		survInfoDialog();
    void		update();
    bool		updateSvyFile();
    bool		writeSurveyName(const char*);
    void		selChange();
    void		doCanvas(CallBacker*);

};

#endif
