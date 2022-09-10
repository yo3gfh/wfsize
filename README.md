## WFSIZE, a small folder/subfolder size Windows app

Copyright (c) 2022 Adrian Petrila, YO3GFH

Born from the need to check a "folder with many subfolders" size, 
and see which subfolder takes more space. Something like the Linux
du command, roughly. Built with Pelle's C compiler.

-------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

-------

**Features**

It's designed to take a single folder path from command line and
go recursively from there to calculate total and subfolder size,
presenting a list with all of them.

The console version takes a maximum "depth" (folder-in-folder) as
a second optional parameter.

The gui version is intended to work with the included Explorer shell 
extension which starts the app with the selected folder. Multiple or 
file selection is ignored. The folder crawling process starts in a
separate thread and works reasonably fast. If so desired, the process
can be interrupted. The resulting list can be sorted ascending or
descending. 

Nothing fancy, but gets the job done in under 100 KBytes :-)

**!!! IMPORTANT !!!** 

You may build as 32 or 64 bit, but UNICODE is mandatory. 

It's taylored to my own needs, modify it to suit your own. 
I'm not a professional programmer, so this isn't the best code you'll find
on the web, you have been warned :-))

All the bugs are guaranteed to be genuine, and are exclusively mine =)

