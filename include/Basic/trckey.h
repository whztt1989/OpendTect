#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris/Bert/Salil
 Date:		2013
________________________________________________________________________

-*/

#include "basicmod.h"
#include "binid.h"

/*!
\brief Represents a unique trace position in one of the surveys that OpendTect
is managing.

The class is a combination of a unique survey ID and a bin position ID which is
currently implemented using a BinID (2D trace number is the crossline).

*/


mExpClass(Basic) TrcKey
{
public:

    typedef Pos::SurvID		SurvID;
    typedef IdxPair::IdxType	IdxType;

			TrcKey()			{ *this = udf(); }

			//3D
			TrcKey(const BinID&); // default 3D surv ID
			TrcKey(SurvID,const BinID&);

			//2D
			TrcKey(Pos::GeomID,Pos::TraceID);

			// Synthetic
    static TrcKey	getSynth(Pos::TraceID);

    bool		is2D() const { return is2D(survid_); }
    static bool		is2D(SurvID);

    Pos::GeomID geomID() const;
    static Pos::GeomID geomID(SurvID,const BinID&);
    TrcKey&		setGeomID(Pos::GeomID);

    bool		operator==(const TrcKey&) const;
    bool		operator!=(const TrcKey& oth) const
			{ return !(*this == oth); }

    inline bool		isUdf() const			{ return *this==udf(); }
    static const TrcKey& udf();
    static SurvID	std2DSurvID();
    static SurvID	std3DSurvID();
    static SurvID	stdSynthSurvID();
    static SurvID	cUndefSurvID();

    float		distTo(const TrcKey&) const;
    SurvID		survID() const			{ return survid_; }
    inline TrcKey&	setSurvID( SurvID id )
			{ survid_ = id; return *this; }

    const BinID&	position() const		{ return pos_; }
    IdxType		lineNr() const			{ return pos_.row(); }
    IdxType		trcNr() const			{ return pos_.col(); }
    const BinID&	binID() const			{ return position(); }
    IdxType		inl() const			{ return lineNr(); }
    IdxType		crl() const			{ return trcNr(); }
    inline TrcKey&	setPosition( const BinID& bid )
			{ pos_ = bid; return *this; }
    inline TrcKey&	setLineNr( IdxType nr )
			{ pos_.row() = nr; return *this; }
    inline TrcKey&	setTrcNr( IdxType nr )
			{ pos_.col() = nr; return *this; }
    inline TrcKey&	setBinID( const BinID& bid )
			{ return setPosition(bid); }
    inline TrcKey&	setInl( IdxType nr )
			{ return setLineNr(nr); }
    inline TrcKey&	setCrl( IdxType nr )
			{ return setTrcNr(nr); }

    TrcKey&		setFrom(const Coord&);  //!< Uses survID
    Coord		getCoord() const;	//!< Uses survID

private:

    SurvID		survid_;
    BinID		pos_;

};
