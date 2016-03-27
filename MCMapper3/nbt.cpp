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
#include <boost\algorithm\string.hpp>
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
	case TAGID_BYTE_ARRAY:
		return new CTagByteArray();
	case TAGID_STRING:
		return new CTagString();
	case TAGID_LIST:
		return new CTagList();
	case TAGID_COMPOUND:
		return new CTagCompound();
	case TAGID_INT_ARRAY:
		return new CTagIntArray();
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
	this->deleteTags();
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
		_ASSERT_EXPR( !m_parentStack.empty(), L"can't create non-full tag when parent stack is empty" );
		_ASSERT_EXPR( m_parentStack.top()->getId() == TAGID_LIST, L"list not at top of parent stack" );
		tagId = reinterpret_cast<CTagList*>(m_parentStack.top())->getChildrenId();
	}

	if( stream.eof() )
		return 0;

	// Create the tag
	pTag = this->createTag( tagId );
	if( !pTag )
		return 0;

	// If it has no parents its a root tag
	if( m_parentStack.empty() )
		m_rootTags.push_back( pTag );
	// If not assign it to parent
	else
		m_parentStack.top()->addChild( pTag );

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
	pTag->read( stream, pBytesRead, fullTag );

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
	InputStream decompStream;

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

	// Setup decompression filter
	decompStream.push( boost::iostreams::gzip_decompressor() );
	decompStream.push( inputStream );

	// Read
	if( !this->read( decompStream ) )
		return false;

	_ASSERT_EXPR( inputStream.is_open(), L"inputStream was closed prematurely" );
	// Close
	boost::iostreams::close( decompStream );
	inputStream.close();

	return true;
}
bool CNBTReader::read( InputStream &stream )
{
	size_t bytesRead;
	CTag *pTag;

	// Read a tag, if it is a parent tag, add it to the parent stack and start reading children, so on and so forth
	// When an END tag is hit, pop off the top of the parent stack

	// Read the tags and assign parents, etc.
	bytesRead = 0;
	try
	{
		while( !stream.eof() )
		{
			// If it a list is at the top of the parent stack,
			// the next tags will not be fully formed (no name, id)
			if( !m_parentStack.empty() ) {
				if( m_parentStack.top()->getId() == TAGID_LIST )
					pTag = this->readTag( stream, &bytesRead, false );
				else // Read normally
					pTag = this->readTag( stream, &bytesRead, true );
			}
			else // Read normally
				pTag = this->readTag( stream, &bytesRead, true );
			if( !pTag && !stream.eof() ) // if we didn't get a tag and we're not EOF
				return false;
			else if( !pTag && stream.eof() )
				break;

			// Add it to the unsorted tag list
			m_tags.push_back( pTag );
		}
	}
	catch( const boost::filesystem::filesystem_error &e ) {
		std::cout << "Failed: filesystem error (" << e.what() << ")" << std::endl;
		return false;
	}

	return true;
}
void CNBTReader::deleteTags()
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
	m_rootTags.clear();
}

TagList CNBTReader::getRootTags() const {
	return m_rootTags;
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
CTag* CTagParent::getChildName( std::string name )
{
	// Find it in the children list
	for( auto it = m_children.begin(); it != m_children.end(); it++ ) {
		if( (*it)->getName().compare( name ) == 0 ) {
			return (*it);
		}
	}
	return 0;
}
CTag* CTagParent::getChildPath( std::string path, boost::int8_t type )
{
	std::vector<std::string> tokens;
	CTag *pNextTag;
	CTagParent *pCurrentParent;

	// Split into tokens
	boost::split( tokens, path, boost::is_any_of( "." ) );
	pCurrentParent = this;
	for( auto it = tokens.begin(); it != tokens.end(); it++ )
	{
		// Search for
		pNextTag = pCurrentParent->getChildName( (*it) );
		if( !pNextTag ) {
			std::cout << "Could not find tag \'" << path.c_str() << "\'" << std::endl;
			return 0;
		}
		// Check if its a parent
		if( pNextTag->isParent() )
			pCurrentParent = reinterpret_cast<CTagParent*>(pNextTag);
		else if( it == tokens.end()-1 ) {
			if( pNextTag->getId() == type )
				return pNextTag;
			else {
				std::cout << "Invalid type for tag specified by path \'" << path.c_str() << "\'" << std::endl;
				return 0;
			}
		}
		else {
			std::cout << "Invalid path \'" << path.c_str() << "\'" << std::endl;
		}
	}

	_ASSERT_EXPR( 0, L"unhandled logic?" );
	return 0;
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

void CTagEnd::read( InputStream &stream, size_t *pBytesRead, bool fullTag ) {
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

void CTagByte::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	if( fullTag )
		this->readName( stream, pBytesRead );
	CTagByte::ReadPayload( stream, pBytesRead, &m_payload );
}

boost::int8_t CTagByte::getPayload() const {
	return m_payload;
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

void CTagShort::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	// Read the name
	if( fullTag )
		this->readName( stream, pBytesRead );
	// Read payload
	CTagShort::ReadPayload( stream, pBytesRead, &m_payload );
}

boost::int16_t CTagShort::getPayload() const {
	return m_payload;
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

void CTagInt::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	if( fullTag )
		this->readName( stream, pBytesRead );
	CTagInt::ReadPayload( stream, pBytesRead, &m_payload );
}

boost::int32_t CTagInt::getPayload() const {
	return m_payload;
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

void CTagLong::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	// Read the name
	if( fullTag )
		this->readName( stream, pBytesRead );
	// Read payload
	CTagLong::ReadPayload( stream, pBytesRead, &m_payload );
}

boost::int64_t CTagLong::getPayload() const {
	return m_payload;
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

void CTagFloat::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	if( fullTag )
		this->readName( stream, pBytesRead );
	CTagFloat::ReadPayload( stream, pBytesRead, &m_payload );
}

float CTagFloat::getPayload() const {
	return m_payload;
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

void CTagDouble::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	if( fullTag )
		this->readName( stream, pBytesRead );
	CTagDouble::ReadPayload( stream, pBytesRead, &m_payload );
}

double CTagDouble::getPayload() const {
	return m_payload;
}

///////////////////
// CTagByteArray //
///////////////////

void CTagByteArray::ReadPayload( InputStream &stream, size_t *pBytesRead, boost::int32_t *pSize, boost::int8_t **pBytes )
{
	// Read the payload size
	CTagInt::ReadPayload( stream, pBytesRead, pSize );
	// Read pSize number of payloads
	if( (*pSize) > 0 ) {
		(*pBytes) = new boost::int8_t[(*pSize)];
		for( boost::int32_t i = 0; i < (*pSize); i++ )
			CTagByte::ReadPayload( stream, pBytesRead, &(*pBytes)[i] );
	}
	else
		(*pBytes) = 0;
}

CTagByteArray::CTagByteArray() {
	m_tagId = TAGID_BYTE_ARRAY;
}
CTagByteArray::~CTagByteArray() {
}

void CTagByteArray::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	boost::int32_t size;
	boost::int8_t *pBytes;

	if( fullTag )
		this->readName( stream, pBytesRead );
	CTagByteArray::ReadPayload( stream, pBytesRead, &size, &pBytes );
	if( size > 0 && pBytes ) {
		m_payload = std::vector<boost::int32_t>( pBytes, pBytes + size );
		delete[] pBytes;
	}
}

std::vector<boost::int32_t> CTagByteArray::getPayload() const {
	return m_payload;
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

void CTagString::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
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
		delete[] pString;
		pString = 0;
	}
}

std::string CTagString::getPayload() const {
	return m_payload;
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

void CTagList::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	// Read name
	if( fullTag )
		this->readName( stream, pBytesRead );
	// Read the type of tags in the list
	CTagByte::ReadPayload( stream, pBytesRead, &m_childrenId );
	// Read how many are in the list
	CTagInt::ReadPayload( stream, pBytesRead, &m_childrenCount );

	// The tag reader will handle assigning children
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

CTagCompound::CTagCompound() {
	m_tagId = TAGID_COMPOUND;
}
CTagCompound::~CTagCompound() {
}

void CTagCompound::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	if( fullTag )
		this->readName( stream, pBytesRead );
}

//////////////////
// CTagIntArray //
//////////////////

void CTagIntArray::ReadPayload( InputStream &stream, size_t *pBytesRead, boost::int32_t *pSize, boost::int32_t **pInts )
{
	// Read the payload size
	CTagInt::ReadPayload( stream, pBytesRead, pSize );
	// Read pSize number of payloads
	if( (*pSize) > 0 ) {
		(*pInts) = new boost::int32_t[(*pSize)];
		for( boost::int32_t i = 0; i < (*pSize); i++ )
			CTagInt::ReadPayload( stream, pBytesRead, &(*pInts)[i] );
	}
	else
		(*pInts) = 0;
}

CTagIntArray::CTagIntArray() {
	m_tagId = TAGID_INT_ARRAY;
}
CTagIntArray::~CTagIntArray() {
}

void CTagIntArray::read( InputStream &stream, size_t *pBytesRead, bool fullTag )
{
	boost::int32_t size;
	boost::int32_t *pInts;

	if( fullTag )
		this->readName( stream, pBytesRead );
	CTagIntArray::ReadPayload( stream, pBytesRead, &size, &pInts );
	if( size > 0 && pInts ) {
		m_payload = std::vector<boost::int32_t>( pInts, pInts + size );
		delete[] pInts;
	}
}

std::vector<boost::int32_t> CTagIntArray::getPayload() const {
	return m_payload;
}