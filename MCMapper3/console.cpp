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

#include <shlobj.h>

#include <iostream>
#include <string>
#include <algorithm>
#include <boost\filesystem.hpp>
#include "console.h"
#include "maploader.h"
#include "generator.h"

CConsole& CConsole::getInstance() {
	static CConsole instance;
	return instance;
}

CConsole::CConsole()
{

}

bool CConsole::initialize( int argc, char *argv[] )
{
	std::vector<char*> arguments;

	// Figure out what we should be doing
	if( argc <= 1 ) {
		this->commandHelp();
		return true; // this isnt really a failure
	}
	// Convert the command list to a vector
	arguments = std::vector<char*>( argv+1, argv  + argc );
	// Run the command specified by the user
	if( !this->run( arguments ) )
		return false;

	return true;
}
bool CConsole::run( std::vector<char*> &arguments )
{
	std::string command;

	_ASSERT_EXPR( arguments.size() > 0, L"argument list was empty" );

	// Identify the command
	command = arguments[0];
	std::transform( command.begin(), command.end(), command.begin(), ::tolower );

	if( command.compare( "help" ) == 0 ) {
		if( arguments.size() >= 2 )
			this->commandHelp( arguments[1] );
		else
			this->commandHelp();
		return true;
	}
	else if( command.compare( "generate" ) == 0 ) {
		if( arguments.size() < 2 ) {
			this->commandHelp( "generate" );
			return true;
		}
		std::string map, output;
		std::vector<char> flags;
		map = arguments[1];
		if( arguments.size() >= 3 ) 
			flags = std::vector<char>( arguments[2], arguments[2]+strlen( arguments[2] ) );
		if( arguments.size() >= 4 )
			output = arguments[3];
		return this->commandGenerate( map, flags, output );
	}
	else {
		std::cout << "\'" << arguments[0] << "\' is not a valid command" << std::endl;
		this->commandHelp();
		return true;
	}

	return true;
}
void CConsole::exit()
{

}

void CConsole::commandHelp()
{
	std::cout << "The following commands are valid, for more info use help [command]" << std::endl;

	std::cout << "HELP\t\tDisplays help information" << std::endl;
	std::cout << "GENERATE\tGenerates map data from a save file" << std::endl;
	std::cout << "GENBLOCKS\tGenerates block colors from Minecraft data" << std::endl;
}
void CConsole::commandHelp( std::string command )
{
	if( command.compare( "help" ) == 0 ) {
		std::cout << "Usage: help [command]" << std::endl;
		std::cout << "Displays general help information, or help for a command specified by [command]" << std::endl;
	}
	else if( command.compare( "generate" ) == 0 ) {
		std::cout << "Usage: generate [save] [flags] [output]" << std::endl;
		std::cout << "Generates map data from the save file specified by [save]\n[save] can be either a path relative to the .minecraft %appdata% folder or an absolute path.\nOutput path is optional, will be outputted to current directory if none is specified" << std::endl;
		std::cout << "Flag format is -[flag chars], valid flags are:" << std::endl;
		std::cout << "O\tWill ignore transparency, including water" << std::endl;
	}
	else
		std::cout << "No help found for command" << std::endl;
}
bool CConsole::commandGenerate( std::string map, std::vector<char> flags, std::string output )
{
	TCHAR appdataPath[MAX_PATH];
	boost::filesystem::path fullMapPath;
	bool foundPath;
	CMapLoader mapLoader;

	// Find the map file

	// Check app data first
	foundPath = false;
	if( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_APPDATA, NULL, 0, appdataPath ) ) ) {
		fullMapPath = appdataPath;
		fullMapPath /= ".minecraft";
		fullMapPath /= "saves";
		fullMapPath /= map;
		if( boost::filesystem::is_directory( fullMapPath ) )
			foundPath = true; // use this
	}
	// Try absolute path
	if( !foundPath ) {
		fullMapPath = map;
		if( boost::filesystem::is_directory( fullMapPath ) )
			foundPath = true;
	}
	// If we didnt find anything
	if( !foundPath ) {
		std::cout << "Could not find map in %appdata% or at \'" << fullMapPath.string().c_str() << "\'" << std::endl;
		return false;
	}

	// Load the map
	std::cout << "Loading map at \'"<< fullMapPath.string().c_str() << "\'" << std::endl;
	if( !mapLoader.load( fullMapPath ) )
		return false;
	std::cout << "Successfully loaded map" << std::endl;

	return true;
}