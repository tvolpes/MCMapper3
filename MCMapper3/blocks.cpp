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
#include <boost\property_tree\xml_parser.hpp>
#include <boost\filesystem.hpp>
#include "blocks.h"

CBlockColors::CBlockColors() {
}
CBlockColors::~CBlockColors() {
}

bool CBlockColors::loadBlockColors()
{
	boost::property_tree::ptree colorTree;
	boost::filesystem::path defaultBlockPath;

	// Find the default blocks
	defaultBlockPath = boost::filesystem::current_path();
	defaultBlockPath /= DEFAULT_BLOCKS;
	if( !boost::filesystem::is_regular_file( defaultBlockPath ) ) {
		std::cout << "Failed: could not find default block XML file, please reinstall" << std::endl;
		return false;
	}
	// Read the file
	try {
		boost::property_tree::read_xml( defaultBlockPath.string(), colorTree );
	}
	catch( boost::property_tree::xml_parser_error &e ) {
		std::cout << "Failed: default block XML file contains syntax errors (" << e.what() << ")" << std::endl;
		return false;
	}
	// Parse all the blocks
	colorTree = colorTree.get_child( "blocks", boost::property_tree::ptree() );
	// For each block
	for( auto it = colorTree.begin(); it != colorTree.end(); it++ )
	{
		int blockId;
		boost::gil::rgb8_pixel_t color;
		// Skip non-blocks
		if( (*it).first.compare( "block" ) != 0 )
			continue;
		// Get id
		blockId = (*it).second.get( "<xmlattr>.id", 0 );
		// Get color
		color = boost::gil::rgb8_pixel_t( (*it).second.get( "<xmlattr>.r", 0 ), (*it).second.get( "<xmlattr>.g", 0 ), (*it).second.get( "<xmlattr>.b", 0 ) );
		// Put it into the map
		if( m_blockColors.find( blockId ) != m_blockColors.end() )
			continue; // ignore
		m_blockColors.insert( std::pair<int, boost::gil::rgb8_pixel_t>( blockId, color ) );
	}

	return true;
}
boost::gil::rgb8_pixel_t CBlockColors::getBlockPixel( int blockId )
{
	if( m_blockColors.find( blockId ) == m_blockColors.end() )
		return boost::gil::rgb8_pixel_t( 0, 0, 0 );
	else
		return m_blockColors[blockId];
}