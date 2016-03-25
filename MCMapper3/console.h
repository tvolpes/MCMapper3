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

#include <vector>

class CConsole
{
private:
	CConsole();

	void commandHelp();
	void commandHelp( std::string command );
	bool commandGenerate( std::string map, std::vector<char> flags, std::string output );
public:
	static CConsole& getInstance();

	CConsole( CConsole const& ) = delete;
	void operator=( CConsole const& ) = delete;
	
	/*
		@method: initialize
		@returns: if program executed successfully
		Takes the argc and argv from main, initializes the program
	*/
	bool initialize( int argc, char *argv[] );
	/*
		method: exit
		returns: none
		Called on exit, cleans up
	*/
	void exit();

	/*
		@method: run
		@returns: if the command executed successfully
		Runs the command specified by arguments[0]
	*/
	bool run( std::vector<char*> &arguments );
};