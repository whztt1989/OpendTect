/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2016
________________________________________________________________________

-*/


#include "refcount.h"

using namespace RefCount;

#define mInvalidRefCount (-1)
#define mStartRefCount (-2)


Referenced::~Referenced()
{
    const od_int32 count = refcount_.count();
    if ( count!=mStartRefCount && refcount_.count()!=mInvalidRefCount )
    {
	pErrMsg("It seems I'm deleted without an unref on an object "
		"that has been referenced." );
    }
}


void Referenced::ref() const
{
    refcount_.ref();
    refNotify();
}


bool Referenced::refIfReffed() const
{
    if ( !refcount_.refIfReffed() )
	return false;

    refNotify();
    return true;
}


void Referenced::unRef() const
{
    unRefNotify();
    if ( refcount_.unRef() )
    {
	refcount_.clearAllObservers();
	const_cast<Referenced*>(this)->prepareForDelete();
	delete this;
    }

    return;
}


void Referenced::unRefNoDelete() const
{
    unRefNoDeleteNotify();
    refcount_.unRefDontInvalidate();
}


int Referenced::nrRefs() const { return refcount_.count(); }


bool Referenced::tryRef() const
{
    if ( refcount_.tryRef() ) { refNotify(); return true; }
    return false;
}


void Referenced::addObserver(WeakPtrBase* obs)
{
    refcount_.addObserver( obs );
}


void Referenced::removeObserver(WeakPtrBase* obs)
{
    refcount_.removeObserver( obs );
}

#ifdef __win__
# define mDeclareCounters	od_int32 oldcount = count_.get(), newcount = 0
#else
# define mDeclareCounters	od_int32 oldcount = count_.get(), newcount;
#endif



Counter::Counter()
    : count_( mStartRefCount )
{}


Counter::Counter(const Counter& a)
    : count_( mStartRefCount )
{}


od_int32 Counter::cInvalidRefCount()
{ return mInvalidRefCount; }


od_int32 Counter::cStartRefCount()
{ return mStartRefCount; }


void Counter::ref()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==mInvalidRefCount )
	{
	    pErrMsg("Invalid ref");
#ifdef __debug__
	    DBG::forceCrash(false);
	    newcount = 0; //To fool unitialized code warning
#else
	    newcount = 1; //Hoping for the best
#endif
	}
	else if ( oldcount==mStartRefCount )
	{
	    newcount = 1;
	}
	else
	{
	    newcount = oldcount+1;
	}

    } while ( !count_.setIfValueIs( oldcount, newcount, &oldcount ) );
}


bool Counter::tryRef()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==mInvalidRefCount )
	{
	    return false;
	}
	else if ( oldcount==mStartRefCount )
	{
	    newcount = 1;
	}
	else
	{
	    newcount = oldcount+1;
	}

    } while ( !count_.setIfValueIs( oldcount, newcount, &oldcount ) );

    return true;
}


bool Counter::unRef()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==mInvalidRefCount )
	{
	    pErrMsg("Invalid reference.");
#ifdef __debug__
	    DBG::forceCrash(false);
	    newcount = 0; //To fool unitialized code warning
#else
	    return false;
#endif
	}
	else if ( oldcount==1 )
	    newcount = mInvalidRefCount;
	else
	    newcount = oldcount-1;

    } while ( !count_.setIfValueIs(oldcount,newcount, &oldcount ) );

    return newcount==mInvalidRefCount;
}


bool Counter::refIfReffed()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==mInvalidRefCount )
	{
	    pErrMsg("Invalid ref");
#ifdef __debug__
	    DBG::forceCrash(false);
#else
	    return false; //Hoping for the best
#endif
	}
	else if ( oldcount==mStartRefCount || !oldcount)
	    //It can be zero only if unRefDontInvalidate has been called
	{
	    return false;
	}

	newcount = oldcount+1;

    } while ( !count_.setIfValueIs( oldcount, newcount, &oldcount ) );

    return true;
}


void Counter::unRefDontInvalidate()
{
    mDeclareCounters;

    do
    {
	if ( oldcount==mInvalidRefCount )
	{
	    pErrMsg("Invalid reference.");
#ifdef __debug__
	    DBG::forceCrash(false);
	    newcount = 0; //Fool the unitialized warning
#else
	    newcount = 0; //Hope for the best
#endif
	}
	else
	    newcount = oldcount-1;

    } while ( !count_.setIfValueIs( oldcount, newcount, &oldcount ) );
}

#undef mDeclareCounters


void Counter::clearAllObservers()
{
    observerslock_.lock();

    mObjectSetApplyToAll( observers_, observers_[idx]->clearPtr() );
    observers_.erase();

    observerslock_.unLock();
}


void Counter::addObserver( WeakPtrBase* obj )
{
    observerslock_.lock();
    observers_.addIfNew( obj );
    observerslock_.unLock();
}


void Counter::removeObserver( WeakPtrBase* obj )
{
    observerslock_.lock();
    observers_ -= obj;
    observerslock_.unLock();
}


WeakPtrBase::WeakPtrBase()
    : ptr_( 0 )
{}

void WeakPtrBase::clearPtr()
{
    lock_.lock();
    ptr_ = 0;
    lock_.unLock();
}


WeakPtrBase::operator bool() const
{
    lock_.lock();
    bool res = ptr_;
    lock_.unLock();
    return res;
}


bool WeakPtrBase::operator!() const
{
    lock_.lock();
    bool res = ptr_;
    lock_.unLock();
    return !res;
}


void WeakPtrBase::set( Referenced* p )
{
    lock_.lock();

    while ( ptr_ )
    {
	if ( !ptr_->tryRef() )
	{
	    //Object is closing down
	    lock_.unLock();

	    //Sleep to let the closing refcount call clear();
	    Threads::sleep( 0.001 );

	    lock_.lock();

	    continue;
	}

#ifdef __debug__
	if ( ptr_->nrRefs()==1 )
	{
	    pErrMsg("Observing object that is not reffed. "
		    "This will always fail");
	}
#endif

	Referenced* tmpptr = ptr_;

	tmpptr->removeObserver(this);
	ptr_ = 0;
	lock_.unLock();
	tmpptr->unRef(); //We may not be locked during unref, for safety
	lock_.lock();
    }

    if ( p && p->tryRef() )
    {
	p->addObserver(this);
	ptr_ = p;

	//We may not be locked during unref as clear may be called
	lock_.unLock();
	p->unRef();
    }
    else
	lock_.unLock();
}


void unRefPtr( const RefCount::Referenced* ptr )
{
    if ( !ptr ) return;
    ptr->unRef();
}


void unRefNoDeletePtr( const RefCount::Referenced* ptr )
{
    if ( !ptr ) return;
    ptr->unRefNoDelete();
}


const RefCount::Referenced* refPtr( const RefCount::Referenced* ptr )
{
    if ( ptr )
	ptr->ref();

    return ptr;
}


