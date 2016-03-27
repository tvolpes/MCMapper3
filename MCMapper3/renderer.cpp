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
	m_outputPath /= regionName + ".jpeg";

	// Create and fill the image
	m_regionImage = boost::gil::rgb8_image_t( 512, 512 );
	boost::gil::fill_pixels( boost::gil::view( m_regionImage ), blankPixels );

	return true;
}
bool CRenderer::finishRegion()
{
	boost::gil::jpeg_write_view( m_outputPath.string(), boost::gil::const_view( m_regionImage ) );

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

	xPos = pChunkData->xPos;
	zPos = pChunkData->zPos;

	// Draw each pixel based on height
	imageView = boost::gil::view( m_regionImage );
	for( int x = 0; x < CHUNK_LENGTH; x++ ) {
		for( int z = 0; z < CHUNK_LENGTH; z++ ) {
			imageView( x+xPos*16, z+zPos*16 ) = boost::gil::rgb8_pixel_t( ((float)pChunkData->HeightMap[x+z*16] / 255.0f) * 255, ((float)pChunkData->HeightMap[x+z*16] / 255.0f) * 255, ((float)pChunkData->HeightMap[x+z*16] / 255.0f) * 255 );
		}
	}
}

unsigned int CRendererClassic::getChunkDataFlags() {
	return CHUNKDATA_HEIGHTMAP;
}