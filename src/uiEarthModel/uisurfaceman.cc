/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2003
________________________________________________________________________

-*/


#include "uisurfaceman.h"

#include "ascstream.h"
#include "ioobjctxt.h"
#include "file.h"
#include "dbman.h"
#include "ioobj.h"
#include "dbkey.h"
#include "oddirs.h"
#include "od_iostream.h"
#include "survinfo.h"

#include "embodytr.h"
#include "emfaultauxdata.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "emsurfauxdataio.h"

#include "uibodyoperatordlg.h"
#include "uibodyregiondlg.h"
#include "uicolor.h"
#include "uigeninputdlg.h"
#include "uihorizonmergedlg.h"
#include "uihorizonrelations.h"
#include "uiimpbodycaldlg.h"
#include "uilistbox.h"
#include "uiioobjmanip.h"
#include "uiioobjselgrp.h"
#include "uiiosurfacedlg.h"
#include "uimsg.h"
#include "uisplitter.h"
#include "uistratlvlsel.h"
#include "uistrattreewin.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbutton.h"
#include "od_helpids.h"


mDefineEnumUtils(uiSurfaceMan,Type,"Surface type")
{
    EMHorizon2DTranslatorGroup::sGroupName(),
    EMHorizon3DTranslatorGroup::sGroupName(),
    EMAnyHorizonTranslatorGroup::sGroupName(),
    EMFaultStickSetTranslatorGroup::sGroupName(),
    EMFault3DTranslatorGroup::sGroupName(),
    EMBodyTranslatorGroup::sGroupName(),
    0
};

mDefineInstanceCreatedNotifierAccess(uiSurfaceMan)



#define mCaseRetCtxt(enm,trgrpnm) \
    case uiSurfaceMan::enm: return trgrpnm##TranslatorGroup::ioContext()

static IOObjContext getIOCtxt( uiSurfaceMan::Type typ )
{
    switch ( typ )
    {
	mCaseRetCtxt(Hor2D,EMHorizon2D);
	mCaseRetCtxt(Hor3D,EMHorizon3D);
	mCaseRetCtxt(AnyHor,EMAnyHorizon);
	mCaseRetCtxt(StickSet,EMFaultStickSet);
	mCaseRetCtxt(Flt3D,EMFault3D);
	default:
	mCaseRetCtxt(Body,EMBody);
    }
}

#define mCaseRetStr(enm,str) \
    case uiSurfaceMan::enm: return toUiString("%1 %2").arg(act).arg(str);

static uiString getActStr( uiSurfaceMan::Type typ, const uiString& act )
{
    switch ( typ )
    {
        mCaseRetStr(Hor2D, EMHorizon2DTranslatorGroup::sTypeName() );
	mCaseRetStr(Hor3D, EMHorizon3DTranslatorGroup::sTypeName());
	mCaseRetStr(StickSet, uiStrings::sFaultStickSet());
	mCaseRetStr(Flt3D, uiStrings::sFault());
	mCaseRetStr(Body, od_static_tr("getActStr","Bodies"));
	default:
	mCaseRetStr(AnyHor, uiStrings::sHorizon(1));
    }
}

static HelpKey getHelpID( uiSurfaceMan::Type typ )
{
    switch ( typ )
    {
case uiSurfaceMan::Hor2D:	return mODHelpKey(mSurface2DManHelpID);
case uiSurfaceMan::StickSet:	return mODHelpKey(mFaultStickSetsManageHelpID);
case uiSurfaceMan::Flt3D:	return mODHelpKey(mFaultsManageHelpID);
case uiSurfaceMan::Body:	return mODHelpKey(mBodyManHelpID);
default:			return mODHelpKey(mSurfaceManHelpID);
    }
}


uiSurfaceMan::uiSurfaceMan( uiParent* p, uiSurfaceMan::Type typ )
    : uiObjFileMan(p,uiDialog::Setup(getActStr(typ,tr("Manage")),
				    mNoDlgTitle, getHelpID(typ)).nrstatusflds(1)
				    .modal(false), getIOCtxt(typ) )
    , type_(typ)
    , attribfld_(0)
    , man2dbut_(0)
    , surfdatarenamebut_(0)
    , surfdataremovebut_(0)
    , copybut_(0)
    , mergehorbut_(0)
    , applybodybut_(0)
    , createregbodybut_(0)
    , volestimatebut_(0)
    , switchvalbut_(0)
{
    createDefaultUI();
    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();

    if ( type_ != Body )
	copybut_ = manipgrp->addButton( "copyobj", tr("Copy to new object"),
					mCB(this,uiSurfaceMan,copyCB) );

    if ( type_ == Hor2D || type_ == AnyHor )
    {
	man2dbut_ = manipgrp->addButton( "man2d",
	  uiStrings::phrManage( EMHorizon2DTranslatorGroup::sTypeName(mPlural)),
                                mCB(this,uiSurfaceMan,man2dCB) );
	man2dbut_->setSensitive( false );
    }
    if ( type_ == Hor3D )
    {
	mergehorbut_ = manipgrp->addButton( "mergehorizons",
		       uiStrings::phrMerge(uiStrings::phrJoinStrings(
		       uiStrings::s3D(), uiStrings::sHorizon(2))),
		       mCB(this,uiSurfaceMan,merge3dCB) );
    }
    if ( type_ == Hor3D || type_ == AnyHor )
    {
	uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Horizon Data"),
			     uiListBox::AboveMid );
	attribfld_ = new uiListBox( listgrp_, su );
	attribfld_->attach( rightOf, selgrp_ );
	attribfld_->setHSzPol( uiObject::Wide );
	attribfld_->box()->setToolTip(
		tr("Horizon Data (Attributes stored in Horizon format)") );
	attribfld_->selectionChanged.notify( mCB(this,uiSurfaceMan,attribSel) );

	uiManipButGrp* butgrp = new uiManipButGrp( attribfld_ );
	surfdataremovebut_ = butgrp->addButton( uiManipButGrp::Remove,
					tr("Remove selected Horizon Data"),
					mCB(this,uiSurfaceMan,removeAttribCB) );
	surfdatarenamebut_ = butgrp->addButton( uiManipButGrp::Rename,
					tr("Rename selected Horizon Data"),
					mCB(this,uiSurfaceMan,renameAttribCB) );
	butgrp->attach( rightTo, attribfld_->box() );

	new uiPushButton( extrabutgrp_, uiStrings::sStratigraphy(),
		mCB(this,uiSurfaceMan,stratSel), false );

	new uiPushButton( extrabutgrp_, tr("Relations"),
		mCB(this,uiSurfaceMan,setRelations), false );
	extrabutgrp_->attach( ensureBelow, attribfld_ );

	setPrefWidth( 50 );
    }
    if ( type_ == Flt3D )
    {
    }
    if ( type_ == Body )
    {
	applybodybut_ = manipgrp->addButton( "set_union",
					     tr("Apply Body operations"),
					   mCB(this,uiSurfaceMan,mergeBodyCB) );
	createregbodybut_ = manipgrp->addButton( "set_implicit",
						 tr("Create region Body"),
				mCB(this,uiSurfaceMan,createBodyRegionCB) );
	volestimatebut_ = manipgrp->addButton( "bodyvolume",
					     tr("Volume estimate"),
					     mCB(this,uiSurfaceMan,calcVolCB) );
	switchvalbut_ = manipgrp->addButton( "switch_implicit",
					     tr("Switch inside/outside value"),
					mCB(this,uiSurfaceMan,switchValCB) );
    }

    mTriggerInstanceCreatedNotifier();
    selChg( this );
}


uiSurfaceMan::~uiSurfaceMan()
{}


void uiSurfaceMan::ownSelChg()
{
    setToolButtonProperties();
}


void uiSurfaceMan::attribSel( CallBacker* )
{
    setToolButtonProperties();
}


#define mSetButToolTip(but,str1,curattribnms,str2,deftt) \
    if ( but ) \
    { \
	if ( but->sensitive() ) \
	{ \
	    tt.setEmpty(); \
	    tt.add( str1 ).add( curattribnms ).add( str2 ); \
	    but->setToolTip( tr(tt) ); \
	} \
	else \
	{ \
	    but->setToolTip( deftt ); \
	} \
    }
uiString uiSurfaceMan::sRenameSelData()
{
    return tr("Rename selected data");
}


uiString uiSurfaceMan::sRemoveSelData()
{
    return tr("Remove selected data");
}


void uiSurfaceMan::setToolButtonProperties()
{
    const bool hasattribs = attribfld_ && !attribfld_->isEmpty();

    BufferString tt, cursel;

    if ( curioobj_ )
	cursel.add( curioobj_->name() );

    if ( surfdatarenamebut_ )
    {
	surfdatarenamebut_->setSensitive( hasattribs );
	mSetButToolTip(surfdatarenamebut_,"Rename '",attribfld_->getText(),
		       "'", sRenameSelData())
    }

    if ( surfdataremovebut_ )
    {
	surfdataremovebut_->setSensitive( hasattribs );
	BufferStringSet attrnms;
	attribfld_->getChosen( attrnms );
	mSetButToolTip(surfdataremovebut_,"Remove ",attrnms.getDispString(2),
		       "", sRemoveSelData())
    }

    if ( copybut_ )
    {
	copybut_->setSensitive( curioobj_ );
	mSetButToolTip(copybut_,"Copy '",cursel,"' to new object",
		       uiStrings::phrCopy(tr("to new object")))
    }

    if ( mergehorbut_ )
    {
	mergehorbut_->setSensitive( curioobj_ );
	BufferStringSet selhornms;
	selgrp_->getChosen( selhornms );
	if ( selhornms.size() > 1 )
	{
	    mSetButToolTip(mergehorbut_,"Merge ",selhornms.getDispString(2),
			   "", uiStrings::phrMerge(uiStrings::phrJoinStrings(
			   uiStrings::s3D(),uiStrings::sHorizon(2))))
	}
	else
	    mergehorbut_->setToolTip(  uiStrings::phrMerge(
				    uiStrings::phrJoinStrings(uiStrings::s3D(),
				    uiStrings::sHorizon(2))) );
    }

     if ( type_ == Body )
     {
	 applybodybut_->setSensitive( curioobj_ );
	 createregbodybut_->setSensitive( curioobj_ );
	 volestimatebut_->setSensitive( curioobj_ );
	 switchvalbut_->setSensitive( curioobj_ );
	 mSetButToolTip(volestimatebut_,"Estimate volume of '",cursel,
			"'", tr("Volume estimate"));
	 mSetButToolTip(switchvalbut_,"Switch inside/outside value of '",
			cursel,"'", tr("Switch inside/outside value"));
     }
}


bool uiSurfaceMan::isCur2D() const
{
    return curioobj_ &&
	   curioobj_->group() == EMHorizon2DTranslatorGroup::sGroupName();
}


bool uiSurfaceMan::isCurFault() const
{
    const BufferString grp = curioobj_ ? curioobj_->group().buf() : "";
    return grp==EMFaultStickSetTranslatorGroup::sGroupName() ||
	   grp==EMFault3DTranslatorGroup::sGroupName();
}


void uiSurfaceMan::copyCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    const bool canhaveattribs = type_ == uiSurfaceMan::Hor3D;
    PtrMan<IOObj> ioobj = curioobj_->clone();
    uiSurfaceRead::Setup su( ioobj->group() );
    su.withattribfld(canhaveattribs).withsubsel(!isCurFault())
      .multisubsel(true).withsectionfld(false);

    uiCopySurface dlg( this, *ioobj, su );
    if ( dlg.go() )
	selgrp_->fullUpdate( ioobj->key() );
}


void uiSurfaceMan::merge3dCB( CallBacker* )
{
    uiHorizonMergeDlg dlg( this, false );
    DBKeySet chsnmids;
    selgrp_->getChosen( chsnmids );
    dlg.setInputHors( chsnmids );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.getNewHorMid() );
}


void uiSurfaceMan::mergeBodyCB( CallBacker* )
{
    uiBodyOperatorDlg dlg( this );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.getBodyMid() );
}


void uiSurfaceMan::calcVolCB( CallBacker* )
{
    if ( !curioobj_ )
	return;

    RefMan<EM::EMObject> emo =
	EM::EMM().loadIfNotFullyLoaded( curioobj_->key(), 0 );
    mDynamicCastGet( EM::Body*, emb, emo.ptr() );
    if ( !emb )
    {
	uiString msg = tr( "Body '%1' is empty" ).arg( curioobj_->uiName() );
	uiMSG().error(msg);
	return;
    }

    uiImplBodyCalDlg dlg( this, *emb );
    uiString dlgtitle = tr( "Body volume estimation for '%1'" )
			  .arg(curioobj_->uiName() );
    dlg.setTitleText( dlgtitle );
    dlg.go();
}


void uiSurfaceMan::createBodyRegionCB( CallBacker* )
{
    uiBodyRegionDlg dlg( this, false );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.getBodyMid() );
}


void uiSurfaceMan::switchValCB( CallBacker* )
{
    uiImplicitBodyValueSwitchDlg dlg( this, curioobj_ );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.getBodyMid() );
}


void uiSurfaceMan::setRelations( CallBacker* )
{
    uiHorizonRelationsDlg dlg( this, isCur2D() );
    dlg.go();
}


void uiSurfaceMan::removeAttribCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    if ( curioobj_->implReadOnly() )
    {
	uiMSG().error(
		tr("Cannot remove Surface Data. Surface is read-only"));
	return;
    }

    BufferStringSet attrnms;
    attribfld_->getChosen( attrnms );
    uiString msg = tr("%1\nwill be removed from disk.\n"
                      "Do you wish to continue?")
                 .arg(attrnms.getDispString(2));
    if ( !uiMSG().askRemove(msg) )
	return;

    if ( curioobj_->group()==EMFault3DTranslatorGroup::sGroupName() )
    {
	EM::FaultAuxData fad( curioobj_->key() );
	for ( int ida=0; ida<attrnms.size(); ida++ )
	    fad.removeData( attrnms.get(ida) );
    }
    else
    {
	for ( int ida=0; ida<attrnms.size(); ida++ )
	    EM::SurfaceAuxData::removeFile( *curioobj_, attrnms.get(ida) );
    }

    selChg( this );
}


#define mErrRet(msg) { uiMSG().error(msg); return; }

void uiSurfaceMan::renameAttribCB( CallBacker* )
{
    if ( !curioobj_ ) return;

    const BufferString attribnm = attribfld_->getText();
    const uiString titl = tr("Rename '%1'").arg(attribnm);
    uiGenInputDlg dlg( this, titl, tr("New name"), new StringInpSpec(attribnm));
    if ( !dlg.go() ) return;

    const char* newnm = dlg.text();
    if ( attribfld_->isPresent(newnm) )
	mErrRet( tr("Name is already in use") )

    if ( curioobj_->group()==EMFault3DTranslatorGroup::sGroupName() )
    {
	EM::FaultAuxData fad( curioobj_->key() );
	fad.setDataName( attribnm, newnm );

	selChg( this );
	return;
    }

    const BufferString filename =
		EM::SurfaceAuxData::getFileName( *curioobj_, attribnm );
    if ( File::isEmpty(filename) )
	mErrRet( tr("Cannot find Horizon Data file") )
    else if ( !File::isWritable(filename) )
	mErrRet( tr("The Horizon Data file is not writable") )

    od_istream instrm( filename );
    if ( !instrm.isOK() )
	mErrRet( tr("Cannot open Horizon Data file for read") )
    const BufferString ofilename( filename, "_new" );
    od_ostream outstrm( ofilename );
    if ( !outstrm.isOK() )
	mErrRet( tr("Cannot open new Horizon Data file for write") )

    ascistream aistrm( instrm );
    ascostream aostrm( outstrm );
    aostrm.putHeader( aistrm.fileType() );
    IOPar iop( aistrm );
    iop.set( sKey::Attribute(), newnm );
    iop.putTo( aostrm );

    outstrm.add( instrm );
    const bool writeok = outstrm.isOK();
    instrm.close(); outstrm.close();

    BufferString tmpfnm( filename ); tmpfnm += "_old";
    if ( !writeok )
    {
	File::remove( ofilename );
	mErrRet( tr("Error during write. Reverting to old name") )
    }

    if ( File::rename(filename,tmpfnm) )
	File::rename(ofilename,filename);
    else
    {
	File::remove( ofilename );
	mErrRet( tr("Cannot rename file(s). Reverting to old name") )
    }

    if ( File::exists(tmpfnm) )
	File::remove( tmpfnm );

    selChg( this );
}


void uiSurfaceMan::fillAttribList()
{
    if ( !attribfld_ ) return;

    attribfld_->setEmpty();
    DBKeySet mids;
    selgrp_->getChosen( mids );
    if ( mids.isEmpty() )
	return;

    const DBKey& firstmid = mids[0];
    EM::IOObjInfo info( firstmid );
    if ( !info.isOK() )
	return;

    BufferStringSet availableattrnms;
    if ( !info.getAttribNames( availableattrnms ) )
	return;

    for ( int midx=1; midx<mids.size(); midx++ )
    {
	const DBKey& mid = mids[midx];
	EM::IOObjInfo eminfo( mid );
	if ( !info.isOK() )
	    return;

	BufferStringSet attrnms;
	eminfo.getAttribNames( attrnms );
	for ( int idx=availableattrnms.size()-1; idx>=0; idx-- )
	{
	    if ( !attrnms.isPresent(availableattrnms.get(idx)) )
		availableattrnms.removeSingle( idx );
	}
    }

    attribfld_->addItems( availableattrnms );
    attribfld_->chooseAll( false );
}


void uiSurfaceMan::mkFileInfo()
{
#define mAddInlCrlRangeTxt() \
    if ( range.isUdf() ) \
	txt += "-\n"; \
    else \
    { \
	txt += range.start; txt += " - "; txt += range.stop; \
	txt += " - "; txt += range.step; txt += "\n"; \
    }

#define mAddZRangeTxt() \
    if ( !zrange.isUdf() ) \
    { \
	txt += "Z range"; txt += SI().zUnitString().getFullString(); \
	txt += ": "; \
	txt += mNINT32( zrange.start * SI().zDomain().userFactor() ); \
	txt += " - "; \
	txt += mNINT32( zrange.stop * SI().zDomain().userFactor() ); \
	txt += "\n"; \
    }

    fillAttribList();
    BufferString txt;
    EM::IOObjInfo eminfo( curioobj_ );
    if ( !eminfo.isOK() )
    {
	txt += eminfo.name(); txt.add( " has no file on disk (yet).\n" );
	setInfo( txt );
	return;
    }


    if ( man2dbut_ )
	man2dbut_->setSensitive( isCur2D() );

    if ( isCur2D() || isCurFault() )
    {
	txt = isCur2D() ? "Nr. 2D lines: " : "Nr. Sticks: ";
	if ( isCurFault() )
	{
	    if ( eminfo.nrSticks() < 0 )
		txt += "Cannot determine number of sticks for this object type";
	    else
		txt += eminfo.nrSticks();
	}
	else
	{
	    BufferStringSet linenames;
	    if ( eminfo.getLineNames(linenames) )
		txt += linenames.size();
	    else
		txt += "-";
	}

	txt += "\n";
    }
    else if ( type_ == Body )
    {
	TrcKeyZSampling cs(false);
	if ( eminfo.getBodyRange(cs) )
	{
	    StepInterval<int> range = cs.hsamp_.lineRange();
	    txt = "In-line range: "; mAddInlCrlRangeTxt()
	    range = cs.hsamp_.trcRange();
	    txt += "Cross-line range: "; mAddInlCrlRangeTxt()
	    const Interval<float>& zrange = cs.zsamp_;
	    mAddZRangeTxt()
	}
    }
    else
    {
	StepInterval<int> range = eminfo.getInlRange();
	txt = "In-line range: "; mAddInlCrlRangeTxt()
	range = eminfo.getCrlRange();
	txt += "Cross-line range: "; mAddInlCrlRangeTxt()

        const Interval<float>& zrange = eminfo.getZRange();
	mAddZRangeTxt()
    }

    txt += getFileInfo();

    BufferStringSet sectionnms;
    eminfo.getSectionNames( sectionnms );
    if ( sectionnms.size() > 1 )
    {
	txt += "Nr of sections: "; txt += sectionnms.size(); txt += "\n";
	for ( int idx=0; idx<sectionnms.size(); idx++ )
	{
	    txt += "\tPatch "; txt += idx+1; txt += ": ";
	    txt += sectionnms[idx]->buf(); txt += "\n";
	}
    }

    setInfo( txt );
    setToolButtonProperties();
}


od_int64 uiSurfaceMan::getFileSize( const char* filenm, int& nrfiles ) const
{
    if ( File::isEmpty(filenm) ) return -1;
    od_int64 totalsz = File::getKbSize( filenm );
    nrfiles = 1;

    const BufferString basefnm( filenm );
    for ( int idx=0; ; idx++ )
    {
	BufferString fnm( basefnm ); fnm += "^"; fnm += idx; fnm += ".hov";
	if ( !File::exists(fnm) ) break;
	totalsz += File::getKbSize( fnm );
	nrfiles++;
    }

    return totalsz;
}


class uiSurfaceStratDlg : public uiDialog
{ mODTextTranslationClass(uiSurfaceStratDlg);
public:
uiSurfaceStratDlg( uiParent* p,  const DBKeySet& ids )
    : uiDialog(p,uiDialog::Setup(uiStrings::sStratigraphy(),mNoDlgTitle,
                                 mNoHelpKey))
    , objids_(ids)
{
    tbl_ = new uiTable( this, uiTable::Setup(ids.size(),3),
			"Stratigraphy Table" );
    BufferStringSet lbls; lbls.add( "Name" ).add( "Color" ).add( "Marker" );
    tbl_->setColumnLabels( lbls );
    tbl_->setTableReadOnly( true );
    tbl_->setRowResizeMode( uiTable::Interactive );
    tbl_->setColumnResizeMode( uiTable::ResizeToContents );
    tbl_->setColumnStretchable( 2, true );
    tbl_->setPrefWidth( 400 );
    tbl_->doubleClicked.notify( mCB(this,uiSurfaceStratDlg,doCol) );

    uiToolButton* sb = new uiToolButton( this, "man_strat",
				      tr("Edit Stratigraphy to define Markers"),
				      mCB(this,uiSurfaceStratDlg,doStrat) );
    sb->attach( rightOf, tbl_ );

    IOPar par;
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	par.setEmpty();
	if ( !EM::EMM().readDisplayPars(ids[idx],par) )
	    continue;
	tbl_->setText( RowCol(idx,0), EM::EMM().objectName(ids[idx]) );

	Color col( Color::White() );
	par.get( sKey::Color(), col );
	tbl_->setColor( RowCol(idx,1), col );

	uiStratLevelSel* levelsel = new uiStratLevelSel( 0, true,
						    uiStrings::sEmptyString() );
	levelsel->selChange.notify( mCB(this,uiSurfaceStratDlg,lvlChg) );
	tbl_->setCellGroup( RowCol(idx,2), levelsel );
	Strat::Level::ID lvlid;
	par.get( sKey::StratRef(), lvlid );
	levelsel->setID( lvlid );
    }
}


protected:

void doStrat( CallBacker* )
{ StratTWin().popUp(); }

void doCol( CallBacker* )
{
    const RowCol& cell = tbl_->notifiedCell();
    if ( cell.col() != 1 )
	return;

    mDynamicCastGet(uiStratLevelSel*,levelsel,
	tbl_->getCellGroup(RowCol(cell.row(),2)))
    const bool havelvl = levelsel && levelsel->getID().isValid();
    if ( havelvl )
    {
	uiMSG().error( tr("Cannot change color of regional marker") );
	return;
    }

    Color newcol = tbl_->getColor( cell );
    if ( selectColor(newcol,this,uiStrings::phrJoinStrings(
	 uiStrings::sHorizon(),uiStrings::sColor())) )
	tbl_->setColor( cell, newcol );

    tbl_->setSelected( cell, false );
}

void lvlChg( CallBacker* cb )
{
    mDynamicCastGet(uiStratLevelSel*,levelsel,cb)
    if ( !levelsel ) return;

    const Color col = levelsel->getColor();
    if ( col == Color::NoColor() ) return;

    const RowCol rc = tbl_->getCell( levelsel );
    tbl_->setColor( RowCol(rc.row(),1), col );
}

bool acceptOK()
{
    for ( int idx=0; idx<objids_.size(); idx++ )
    {
	IOPar par;
	Color col = tbl_->getColor( RowCol(idx,1) );
	par.set( sKey::Color(), col );

	mDynamicCastGet(uiStratLevelSel*,levelsel,
			tbl_->getCellGroup(RowCol(idx,2)))
	const Strat::Level::ID lvlid = levelsel ? levelsel->getID()
					: Strat::Level::ID::getInvalid();
	IOPar displaypar;
	displaypar.set( sKey::StratRef(), lvlid );
	displaypar.set( sKey::Color(), col );
	EM::EMM().writeDisplayPars( objids_[idx], displaypar );
    }

    return true;
}


    uiTable*	tbl_;
    const DBKeySet& objids_;

};


void uiSurfaceMan::stratSel( CallBacker* )
{
    uiSurfaceStratDlg dlg( this, selgrp_->getIOObjIds() );
    dlg.go();
}


class uiSurface2DMan : public uiDialog
{ mODTextTranslationClass(uiSurface2DMan)
public:

uiSurface2DMan( uiParent* p, const EM::IOObjInfo& info )
    :uiDialog(p,uiDialog::Setup(tr("2D Horizons management"),
        uiStrings::phrManage( EMHorizon2DTranslatorGroup::sTypeName(mPlural)),
        mODHelpKey(mSurface2DManHelpID) ))
    , eminfo_(info)
{
    setCtrlStyle( CloseOnly );

    uiGroup* topgrp = new uiGroup( this, "Top" );
    uiListBox::Setup su( OD::ChooseOnlyOne, tr("2D lines"),
			 uiListBox::AboveMid );
    linelist_ = new uiListBox( topgrp, su );
    BufferStringSet linenames;
    info.getLineNames( linenames );
    linelist_->addItems( linenames );
    linelist_->selectionChanged.notify( mCB(this,uiSurface2DMan,lineSel) );

    uiGroup* botgrp = new uiGroup( this, "Bottom" );
    infofld_ = new uiTextEdit( botgrp, "File Info", true );
    infofld_->setPrefHeightInChar( 8 );
    infofld_->setPrefWidthInChar( 50 );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", false );
    splitter->addGroup( topgrp );
    splitter->addGroup( botgrp );

    lineSel( 0 );
}


void lineSel( CallBacker* )
{
    const int curitm = linelist_->currentItem();
    TypeSet< StepInterval<int> > trcranges;
    eminfo_.getTrcRanges( trcranges );

    BufferString txt;
    if ( trcranges.validIdx(curitm) )
    {
	StepInterval<int> trcrg = trcranges[ curitm ];
	txt += BufferString( sKey::FirstTrc(), ": " ); txt += trcrg.start;
	txt += "\n";
	txt += BufferString( sKey::LastTrc(), ": " ); txt += trcrg.stop;
	txt += "\n";
	txt += BufferString( "Trace Step: " ); txt += trcrg.step;
    }

    infofld_->setText( txt );
}

    uiListBox*			linelist_;
    uiTextEdit*			infofld_;
    const EM::IOObjInfo&	eminfo_;

};


void uiSurfaceMan::man2dCB( CallBacker* )
{
    EM::IOObjInfo eminfo( curioobj_->key() );
    uiSurface2DMan dlg( this, eminfo );
    dlg.go();
}
