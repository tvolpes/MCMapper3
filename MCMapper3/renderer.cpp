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
#pragma warning( disable:4996 )
#include <boost\gil\extension\io\jpeg_dynamic_io.hpp>
#pragma warning( default:4996 )
#include "renderer.h"
#include "maploader.h"

int CRenderer::PixelToBlockRatios[ZOOM_LEVELS] ={ 1, 2, 4, 8 };

CRenderer::CRenderer() {
}
CRenderer::~CRenderer() {
}

bool CRenderer::beginRegion( std::string mapName, std::string regionName )
{
	boost::gil::rgb8_pixel_t blankPixels( 200, 200, 200 );

	// Construct the output path
	m_outputPath = boost::filesystem::current_path() / "maps";
	m_outputPath /= mapName;
	// Make sure the directory exists
	if( !boost::filesystem::is_directory( m_outputPath ) ) {
		if( !boost::filesystem::create_directories( m_outputPath ) ) {
			std::cout << "Failed: could not create output directory" << std::endl;
			return false;
		}
	}
	m_regionName = regionName;

	// Create and fill the image
	m_regionImage = boost::gil::rgb8_image_t( 512, 512 );
	boost::gil::fill_pixels( boost::gil::view( m_regionImage ), blankPixels );

	return true;
}
bool CRenderer::finishRegion()
{
	boost::filesystem::path zeroZoom;

	// Write the zero zoom
	zeroZoom = m_outputPath / "0";
	if( !boost::filesystem::is_directory( zeroZoom ) ) {
		if( !boost::filesystem::create_directories( zeroZoom ) ) {
			std::cout<< " > Failed: could not create directory for images" << std::endl;
			return false;
		}
	}
	zeroZoom /= m_regionName + "-0.jpeg";
	boost::gil::jpeg_write_view( zeroZoom.string(), boost::gil::const_view( m_regionImage ) );

	// Generate the zoom images
	if( !this->generateZoom() )
		return false;

	return true;
}

bool CRenderer::generateZoom()
{
	int subdivisionCount;
	int sideLength, sideSubdivisions;
	int xOffset, zOffset;
	boost::gil::rgb8_image_t::view_t currentView, regionView;
	boost::filesystem::path zoomOutput, subdivisionOutput;

	std::cout << " > Generating zoom images for region..." << std::endl;
	// For each zoom we subdivide the region
	regionView = boost::gil::view( m_regionImage );
	for( int i = 1; i < ZOOM_LEVELS; i++ ) // skip the first one because its always 1
	{
		subdivisionCount = (int)pow( 4, i );
		sideLength = REGION_PIXEL_LENGTH / CRenderer::PixelToBlockRatios[i];
		sideSubdivisions = 512 / sideLength;
		zoomOutput = m_outputPath / std::to_string( i );

		// Make sure thge directory exists
		if( !boost::filesystem::is_directory( zoomOutput ) ) {
			if( !boost::filesystem::create_directories( zoomOutput ) ) {
				std::cout<< " > Failed: could not create directory for images" << std::endl;
				return false;
			}
		}

		// Render each
		for( int j = 0; j < subdivisionCount; j++ )
		{
			boost::gil::rgb8_image_t subdivision( REGION_PIXEL_LENGTH, REGION_PIXEL_LENGTH );

			// Fill with blocks at the appropriate ratio
			xOffset = (unsigned int)(j % sideSubdivisions)*sideLength;
			zOffset = (unsigned int)(j / sideSubdivisions)*sideLength;
			currentView = boost::gil::view( subdivision );
			for( int x = 0; x < sideLength; x++ ) {
				for( int z = 0; z < sideLength; z++ ) {
					boost::gil::fill_pixels( boost::gil::subimage_view( boost::gil::view( subdivision ), x*CRenderer::PixelToBlockRatios[i], z*CRenderer::PixelToBlockRatios[i], CRenderer::PixelToBlockRatios[i], CRenderer::PixelToBlockRatios[i] ), regionView( x+xOffset, z+zOffset ) );
				}
			}
			// Write it
			subdivisionOutput = zoomOutput;
			subdivisionOutput /= (m_regionName + "-" + std::to_string(j) + ".jpeg");
			boost::gil::jpeg_write_view( subdivisionOutput.string(), boost::gil::const_view( subdivision ) );
		}
	}

	return true;
}

//////////////////////
// CRendererClassic //
//////////////////////

CRendererClassic::CRendererClassic() {

}
CRendererClassic::~CRendererClassic() {

}

bool CRendererClassic::beginRegion( std::string mapName, std::string regionName )
{
	if( !CRenderer::beginRegion( mapName, regionName ) )
		return false;

	return true;
}

void CRendererClassic::renderChunk( ChunkData *pChunkData )
{
	int xPos, zPos;
	boost::gil::rgb8_image_t::view_t imageView;

	xPos = abs( pChunkData->xPos % 32 );
	zPos = abs( pChunkData->zPos % 32 );

	if( pChunkData->zPos < 0 )
		zPos = 32 - zPos;
	if( pChunkData->xPos < 0 )
		xPos = 32 - xPos;

	// Draw each pixel based on height
	imageView = boost::gil::view( m_regionImage );
	for( int x = 0; x < CHUNK_LENGTH; x++ ) {
		for( int z = 0; z < CHUNK_LENGTH; z++ )
		{
			int height, section, heightOffset;
			unsigned char blockId;

			// Find the top block
			height = pChunkData->HeightMap[x+z*16];
			section = height / SECTION_HEIGHT;
			heightOffset = height - (section*SECTION_HEIGHT);
			blockId = pChunkData->Sections[section].BlockIds[x+(z*16)+(heightOffset*CHUNK_LENGTH*CHUNK_LENGTH)];

			imageView( x+xPos*16, z+zPos*16 ) = boost::gil::rgb8_pixel_t( (unsigned char)(((float)pChunkData->HeightMap[x+z*16] / 255.0f) * 255.0f), (unsigned char)(((float)pChunkData->HeightMap[x+z*16] / 255.0f) * 255.0f), (unsigned char)(((float)pChunkData->HeightMap[x+z*16] / 255.0f) * 255.0f) );
		}
	}
}

unsigned int CRendererClassic::getChunkDataFlags() {
	return CHUNKDATA_HEIGHTMAP | CHUNKDATA_BLOCKIDS;
}