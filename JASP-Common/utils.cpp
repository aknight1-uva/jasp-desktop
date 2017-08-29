//
// Copyright (C) 2013-2017 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "utils.h"

#ifdef __WIN32__
#include "windows.h"
#else
#include <sys/stat.h>
#include <utime.h>
#endif

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/nowide/convert.hpp>
#include <boost/algorithm/string/predicate.hpp>

using namespace std;
using namespace boost::posix_time;
using namespace boost;

const char* Utils::getFileTypeString(const Utils::FileType &fileType) {
	switch (fileType) {
        case Utils::csv: return "csv";
		case Utils::txt: return "txt";
		case Utils::sav: return "sav";
		case Utils::ods: return "ods";
		case Utils::jasp: return "jasp";
        case Utils::html: return "html";
        case Utils::pdf: return "pdf";
		default: return "";
	}
}

Utils::FileType Utils::getTypeFromFileName(const JaspFileTypes::FilePath &path)
{

	Utils::FileType filetype =  Utils::unknown;

	for (int i = 0; i < Utils::empty; i++)
	{
		Utils::FileType it = static_cast<Utils::FileType>(i);
		std::string it_str(".");
		it_str += Utils::getFileTypeString(it);
		if (path.extension() == it_str)
		{
			filetype = it;
			break;
		}
	}

	if (filetype == Utils::unknown)
	{
		if (path.extension() == ".")
			filetype =  Utils::empty;
	}

	return filetype;
}

long Utils::currentMillis()
{
	ptime epoch(boost::gregorian::date(1970,1,1));
	ptime t = microsec_clock::local_time();
	time_duration elapsed = t - epoch;

	return elapsed.total_milliseconds();
}

long Utils::currentSeconds()
{
	time_t now;
	time(&now);

	return now;
}

long Utils::getFileModificationTime(const JaspFileTypes::FilePath &filename)
{
#ifdef __WIN32__

    HANDLE file = CreateFileW(filename.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE)
		return -1;

	FILETIME modTime;

	bool success = GetFileTime(file, NULL, NULL, &modTime);
	CloseHandle(file);

	if (success)
	{
		ptime pt = from_ftime<ptime>(modTime);
		ptime epoch(boost::gregorian::date(1970,1,1));

		return (pt - epoch).total_seconds();
	}
	else
	{
		return -1;
	}
#elif __APPLE__

	struct stat attrib;
	stat(filename.c_str(), &attrib);
	time_t modificationTime = attrib.st_mtimespec.tv_sec;

	return modificationTime;

#else
    struct stat attrib;
    stat(filename.c_str(), &attrib);
    time_t modificationTime = attrib.st_mtim.tv_sec;

    return modificationTime;
#endif
}

long Utils::getFileSize(const JaspFileTypes::FilePath &filename)
{
	system::error_code ec;
	uintmax_t fileSize = filesystem::file_size(filename, ec);
	return (ec == 0) ? fileSize :  -1;
}

void Utils::touch(const JaspFileTypes::FilePath &filename)
{
#ifdef __WIN32__

	HANDLE file = CreateFile(filename.native().c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE)
		return;

	FILETIME ft;
	SYSTEMTIME st;

	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);
	SetFileTime(file, NULL, NULL, &ft);

	CloseHandle(file);

#else
	struct utimbuf newTime;

	time_t newTimeT;
	time(&newTimeT);

	newTime.actime = newTimeT;
	newTime.modtime = newTimeT;

	utime(filename.native().c_str(), &newTime);
#endif
}

bool Utils::renameOverwrite(const JaspFileTypes::FilePath &oldName, const JaspFileTypes::FilePath &newName)
{
	system::error_code ec;

#ifdef __WIN32__
	system::error_code er;
	if (filesystem::exists(newName, er)) {
		filesystem::file_status s = filesystem::status(newName);
		bool readOnly = (s.permissions() & filesystem::owner_write) == 0;
		if (readOnly)
			filesystem::permissions(newName, filesystem::owner_write);
	}
#endif

	boost::filesystem::rename(oldName, newName, ec);

	return ec == 0;
}

bool Utils::removeFile(const JaspFileTypes::FilePath &path)
{
	system::error_code ec;

	boost::filesystem::remove(path, ec);

	return ec == 0;
}

/*
SystemDepFileTypes::FilePath Utils::osPath(const string &path)
{
#ifdef __WIN32__
	return filesystem::path(nowide::widen(path));
#else
	return filesystem::path(path);
#endif
}

string Utils::osPath(const filesystem::path &path)
{
#ifdef __WIN32__
	return nowide::narrow(path.generic_wstring());
#else
	return path.generic_string();
#endif
}  */

/*
void Utils::remove(vector<string> &target, const vector<string> &toRemove)
{
	BOOST_FOREACH (const string &remove, toRemove)
	{
		vector<string>::iterator itr = target.begin();
		while (itr != target.end())
		{
			if (*itr == remove)
				target.erase(itr);
			else
				itr++;
		}
	}
} */

void Utils::remove(vector<JaspFileTypes::FilePath> &target, const vector<JaspFileTypes::FilePath> &filesToRemove)
{
	BOOST_FOREACH (const JaspFileTypes::FilePath &remove, filesToRemove)
	{
		vector<JaspFileTypes::FilePath>::iterator itr = target.begin();
		while (itr != target.end())
		{
			if (itr->filename() == remove.filename())
				target.erase(itr);
			else
				itr++;
		}
	}
}


void Utils::sleep(int ms)
{

#ifdef __WIN32__
    Sleep(DWORD(ms));
#else
	struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
	nanosleep(&ts, NULL);
#endif
}

/**
 * @brief deleteList Attempts to delete all the files mentioned.
 * @param files A vector of files to delete. The file paths are assymmmed to be complete.
 */
void Utils::deleteListFullPaths(const vector<JaspFileTypes::FilePath> &files)
{
	system::error_code error;

	BOOST_FOREACH (const JaspFileTypes::FilePath &file, files)
	{
		filesystem::remove(file, error);
	}
}
