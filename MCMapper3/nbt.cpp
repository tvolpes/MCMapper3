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
#include <boost\filesystem\fstream.hpp>
#include <boost\iostreams\filtering_stream.hpp>
#pragma warning( disable:4244 )
#include <boost\iostreams\filter\gzip.hpp>
#pragma warning( default:4244 )
#include "nbt.h"

CNBTReader::CNBTReader()
{
}
CNBTReader::~CNBTReader()
{
}

bool CNBTReader::read( boost::filesystem::path fullPath )
{
	boost::filesystem::ifstream inputStream;
	boost::iostreams::filtering_istream decompStream;

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
		// Setup decompression filter
		decompStream.push( boost::iostreams::gzip_decompressor() );
		decompStream.push( inputStream );
	}
	catch( const boost::filesystem::filesystem_error &e ) {
		std::cout << "Failed: could not open level.dat (" << e.what() << ")" << std::endl;
		return false;
	}

	return true;
}