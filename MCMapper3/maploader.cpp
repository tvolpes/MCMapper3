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
#include <boost\timer.hpp>
#include "maploader.h"
#include "nbt.h"
#include "renderer.h"
#include "blocks.h"

CMapLoader::CMapLoader()
{
	m_mapName = "";
	m_regionsRendered = 0;
	m_regionCount = 0;
	m_pRenderer = 0;
	m_pBlockColors = 0;
}
CMapLoader::~CMapLoader()
{
	m_pRenderer = 0;
	if( m_pBlockColors ) {
		delete m_pBlockColors;
		m_pBlockColors = 0;
	}
}

bool CMapLoader::initialize()
{
	// Load  block colors
	std::cout << "Loading block data..." << std::endl;
	m_pBlockColors = new CBlockColors();
	if( !m_pBlockColors->loadBlockColors() )
		return false;
	std::cout << "Successfully loaded block data" << std::endl;

	return true;
}
bool CMapLoader::load( boost::filesystem::path fullPath )
{
	CNBTReader levelDat;
	boost::filesystem::path levelDatPath, regionsPath;
	std::vector<boost::filesystem::path> regionFiles;

	m_mapName = fullPath.string().substr( fullPath.string().find_last_of( "\\" )+1 );

	levelDatPath = fullPath / "level.dat";
	// First load level.dat
	if( !levelDat.read( levelDatPath ) )
		return false;

	// Determine what region files to load
	regionsPath = fullPath / "region";
	std::cout << "Evaluating regions..." << std::endl;
	if( !boost::filesystem::is_directory( regionsPath ) ) {
		std::cout << "Failed: Could not find regions directory" << std::endl;
		return false;
	}
	// Get all the valid region files
	for( auto &entry : boost::make_iterator_range( boost::filesystem::directory_iterator( regionsPath ), {} ) ) {
		// If the extension is correct
		std::string ext = entry.path().extension().string();
		std::transform( ext.begin(), ext.end(), ext.begin(), ::tolower );
		if( ext.compare( ".mca" ) == 0 )
			m_regionPaths.push( entry.path() );
	}
	m_regionCount = m_regionPaths.size();

	return true;
}

bool CMapLoader::nextRegion()
{
	boost::filesystem::path currentPath;
	boost::filesystem::ifstream inputStream;
	RegionHeader regionHeader;
	boost::timer renderTimer;
	
	_ASSERT_EXPR( m_regionPaths.size() > 0, L"region queue was empty" );
	_ASSERT_EXPR( m_pRenderer, L"no renderer" );

	// Get the current path
	currentPath = m_regionPaths.front();

	std::cout << " > Rendering region " << currentPath.stem() << " (" << m_regionsRendered+1 << "/" << m_regionCount << ")..." << std::endl;

	// Attempt to open the region file
	if( !boost::filesystem::is_regular_file( currentPath ) ) {
		std::cout << " > Failed: could not open region file, skipping region" << std::endl;
		return false;
	}
	inputStream.open( currentPath, std::ios::in | std::ios::binary );

	// Read the chunk locations
	for( unsigned int i = 0; i < 1024; i++ )
	{
		unsigned char offset[3];
		inputStream.read( reinterpret_cast<char*>(&offset), 3 );
		regionHeader.locations[i].offset = offset[2] + (offset[1] << 8) + (offset[0] << 16);
		//regionHeader.locations[i].offset = boost::endian::big_to_native( regionHeader.locations[i].offset );
		inputStream.read( reinterpret_cast<char*>(&(regionHeader.locations[i].sectorCount)), 1 );
	}
	// Read the chunk timestamps
	for( unsigned int i = 0; i < 1024; i++ ){
		inputStream.read( reinterpret_cast<char*>(&regionHeader.timestamps[i]), sizeof( boost::int32_t ) );
		regionHeader.timestamps[i] = boost::endian::big_to_native( regionHeader.timestamps[i] );
	}

	// Load each chunk and render
	m_pRenderer->beginRegion( m_mapName, currentPath.stem().string() );
	for( unsigned int i = 0; i < 1024; i++ )
	{
		boost::int32_t chunkLength;
		unsigned char compression;
		char *pChunkData;
		InputStream decompStream;
		CNBTReader chunkReader;
		ChunkData *pParsedChunk;

		// Check if we have a chunk here
		if( regionHeader.locations[i].sectorCount == 0 )
			continue;
		// If we do, jump to it
		inputStream.seekg( regionHeader.locations[i].offset * 4096, std::ios::beg );
		// Read the length
		inputStream.read( reinterpret_cast<char*>(&chunkLength), sizeof( boost::int32_t ) );
		chunkLength = boost::endian::big_to_native( chunkLength );
		// Read compression
		inputStream.read( reinterpret_cast<char*>(&compression), 1 );
		if( compression != 2 ) {
			std::cout << " > Failed: Compressiong type was GZip, only ZLib is supported, skipping chunk" << std::endl;
			continue;
		}
		// Read the chunk data
		if( chunkLength <= 0 ) {
			std::cout << " > Failed: found empty chunk, skipping chunk" << std::endl;
			continue;
		}
		pChunkData = new char[chunkLength-1];
		inputStream.read( pChunkData, chunkLength-1 );
		// Setup decompression and run nbt reader
		decompStream.push( boost::iostreams::zlib_decompressor() );
		decompStream.push( boost::iostreams::array_source( pChunkData, chunkLength-1 ) );
		if( !chunkReader.read( decompStream ) ) {
			std::cout << " > Failed: could not read chunk data, skipping chunk" << std::endl;
			continue;
		}
		boost::iostreams::close( decompStream );
		if( pChunkData ) {
			delete[] pChunkData;
			pChunkData = 0;
		}

		// Parse the data
		pParsedChunk = this->parseChunkData( chunkReader );
		if( !pParsedChunk )
			return false;
		m_pRenderer->renderChunk( pParsedChunk, m_pBlockColors );
		delete pParsedChunk;
		pParsedChunk = 0;
	}
	m_pRenderer->finishRegion();

	// Pop off the queue
	m_regionPaths.pop();

	// Show how long it took
	std::cout << " > Finished region (t=" << renderTimer.elapsed() << "s)" << std::endl;
	m_regionsRendered++;

	return true;
}

ChunkData* CMapLoader::parseChunkData( CNBTReader &nbtReader )
{
	ChunkData *pChunkData;
	CTagCompound *pRootTag;
	CTagInt *pXpos, *pZpos;
	CTagIntArray *pHeightMap;
	CTagList *pSections;
	TagList sectionTags;
	int section;

	if( nbtReader.getRootTags().size() < 1 ) {
		std::cout << " > Failed: Could not parse empty chunk, skipping chunk" << std::endl;
		return 0;
	}
	else if( nbtReader.getRootTags()[0]->getId() != TAGID_COMPOUND ) {
		std::cout << " > Failed: Could not find root tag, skipping chunk" << std::endl;
		return 0;
	}

	pRootTag = reinterpret_cast<CTagCompound*>(nbtReader.getRootTags()[0]);

	// Save the position
	pXpos = reinterpret_cast<CTagInt*>(pRootTag->getChildPath( "Level.xPos", TAGID_INT ));
	pZpos = reinterpret_cast<CTagInt*>(pRootTag->getChildPath( "Level.zPos", TAGID_INT ));
	pHeightMap = reinterpret_cast<CTagIntArray*>(pRootTag->getChildPath( "Level.HeightMap", TAGID_INT_ARRAY ));
	pSections = reinterpret_cast<CTagList*>(pRootTag->getChildPath( "Level.Sections", TAGID_LIST ));
	if( !pXpos || !pZpos || !pHeightMap || !pSections ) {
		std::cout << " > Failed: invalid chunk data" << std::endl;
		return 0;
	}

	pChunkData = new ChunkData();
	pChunkData->xPos = pXpos->getPayload();
	pChunkData->zPos = pZpos->getPayload();

	// Save the height map
	if( pHeightMap->getPayload().size() != CHUNK_LENGTH*CHUNK_LENGTH ) {
		std::cout << " > Failed: invalid height map dimensions, skipping chunk" << std::endl;
		delete pChunkData;
		return 0;
	}
	memcpy( &pChunkData->HeightMap[0], &pHeightMap->getPayload()[0], sizeof( boost::int32_t )*256 );

	// Save the block sections
	sectionTags = pSections->getChildren();
	section = 0;
	for( auto it = sectionTags.begin(); it != sectionTags.end(); it++ )
	{
		ChunkSection sectionData;
		CTagList *pCurrentSection;
		CTagByte *pY;
		CTagByteArray *pBlockIds;

		if( (*it)->getId() != TAGID_COMPOUND ) {
			std::cout << " > Failed: invalid section tag, skipping chunk" << std::endl;
			delete pChunkData;
			return 0;
		}
		pCurrentSection = reinterpret_cast<CTagList*>((*it));
		pY = reinterpret_cast<CTagByte*>(pCurrentSection->getChildName( "Y" ));
		pBlockIds = reinterpret_cast<CTagByteArray*>(pCurrentSection->getChildName( "Blocks" ));
		if( !pY || !pBlockIds ) {
			std::cout << " > Failed: invalid section tag, skipping chunk" << std::endl;
			delete pChunkData;
			return 0;
		}
		// Copy the data
		sectionData.Y = pY->getPayload();
		memcpy( &sectionData.BlockIds[0], &pBlockIds->getPayload()[0], 4096 );

		pChunkData->Sections[section] = sectionData;
		section++;
	}

	return pChunkData;
}

size_t CMapLoader::getRegionCount() const {
	return m_regionCount;
}

void CMapLoader::setRenderer( CRenderer *pRenderer ) {
	m_pRenderer = pRenderer;
}
CRenderer* CMapLoader::getRenderer() const {
	return m_pRenderer;
}