/*
	MIT License

	Copyright (c) 2016 Timothy Volpe

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#include <iostream>
#include <boost\endian\conversion.hpp>
#include "nbt.h"

CTag* CNBTReader::createTag( boost::int8_t tagId )
{
	switch( tagId )
	{
	case TAGID_END:
		return new CTagEnd();
	case TAGID_BYTE:
		return new CTagByte();
	case TAGID_SHORT:
		return new CTagShort();
	case TAGID_INT:
		return new CTagInt();
	case TAGID_LONG:
		return new CTagLong();
	case TAGID_FLOAT:
		return new CTagFloat();
	case TAGID_DOUBLE:
		return new CTagDouble();
	case TAGID_STRING:
		return new CTagString();
	case TAGID_LIST:
		return new CTagList();
	case TAGID_COMPOUND:
		return new CTagCompound();
	default:
		std::cout << "Failed: Unknown tag id " << (int)tagId << std::endl;
		return 0;
	}
}

CNBTReader::CNBTReader()
{
}
CNBTReader::~CNBTReader()
{
	// Delete all the tags
	for( TagList::iterator it = m_tags.begin(); it != m_tags.end(); it++ )
	{
		if( (*it) ) {
			delete (*it);
			(*it) = 0;
		}
	}
	m_tags.clear();
}

CTag* CNBTReader::readTag( InputStream &stream, size_t *pBytesRead, bool fullTag = true )
{
	boost::int8_t tagId;
	CTag *pTag;

	// Read the tag ID
	if( fullTag ) {
		stream.read( reinterpret_cast<char*>(&tagId), sizeof( tagId ) );
		(*pBytesRead) += sizeof( tagId );
	}
	else {
		_ASSERT_EXPR( !m_parentStack.empty(), "can't create non-full tag when parent stack is empty" );
		_ASSERT_EXPR( m_parentStack.top()->getId() == TAGID_LIST, "list not at top of parent stack" );
		tagId = reinterpret_cast<CTagList*>(m_parentStack.top())->getChildrenId();
	}

	if( stream.eof() )
		return 0;

	// Create the tag
	pTag = this->createTag( tagId );
	if( !pTag )
		return 0;

	// Check if its an end tag
	if( pTag->getId() == TAGID_END ) {
		if( m_parentStack.empty() ) {
			std::cout << "Failed: mismatched end tag" << std::endl;
			delete pTag;
			return 0;
		}
		m_parentStack.pop();
	}
	// Check if its a parent
	if( pTag->isParent() )
		m_parentStack.push( reinterpret_cast<CTagParent*>( pTag ) );

	// Read the tag
	if( !pTag->read( stream, pBytesRead, fullTag ) ) {
		delete pTag;
		return 0;
	}

	// Check if the list is full
	if( !m_parentStack.empty() ) {
		if( m_parentStack.top()->getId() == TAGID_LIST ) {
			if( m_parentStack.top()->getChildren().size() == reinterpret_cast<CTagList*>(m_parentStack.top())->getChildrenCount() )
				m_parentStack.pop(); // pop off the list
		}
	}

	/*if( pTag->isParent() )
		for( unsigned int i = 0; i < m_parentStack.size()-1; i++ )
			std::cout << "\t";
	else
		for( unsigned int i = 0; i < m_parentStack.size(); i++ )
			std::cout << "\t";
	std::cout << (int)pTag->getId() << "(" << pTag->getName().c_str() << ")" << std::endl;*/

	return pTag;
}

bool CNBTReader::read( boost::filesystem::path fullPath )
{
	boost::filesystem::ifstream inputStream;

	std::cout << "Reading level.dat" << std::endl;

	// Open a stream and decompress
	try
	{
		// Make sure it exists and is valid
		if( !boost::filesystem::exists( fullPath ) ) {
			std::cout << "Failed: could not open level.dat because it does not exist" << std::endl;
			return false;
		}
		else if( !boost::filesystem::is_regular_file( fullPath ) ) {
			std::cout << "Failed: could not open level.dat because it is invalid" << std::endl;
			return false;
		}
		// Open the stream
		inputStream.open( fullPath, std::ios::in | std::ios::binary );
		if( !inputStream ) {
			std::cout << "Failed: could not open level.dat" << std::endl;
			return false;
		}
	}
	catch( const boost::filesystem::filesystem_error &e ) {
		std::cout << "Failed: could not open level.dat (" << e.what() << ")" << std::endl;
		return false;
	}

	// Read
	if( !this->read( inputStream, 0, 0 ) )
		return false;

	_ASSERT_EXPR( inputStream.is_open(), "inputStream was closed prematurely" );
	inputStream.close();

	return true;
}
bool CNBTReader::read( boost::filesystem::ifstream &stream, size_t start, size_t length )
{
	InputStream decompStream;
	size_t bytesRead;
	CTag *pTag;

	// Setup decompression filter
	decompStream.push( boost::iostreams::gzip_decompressor() );
	decompStream.push( stream );

	// If length is 0, read the whole file
	if( length == 0 ) {
		length = (unsigned int)-1;
	}
	// Go to start
	stream.seekg( start );

	// Read a tag, if it is a parent tag, add it to the parent stack and start reading children, so on and so forth
	// When an END tag is hit, pop off the top of the parent stack

	// Read the tags and assign parents, etc.
	bytesRead = 0;
	while( !decompStream.eof() && start < length )
	{
		// If it a list is at the top of the parent stack,
		// the next tags will not be fully formed (no name, id)
		if( !m_parentStack.empty() ) {
			if( m_parentStack.top()->getId() == TAGID_LIST )
				pTag = this->readTag( decompStream, &bytesRead, false );
			else // Read normally
				pTag = this->readTag( decompStream, &bytesRead, true );
		}
		else // Read normally
			pTag = this->readTag( decompStream, &bytesRead, true );
		if( !pTag && !decompStream.eof() ) // if we didn't get a tag and we're not EOF
			return false;
		else if( !pTag && decompStream.eof() )
			break;

		// Add it to the unsorted tag list
		m_tags.push_back( pTag );
		// Assign it to it's parent
		if( !m_parentStack.empty() )
			m_parentStack.top()->addChild( pTag );
	}

	// Close
	boost::iostreams::close( decompStream );

	return true;
}

//////////
// CTag //
//////////

CTag::CTag() {
	m_tagId = 0;
	m_tagName = "";
}
CTag::~CTag() {
}

void CTag::readName( InputStream &stream, size_t *pBytesRead )
{
	boost::int16_t nameLength;
	char *pName;

	// Read a TAG_String payload for the name
	CTagString::ReadPayload( stream, pBytesRead, &nameLength, &pName );
	if( nameLength < 0 || !pName )
		m_tagName = "";
	else {
		m_tagName = std::string( pName );
		delete pName;
		pName = 0;
	}
}

boost::int8_t CTag::getId() const {
	return m_tagId;
}
std::string CTag::getName() const {
	return m_tagName;
}
bool CTag::isParent() const {
	return false;
}

////////////////
// CTagParent //
////////////////

CTagParent::CTagParent() {
}
CTagParent::~CTagParent() {
	m_children.clear();
}

void CTagParent::addChild( CTag *pTag ) {
	m_children.push_back( pTag );
}

TagList CTagParent::getChildren() const {
	return m_children;
}
bool CTagParent::isParent() const {
	return true;
}

/////////////
// CTagEnd //
/////////////

CTagEnd::CTagEnd() {
	m_tagId = TAGID_END;
	m_tagName = "#END-TAG";
}
CTagEnd::~CTagEnd() {
}

bool CTagEnd::read( InputStream &stream, size_t *pBytesRead, bool fullTag ) {
	return true;
}

//////////////
// CTagByte //
//////////////

void CTagByte::ReadPayload( InputStream &stream, size_t *pBytesRead, boost::int8_t *pByte )
{
	// Read a 8-bit byte
	stream.read( reinterpret_cast<char*>(pByte), sizeof( boost::int8_t ) );
	(*pBytesRead) += sizeof( boost::int8_t );
}

CTagByte::CTagByte() {
	m_tagId = TAGID_BYTE;
	m_payload = 0;
}
CTagByte::~CTagByte() {
}

bool CTagByte::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	if( fullTag )
		this->readName( stream, pBytesRead );
	CTagByte::ReadPayload( stream, pBytesRead, &m_payload );

	return true;
}

///////////////
// CTagShort //
///////////////

void CTagShort::ReadPayload( InputStream &stream, size_t *pBytesRead, boost::int16_t *pShort )
{
	// Read a 16-bit short
	stream.read( reinterpret_cast<char*>(pShort), sizeof( boost::int16_t ) );
	(*pBytesRead) += sizeof( boost::int16_t );
	*pShort = boost::endian::big_to_native( *pShort );
}

CTagShort::CTagShort() {
	m_tagId = TAGID_SHORT;
	m_payload = 0;
}
CTagShort::~CTagShort() {
}

bool CTagShort::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	// Read the name
	if( fullTag )
		this->readName( stream, pBytesRead );
	// Read payload
	CTagShort::ReadPayload( stream, pBytesRead, &m_payload );

	return true;
}

/////////////
// CTagInt //
/////////////

void CTagInt::ReadPayload( InputStream &stream, size_t *pBytesRead, boost::int32_t *pInt )
{
	// Read a 32-bit int
	stream.read( reinterpret_cast<char*>(pInt), sizeof( boost::int32_t ) );
	(*pBytesRead) += sizeof( boost::int32_t );
	*pInt = boost::endian::big_to_native( *pInt );
}

CTagInt::CTagInt() {
	m_tagId = TAGID_INT;
	m_payload = 0;
}
CTagInt::~CTagInt() {
}

bool CTagInt::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	if( fullTag )
		this->readName( stream, pBytesRead );
	CTagInt::ReadPayload( stream, pBytesRead, &m_payload );

	return true;
}

//////////////
// CTagLong //
//////////////

void CTagLong::ReadPayload( InputStream &stream, size_t *pBytesRead, boost::int64_t *pLong )
{
	// Read a 64-bit long
	stream.read( reinterpret_cast<char*>(pLong), sizeof( boost::int64_t ) );
	(*pBytesRead) += sizeof( boost::int64_t );
	*pLong = boost::endian::big_to_native( *pLong );
}

CTagLong::CTagLong() {
	m_tagId = TAGID_LONG;
	m_payload = 0;
}
CTagLong::~CTagLong() {
}

bool CTagLong::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	// Read the name
	if( fullTag )
		this->readName( stream, pBytesRead );
	// Read payload
	CTagLong::ReadPayload( stream, pBytesRead, &m_payload );

	return true;
}

///////////////
// CTagFloat //
///////////////

void CTagFloat::ReadPayload( InputStream &stream, size_t *pBytesRead, float *pFloat )
{
	boost::int32_t payload;

	// Read a 64-bit double
	stream.read( reinterpret_cast<char*>(&payload), sizeof( boost::int32_t ) );
	(*pBytesRead) += sizeof( boost::int32_t );
	payload = boost::endian::big_to_native( payload );
	(*pFloat) = (float)payload;
}

CTagFloat::CTagFloat() {
	m_tagId = TAGID_FLOAT;
	m_payload = 0.0f;
}
CTagFloat::~CTagFloat() {
}

bool CTagFloat::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	if( fullTag )
		this->readName( stream, pBytesRead );
	CTagFloat::ReadPayload( stream, pBytesRead, &m_payload );
	return true;
}

////////////////
// CTagDouble //
////////////////

void CTagDouble::ReadPayload( InputStream &stream, size_t *pBytesRead, double *pDouble )
{
	boost::int64_t payload;

	// Read a 64-bit double
	stream.read( reinterpret_cast<char*>(&payload), sizeof( boost::int64_t ) );
	(*pBytesRead) += sizeof( boost::int64_t );
	payload = boost::endian::big_to_native( payload );
	(*pDouble) = (double)payload;
}

CTagDouble::CTagDouble() {
	m_tagId = TAGID_DOUBLE;
	m_payload = 0.0;
}
CTagDouble::~CTagDouble() {
}

bool CTagDouble::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	if( fullTag )
		this->readName( stream, pBytesRead );
	CTagDouble::ReadPayload( stream, pBytesRead, &m_payload );
	return true;
}

////////////////
// CTagString //
////////////////

void CTagString::ReadPayload( InputStream &stream, size_t *pBytesRead, boost::int16_t *pStringLength, char **pString )
{
	// Read the string length (TAG_Short)
	CTagShort::ReadPayload( stream, pBytesRead, pStringLength );
	// Read the name
	if( (*pStringLength) > 0 )
	{
		(*pString) = new char[(*pStringLength)+1];
		stream.read( (*pString), (*pStringLength) );
		(*pBytesRead) += (*pStringLength);
		(*pString)[(*pStringLength)] = '\0'; // null terminate
	}
	else
		(*pString) = 0;
}

CTagString::CTagString() {
	m_tagId = TAGID_STRING;
	m_payload = "";
}
CTagString::~CTagString() {
}

bool CTagString::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	boost::int16_t stringLength;
	char *pString;

	// Read the name
	if( fullTag )
		this->readName( stream, pBytesRead );
	// Read payload
	CTagString::ReadPayload( stream, pBytesRead, &stringLength, &pString );
	if( stringLength < 0 || !pString )
		m_payload = "";
	else {
		m_payload = std::string( pString );
		delete pString;
		pString = 0;
	}

	return true;
}

//////////////
// CTagList //
//////////////

CTagList::CTagList() {
	m_tagId = TAGID_LIST;
	m_childrenId = 0;
	m_childrenCount = 0;
}
CTagList::~CTagList() {
}

bool CTagList::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	// Read name
	if( fullTag )
		this->readName( stream, pBytesRead );
	// Read the type of tags in the list
	CTagByte::ReadPayload( stream, pBytesRead, &m_childrenId );
	// Read how many are in the list
	CTagInt::ReadPayload( stream, pBytesRead, &m_childrenCount );

	// The tag reader will handle assigning children

	return true;
}

boost::int8_t CTagList::getChildrenId() const {
	return m_childrenId;
}
boost::int8_t CTagList::getChildrenCount() const {
	return m_childrenCount;
}

//////////////////
// CTagCompound //
//////////////////

bool CTagCompound::ReadPayload( InputStream &stream, size_t *pBytesRead )
{
	return true;
}

CTagCompound::CTagCompound() {
	m_tagId = TAGID_COMPOUND;
}
CTagCompound::~CTagCompound() {
}

bool CTagCompound::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	if( fullTag )
		this->readName( stream, pBytesRead );

	return true;
}