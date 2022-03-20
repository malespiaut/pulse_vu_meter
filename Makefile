#*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#+
#+     Internet Radio Automation & Encoding Toolkit
#+
#+     Copyright (C) 2021 by Kevin C. O'Kane
#+
#+     Kevin C. O'Kane
#+     kc.okane@gmail.com
#+     https://www.cs.uni.edu/~okane
#+     https://threadsafebooks.com/
#+
#+ This program is free software; you can redistribute it and/or modify
#+ it under the terms of the GNU General Public License as published by
#+ the Free Software Foundation; either version 2 of the License, or
#+ (at your option) any later version.
#+
#+ This program is distributed in the hope that it will be useful,
#+ but WITHOUT ANY WARRANTY; without even the implied warranty of
#+ MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#+ GNU General Public License for more details.
#+
#+ You should have received a copy of the GNU General Public License
#+ along with this program; if not, write to the Free Software
#+ Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#+
#++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#	August 14, 2021

# -w -Wfatal-errors

DEP =	sink.h \
	functions.h \
	global-variables.h \
	global-extern-variables.h \
	gtk-code.c \
	pulse.c \
	resource.xml \
	part1.glade \
	vumeter.c \
	icons/meter.png


FLAGS = -fno-diagnostics-color -w -Wfatal-errors -Wno-format -Ofast \
	-Wno-deprecated-declarations -Wno-format-security \
	`pkg-config --cflags --libs gdkmm-3.0` \
	`pkg-config --cflags --libs glibmm-2.4 giomm-2.4` \
	`pkg-config --cflags --libs gtkmm-3.0` \
	-I/usr/include/glib-2.0 \
	-lm -lfftw3 \
	-lpulse-mainloop-glib -lpulse \
	-export-dynamic 

vumeter-bin :	vumeter.o resource.o pulse.o gtk-code.o 
	gcc -o vumeter-bin vumeter.o resource.o pulse.o gtk-code.o $(LFLAGS) $(FLAGS)

vumeter.o : vumeter.c $(DEP)
	gcc -c vumeter.c $(FLAGS) 

gtk-code.o : gtk-code.c $(DEP)
	gcc -c gtk-code.c $(FLAGS) 

pulse.o : pulse.c $(DEP)
	gcc -c pulse.c $(FLAGS) 

resource.o : resource.c  part1.glade
	gcc -c resource.c $(FLAGS) 

resource.c : resource.xml part1.glade
	glib-compile-resources --target=resource.c --generate-source resource.xml


