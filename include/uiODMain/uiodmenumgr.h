#ifndef uiodmenumgr_h
#define uiodmenumgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodmenumgr.h,v 1.46 2009-03-24 04:41:08 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uiodapplmgr.h"

class uiMenuItem;
class uiODHelpMenuMgr;
class uiPopupMenu;
class uiToolBar;
class DirList;
class uiPopupMenu;


/*!\brief The OpendTect menu manager

  The uiODMenuMgr instance can be accessed like:
  ODMainWin()->menuMgr()

  All standard menus should be reachable directly without searching for
  the text. It is easy to add your own menu items. And tool buttons, for that
  matter.

*/

mClass uiODMenuMgr : public CallBacker
{

    friend class	uiODMain;
    friend class	uiODHelpMenuMgr;

public:

    // TODO: winMnu() only here for backward compatibility
    // Remove in version 3.4
    uiPopupMenu*	fileMnu()		{ return surveymnu_; }
    uiPopupMenu*	surveyMnu()		{ return surveymnu_; }
    uiPopupMenu*	analMnu()		{ return analmnu_; }
    uiPopupMenu*	procMnu()		{ return procmnu_; }
    uiPopupMenu*	winMnu()		{ return scenemnu_; }
    uiPopupMenu*	sceneMnu()		{ return scenemnu_; }
    uiPopupMenu*	viewMnu()		{ return viewmnu_; }
    uiPopupMenu*	utilMnu()		{ return utilmnu_; }
    uiPopupMenu*	helpMnu()		{ return helpmnu_; }
    uiPopupMenu*	settMnu()		{ return settmnu_; }
    uiPopupMenu*	toolsMnu()		{ return toolsmnu_; }

    uiPopupMenu*	getBaseMnu(uiODApplMgr::ActType);
    			//! < Within Survey menu
    uiPopupMenu*	getMnu(bool imp,uiODApplMgr::ObjType);
    			//! < Within Survey - Import or Export

    uiToolBar*		dtectTB()		{ return dtecttb_; }
    uiToolBar*		coinTB()		{ return cointb_; }
    uiToolBar*		manTB()			{ return mantb_; }


    			// Probably not needed by plugins
    void		updateStereoMenu();
    void		updateViewMode(bool);
    void		updateAxisMode(bool);
    bool		isSoloModeOn() const;
    void		enableMenuBar(bool);
    void		enableActButton(bool);
    void		setCameraPixmap(bool isperspective);
    void		updateSceneMenu();
//    void		updateWindowsMenu() { updateSceneMenu(); }
    			// Backward compatibility, remove in od3.4

    Notifier<uiODMenuMgr> dTectTBChanged;
    Notifier<uiODMenuMgr> dTectMnuChanged;

protected:

			uiODMenuMgr(uiODMain*);
			~uiODMenuMgr();
    void		initSceneMgrDepObjs(uiODApplMgr*,uiODSceneMgr*);

    uiODMain&		appl_;
    uiODHelpMenuMgr*	helpmgr_;

    uiPopupMenu*	surveymnu_;
    uiPopupMenu*	analmnu_;
    uiPopupMenu*	procmnu_;
    uiPopupMenu*	scenemnu_;
    uiPopupMenu*	viewmnu_;
    uiPopupMenu*	utilmnu_;
    uiPopupMenu*	impmnu_;
    uiPopupMenu*	expmnu_;
    uiPopupMenu*	manmnu_;
    uiPopupMenu*	preloadmnu_;
    uiPopupMenu*	helpmnu_;
    uiPopupMenu*	settmnu_;
    uiPopupMenu*	toolsmnu_;
    ObjectSet<uiPopupMenu> impmnus_;
    ObjectSet<uiPopupMenu> expmnus_;

    uiToolBar*		dtecttb_;
    uiToolBar*		cointb_;
    uiToolBar*		mantb_;

    void		fillSurveyMenu();
    void		fillImportMenu();
    void		fillExportMenu();
    void		fillManMenu();
    void		fillAnalMenu();
    void		fillProcMenu();
    void		fillSceneMenu();
    void		fillViewMenu();
    void		fillUtilMenu();
    void		fillDtectTB(uiODApplMgr*);
    void		fillCoinTB(uiODSceneMgr*);
    void		fillManTB();

    void		selectionMode(CallBacker*);
    void		handleToolClick(CallBacker*);
    void		handleViewClick(CallBacker*);
    void		handleClick(CallBacker*);
    void		dispColorBar(CallBacker*);
    void		manSeis(CallBacker*);
    void		manHor(CallBacker*);
    void		manFlt(CallBacker*);
    void		manWll(CallBacker*);
    void		manPick(CallBacker*);
    void		manWvlt(CallBacker*);
    void		manStrat(CallBacker*);
    void		updateDTectToolBar(CallBacker*);
    void		updateDTectMnus(CallBacker*);
    void		toggViewMode(CallBacker*);
    void		create2D3DMnu(uiPopupMenu*,const char*,int,int);

    uiMenuItem*		stereooffitm_;
    uiMenuItem*		stereoredcyanitm_;
    uiMenuItem*		stereoquadbufitm_;
    uiMenuItem*		stereooffsetitm_;
    uiMenuItem*		addtimedepthsceneitm_;
    int			axisid_, actviewid_, cameraid_, soloid_;
    int			coltabid_, polyselectid_,viewselectid_,curviewmode_ ;
    bool		inviewmode_;

    inline uiODApplMgr&	applMgr()	{ return appl_.applMgr(); }
    inline uiODSceneMgr& sceneMgr()	{ return appl_.sceneMgr(); }

    void		showLogFile();
    void		mkViewIconsMnu();
    void		addIconMnuItems(const DirList&,uiPopupMenu*,int&);
};


#endif
