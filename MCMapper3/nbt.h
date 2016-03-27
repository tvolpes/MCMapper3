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

#pragma once

#include <boost\filesystem\fstream.hpp>
#include <boost\iostreams\filtering_stream.hpp>
#pragma warning( disable:4244 )
#include <boost\iostreams\filter\gzip.hpp>
#pragma warning( default:4244 )
#include <boost\filesystem.hpp>
#include <boost\integer.hpp>
#include <stack>
#include <vector>

class CTag;
class CTagParent;
class CTagEnd;
class CTagByte;
class CTagShort;
class CTagInt;
class CTagLong;
class CTagFloat;
class CTagDouble;
class CTagByteArray;
class CTagString;
class CTagList;
class CTagCompound;
class CTagIntArray;

typedef boost::iostreams::filtering_istream InputStream;
typedef std::vector<CTag*> TagList;

enum : boost::int8_t
{
	TAGID_END			= 0,
	TAGID_BYTE			= 1,
	TAGID_SHORT			= 2,
	TAGID_INT			= 3,
	TAGID_LONG			= 4,
	TAGID_FLOAT			= 5,
	TAGID_DOUBLE		= 6,
	TAGID_BYTE_ARRAY	= 7,
	TAGID_STRING		= 8,
	TAGID_LIST			= 9,
	TAGID_COMPOUND		= 10,
	TAGID_INT_ARRAY		= 11,
	TAGID_COUNT
};

class CNBTReader
{
private:
	std::stack<CTagParent*> m_parentStack;
	TagList m_rootTags;
	TagList m_tags;

	CTag* readTag( InputStream &stream, size_t *pBytesRead, bool fullTag );
public:
	static CTag* createTag( boost::int8_t tagId );

	CNBTReader();
	~CNBTReader();

	bool read( boost::filesystem::path fullPath );
	bool read( InputStream &stream );
	void deleteTags();

	TagList getRootTags() const;
};

//////////
// CTag //
//////////

class CTag
{
protected:
	boost::int8_t m_tagId;
	std::string m_tagName;

	void readName( InputStream &stream, size_t *pBytesRead );
public:
	CTag();
	virtual ~CTag();

	virtual void read( InputStream &stream, size_t *pBytesRead, bool fullTag ) = 0;

	boost::int8_t getId() const;
	std::string getName() const;
	virtual bool isParent() const;
};

////////////////
// CTagParent //
////////////////

class CTagParent : public CTag
{
private:
	TagList m_children;
public:
	CTagParent();
	virtual ~CTagParent();

	void addChild( CTag *pTag );

	TagList getChildren() const;
	virtual bool isParent() const;
	CTag* getChildName( std::string name );
	CTag* getChildPath( std::string path, boost::int8_t type );
};

/////////////
// CTagEnd //
/////////////

class CTagEnd : public CTag
{
public:
	CTagEnd();
	~CTagEnd();

	void read( InputStream &stream, size_t *pBytesRead, bool fullTag );
};

//////////////
// CTagByte //
//////////////

class CTagByte : public CTag
{
private:
	boost::int8_t m_payload;
public:
	static void ReadPayload( InputStream &stream, size_t *pBytesRead, boost::int8_t *pByte );

	CTagByte();
	~CTagByte();

	void read( InputStream &stream, size_t *pBytesRead, bool fullTag );

	boost::int8_t getPayload() const;
};

///////////////
// CTagShort //
///////////////

class CTagShort : public CTag
{
private:
	boost::int16_t m_payload;
public:
	static void ReadPayload( InputStream &stream, size_t *pBytesRead, boost::int16_t *pShort );

	CTagShort();
	~CTagShort();

	void read( InputStream &stream, size_t *pBytesRead, bool fullTag );

	boost::int16_t getPayload() const;
};

/////////////
// CTagInt //
/////////////

class CTagInt : public CTag
{
private:
	boost::int32_t m_payload;
public:
	static void ReadPayload( InputStream &stream, size_t *pBytesRead, boost::int32_t *pInt );

	CTagInt();
	~CTagInt();

	void read( InputStream &stream, size_t *pBytesRead, bool fullTag );

	boost::int32_t getPayload() const;
};

//////////////
// CTagLong //
//////////////

class CTagLong : public CTag
{
private:
	boost::int64_t m_payload;
public:
	static void ReadPayload( InputStream &stream, size_t *pBytesRead, boost::int64_t *pLong );

	CTagLong();
	~CTagLong();

	void read( InputStream &stream, size_t *pBytesRead, bool fullTag );

	boost::int64_t getPayload() const;
};

///////////////
// CTagFloat //
///////////////

class CTagFloat : public CTag
{
private:
	float m_payload;
public:
	static void ReadPayload( InputStream &stream, size_t *pBytesRead, float *pFloat );

	CTagFloat();
	~CTagFloat();

	void read( InputStream &stream, size_t *pBytesRead, bool fullTag );

	float getPayload() const;
};

////////////////
// CTagDouble //
////////////////

class CTagDouble : public CTag
{
private:
	double m_payload;
public:
	static void ReadPayload( InputStream &stream, size_t *pBytesRead, double *pDouble );

	CTagDouble();
	~CTagDouble();

	void read( InputStream &stream, size_t *pBytesRead, bool fullTag );

	double getPayload() const;
};

///////////////////
// CTagByteArray //
///////////////////

class CTagByteArray : public CTag
{
private:
	std::vector<boost::int8_t> m_payload;
public:
	static void ReadPayload( InputStream &stream, size_t *pBytesRead, boost::int32_t *pSize, boost::int8_t **pBytes );

	CTagByteArray();
	~CTagByteArray();

	void read( InputStream &stream, size_t *pBytesRead, bool fullTag );

	std::vector<boost::int8_t> getPayload() const;
};


////////////////
// CTagString //
////////////////

class CTagString : public CTag
{
private:
	std::string m_payload;
public:
	static void ReadPayload( InputStream &stream, size_t *pBytesRead, boost::int16_t *pStringLength, char **pString );

	CTagString();
	~CTagString();

	void read( InputStream &stream, size_t *pBytesRead, bool fullTag );

	std::string getPayload() const;
};

//////////////
// CTagList //
//////////////

class CTagList : public CTagParent
{
private:
	boost::int8_t m_childrenId;
	boost::int32_t m_childrenCount;
public:
	CTagList();
	~CTagList();

	void read( InputStream &stream, size_t *pBytesRead, bool fullTag );

	boost::int8_t getChildrenId() const;
	boost::int8_t getChildrenCount() const;
};

//////////////////
// CTagCompound //
//////////////////

class CTagCompound : public CTagParent
{
public:
	CTagCompound();
	~CTagCompound();

	void read( InputStream &stream, size_t *pBytesRead, bool fullTag );
};

//////////////////
// CTagIntArray //
//////////////////

class CTagIntArray : public CTag
{
private:
	std::vector<boost::int32_t> m_payload;
public:
	static void ReadPayload( InputStream &stream, size_t *pBytesRead, boost::int32_t *pSize, boost::int32_t **pInts );

	CTagIntArray();
	~CTagIntArray();

	void read( InputStream &stream, size_t *pBytesRead, bool fullTag );

	std::vector<boost::int32_t> getPayload() const;
};
