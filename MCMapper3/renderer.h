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

#include <boost\filesystem.hpp>
#include <boost\gil\gil_all.hpp>
#include <string>

struct ChunkData;

class CRenderer
{
protected:
	boost::filesystem::path m_outputPath;
	boost::gil::rgb8_image_t m_regionImage;
public:
	CRenderer();
	virtual ~CRenderer();

	virtual bool beginRegion( std::string mapName, std::string regionName );
	bool finishRegion();

	virtual void renderChunk( ChunkData *pChunkData ) = 0;

	virtual unsigned int getChunkDataFlags() = 0;
};

//////////////////////
// CRendererClassic //
//////////////////////

class CRendererClassic : public CRenderer
{
public:
	CRendererClassic();
	~CRendererClassic();

	bool beginRegion( std::string mapName, std::string regionName );

	void renderChunk( ChunkData *pChunkData );

	unsigned int getChunkDataFlags();
};
