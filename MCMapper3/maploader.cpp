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
#include "maploader.h"
#include "nbt.h"

CMapLoader::CMapLoader()
{
	m_regionsRendered = 0;
}
CMapLoader::~CMapLoader()
{
}

bool CMapLoader::load( boost::filesystem::path fullPath )
{
	CNBTReader levelDat;
	boost::filesystem::path levelDatPath, regionsPath;
	std::vector<boost::filesystem::path> regionFiles;

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

	return true;
}

bool CMapLoader::nextRegion()
{
	boost::filesystem::path currentPath;
	
	_ASSERT_EXPR( m_regionPaths.size() > 0, "region queue was empty" );

	// Get the current path
	currentPath = m_regionPaths.front();

	// Render this region
	std::cout << "Rendering region " << currentPath.stem() << " (" << m_regionsRendered+1 << "/" << m_regionPaths.size() << ")..." << std::endl;

	// Pop off the queue
	m_regionPaths.pop();

	return true;
}

size_t CMapLoader::getRegionCount() const {
	return m_regionPaths.size();
}