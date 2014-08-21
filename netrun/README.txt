Students can get a NetRun account at:
	http://lawlor.cs.uaf.edu/netrun/pwreset

You can then log in at:
	http://lawlor.cs.uaf.edu/netrun/run
or the "secret" instructor login:
	http://lawlor.cs.uaf.edu/netrun/runt
(this allows you to see the "tHW3" homework numbers)

----------- NetRun server-side setup -----------

Overall web server run path: 
	apache
	run.cgi
	sandsend
		(runs as wwwrun user)

Netrun execution server backend:
	sandserv.sh
		(starts as root, su's to "netrun" user)
	make
	... make; lots of shell scripts ...
	project/netrun/safe_run.sh
	/usr/local/bin/s4g_chroot
		(setuid, runs as root for a few moments)
	<untrusted user code.exe>
		(runs as user ID 6661313)


Setting up a new backend:
	- Add netrun user in "nobody" group:
		useradd -c "NetRun user" -g nobody -m netrun
	- Copy over serve/ directory
	- Make and install s4g_chroot as root
		cd serve/s4g_chroot
		make install
	- (If /usr/local/bin/s4g_libs and home/netrun are on different filesystems, move s4g_libs to home/netrun, and softlink from /usr/local.)
	- Make and copy over sandserv
		cd serve/sandrun
		make
		cp sandserv /home/netrun
	- Copy scripts and set up box:
		cp ../new_box/*.sh /home/netrun/
		cd /home/netrun
		./perm.sh
		./sandserv.sh &
		netstat -ntap | grep sand
	- Be SURE to add sandserv.sh to the init scripts!
		(cd /home/netrun; ./sandserv.sh) &
	- Add machine to runt.cgi, both in Machine popup and in backend $mach testing
	- Test out machine
	- Run /www_support/netrun/support/rebuild to rebuild main.obj (for speed)

On a linux box, be sure to install "yasm", and it can help debugging
if you disable stack randomization with (in rc.local):
echo 0 > /proc/sys/kernel/randomize_va_space


------------------ NetRun Files ----------------
bin/run.cgi: Main perl script
bin/runt.cgi: Copy of run.cgi, used for development
bin/config.pl: NetRun HTML parameters.  Incomplete parameterization, really.
bin/util.pl: Small utility routines.
bin/pwreset.cgi: Small perl script. Resets password in .htpasswd, pwreset/<id>, and sends email.
bin/sandsend: C++ program used to talk to sandrun.
bin/testenv.sh: Set environment variables to run NetRun from command line.   Use like ". testenv.sh; ./runt.cgi"
bin/.htaccess: Limits NetRun access to students listed in .htpasswd file.

.htpasswd: List of hashed student passwords, written by pwreset CGI.

class/<class ID>/: Files used per class
	make_gradedir.sh: run at beginning of semester with class list.  Preps student run dirs.
	students.txt: plain-text list of email addresses, used only by make_gradedir.sh.
class/<class ID>/info.html: HTML header at start of course's homeworks.
class/<class ID>/HW<num>/: Collection of homework problems:
	info.html: Problem description.
	<problem>.html: Human-readable description of what to do.
	<problem>.sav: Template for students to start with (usual netrun saved/ format)
	<problem>.sh: Shell script used for grading.


run/<UAF email>: per-student information, written by NetRun CGI.
	saved/<run ID>: .sav files for every run name ever used.
	hw/<class ID>_HW<i>_<prob>/: contains source file for last correct answer to each homework.
	class/<class ID>: per-class softlink, created by make_gradedir.sh
	grades/<class ID>: overall course grades, HTML exported by grading script.
	journal: append-only per-student log file.
	
	archive_<name>.tar: temporarily used for "Download this file" link.
	project.tar: temporarily used for sending off to sandserv.
	project: temporarily used for creating project.tar.
	log: temporary output of last run.
	last.jpg: temporary image used for OpenGL display.
	
	HW<i>/info.html: Homework due date, etc.
	HW<i>/<prob>.sav: .sav file, usually copied from prof's run/ directory.
	HW<i>/<prob>.html: HTML description of the problem, shown at top of NetRun.
	HW<i>/<prob>.grd: Grading shell script used for this problem.


doc/: HTML files served on errors

serve/: Network servers used to actually execute programs.
	new_box: shell scripts to go into /home/netrun, and run sandrun.
	sandrun: receives programs from network, runs in sandbox.
	s4g_chroot: little sandboxing application in /usr/local/bin.

support/project: files assembled by netrun and used to build and run.
support/project_sample: testing directory, containing softlinked copies of project.

old/grade: Bourne shell grading scripts, grabbed by netrun if the name matches.  Now replaced by "class" links, and "hwnum" hidden variable.

----------------------
Syntax Hilighting via GreaseMonkey

http://cgranade.googlepages.com/netrunextensor.user.js


-----------------------
------------ Course Setup ------------
Briefly, to set up a new class you'll need to:

1.) Make the "/www_support/netrun/class/2008_CS321" directory.
This can be owned and writeable only by you, but it's got to 
be world-readable for the web server to find it.

2.) Make an "info.html" header for the course, for display.

3.) Make a "students.txt" list of NetRun IDs (email 
addresses) for the make_gradedir.sh script.  You can start 
with the 301 list, since most of the students rolled over:
       /www_support/netrun/class/2007_CS301/students.txt
You can also start with the list from Blackboard.

4.) Run the "make_gradedir.sh" script, to link your new 
class directory into the students' run directories:
       cd /www_support/netrun/class
       sudo ./make_gradedir.sh 2008_CS321
You can re-run make_gradedir.sh if you change students.txt.

I always add myself to the "students.txt" list, so I can 
verify that this step added the class to the bottom of my 
NetRun list!

------------ Homework -----------
To add a homework, you create a directory named
       /www_support/netrun/class/2008_CS321/HW1
(it *has* to start with "HW" to show up).

Inside the homework directory, you add a human-readable
"info.html" file, giving the homework due date.  (NetRun
doesn't enforce due dates, but it's good documentation!)

For each homework problem, you add three files:

<problemNumber>.html is a human-readable problem 
description, shown at the top when the student is working on 
the problem.

<problemNumber>.sav is the "NetRun Save File" for the 
homework sample code.  I always prepare these by saving the 
sample code as a NetRun "run name" with the appropriate name 
(I've settled on 321hw2_3), and then just copy the file 
from my own "saved" directory:
       cp /www_support/netrun/run/ffosl/saved/321hw2_3.sav 3.sav

<problemNumber>.grd is a shell script that grades the 
student's answer, used for online grading.  These scripts 
almost always use the utility routines in the 
"grade_util.sh" script, which is at:
       /www_support/netrun/support/project/netrun/grade_util.sh
It's really easy to write a grade script that runs the 
program to verify (standard) input data against expected 
command-line output; this script verifies that the input 1 
returns 14, and input 7 returns 15.

#!/bin/sh
# Shell script that grades a homework assignment.
. netrun/grade_util.sh

in='1'
out='Program complete.  Return 14 (0xE)'
grade_prog

in='7'
out='Program complete.  Return 15 (0xF)'
grade_prog

grade_done

See the grading scripts from my classes for many more examples.


When NetRun sees the GRADEVAL="@<YES!>&" string come back 
from a run, it copies the working source code to this file:
       /www_support/netrun/run/<user>/hw/<class>_HW<num>_<problem>
The "OK!" shown in NetRun comes from the presence of this 
file.

Once the due date has passed, I go back and collect these 
"hw" files from the 12:15am backup of the next day.  NetRun 
backups are stored as tarballs in
       ~olawlor/netrun/backup/<year>_<month>_<day>.tgz
There's a new backup every night.  Even backing up 
everything everybody's ever written, it's still under 5MB/day.
I can then efficiently flip through students' homeworks.

Because students can see your half-finished homeworks, you 
can name your not-yet-assigned homework directory 
"tHW<whatever>", like:
       /www_support/netrun/class/2008_CS321/tHW1 
These "testing" homeworks only show up if you access the
"testing" netrun, http://lawlor.cs.uaf.edu/netrun/runt.
Students could theoretically access this URL to see
unfinished homeworks, although the graded versions show up
with the "tHW" prefix, so they couldn't take advantage of a
broken under-development grading script.  Once the homework 
is ready, just move tHW1 to "HW1", and everybody'll see it.


Whew!  I can see I really need to write an "admin" HTML 
interface, where you can more easily:
       - Create new courses
       - Create assignments and grading scripts
       - View results from created assignments
Sadly, at the moment you're stuck in the shell for all 
these!  Let me know if you have any problems or questions!
