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
#include <queue>
#include "nbt.h"

#define CHUNK_LENGTH 16
#define SECTION_HEIGHT 16

enum : unsigned int
{
	CHUNKDATA_NONE			= 1 << 0,
	CHUNKDATA_HEIGHTMAP		= 1 << 1,
	CHUNKDATA_BLOCKIDS		= 1 << 2
};

#pragma pack(push, 1)
struct ChunkLocation
{
	boost::int32_t offset;
	unsigned char sectorCount;
};
struct RegionHeader
{
	ChunkLocation locations[1024];
	boost::int32_t timestamps[1024];
};
#pragma pack(pop)

struct ChunkSection
{
	boost::int8_t Y;
	boost::int8_t BlockIds[4096];
};
struct ChunkData
{
	boost::int32_t xPos, zPos;
	boost::int32_t HeightMap[CHUNK_LENGTH*CHUNK_LENGTH];
	ChunkSection Sections[16];
};

class CRenderer;
class CBlockColors;

class CMapLoader
{
private:
	std::string m_mapName;

	std::queue<boost::filesystem::path> m_regionPaths;
	unsigned int m_regionCount;
	unsigned int m_regionsRendered;

	CRenderer *m_pRenderer;
	CBlockColors *m_pBlockColors;

	ChunkData* parseChunkData( CNBTReader &nbtReader );
public:
	CMapLoader();
	~CMapLoader();

	/*
		@method: initialize
		@returns: if initialization was successful
		Loads block colors
	*/
	bool initialize();

	/*
		@method: load
		@returns: if the map was loaded successfully
		Reads level.dat and sets up the regions to be read
	*/
	bool load( boost::filesystem::path fullPath );

	/*
		@method: nextRegion
		@returns: if the region was read successfully
		Loads the next region into memory
	*/
	bool nextRegion();

	void setRenderer( CRenderer *pRenderer );
	CRenderer* getRenderer() const;
	size_t getRegionCount() const;
};
