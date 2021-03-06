#include "Logger.h"

#include <cstdio>
#include <iostream>
#include <mutex>

Logger Log;
std::mutex log_lock;


Logger::OutputFileHandle::OutputFileHandle (std::string file_name)
{
	fp = std::fopen (file_name.c_str (), "w");
	if (!fp)
	{
		fmt::print ("File opening failed");
	}
}

Logger::OutputFileHandle::~OutputFileHandle () { std::fclose (fp); }

Logger::Logger () : fp_debug ("output.txt"), fp_error ("error.txt") {}

void Logger::debug (std::string_view str_v)
{
	std::lock_guard<std::mutex> lock (log_lock);
	fmt::print (str_v);
	fmt::print ("\n");
	fmt::print (fp_debug.fp, str_v);
	fmt::print (fp_debug.fp, "\n");
}

void Logger::error (std::string_view str_v)
{
	std::lock_guard<std::mutex> lock (log_lock);
	fmt::print (stderr, str_v);
	fmt::print (stderr, "\n");
	fmt::print (fp_debug.fp, str_v);
	fmt::print (fp_debug.fp, "\n");
	fmt::print (fp_error.fp, str_v);
	fmt::print (fp_error.fp, "\n");
}