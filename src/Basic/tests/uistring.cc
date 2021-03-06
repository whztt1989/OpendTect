/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2014
 * FUNCTION :
-*/


#include "uistring.h"
#include "testprog.h"
#include "uistrings.h"
#include "texttranslator.h"
#include "filepath.h"
#include "oddirs.h"

#include <QString>
#include <QByteArray>
#include <QTranslator>


bool testSetEmpty()
{
    uiString str = toUiString("Hello");
    str.setEmpty();
    mRunStandardTest( str.isEmpty(), "isEmpty" );

    str.setFrom( QString("Hello" ) );
    str.setEmpty();
    mRunStandardTest( str.isEmpty(), "isEmpty after setFrom(QString)" );

    return true;
}

class TestTranslator
{ mTextTranslationClass( TestTranslator, "test_uistring" );
public:
    static bool testTranslation();

};


bool TestTranslator::testTranslation()
{
    uiString a = tr("I am an A");
    uiString b = tr("I am a B" );
    uiString join = uiStrings::phrJoinStrings( a, b );

    QTranslator trans;
    File::Path path;
    TextTranslateMgr::GetLocalizationDir( path );
    path.add( "uistring.qm" );
    mRunStandardTest( trans.load( QString(path.fullPath().buf())),
		    "Load test translation");

    QString qres;
    mRunStandardTest( join.translate( trans, qres ),
		      "Run translation" );

    BufferString res( qres );
    mRunStandardTest( res=="A B", "Translation content");

    uiString hor3d =
	uiStrings::phrJoinStrings(uiStrings::s3D(), uiStrings::sHorizon() );

    qres = hor3d.getQString();
    res = qres;
    mRunStandardTest( res=="3D Horizon", "Translation content (Horizon)");

    hor3d =
     uiStrings::phrJoinStrings(uiStrings::s3D(), uiStrings::sHorizon(mPlural) );

    qres = hor3d.getQString();
    res = qres;
    mRunStandardTest( res=="3D Horizons", "Translation content (Horizons)");

    return true;
}



bool testArg()
{
    uiString composite = toUiString( "%1 plus %2 is %3")
		.arg( 4 )
		.arg( 5 )
		.arg( 9 );

    mRunStandardTest( composite.getFullString()=="4 plus 5 is 9",
		      "Composite test" );

    const char* desoutput = "Hello Dear 1";

    uiString string = toUiString( "Hello %1 %2").arg( "Dear" ).arg(toString(1));
    mRunStandardTest( string.getQString()==QString( desoutput ),
		     "Standard argument order");

    string = toUiString( "Hello %2 %1").arg( toString( 1 ) ).arg( "Dear" );
    mRunStandardTest( string.getQString()==QString(desoutput),
		     "Reversed argument order");

    string = toUiString( "Hello %1 %2");
    string.arg( "Dear" ).arg( toString(1) );
    mRunStandardTest( string.getQString()==QString(desoutput),
		     "In-place");


    BufferString expargs = string.getFullString();

    mRunStandardTest( expargs==desoutput, "Argument expansion" );

    uiString cloned;
    cloned = string;
    cloned.makeIndependent();

    mRunStandardTest( string.getQString()==cloned.getQString(), "copyFrom" );

    uiString part1 = toUiString( "Part 1" );
    part1.append( ", Part 2", false );
    mRunStandardTest(
	    FixedString(part1.getFullString())=="Part 1, Part 2", "append" );
    part1.append( ", Part 2", true );
    mRunStandardTest(
	    FixedString(part1.getFullString())=="Part 1, Part 2\n, Part 2",
			"append with newline" );

    return true;
}


bool testUTF8()
{
    //Convert some chinese from base 64 to qstring. Then get the utf8 out.
    //Expected values comes from an online conversion tool.
    const QString input = QByteArray::fromBase64( 
					    QByteArray("5omL5py66Zi/6YeM") );
    uiString uistring;
    uistring.setFrom( input );
    BufferString utf8;
    uistring.fillUTF8String( utf8 );
    const unsigned char expected[] =  { 0xE6, 0x89, 0x8B, 0xE6, 0x9C, 0xBA,
			      0xE9, 0x98, 0xBF, 0xE9, 0x87, 0x8C };
#ifndef __win__
    mRunStandardTest( !strcmp( (const char*)expected, utf8.buf() ), 
							    "UTF conversion" );
#endif
    return true;
}



bool testSharedData()
{
    uiString a = toUiString("Hello %1%2").arg( "World" );
    uiString b = a;

    b.arg( "s" );
    mRunStandardTest( b.getFullString()=="Hello Worlds" &&
		      BufferString(a.getFullString())!=
		      BufferString(b.getFullString()), "arg on copy" );

    uiString c = b;
    c = toUiString("Another message");
    mRunStandardTest( BufferString(c.getFullString())!=
		      BufferString(b.getFullString()),
		      "assignment of copy" );

    uiString d = b;
    mRunStandardTest( d.getOriginalString()==b.getOriginalString(),
		      "Use of same buffer on normal operations" );

    return true;
}


bool testIsEqual()
{
    const uiString a = toUiString( "A" );
    const uiString a2 = toUiString( "A" );
    const uiString b = toUiString( "B" );

    mRunStandardTest( a.isEqualTo(a2),
	    "Is equal is true for equal strings, test 1" );
    mRunStandardTest( !a.isEqualTo(b),
	    "Is equal is false for different strings, test 1" );

    mRunStandardTest( a2.isEqualTo(a),
	    "Is equal is true for equal strings, test 2" );
    mRunStandardTest( !b.isEqualTo(a),
	    "Is equal is false for different strings, test 2" );

    return true;
}


bool testQStringAssignment()
{
    const char* message = "Hello World";
    uiString string;
    string.setFrom( QString( message ) );

    BufferString res = string.getFullString();
    mRunStandardTest( res==message, "QString assignment" );

    return true;
}


bool testNumberStrings()
{
    uiString string = toUiString( 0.9, 3 );
    QString qstr = string.getQString();
    BufferString bstr( qstr );
    mRunStandardTest( bstr=="0.900", "Number string" );

    ArrPtrMan<wchar_t> wbuf = string.createWCharString();
    qstr = QString::fromWCharArray( wbuf );
    bstr = BufferString( qstr );
    mRunStandardTest( bstr=="0.900", "Number string from wchar" );
    
    return true;
}


bool testLargeNumberStrings()
{
    uiString string = toUiString( 12500 );
    QString qstr = string.getQString();
    BufferString bstr( qstr );
    mRunStandardTest( bstr=="12500", "Large number string" );

    ArrPtrMan<wchar_t> wbuf = string.createWCharString();
    qstr = QString::fromWCharArray( wbuf );
    bstr = BufferString( qstr );
    mRunStandardTest( bstr=="12500", "Large number string from wchar" );
    
    return true;
}


bool testToLower()
{
    uiString string = uiStrings::phrJoinStrings( uiStrings::sInput(),
					    uiStrings::sHorizon().toLower() );
    BufferString bstr = string.getFullString();
    mRunStandardTest( bstr=="Input horizon", "To lower" );

    uiString string2 = uiStrings::phrJoinStrings( uiStrings::sInput(),
						 uiStrings::sHorizon() );
    bstr = string2.getFullString();
    mRunStandardTest( bstr=="Input Horizon", "To lower does not affect orig" );


    return true;
}


bool testOptionStrings()
{
    uiString strings[] = { toUiString( "One" ), toUiString( "Two" ),
			    toUiString( "Three" ), toUiString( "Four" ),
   			    uiString() };
    uiStringSet options( strings );

    mRunStandardTest(
	    options.createOptionString( true, -1, ' ').getFullString()==
	              "One, Two, Three, and Four", "createOptionString and" );
    mRunStandardTest(
	    options.createOptionString( false, -1, ' ').getFullString()==
	              "One, Two, Three, or Four", "createOptionString or" );

    mRunStandardTest(
	    options.createOptionString( false, 3, ' ').getFullString()==
	              "One, Two, Three, ...", "createOptionString limited" );

    return true;
}


bool testHexEncoding()
{
    uiString str;
    mRunStandardTest( str.setFromHexEncoded("517420697320677265617421") &&
	              str.getFullString()=="Qt is great!",
	   	      "Reading hard-coded string" );


    BufferString msg( "Espana" );
    ((unsigned char*) msg.buf() )[4] = 241;

    uiString original( toUiString(msg) );
    BufferString encoding;
    original.getHexEncoded( encoding );
    
    mRunStandardTest( str.setFromHexEncoded( encoding ) &&
		      original.getQString()==str.getQString(),
		      "Reading encoded string" );

    return true;
}


int testMain( int argc, char** argv )
{
    mInitTestProg();

    if ( !testArg() || !testSharedData() || !testQStringAssignment() ||
	 !testOptionStrings() || !testHexEncoding() || !testIsEqual() ||
	 !testSetEmpty() || !testNumberStrings() || !testLargeNumberStrings() ||
	 !testToLower() || !TestTranslator::testTranslation() || !testUTF8() )
	return 1;

    return 0;
}
