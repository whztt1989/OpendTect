#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
________________________________________________________________________

-*/

#include "gendefs.h"
#include "atomic.h"
#include "refcount.h"
#include <stdlib.h>

#ifdef __debug__
# include "debug.h"
#endif

/*!Convenience function to delete and zero pointer. */

template <class T>
void deleteAndZeroPtr( T*& ptr, bool isowner=true )
{ if ( isowner ) delete ptr; ptr = 0; }


template <class T>
void deleteAndZeroArrPtr( T*& ptr, bool isowner=true )
{ if ( isowner ) delete [] ptr; ptr = 0; }

template <class T> T* createSingleObject()		{ return new T; }
template <class T> T* createObjectArray(od_int64 sz)	{ return new T[sz]; }

/*! Base class for smart pointers. Don't use directly, use PtrMan, ArrPtrMan
    or RefMan instead. */

template<class T>
mClass(Basic) PtrManBase
{
public:
    inline bool		operator !() const	{ return !ptr_; }
    inline T*		set(T* p, bool doerase=true);
			//!<Returns old pointer if not erased
    inline T*		release() { return  set(0,false); }
			//!<Returns pointer. I won't take care of it any longer
    inline void		erase() { set( 0, true ); }

    inline bool		setIfNull(T* p,bool takeover);
			/*!<If takeover==true, pointer will be deleted if
			    object was not set. */

    typedef T*		(*PointerCreator)();
    inline T*		createIfNull(PointerCreator=createSingleObject<T>);
			/*!<If null, PointerCrator will be called to
			    create new object.  */
protected:

    typedef void		(*PtrFunc)(T*);
    inline			PtrManBase(PtrFunc setfunc,PtrFunc deletor,T*);
    virtual			~PtrManBase()		{ set(0,true); }

    Threads::AtomicPointer<T>	ptr_;

    PtrFunc			setfunc_;
    PtrFunc			deletefunc_;
};


template<class T>
mClass(Basic) ConstPtrManBase : public PtrManBase<T>
{
public:
    inline const T*	ptr() const		{ return this->ptr_; }
    inline		operator const T*() const { return this->ptr_; }
    inline const T*	operator->() const	{ return this->ptr_; }
    inline const T&	operator*() const	{ return *this->ptr_; }
    inline T*		getNonConstPtr() const
			{ return const_cast<T*>(this->ptr()); }
protected:
    typedef void	(*PtrFunc)(T*);
    inline		ConstPtrManBase(PtrFunc setfunc,PtrFunc deletor,T* p)
			    : PtrManBase<T>( setfunc, deletor, p )
			{}
};


template<class T>
mClass(Basic) NonConstPtrManBase : public PtrManBase<T>
{
public:
    inline const T*	ptr() const		{ return this->ptr_; }
    inline		operator const T*() const { return this->ptr_; }
    inline const T*	operator->() const	{ return this->ptr_; }
    inline const T&	operator*() const	{ return *this->ptr_; }
    inline T*		ptr()			{ return this->ptr_; }
    inline		operator T*()		{ return this->ptr_; }
    inline T*		operator ->()		{ return this->ptr_; }
    inline T&		operator *()		{ return *this->ptr_; }
    inline T*		getNonConstPtr() const
			{ return const_cast<T*>(this->ptr()); }
protected:
    typedef void	(*PtrFunc)(T*);
    inline		NonConstPtrManBase(PtrFunc setfunc,PtrFunc deletor,T* p)
			    : PtrManBase<T>( setfunc, deletor, p )
			{}
};



/*!Smart pointer for normal pointers. */
template <class T>
mClass(Basic) PtrMan : public NonConstPtrManBase<T>
{
public:
			PtrMan(const PtrMan<T>&);
			//!<Don't use
    inline		PtrMan(T* = 0);
    PtrMan<T>&		operator=( T* p );

    PtrMan<T>&		operator=(const PtrMan<T>&);
			//!<Don't use

private:

    static void		deleteFunc( T* p )    { delete p; }

};


/*!Smart pointer for normal const pointers. */
template <class T>
mClass(Basic) ConstPtrMan : public ConstPtrManBase<T>
{
public:
			ConstPtrMan(const ConstPtrMan<T>&);
			//Don't use
    inline		ConstPtrMan(const T* = 0);
    ConstPtrMan<T>&	operator=(const T* p);
    ConstPtrMan<T>&	operator=(const ConstPtrMan<T>&);
			//!<Don't use
private:

    static void		deleteFunc( T* p )    { delete p; }
};


/*!Smart pointer for pointers allocated as arrays. */
template <class T>
mClass(Basic) ArrPtrMan : public NonConstPtrManBase<T>
{
public:
				ArrPtrMan(const ArrPtrMan<T>&);
				//!<Don't use
    inline			ArrPtrMan(T* = 0);
    ArrPtrMan<T>&		operator=( T* p );
    inline ArrPtrMan<T>&	operator=(const ArrPtrMan<T>& p );
				//!<Don't use

#ifdef __debug__
    T&				operator[](int);
    const T&			operator[](int) const;
    T&				operator[](od_int64);
    const T&			operator[](od_int64) const;

#endif
    void			setSize(od_int64 size) { size_=size; }

private:

    static void		deleteFunc( T* p )    { delete [] p; }

    od_int64		size_;
};


/*!Smart pointer for const pointers allocated as arrays. */
template <class T>
mClass(Basic) ConstArrPtrMan : public ConstPtrManBase<T>
{
public:
			ConstArrPtrMan(const ConstArrPtrMan<T>&);
			//Don't use
    inline		ConstArrPtrMan(const T* = 0);
    ConstArrPtrMan<T>&	operator=(const T* p);
    ConstArrPtrMan<T>&	operator=(const ConstArrPtrMan<T>&);
			//!< Will give linkerror if used
private:

    static void		deleteFunc( T* p )    { delete p; }
};


/*!Smart pointer for reference counted objects. */
template <class T>
mClass(Basic) RefMan : public NonConstPtrManBase<T>
{
public:

    template <class TT> inline	RefMan(const RefMan<TT>&);
    inline			RefMan(const RefMan<T>&);
    inline			RefMan(const WeakPtr<T>&);
    inline			RefMan(T* = 0);
    inline RefMan<T>&		operator=( T* p )
				{ this->set( p, true ); return *this; }
    template <class TT>
    inline RefMan<T>&		operator=(const RefMan<TT>&);
    inline RefMan<T>&		operator=(const RefMan<T>&);
    inline RefMan<T>&		operator=(const WeakPtr<T>&);

    void			setNoDelete(bool yn);

private:

    static void		ref(T* p);
    static void		unRef(T* p);
    static void		unRefNoDelete(T* p);
};


/*!Smart pointer for reference counted objects. */
template <class T>
mClass(Basic) ConstRefMan : public ConstPtrManBase<T>
{
public:
    inline			ConstRefMan(const ConstRefMan<T>&);
    template <class TT> inline	ConstRefMan(const ConstRefMan<TT>&);
    template <class TT> inline	ConstRefMan(const RefMan<TT>&);

    inline			ConstRefMan(const T* = 0);
    ConstRefMan<T>&		operator=(const T* p);
    template <class TT>
    ConstRefMan<T>&		operator=(const RefMan<TT>&);
    template <class TT>
    ConstRefMan<T>&		operator=(const ConstRefMan<TT>&);
    ConstRefMan<T>&		operator=(const ConstRefMan<T>&);


    void			setNoDelete(bool yn);

private:
    static void			ref(T* p);
    static void			unRef(T* p);
    static void			unRefNoDelete(T* p);

};

//Implementations below

template <class T> inline
PtrManBase<T>::PtrManBase( PtrFunc setfunc, PtrFunc deletor, T* p )
    : deletefunc_( deletor )
    , setfunc_( setfunc )
{
    this->set(p);
}


template <class T> inline
T* PtrManBase<T>::set( T* p, bool doerase )
{
    if ( setfunc_ && p )
	setfunc_(p);

    T* oldptr = ptr_.exchange(p);
    if ( doerase )
    {
	deletefunc_( oldptr );
	return 0;
    }

    return oldptr;
}


template <class T> inline
bool PtrManBase<T>::setIfNull( T* p, bool takeover )
{
    if ( ptr_.setIfEqual( 0, p ) )
    {
	if ( setfunc_ && p )
	    setfunc_(p);
	return true;
    }

    if ( takeover && p )
    {
	if ( setfunc_ ) setfunc_(p);
	if ( deletefunc_ ) deletefunc_(p);
    }

    return false;
}


template <class T> inline
T* PtrManBase<T>::createIfNull(PointerCreator creator)
{
    if ( ptr_ )
	return ptr_;

    T* newptr = creator();
    if ( !newptr )
	return 0;

    setIfNull(newptr,true);

    return ptr_;
}


template <class T> inline
PtrMan<T>::PtrMan( const PtrMan<T>& )
    : NonConstPtrManBase<T>( 0, deleteFunc, 0 )
{
    pErrMsg("Should not be called");
}


template <class T> inline
PtrMan<T>& PtrMan<T>::operator=( const PtrMan<T>& )
{
    PtrManBase<T>::set( 0, true );
    pErrMsg("Should not be called");
    return *this;
}


template <class T> inline
PtrMan<T>::PtrMan( T* p )
    : NonConstPtrManBase<T>( 0, deleteFunc, p )
{}


template <class T> inline
PtrMan<T>& PtrMan<T>::operator=( T* p )
{
    this->set( p );
    return *this;
}


template <class T> inline
ConstPtrMan<T>::ConstPtrMan( const ConstPtrMan<T>& )
    : ConstPtrManBase<T>( 0, deleteFunc, 0 )
{
    pErrMsg("Should not be called");
}


template <class T> inline
ConstPtrMan<T>& ConstPtrMan<T>::operator=( const ConstPtrMan<T>& )
{
    PtrManBase<T>::set( 0, true );
    pErrMsg("Should not be called");
    return *this;
}


template <class T> inline
ConstPtrMan<T>::ConstPtrMan( const T* p )
    : ConstPtrManBase<T>( 0, deleteFunc, const_cast<T*>(p) )
{}


template <class T> inline
ConstPtrMan<T>& ConstPtrMan<T>::operator=( const T* p )
{
    this->set( const_cast<T*>( p ) );
    return *this;
}


template <class T> inline
ArrPtrMan<T>::ArrPtrMan( const ArrPtrMan<T>& )
    : NonConstPtrManBase<T>( 0, deleteFunc, 0 )
{
    pErrMsg("Should not be called");
}


template <class T> inline
ArrPtrMan<T>& ArrPtrMan<T>::operator=( const ArrPtrMan<T>& )
{
    PtrManBase<T>::set( 0, true );
    pErrMsg("Should not be called");
    return *this;
}


template <class T> inline
ArrPtrMan<T>::ArrPtrMan( T* p )
    : NonConstPtrManBase<T>( 0, deleteFunc, p )
    , size_(-1)
{}


template <class T> inline
ArrPtrMan<T>& ArrPtrMan<T>::operator=( T* p )
{
    this->set( p );
    return *this;
}

#ifdef __debug__

template <class T> inline
T& ArrPtrMan<T>::operator[]( int idx )
{
    if ( idx<0 || (size_>=0 && idx>=size_) )
    {
	DBG::forceCrash(true);
    }
    return this->ptr_[(size_t) idx];
}


template <class T> inline
const T& ArrPtrMan<T>::operator[]( int idx ) const
{
    if ( idx<0 || (size_>=0 && idx>=size_) )
    {
	DBG::forceCrash(true);
    }
    return this->ptr_[(size_t) idx];
}


template <class T> inline
T& ArrPtrMan<T>::operator[]( od_int64 idx )
{
    if ( idx<0 || (size_>=0 && idx>=size_) )
    {
	DBG::forceCrash(true);
    }
    return this->ptr_[(size_t) idx];
}


template <class T> inline
const T& ArrPtrMan<T>::operator[]( od_int64 idx ) const
{
    if ( idx<0 || (size_>=0 && idx>=size_) )
    {
	DBG::forceCrash(true);
    }
    return this->ptr_[(size_t) idx];
}

#endif



template <class T> inline
ConstArrPtrMan<T>::ConstArrPtrMan( const ConstArrPtrMan<T>& p )
    : ConstPtrManBase<T>( 0, deleteFunc, 0 )
{
    pErrMsg("Shold not be called");
}


template <class T> inline
ConstArrPtrMan<T>::ConstArrPtrMan( const T* p )
    : ConstPtrManBase<T>( 0, deleteFunc, const_cast<T*>(p) )
{}


template <class T> inline
ConstArrPtrMan<T>& ConstArrPtrMan<T>::operator=( const T* p )
{
    this->set( const_cast<T*>(p) );
    return *this;
}


template <class T> inline
RefMan<T>::RefMan( const RefMan<T>& p )
    : NonConstPtrManBase<T>( ref, unRef, const_cast<T*>(p.ptr()) )
{}


template <class T> inline
RefMan<T>::RefMan( const WeakPtr<T>& p )
    : NonConstPtrManBase<T>( ref, unRef, 0 )
{
    *this = p;
}


template <class T>
template <class TT> inline
RefMan<T>::RefMan( const RefMan<TT>& p )
    : NonConstPtrManBase<T>( ref, unRef, (T*) p.ptr() )
{}


template <class T> inline
RefMan<T>::RefMan( T* p )
    : NonConstPtrManBase<T>( ref, unRef, p )
{}


template <class T> inline
RefMan<T>& RefMan<T>::operator=( const RefMan<T>& p )
{
    this->set( const_cast<T*>(p.ptr()) );
    return *this;
}


template <class T> inline
RefMan<T>& RefMan<T>::operator=( const WeakPtr<T>& p )
{
    return RefMan<T>::operator=( p.get() );
}


template <class T>
template <class TT> inline
RefMan<T>& RefMan<T>::operator=( const RefMan<TT>& p )
{
    this->set( (T*) p.ptr() );
    return *this;
}



template <class T> inline
void RefMan<T>::setNoDelete( bool yn )
{
    this->deletefunc_ = yn ? unRefNoDelete : unRef;
}


template <class T> inline
void RefMan<T>::ref(T* p) { refPtr((RefCount::Referenced*) p ); }


template <class T> inline
void RefMan<T>::unRef(T* p) { unRefPtr((RefCount::Referenced*) p); }


template <class T> inline
void RefMan<T>::unRefNoDelete(T* p)
{ unRefNoDeletePtr((RefCount::Referenced*) p ); }



template <class T> inline
ConstRefMan<T>::ConstRefMan( const ConstRefMan<T>& p )
    : ConstPtrManBase<T>( ref, unRef, const_cast<T*>(p.ptr()) )
{}


template <class T> inline
ConstRefMan<T>::ConstRefMan( const T* p )
    : ConstPtrManBase<T>( ref, unRef, const_cast<T*>(p) )
{}


template <class T>
template <class TT> inline
ConstRefMan<T>::ConstRefMan( const ConstRefMan<TT>& p )
    : ConstPtrManBase<T>( ref, unRef, (T*) p.ptr() )
{}


template <class T>
template <class TT> inline
ConstRefMan<T>::ConstRefMan( const RefMan<TT>& p )
    : ConstPtrManBase<T>( ref, unRef, (T*) p.ptr() )
{}


template <class T>
template <class TT> inline
ConstRefMan<T>& ConstRefMan<T>::operator=( const ConstRefMan<TT>& p )
{
    this->set( (T*) p.ptr() );
    return *this;
}


template <class T> inline
ConstRefMan<T>& ConstRefMan<T>::operator=( const ConstRefMan<T>& p )
{
    this->set( (T*) p.ptr() );
    return *this;
}


template <class T> inline
ConstRefMan<T>&	ConstRefMan<T>::operator=(const T* p)
{
    this->set( const_cast<T*>( p ) );
    return *this;
}


template <class T>
template <class TT> inline
ConstRefMan<T>& ConstRefMan<T>::operator=( const RefMan<TT>& p )
{
    this->set( (T*) p.ptr() );
    return *this;
}


template <class T> inline
void ConstRefMan<T>::setNoDelete( bool yn )
{
    this->deletefunc_ = yn ? unRefNoDelete : unRef;
}


template <class T> inline
void ConstRefMan<T>::ref(T* p) { refPtr((RefCount::Referenced*) p ); }


template <class T> inline
void ConstRefMan<T>::unRef(T* p) { unRefPtr((RefCount::Referenced*) p); }


template <class T> inline
void ConstRefMan<T>::unRefNoDelete(T* p)
{ unRefNoDeletePtr((RefCount::Referenced*) p ); }
