/*
 *  Created: Feb 16 2019
 * 	Author: Burl26
 *
     This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Browser.H>
#include <FL/fl_ask.H>
#include "metro1.h"		// GENERATED FROM FLUID
#include "main_icon.xpm"
#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#define MAX_PATH_LENGTH 1024
#else
#define MAX_PATH_LENGTH MAX_PATH
#endif

int main(int argc, char **argv) {
	MainUI *mainui = new MainUI;

	mainui->datfile = (char *)malloc(MAX_PATH_LENGTH+1);
	// do filename check before window is initialized
	// init the UI
	if (argc > 1) {
		// file length check
		if (strlen(argv[1]) > MAX_PATH_LENGTH) {
			fl_alert("Filename is too long: '%s'", argv[1]);
			exit(-1);
		}
		strncpy(mainui->datfile, argv[1], MAX_PATH_LENGTH);
	} else {
		// get current directory
#ifdef WIN32
		GetCurrentDirectory(MAX_PATH_LENGTH - 10, mainui->datfile);
#else
		getcwd(mainui->datfile, MAX_PATH_LENGTH - 10);
		strcat(mainui->datfile,"/metro.txt");
#endif
	}
	// try to open the file in the argument
	FILE *f;
	if (NULL == (f = fopen(mainui->datfile, "r"))) {
		fl_alert("File not found in current directory.  It will be created: '%s'", mainui->datfile);
	} else fclose(f);
	Fl::visual(FL_RGB|FL_DOUBLE|FL_INDEX);
	//load icon before showing main window
	Fl_Pixmap myicon(main_icon_xpm);
	Fl_RGB_Image mainicon(&myicon, Fl_Color(0));
	mainui->mainWindow->icon(&mainicon);

	mainui->init();
	mainui->show();
	Fl::run();
	return 0;
}
