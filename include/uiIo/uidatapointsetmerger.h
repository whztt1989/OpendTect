#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2011
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "bufstringset.h"
#include "ctxtioobj.h"
#include "datapointset.h"
#include "executor.h"

#include "uidialog.h"

class uiComboBox;
class uiGenInput;
class uiIOObjSel;
class uiTable;

/*! \brief
CrossPlot manager
*/

mExpClass(uiIo) DPSMergerProp
{ mODTextTranslationClass(DPSMergerProp);
public:

    typedef DataPack::ID	PackID;

				DPSMergerProp( const DBKey& id, PackID mid,
					       PackID sid )
				    : masterdpsid_(mid), slavedpsid_(sid)
				    , newdpsid_(id), maxz_(mUdf(float))
				    , maxhordist_(mUdf(float))
				    , dooverwriteundef_(false)	{}

   void				setColid(int masterid,int slaveid);

   enum	MatchPolicy		{ Exact, Nearest, NoMatch };
   void				setMatchPolicy( MatchPolicy pol )
				{ matchpol_ = pol; }
   MatchPolicy			matchPolicy() const	{ return matchpol_; }

   enum	ReplacePolicy		{ No, Yes, Average };
   void				setReplacePolicy( ReplacePolicy pol )
				{ replacepol_ = pol; }

   ReplacePolicy		replacePolicy() const	{ return replacepol_; }

   PackID			masterDPID() const	{ return masterdpsid_;}
   PackID			slaveDPID() const	{ return slavedpsid_; }
   DBKey			newDPSID() const	{ return newdpsid_; }
   const TypeSet<int>&		masterColIDs() const	{return mastercolids_;}
   const TypeSet<int>&		slaveColIDs() const	{ return slavecolids_;}

   float			maxAllowedHorDist() const
				{ return maxhordist_; }
   void				setMaxAllowedHorDist( float maxdist )
				{ maxhordist_ = maxdist; }

   float			maxAllowedZDist() const	{ return maxz_; }
   void				setMaxAllowedZDist( float maxz )
				{ maxz_ = maxz; }

   bool				overWriteUndef() const
				{ return dooverwriteundef_; }
   void				setOverWriteUndef( bool yn )
				{ dooverwriteundef_ = yn; }
protected:

   MatchPolicy			matchpol_;
   ReplacePolicy		replacepol_;

   bool				dooverwriteundef_;
   PackID			masterdpsid_;
   PackID			slavedpsid_;
   TypeSet<int>			mastercolids_;
   TypeSet<int>			slavecolids_;
   DBKey			newdpsid_;
   float			maxhordist_;
   float			maxz_;
};


mExpClass(uiIo) DPSMerger : public Executor
{ mODTextTranslationClass(DPSMerger);
public:
				DPSMerger(const DPSMergerProp&);

    void			addNewCols(const BufferStringSet&);
    od_int64			nrDone() const		{ return rowdone_; }
    od_int64			totalNr() const		{return sdps_->size();}
    uiString			uiNrDoneText() const
				{return uiStrings::phrJoinStrings(
				uiStrings::sPosition(mPlural),tr("processed"));}
    RefMan<DataPointSet>	getNewDPS()		{ return newdps_; }
protected:
    DPSMergerProp		prop_;
    RefMan<DataPointSet>	mdps_;
    RefMan<DataPointSet>	sdps_;
    RefMan<DataPointSet>	newdps_;
    int			rowdone_;

    int			nextStep();

    int			getSlaveColID(int mcolid);
    DataPointSet::DataRow	getDataRow(int,int);
    DataPointSet::DataRow	getNewDataRow(int);
    int				findMatchingMrowID(int);
};


mExpClass(uiIo) uiDataPointSetMerger : public uiDialog
{ mODTextTranslationClass(uiDataPointSetMerger);
public:
				uiDataPointSetMerger(uiParent*,DataPointSet*,
						     DataPointSet*);
				~uiDataPointSetMerger();
protected:

    RefMan<DataPointSet>	mdps_;
    RefMan<DataPointSet>	sdps_;
    CtxtIOObj			ctio_;

    uiTable*			tbl_;
    uiComboBox*			matchpolfld_;
    uiComboBox*			replacepolfld_;
    uiGenInput*			overwritefld_;
    uiGenInput*			addcoloptfld_;
    uiGenInput*			distfld_;
    uiGenInput*			zgatefld_;
    uiIOObjSel*			outfld_;

    void			setTable();
    BufferStringSet		checkForNewColumns() const;
    void			checkForSameColNms(BufferStringSet&) const;
    bool			acceptOK();
    void			attribChangedCB(CallBacker*);
    void			matchPolChangedCB(CallBacker*);
};
