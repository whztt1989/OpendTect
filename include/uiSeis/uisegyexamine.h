#ifndef uisegyexamine_h
#define uisegyexamine_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:		$Id: uisegyexamine.h,v 1.3 2008-09-16 08:24:44 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "segyfiledef.h"
class Timer;
class SeisTrc;
class uiTable;
class uiTextEdit;
class SeisTrcBuf;
class SeisTrcReader;
class SEGYSeisTrcTranslator;


class uiSEGYExamine : public uiDialog
{
public:

    struct Setup : public uiDialog::Setup
    {
				Setup(int nrtraces=100);

	mDefSetupMemb(int,nrtrcs)
	mDefSetupMemb(SEGY::FileSpec,fs)
	mDefSetupMemb(SEGY::FilePars,fp)

	void			usePar(const IOPar&);
	static const char*	sKeyNrTrcs;
    };

			uiSEGYExamine(uiParent*,const Setup&);
			~uiSEGYExamine();

    int			getRev() const; // -1 = err, 1 = Rev 1

protected:

    Setup		setup_;
    SeisTrcReader*	rdr_;
    BufferString	txtinfo_;
    BufferString	fname_;
    SeisTrcBuf&		tbuf_;
    Timer&		timer_;

    uiTextEdit*		txtfld_;
    uiTable*		tbl_;

    void		onStartUp(CallBacker*);
    void		dispSeis(CallBacker*);
    void		updateInput(CallBacker*);
    void		vwrClose(CallBacker*);

    void		openInput();
    void		updateInp();
    void		handleFirstTrace(const SeisTrc&,
	    				 const SEGYSeisTrcTranslator&);
    bool		rejectOK();

    void		outInfo(const char*);

};


#endif
