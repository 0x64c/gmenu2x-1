/***************************************************************************
 *   Copyright (C) 2006 by Massimiliano Torromeo                           *
 *   massimiliano.torromeo@gmail.com                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "filelister.h"

#include "debug.h"
#include "utilities.h"

//for browsing the filesystem
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <algorithm>
#include <cstring>

using namespace std;

FileLister::FileLister(const string &startPath, bool showDirectories,
	bool showFiles) : showDirectories(showDirectories), showFiles(showFiles)
{
	setPath(startPath, false);
}

const string &FileLister::getPath()
{
	return path;
}

void FileLister::setPath(const string &path, bool doBrowse)
{
	this->path = path;

	if (this->path[path.length() - 1]!='/')
		this->path += "/";

	if (doBrowse)
		browse();
}

const string &FileLister::getFilter()
{
	return filter;
}

void FileLister::setFilter(const string &filter)
{
	this->filter = filter;
}

void FileLister::browse(bool clean)
{
	if (clean) {
		directories.clear();
		files.clear();
	}

	if (showDirectories || showFiles) {
		DIR *dirp;
		if ((dirp = opendir(path.c_str())) == NULL) {
			ERROR("Unable to open directory: %s\n", path.c_str());
			return;
		}

		vector<string> vfilter;
		split(vfilter, getFilter(), ",");

		string filepath, file;
		struct stat st;
		struct dirent *dptr;

		while ((dptr = readdir(dirp))) {
			file = dptr->d_name;

			if (file[0] == '.' && file != "..")
				continue;

			filepath = path + file;
			int statRet = stat(filepath.c_str(), &st);
			if (statRet == -1) {
				ERROR("Stat failed on '%s' with error '%s'\n", filepath.c_str(), strerror(errno));
				continue;
			}
			if (find(excludes.begin(), excludes.end(), file) != excludes.end())
				continue;

			if (S_ISDIR(st.st_mode)) {
				if (!showDirectories)
					continue;

				if (std::find(directories.begin(), directories.end(), file) != directories.end())
				  continue;

				directories.push_back(file);
			} else {
				if (!showFiles)
					continue;

				if (std::find(files.begin(), files.end(), file) != files.end())
				  continue;

				for (vector<string>::iterator it = vfilter.begin(); it != vfilter.end(); ++it) {
					if (file.find('.') == string::npos) {
						if (!it->empty())
							continue;

						files.push_back(file);
						break;
					}

					if (it->length() < file.length()) {
						if (file[file.length() - it->length() - 1] != '.')
							continue;

						string file_lowercase =
									file.substr(file.length() - it->length());

						/* XXX: This won't accept UTF-8 codes.
						 * Thanksfully file extensions shouldn't contain any. */
						transform(file_lowercase.begin(), file_lowercase.end(),
									file_lowercase.begin(), ::tolower);

						if (file_lowercase.compare(0, it->length(), *it) == 0) {
							files.push_back(file);
							break;
						}
					}
				}
			}
		}

		closedir(dirp);
		sort(files.begin(), files.end(), case_less());
		sort(directories.begin(), directories.end(), case_less());
	}
}

unsigned int FileLister::size()
{
	return files.size() + directories.size();
}

unsigned int FileLister::dirCount()
{
	return directories.size();
}

unsigned int FileLister::fileCount()
{
	return files.size();
}

string FileLister::operator[](uint x)
{
	return at(x);
}

string FileLister::at(uint x)
{
	if (x < directories.size())
		return directories[x];
	else
		return files[x-directories.size()];
}

bool FileLister::isFile(unsigned int x)
{
	return x >= directories.size() && x < size();
}

bool FileLister::isDirectory(unsigned int x)
{
	return x < directories.size();
}

void FileLister::insertFile(const string &file) {
	files.insert(files.begin(), file);
}

void FileLister::addExclude(const string &exclude) {
	excludes.push_back(exclude);
}
