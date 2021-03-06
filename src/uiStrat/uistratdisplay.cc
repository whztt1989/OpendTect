/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/

#include "uistratdisplay.h"

#include "uitoolbutton.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uimenu.h"
#include "uifont.h"
#include "uispinbox.h"
#include "uistratutildlgs.h"
#include "uistratreftree.h"
#include "uitoolbar.h"
#include "uimenuhandler.h"

#include "scaler.h"
#include "survinfo.h"

uiStratDisplay::uiStratDisplay( uiParent* p, uiStratRefTree& uitree )
    : uiGraphicsView(p,"Stratigraphy viewer")
    , drawer_(uiStratDrawer(scene(),data_))
    , uidatawriter_(uiStratDispToTree(uitree ))
    , uidatagather_(0)
    , uicontrol_(0)
    , islocked_(false)
    , maxrg_(Interval<float>(0,2e3))
{
    uidatagather_ = new uiStratTreeToDisp( data_ );
    uidatagather_->newtreeRead.notify( mCB(this,uiStratDisplay,reDraw) );

    getMouseEventHandler().buttonReleased.notify(
					mCB(this,uiStratDisplay,usrClickCB) );
    reSize.notify( mCB(this,uiStratDisplay,reDraw) );

    disableScrollZoom();
    setDragMode( uiGraphicsViewBase::ScrollHandDrag );
    setSceneBorder( 2 );
    setPrefWidth( 650 );
    setPrefHeight( 400 );
    createDispParamGrp();
    setRange();
    reDraw( 0 );
}


uiStratDisplay::~uiStratDisplay()
{
    delete uicontrol_;
    delete uidatagather_;
}


void uiStratDisplay::setTree()
{
    uidatagather_->setTree();
    setRange();
}


void uiStratDisplay::setRange()
{
    if ( data_.nrCols() && data_.nrUnits(0) )
    {
	const StratDispData::Unit& unstart = *data_.getUnit( 0, 0 );
	const StratDispData::Unit& unstop =*data_.getUnit(0,data_.nrUnits(0)-1);
	Interval<float> viewrg( unstart.zrg_.start, unstop.zrg_.stop );
	float wdth = viewrg.width(); wdth /= (float)10;
	if ( wdth <= 0 ) wdth = 10;
	viewrg.stop += wdth;
	setZRange( viewrg );
    }
    else
	setZRange( maxrg_ );
}


void uiStratDisplay::addControl( uiToolBar* tb )
{
    mDynamicCastGet(uiGraphicsView*,v,const_cast<uiStratDisplay*>(this))
    uiStratViewControl::Setup su( maxrg_ ); su.tb_ = tb;
    uicontrol_ = new uiStratViewControl( *v, su );
    uicontrol_->rangeChanged.notify( mCB(this,uiStratDisplay,controlRange) );
    uicontrol_->setRange( rangefld_->getFInterval() );
}


void uiStratDisplay::controlRange( CallBacker* )
{
    if ( uicontrol_ )
    {
	rangefld_->setValue( uicontrol_->range() );
	dispParamChgd(0);
    }
}


void uiStratDisplay::createDispParamGrp()
{
    dispparamgrp_ = new uiGroup( parent(), "display Params Group" );
    dispparamgrp_->attach( centeredBelow, this );
    rangefld_ = new uiGenInput( dispparamgrp_, tr("Display between Ages (My)"),
		FloatInpIntervalSpec()
		    .setName(BufferString("range start"),0)
		    .setName(BufferString("range stop"),1) );
    rangefld_->valuechanged.notify( mCB(this,uiStratDisplay,dispParamChgd ) );

    const CallBack cbv = mCB( this, uiStratDisplay, selCols );
    viewcolbutton_ = new uiPushButton( dispparamgrp_,uiStrings::sView(),
                                       cbv,true );
    viewcolbutton_->attach( rightOf, rangefld_ );
}


class uiColViewerDlg : public uiDialog
{ mODTextTranslationClass(uiColViewerDlg);
public :
    uiColViewerDlg( uiParent* p, uiStratDrawer& drawer, StratDispData& ad )
	: uiDialog(p,uiDialog::Setup(tr("View Columns"),
				     uiString::emptyString(), mNoHelpKey))
	, drawer_(drawer)
	, data_(ad)
    {
	setCtrlStyle( CloseOnly );

	BufferStringSet colnms;
	for ( int idx=0; idx<data_.nrCols(); idx++ )
	    colnms.add( data_.getCol( idx )->name_ );

	for ( int idx=0; idx<colnms.size(); idx++ )
	{
	    uiCheckBox* box = new uiCheckBox(this, toUiString(colnms.get(idx)));
	    box->setChecked( data_.getCol( idx )->isdisplayed_ );
	    box->activated.notify( mCB(this,uiColViewerDlg,selChg) );
	    colboxflds_ += box;
	    if ( idx ) box->attach( alignedBelow, colboxflds_[idx-1] );
	}
	if ( colnms.size() )
	{
	    allboxfld_ = new uiCheckBox( this, uiStrings::sAll() );
	    allboxfld_->attach( alignedAbove, colboxflds_[0] );
	    allboxfld_->activated.notify( mCB(this,uiColViewerDlg,selAll) );
	}
    }

    void selAll( CallBacker* cb )
    {
	bool allsel = allboxfld_->isChecked();
	for ( int idx=0; idx<colboxflds_.size(); idx++ )
	    colboxflds_[idx]->setChecked( allsel );
    }

    void selChg( CallBacker* cb )
    {
	mDynamicCastGet(uiCheckBox*,box,cb)
	if ( !box ) return;

	for ( int idbox=0; idbox<colboxflds_.size(); idbox++ )
	{
	    NotifyStopper ns( colboxflds_[idbox]->activated );
	    bool ison = false;
	    ison = colboxflds_[idbox]->isChecked();
	    data_.getCol( idbox )->isdisplayed_ = ison;
	}
	drawer_.draw();
    }

protected:

    ObjectSet<uiCheckBox>	colboxflds_;
    uiCheckBox*		allboxfld_;
    uiStratDrawer&		drawer_;
    StratDispData&		data_;
};


void uiStratDisplay::selCols( CallBacker* cb )
{
    uiColViewerDlg dlg( parent(), drawer_, data_ );
    dlg.go();
}


void uiStratDisplay::reDraw( CallBacker* cb )
{
    drawer_.draw();
}


void uiStratDisplay::setZRange( Interval<float> zrg )
{
    rangefld_->setValue( zrg );
    if ( uicontrol_ )
	uicontrol_->setRange( zrg );
    zrg.sort(false);
    drawer_.setZRange( zrg );
    drawer_.draw();
}


void uiStratDisplay::display( bool yn, bool shrk, bool maximize )
{
    uiGraphicsView::display( yn );
    dispparamgrp_->display( yn );
}


void uiStratDisplay::dispParamChgd( CallBacker* cb )
{
    Interval<float> rg = rangefld_->getFInterval();
    if ( rg.start < maxrg_.start || rg.stop > maxrg_.stop
	    || rg.stop <= rg.start || rg.stop <= 0 )
	rg = maxrg_;

    setZRange( rg );
}


void uiStratDisplay::usrClickCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    if ( !mevh || !mevh->hasEvent() || mevh->isHandled() )
	return;

    mevh->setHandled( handleUserClick(mevh->event()) );
}


bool uiStratDisplay::handleUserClick( const MouseEvent& ev )
{
    if ( ev.rightButton() && !ev.ctrlStatus() && !ev.shiftStatus() &&
	    !ev.altStatus() && !islocked_ )
    {
	if ( getColIdxFromPos() == uidatagather_->levelColIdx() )
	{
	    const StratDispData::Level* lvl = getLevelFromPos();
	    if ( !lvl ) return false;
	    uiAction* assmnuitm = new uiAction( tr("Assign marker boundary") );
	    uiMenu menu( parent(), uiStrings::sAction() );
	    menu.insertItem( assmnuitm, 1 );
	    const int mnuid = menu.exec();
	    if ( mnuid<0 )
		return false;
	    else if ( mnuid == 1 )
		uidatawriter_.setUnitLvl( lvl->unitcode_ );
	}
	else if ( getUnitFromPos() )
	{
	    uidatawriter_.handleUnitMenu( getUnitFromPos()->fullCode() );
	}
	else if ( getParentUnitFromPos() || getColIdxFromPos() == 0 )
	{
	    const StratDispData::Unit* unit = getColIdxFromPos() > 0 ?
						getParentUnitFromPos() : 0;
	    bool addunit = data_.nrUnits( 0 ) == 0;
	    if ( !addunit )
	    {
		uiAction* assmnuitm = new uiAction( tr("Add Unit") );
		uiMenu menu( parent(), uiStrings::sAction() );
		menu.insertItem( assmnuitm, 0 );
		const int mnuid = menu.exec();
		addunit = mnuid == 0;
	    }
	    if ( addunit )
		uidatawriter_.addUnit( unit ? unit->fullCode() : 0 );
	}
	return true;
    }
    else if ( ev.leftButton() && getUnitFromPos() )
	return uidatawriter_.setCurrentTreeItem( getUnitFromPos()->fullCode() );

    return false;
}


Geom::Point2D<float> uiStratDisplay::getPos() const
{
    uiStratDisplay* self = const_cast<uiStratDisplay*>( this );
    const float xpos = drawer_.xAxis()->getVal(
			self->getMouseEventHandler().event().pos().x_ );
    const float ypos = drawer_.yAxis()->getVal(
			self->getMouseEventHandler().event().pos().y_ );
    return Geom::Point2D<float>( xpos, ypos );
}


int uiStratDisplay::getColIdxFromPos() const
{
    float xpos = getPos().x_;
    Interval<int> borders(0,0);
    for ( int idx=0; idx<data_.nrCols(); idx++ )
    {
	borders.stop += drawer_.colItem(idx).size_;
	if ( borders.includes( xpos, true ) )
	    return idx;
	borders.start = borders.stop;
    }
    return -1;
}


const StratDispData::Unit* uiStratDisplay::getUnitFromPos() const
{
    return getUnitFromPos( getColIdxFromPos() );
}


const StratDispData::Unit* uiStratDisplay::getParentUnitFromPos() const
{
    return getUnitFromPos( getColIdxFromPos()-1 );
}


const StratDispData::Unit* uiStratDisplay::getUnitFromPos( int cidx ) const
{
    if ( cidx >=0 && cidx<data_.nrCols() )
    {
	Geom::Point2D<float> pos = getPos();
	for ( int idunit=0; idunit<data_.nrUnits(cidx); idunit++ )
	{
	    const StratDispData::Unit* unit = data_.getUnit( cidx, idunit );
	    if ( pos.y_ < unit->zrg_.stop && pos.y_ >= unit->zrg_.start )
		return unit;
	}
    }
    return 0;
}


#define mEps drawer_.yAxis()->range().width()/100
const StratDispData::Level* uiStratDisplay::getLevelFromPos() const
{
    const int cidx = getColIdxFromPos();
    if ( cidx >=0 && cidx<data_.nrCols() )
    {
	Geom::Point2D<float> pos = getPos();
	for ( int idlvl=0; idlvl<data_.nrLevels(cidx); idlvl++ )
	{
	    const StratDispData::Level* lvl= data_.getLevel( cidx, idlvl );
	    if ( pos.y_ < (lvl->zpos_+mEps)  && pos.y_ > (lvl->zpos_-mEps) )
		return lvl;
	}
    }
    return 0;
}


static int border = 20;

uiStratDrawer::uiStratDrawer( uiGraphicsScene& sc, const StratDispData& ad )
    : data_(ad)
    , scene_(sc)
    , xax_(0)
    , yax_(0)
    , emptyitm_(0)
{
    initAxis();
}


void uiStratDrawer::initAxis()
{
    uiAxisHandler::Setup xsu( uiRect::Top );
    uiAxisHandler* xaxis = new uiAxisHandler( &scene_, xsu );
    xaxis->setBounds( Interval<float>( 0, 100 ) );
    setNewAxis( xaxis, true );
    uiBorder uiborder = uiBorder( border );
    uiAxisHandler::Setup ysu( uiRect::Left );
    ysu.border( uiborder ).nogridline( true );
    uiAxisHandler* yaxis = new uiAxisHandler( &scene_, ysu );
    setNewAxis( yaxis, false );
}


uiStratDrawer::~uiStratDrawer()
{
    eraseAll();
}


void uiStratDrawer::setNewAxis( uiAxisHandler* axis, bool isxaxis )
{
    if ( isxaxis )
	{ delete xax_; xax_ = axis; }
    else
	{ delete yax_; yax_ = axis; }

    if ( xax_ && yax_ )
    {
	xax_->setBegin( yax_ );
	yax_->setBegin( xax_ );
    }
}


void uiStratDrawer::updateAxis()
{
    xax_->updateDevSize();
    yax_->updateDevSize();
    yax_->updateScene();
}


void uiStratDrawer::draw()
{
    eraseAll();
    updateAxis();
    drawColumns();
}


void uiStratDrawer::drawColumns()
{
    eraseAll();
    int pos = 0;
    const int nrcols = data_.nrCols();

    for ( int idcol=0; idcol<nrcols; idcol++ )
    {
	if ( !data_.getCol( idcol )->isdisplayed_ ) continue;
	ColumnItem* colitm = new ColumnItem( data_.getCol( idcol )->name_ );
	colitms_ += colitm;
	colitm->pos_ = pos;
	colitm->size_ = (int)xax_->getVal( (int)(scene_.width()+10) )
		      /( data_.nrDisplayedCols() ) ;
	if ( colitm->size_ <0 )
	    colitm->size_ = 0;
	drawBorders( *colitm );
	drawLevels( *colitm );
	drawUnits( *colitm );
	pos ++;
    }

    if ( nrcols && data_.nrUnits(0) == 0 )
	drawEmptyText();
    else
	{ delete emptyitm_; emptyitm_ = 0; }
}


void uiStratDrawer::eraseAll()
{
    for ( int idx = colitms_.size()-1; idx>=0; idx-- )
    {
	ColumnItem* colitm = colitms_[idx];

	delete colitm->borderitm_; colitm->borderitm_ = 0;
	delete colitm->bordertxtitm_; colitm->bordertxtitm_ = 0;
	colitm->txtitms_.erase();
	colitm->lvlitms_.erase();
	colitm->unititms_.erase();
    }
    deepErase( colitms_ );
}


void uiStratDrawer::drawBorders( ColumnItem& colitm )
{
    int x1 = xax_->getPix( mCast( float, (colitm.pos_)*colitm.size_ ) );
    int x2 = xax_->getPix( mCast( float, (colitm.pos_+1)*colitm.size_ ) );
    int y1 = yax_->getPix( yax_->range().stop );
    int y2 = yax_->getPix( yax_->range().start );

    TypeSet<uiPoint> rectpts;
    rectpts += uiPoint( x1, y1 );
    rectpts += uiPoint( x2, y1  );
    rectpts += uiPoint( x2, y2  );
    rectpts += uiPoint( x1, y2  );
    rectpts += uiPoint( x1, y1  );
    uiPolyLineItem* pli = scene_.addItem( new uiPolyLineItem( rectpts ) );
    pli->setPenStyle( OD::LineStyle(OD::LineStyle::Solid,1,Color::Black()) );
    colitm.borderitm_ = pli;

    uiTextItem* ti = scene_.addItem(new uiTextItem( toUiString(colitm.name_) ));
    ti->setTextColor( Color::Black() );
    ti->setPos( mCast(float,(x1+x2)/2), mCast(float,y1-18) );
    ti->setAlignment( OD::Alignment::HCenter );
    ti->setZValue( 2 );
    colitm.bordertxtitm_ = ti;
}


void uiStratDrawer::drawLevels( ColumnItem& colitm )
{
    if ( !colitm.lvlitms_.isEmpty() )
	{ colitm.lvlitms_.erase(); colitm.txtitms_.erase(); }
    const int colidx = colitms_.indexOf( &colitm );
    if ( colidx < 0 ) return;
    for ( int idx=0; idx<data_.getCol(colidx)->levels_.size(); idx++ )
    {
	const StratDispData::Level& lvl = *data_.getCol(colidx)->levels_[idx];

	int x1 = xax_->getPix( mCast( float, (colitm.pos_)*colitm.size_ ) );
	int x2 = xax_->getPix( mCast( float, (colitm.pos_+1)*colitm.size_ ) );
	int y = yax_->getPix( lvl.zpos_ );

	uiLineItem* li = scene_.addItem( new uiLineItem(x1, y, x2, y ) );

	OD::LineStyle::Type lst = lvl.name_.isEmpty() ? OD::LineStyle::Dot
						  : OD::LineStyle::Solid;
	li->setPenStyle( OD::LineStyle(lst,2,lvl.color_) );
	uiTextItem* ti = scene_.addItem( new uiTextItem(
					        toUiString(lvl.name_) ) );
	ti->setPos( mCast( float, x1 + (x2-x1)/2 ), mCast( float, y ) );
	ti->setZValue( 2 );
	ti->setTextColor( lvl.color_ );

	colitm.txtitms_ += ti;
	colitm.lvlitms_ += li;
    }
}


void uiStratDrawer::drawEmptyText()
{
    delete emptyitm_; emptyitm_ =0;

    const int x = xax_->getPix( 0 );
    const int y1 = yax_->getPix( yax_->range().stop );
    const int y2 = yax_->getPix( yax_->range().start );

    uiTextItem* ti = scene_.addItem( new uiTextItem( tr("<Click to add>") ) );
    ti->setTextColor( Color::Black() );
    ti->setPos( mCast(float,x), mCast(float,y2 - abs((y2-y1)/2) -10) );
    ti->setZValue( 2 );
    emptyitm_ = ti;
}


void uiStratDrawer::drawUnits( ColumnItem& colitm )
{
    colitm.txtitms_.erase(); colitm.unititms_.erase();
    const int colidx = colitms_.indexOf( &colitm );
    if ( colidx < 0 ) return;

    const Interval<float> rg = yax_->range();

    for ( int unidx=0; unidx<data_.getCol(colidx)->units_.size(); unidx++ )
    {
	const StratDispData::Unit& unit = *data_.getCol(colidx)->units_[unidx];
	Interval<float> unitrg = unit.zrg_;
	if ( ( ( !rg.includes(unitrg.start,true) &&
		 !rg.includes(unitrg.stop,true) )
	    && ( !unitrg.includes(rg.start,true) &&
		 !unitrg.includes(rg.stop,true) ) )
		|| !unit.isdisplayed_ ) continue;
	unitrg.limitTo( rg );

	int x1 = xax_->getPix( mCast(float,(colitm.pos_)*colitm.size_) );
	int x2 = xax_->getPix( mCast(float,(colitm.pos_+1)*colitm.size_) );
	bool ztop = ( unitrg.start < rg.stop );
	bool zbase = ( unitrg.stop > rg.start );
	int y1 = yax_->getPix( ztop ? rg.stop : unitrg.start );
	int y2 = yax_->getPix( zbase ? rg.start : unitrg.stop );

	TypeSet<uiPoint> rectpts;
	rectpts += uiPoint( x1, y1 );
	rectpts += uiPoint( x2, y1 );
	rectpts += uiPoint( x2, y2 );
	rectpts += uiPoint( x1, y2 );
	rectpts += uiPoint( x1, y1 );
	uiPolygonItem* pli = scene_.addPolygon( rectpts, true );
	pli->setPenColor( Color::Black() );
	if ( unit.color_ != Color::White() )
	    pli->setFillColor( unit.color_, true );

	BufferString unm( unit.name() );
	for ( int idx=1; idx<unm.size(); idx++ )
	{
	    BufferString tmpnm = unm; tmpnm[idx] = '\0';
	    if ( FontList().get().width( toUiString(tmpnm) ) > ( x2-x1 ) )
		{ unm[idx-1] = '\0'; break; }
	}

	uiTextItem* ti = scene_.addItem( new uiTextItem( toUiString(unm )) );
	ti->setTextColor( Color::Black() );
	ti->setPos( mCast(float,(x1+x2)/2), mCast(float,y2-abs((y2-y1)/2)-10) );
	ti->setAlignment( OD::Alignment::HCenter );
	ti->setZValue( 2 );
	colitm.txtitms_ += ti;
	colitm.unititms_ += pli;
    }
}



#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton( tb_, fnm, tt, mCB(this,uiStratViewControl,cbnm) ); \
    tb_->addButton( but );

uiStratViewControl::uiStratViewControl( uiGraphicsView& v, Setup& su )
    : viewer_(v)
    , rangeChanged(this)
    , tb_(su.tb_)
    , boundingrange_(su.maxrg_)
{
    if ( tb_ )
	tb_->addSeparator();
    else
    {
	tb_ = new uiToolBar( v.parent(), toUiString("Viewer toolbar"),
							      uiToolBar::Top );
	mDynamicCastGet(uiMainWin*,mw,v.parent())
	if ( mw )
	    mw->addToolBar( tb_ );
    }

    mDefBut(rubbandzoombut_,"rubbandzoom",dragModeCB,tr("Rubberband zoom"));
    mDefBut(vertzoominbut_,"vertzoomin",zoomCB,tr("Zoom in"));
    mDefBut(vertzoomoutbut_,"vertzoomout",zoomCB,tr("Zoom out"));
    mDefBut(cancelzoombut_,"cancelzoom",cancelZoomCB,tr("Cancel zoom"));
    rubbandzoombut_->setToggleButton( true );

    viewer_.getKeyboardEventHandler().keyPressed.notify(
				mCB(this,uiStratViewControl,keyPressed) );

    MouseEventHandler& meh = mouseEventHandler();
    meh.wheelMove.notify( mCB(this,uiStratViewControl,wheelMoveCB) );
    meh.buttonPressed.notify(mCB(this,uiStratViewControl,handDragStarted));
    meh.buttonReleased.notify(mCB(this,uiStratViewControl,handDragged));
    meh.movement.notify( mCB(this,uiStratViewControl,handDragging));
    viewer_.rubberBandUsed.notify( mCB(this,uiStratViewControl,rubBandCB) );
}


void uiStratViewControl::setSensitive( bool yn )
{
    rubbandzoombut_->setSensitive( yn );
    vertzoominbut_->setSensitive( yn );
    vertzoomoutbut_->setSensitive( yn );
    cancelzoombut_->setSensitive( yn );
}


static float zoomfwdfac = 0.8;

void uiStratViewControl::zoomCB( CallBacker* but )
{
    const bool zoomin = but == vertzoominbut_;
    const Interval<float>& brge = boundingrange_;
    if ( (!zoomin && range_==brge) || (zoomin && range_.width()<=0.1) ) return;

    const MouseEventHandler& meh = mouseEventHandler();
    const uiRect& allarea = viewer_.getSceneRect();
    LinScaler scaler( allarea.top()+border, range_.start,
		      allarea.bottom()-border, range_.stop );
    float rgpos = meh.hasEvent() ? (float)scaler.scale(meh.event().pos().y_)
				 : range_.center();
    const float zoomfac = zoomin ? zoomfwdfac : 1/zoomfwdfac;
    const float twdth = (rgpos-range_.start) * zoomfac;
    const float bwdth = (range_.stop-rgpos) * zoomfac;

    if ( rgpos - twdth < brge.start )	rgpos = brge.start + twdth;
    if ( rgpos + bwdth > brge.stop )	rgpos = brge.stop - bwdth;
    range_.set( rgpos - twdth, rgpos + bwdth );
    rangeChanged.trigger();

    updatePosButtonStates();
}


void uiStratViewControl::cancelZoomCB( CallBacker* )
{
    range_ = boundingrange_;
    rangeChanged.trigger();
    updatePosButtonStates();
}


void uiStratViewControl::updatePosButtonStates()
{
    const bool iszoomatstart = range_==boundingrange_;
    vertzoomoutbut_->setSensitive( !iszoomatstart );
    cancelzoombut_->setSensitive( !iszoomatstart );
}


MouseEventHandler& uiStratViewControl::mouseEventHandler()
{
    return viewer_.getNavigationMouseEventHandler();
}


void uiStratViewControl::wheelMoveCB( CallBacker* )
{
    const MouseEventHandler& mvh = mouseEventHandler();
    const MouseEvent& ev = mvh.event();
    if ( mIsZero(ev.angle(),0.01) )
	return;
    zoomCB( ev.angle() < 0 ? vertzoominbut_ : vertzoomoutbut_ );
}


void uiStratViewControl::dragModeCB( CallBacker* )
{
    viewer_.setDragMode( rubbandzoombut_->isOn() ?
			 uiGraphicsViewBase::RubberBandDrag :
			 uiGraphicsViewBase::ScrollHandDrag );
}


void uiStratViewControl::keyPressed( CallBacker* )
{
    const KeyboardEvent& ev = viewer_.getKeyboardEventHandler().event();
    if ( ev.key_ == OD::KB_Escape )
    {
	rubbandzoombut_->setOn( !rubbandzoombut_->isOn() );
	dragModeCB( rubbandzoombut_ );
    }
}


void uiStratViewControl::handDragStarted( CallBacker* )
{
    if ( mouseEventHandler().event().rightButton() )
	return;
    mousepressed_ = true;
    startdragpos_ = mCast( float, mouseEventHandler().event().pos().y_ );
}


void uiStratViewControl::handDragging( CallBacker* )
{
    if ( viewer_.dragMode() != uiGraphicsViewBase::ScrollHandDrag
	|| !mousepressed_ ) return;

    const float newpos = mCast(float,mouseEventHandler().event().pos().y_);
    const uiRect& allarea = viewer_.getSceneRect();
    LinScaler scaler( allarea.top()+border, range_.start,
		      allarea.bottom()-border, range_.stop );
    const float shift=(float)(scaler.scale(newpos)-scaler.scale(startdragpos_));
    startdragpos_ = newpos;

    Interval<float> rg( range_.start - shift, range_.stop - shift );
    if ( rg.start < boundingrange_.start )
	rg.set( boundingrange_.start, range_.stop );
    if ( rg.stop > boundingrange_.stop )
	rg.set( range_.start, boundingrange_.stop );
    range_ = rg;

    rangeChanged.trigger();
}


void uiStratViewControl::handDragged( CallBacker* cb )
{
    handDragging( cb );
    mousepressed_ = false;
}


void uiStratViewControl::rubBandCB( CallBacker* )
{
    const uiRect* selarea = viewer_.getSelectedArea();
    if ( !selarea || (selarea->topLeft() == selarea->bottomRight())
	    || (selarea->width()<5 && selarea->height()<5) )
	return;

    const uiRect& allarea = viewer_.getSceneRect();
    LinScaler scaler( allarea.top()+border, range_.start,
		      allarea.bottom()-border, range_.stop );
    const Interval<float> rg( mCast(float,scaler.scale(selarea->top())),
			      mCast(float,scaler.scale(selarea->bottom())) );
    if ( rg.width()<=0.1 ) return;

    range_ = rg;
    range_.sort();
    range_.limitTo( boundingrange_ );
    rangeChanged.trigger();

    rubbandzoombut_->setOn( false );
    dragModeCB( rubbandzoombut_ );
    updatePosButtonStates();
}
