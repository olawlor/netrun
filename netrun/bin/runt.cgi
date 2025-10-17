#!/usr/bin/perl -Tw
# NetRun CGI Script, with editor support
#  Orion Sky Lawlor, olawlor@acm.org, 2005-2016 (Public domain)

use strict;

require "./util.pl";
BEGIN { require "./config.pl"; }

use Sys::Syslog;
openlog 'netrun_run', '', 'local1';        # don't forget this

#use CGI qw/:standard -nosticky/;
#use CGI qw/:standard/;
use CGI qw(-utf8);

our $q = new CGI;
$q->param(); # Check for CGI errors early...
my $error = $q->cgi_error;
if ($error) { err("Error '$error' in CGI headers!"); }

$CGI::LIST_CONTEXT_WARN=0; # Avoid log clutter

# The user is taking this (single-line) action--record it.
sub journal {
	my $action="$ENV{'REMOTE_USER'} ".shift;
	my $date=`date '+%Y_%m_%d_%H_%M_%S'`;  # or localtime time
	chomp($date);
	my $info="ip '".$ENV{'REMOTE_ADDR'}."' port '".$ENV{'REMOTE_PORT'}."' agent '".$ENV{'HTTP_USER_AGENT'}."'";
	my $log="$action $date $info\n";
	open J,">>journal";
	print J $log;
	close J;
	#system("echo '$log' >> journal");
}

# from http://www.perl.com/pub/a/2002/10/01/hashes.html
  # Return the hashed value of a string: $hash = perlhash("key")
  # (Defined by the PERL_HASH macro in hv.h)
sub perlhash
  {
      my $hash = 0;
      foreach (split //, shift) {
          $hash = int($hash*33 + ord($_))&0x00ffffff;  # <- OSL modified
      }
      return $hash;
  }

# untaint this (tainted) string for use inside a single-quoted string
sub untaint_singlequote
{
	my $str=shift;
	if ($str =~ /^([\w\s~'"!#\$@%\^&*\(\)\{\}_\-+=\]\[\|\\\/:;<>,\.?]*)$/) {
		$str=$1; # Looks OK-- untaint it.
	} else {
		my_security("Single-quoted string '$str' contains invalid characters");
	}
	$str =~ s/'/'\\''/g; # quote out all the single quotes
	$str =~ s/\r//g; # get rid of annoying carriage-returns...
	return $str;
}

############## Setup
# Parse a few parameters before doing anything else:
my $rel_url=$q->url(-relative=>1);
if (length($rel_url)>5) { $rel_url="run"; }
my $script_url="$config::url_dir/$rel_url";

my $user="$ENV{'REMOTE_USER'}";
if (!$user) {$user="commandlinetest";} # For testing on the command line
if ($user =~ /^(\w[\w.]+)$/ ) {
	$user = $1; # Untaint
} else {
	my_security("User name '$user' contains invalid characters");
}
my $userdir="$config::run_dir/run/$user/";
my_mkdir($userdir);
chdir($userdir);

# Download a problem tarball:
my $tarball=$q->param('tarball');
if ($tarball) {  
	$tarball = untaint_name($tarball);
	print $q->header(-type  =>  'application/octet-stream',
		-content_disposition => "attachment; filename=$tarball.tar");
	system("cat","project.tar");
	journal("downloading $tarball");
	exit(0);
}

# Download a tarball of all saved files
my $tarballA=$q->param('tarballA');
if ($tarballA) { 
	print $q->header(-type  =>  'application/octet-stream',
		-content_disposition => "attachment; filename=netrun_all.tar.gz");
	system("ln -fs ../../bin bin");
	system("tar czf - bin/run.cgi bin/config.pl bin/util.pl saved");
	journal("download_all");
	exit(0);
}

# Display a computed image (jpeg)
my $imgdisp=$q->param('imgdisp');
if ($imgdisp) { # Display the last-saved jpeg image
	print $q->header(-type => 'image/jpeg');
	journal("display_image");
	exec("cat","last.jpg");
	exit(0);
}

# Dump grade information (READ ONLY)
my $grades=$q->param('grades');
if ($grades) {
	if ($grades =~ /^(\w[a-zA-Z0-9\/_]+)$/ ) {
		$grades = $1; # Untaint
	} else {
		my_security("Grades name '$grades' contains invalid characters");
	}
	journal("dump_grades $grades");
	print $q->header(-type  =>  'text/html', -charset => "UTF-8");
	if (-r "grades/$grades" ) {
		system("cat","grades/$grades");
		exit(0);
	} else { # Not found
		my_errlog("Grading file $grades not found","Referer=$ENV{'HTTP_REFERER'}");
		exit(0);
	}
}

############# Output HTML
# print $q->header(-type  =>  'text/html', -charset => "UTF-8");
print $q->header;  # force ISO codepage

print $q->start_html(-title=>'NetRun',
		-script=>"
//  NetRun script support.  Dr. Orion Lawlor, olawlor\@acm.org, 2005/10
// Click-on, click-off check_input checkbox:
function updateButton(pageItem) {
        div=document.getElementById('div_'+pageItem);
        but=document.getElementById('check_'+pageItem);
        if (but.checked) {div.style.display='block';}
        else {div.style.display='none';}
}

// Automatic 'Code to run:' textarea resizing:
// Adapted by Dr. Lawlor from: http://tuckey.org/textareasizer/
function getDisplayLines(strtocount) {
    var hard_lines = 0; // newline count
    var last = 0; // character index
    while ( true ) {
        last = strtocount.indexOf('\\n', last+1); // find next newline
        if ( last == -1 ) break; // no more newlines
        hard_lines ++;
    }
    var soft_lines = Math.round(strtocount.length / 80); // wrapped
    var lines = hard_lines+2;
    if ( soft_lines > lines ) lines = soft_lines;
    var max_lines=24; // FIXME: adjustable max size?
    if ( lines > max_lines ) lines = max_lines; 
    return lines;
}
function resizeForm() {
    var the_form = document.forms[0];
    for ( var x in the_form ) {
        if ( ! the_form[x] ) continue;
        if( typeof the_form[x].rows != 'number' ) continue;
        the_form[x].rows = getDisplayLines(the_form[x].value);
    }
    //setTimeout('resizeForm();', 500); // Makes firefox use CPU time continually!
}

//Tabs courtesy of Alex Ross, in turn from
// http://www.alexking.org/blog/2003/06/02/inserting-at-the-cursor-using-javascript/
function insertAtCursor(myField, myValue) {
   if (document.selection) {
      myField.focus();
      var sel = document.selection.createRange();
      sel.text = myValue;
   }
   else if (myField.selectionStart || myField.selectionStart == '0') {
      var startPos = myField.selectionStart;
      var endPos = myField.selectionEnd;
      myField.value = myField.value.substring(0, startPos) + myValue + myField.value.substring(endPos, myField.value.length);
      /* Don't let value change mess with cursor... */
      myField.selectionStart = myField.selectionEnd = endPos+1;
   } else {
      myField.value += myValue;
   }
}

// Insert tab into textbox everywhere;
//  also intercepts the tab character on IE.
function InsertTab(obj,e) {
   //alert('InterceptTab called with '+e.keyCode);
   if (e.keyCode ==13) { resizeForm(); }
   if (e.keyCode == 9) { // catch tab.
      //alert('InterceptTabs caught a tab');
      insertAtCursor(obj, '	');
      e.cancelBubble = true; // IE....
      return false;
   }
   return true;
}
// Disables tabs on Mozilla & other DOM browsers; 
//   never even gets called on IE (where onkeypress doesn't work with tabs)
function EatTab(obj,e) {
   //alert('EatTab called with '+e.keyCode+' which '+e.which);

   if (e.keyCode ==13) { resizeForm(); }
   if (e.keyCode == 9) { // catch tab.
      if (e.stopPropogation) e.stopPropogation();
      return false;
   }
   return true;
}

function startupCode() {
	updateButton('input');
	updateButton('options');
	resizeForm();

	// Theme selection
	var themeSelect = document.getElementById('theme');
	themeSelect.value = window.localStorage.theme || 'dark';
	function updateTheme() { // make the page match themeSelect.value
		if (themeSelect.value === 'light') {
			document.body.classList.remove('dark');
			editor.setTheme('ace/theme/github');
		} else {
			document.body.classList.add('dark');
			editor.setTheme('ace/theme/twilight');
		}
	}
	
	themeSelect.addEventListener('change', () => {
		window.localStorage.theme = themeSelect.value;
		updateTheme(); // change theme
	})
	updateTheme(); // set initial theme

	// Editor syntax highlighting (by Katlyn Lorimer, 2022-09)
	var langSelect = document.getElementsByName('lang')[0];
	var langMap = { // Map netrun name to ace editor mode
		'Assembly-NASM': 'assembly_x86',
		'Assembly': 'assembly_x86',
		'Fortran 77': 'plain_text',
		'Python': 'python',
		'Python3': 'python',
		'Perl': 'perl',
		'Postscript': 'plain_text',
		'PHP': 'php',
		'Scheme': 'scheme',
		'JavaScript': 'javascript',
		'Ruby': 'ruby',
		'Bash': 'sh',
		'Prolog': 'prolog',
		'vhdl': 'vhdl'
	}
	function updateLanguage() { // Update the ace editor mode to match the selected language
		var lang = langSelect.value;
		var aceName = langMap[lang] || 'c_cpp';
		editor.getSession().setMode('ace/mode/'+aceName);
	}
	langSelect.addEventListener('change', () => {
		updateLanguage();
	})
	updateLanguage();
}

//]]></script>

<style type='text/css' media='screen'>
body.dark, .dark input, .dark textarea, .dark button, .dark select {
	color: rgb(200,200,200);
}
.dark a:link { color: rgb(100,100,255); }
.dark a:visited { color: rgb(200,0,200); }
body.dark{ 
	background-color: #141414;
}
.dark input, .dark textarea {
	background-color: #202020;
}
.dark input[type=submit], .dark input[type=text], .dark select {
	background-color: #301430;
}

/* Table styles */
td.default { /* Default style for output */
	background-color: #CCCCCC;
}
td.error { /* Errors and bad diffs */
	background-color: #FF8888;
}
td.success { /* grade_done */
	background-color: #88ffff;
}

/* Dark mode tables */
.dark td.default { /* Default style for output */
	background-color: #404040;
}
.dark td.error { /* Errors and bad diffs */
	background-color: #800000;
}
.dark td.success { /* grade_done */
	background-color: #00A080;
}

input, textarea, pre {
	font-family: monospace,sans-serif,courier;
	font-weight: 900;
}

p {margin-bottom:0;}
ul{margin-top:0;}

#ace_editor_resize {
	position: relative;
	width: 800px;
	height: 600px;
	min-width: 300px;
	min-height: 150px;
	border: 1px solid black;
}

#ace_editor {
	position: absolute;
	top: 0;
	right: 0;
	bottom: 0;
	left: 0;
}
</style>

<!-- jquery is used for resizing the ace editor area -->
<script src='ui/jquery-2.2.0.min.js'></script>
<link rel='stylesheet' href='ui/jquery-ui.min.css'>
<script src='ui/jquery-ui.min.js'></script>

<script type='text/javascript'>//<![CDATA[

",   ######### javascript ends here
		-class=>"dark", -onLoad=>"startupCode()");

# print $q->h1('UAF CS NetRun');



if ($rel_url eq "runa") { # ABET printing
 my $dir = $q->param('dir');
 if ($dir) {
  if ($dir =~ /^([\w]+[\w\/]*)$/) {
	$dir=$1; # Looks OK-- untaint it.
	foreach my $hw (<$userdir/saved/$dir/*.sav>) {
		print $q->h2("Loading " . $hw);
		&load_file($hw);
		&print_main_form();
		&compile_and_run();
		&print_end_form();
	}
  }
 }
}


print $q->start_form(-action=>$script_url,-autocomplete=>"off"); 
# autoComplete (e.g., LastPass) overwrites your new code with old code.
# POST-style always works, but not bookmarkable.
# print $q->start_form("GET"); # Bookmarkable, but doesn't work for long code...


# Load up CGI parameters from this .sav file: load_file("saved/foo.sav");
sub load_file {
	my $src=shift;
	open FILE,"<","$src" or my_security("File name '$src' does not exist");
	$q=new CGI(*FILE);
	close(FILE);
}
# Print a link to this saved file name: print "Try out ".saved_file_link("Testing");
sub saved_file_link {
	my $f=shift;
	return '<a href="'.$rel_url.'?file='.$f.'">'.$f.'</a>';
}

# File input
my $file=$q->param('file');
if ($file) {  # "file" mode: Loading up a saved input file
	$file = untaint_name($file);
	print("Loading from file '$file'<br>\n");
	load_file("saved/$file.sav");
} 

# Return a checked, untainted homework field number, like 2006_CS301/HW1/1
sub checkhwnum() {
	my $hw=$q->param('hwnum');
	if (!$hw) {return "";}
	if ($hw =~ /^([\w]+[\w\/]+)$/ ) {
		return $1; # Untaint
	} else {
		my_security("Homework field '$hw' contains invalid characters");
	}
}

# Howework load
my $hw=$q->param('hw');
if ($hw) {  # "hw" mode: Loading up a homework assignment
	if ($hw =~ /^(\w[\w+-\/]+)$/ ) {
		$hw = $1; # Untaint
	} else {
		my_security("Homework name '$hw' contains invalid characters");
	}
	my $reset=$q->param('reset');

	print("Loading homework $hw.\n");
	load_file("class/$hw.sav");

	# Stash the homework number in a hidden "hwnum" field.  This isn't ideal, frankly.
	$q->param('hwnum',$hw);

	my $name=$q->param('name');
	if ( -r "saved/$name.sav" ) {
	# Already have a saved homework with this name!  Use it instead
		if ("$reset" ne "1") {
			print("Resuming saved homework $name. <br>");
			my $oldq=$q;
			load_file("saved/$name.sav");
			my $newhw=$q->param('hwnum');
			if ($newhw ne $hw) 
			{ # Saved homework is not the one we're trying to load
				print("Saved homework $newhw does not match $hw, doing clean reset. <br>");
				$q=$oldq;
			}
		} else { # Reset to factory defaults
			print("<p>Clean reset to original instructor's starting code.");
		}
	}

	print("<br>");
} 



if ($q->param('code')) {  # Has code already:
	if (!$q->param('foo_ret')) { # No explicit return type yet: set it
		$q->param('foo_ret','int'); # default to int, for pre-2014-08 backward compat
		$q->param('foo_arg0','void'); # default
	}
}



&print_main_form();

if ($q->param('code')) { &compile_and_run(); }

&print_end_form();




#print "<ul>\n";

# Make list of saved files
print "<p>Saved files:<ul>";
my $prevpre="notreallyaprefix";
foreach my $file (<$userdir/saved/*.sav>) {
    if ( $file =~ s/(\w[\w.+-]+).sav$//g ) {
        my $f=$1;
	my $pre=$f;
	$pre =~ s/[._].*//g;
        if ($pre ne $prevpre) { # Start a new row with each new prefix
		print "<li>$pre:  ";
		$prevpre=$pre;
	}
	print saved_file_link($f)."\n";
    }
}
print "</ul>\n";

# Print out one homework assignment
sub print_hw {
    $hw=shift;
    my $hw_short=endname($hw);
    print "<li><a name=\"$hw_short\">".endname($hw);
    $hw =~ m/(.*)/; $hw=$1;
    my $info="$hw/info.html";
    if (-r $info) {print(" ".my_cat($info)."<br>\n");}
    print " Problems: ";
    foreach my $prob (<$hw/*\.sav>) {
      $prob =~ m/class\/([\w\/]*)\.sav/;
      my $hwnum = $1;  
      $prob =~ m/([\w]+)\.sav$/;
      $prob=$1;
      print "\n".'<a href="',$rel_url,'?hw=',$hwnum,'">',$prob,"</a>";
      my $hw_und=slash_to_underscore($hwnum);
      if ( -r "hw/$hw_und" ) { print ' OK! &nbsp; '; }
    }
}

# Make list of classes, and homework assignments in each class
foreach my $class (reverse <$userdir/class/*>) {
  if ($class =~ m/($userdir\/class\/[\w\/]*)/ ) {
    $class = $1;
  } else { my_security("Class directory $class invalid!");}
  print "\n<p>".my_cat("$class/info.html")."\n<ul>\n";
  if ($rel_url eq "runt") {
   foreach my $hw (reverse <$class/tHW*>) {
      print_hw($hw);
    }
   foreach my $hw (reverse <$class/t[MFL]*>) {
      print_hw($hw);
    }
  }
  foreach my $hw (reverse <$class/HW1[0-9]*>) {
    print_hw($hw);
  }
  foreach my $hw (reverse <$class/HW[2-9]*>) {
    print_hw($hw);
  }
  foreach my $hw (reverse <$class/HW[01]>) {
    print_hw($hw);
  }
  foreach my $hw (reverse <$class/[MFL]*>) {
    print_hw($hw);
  }
  foreach my $hw (reverse <$class/[0-9]*>) {
    print_hw($hw);
  }
  if ( -d "$class/Example" ) {print_hw("$class/Example");}
  print "</ul>\n";
}
#print "</ul>\n";



print '<p><a href="help.html">NetRun Help</a> and <a href="examples.html">Examples</a>';

if ($q->param('name')) {
  print '<p><a href="'."$rel_url?tarball=".$q->param('name').'">Download this file as a .tar archive</a>';
}
  print '<p><a href="'."$rel_url".'?tarballA=1">Download all saved files as a big .tar.gz archive</a>';

# Make it possible for me to save runs as a big URL...
if ($q->param()) {
	my $url="$config::url_dir/run?" .
		'name='."Testing". #  uri_escape($q->param('name')). # Don't overwrite...
		&param_to_cgiarg('code').&param_to_cgiarg('lang').
		&param_to_cgiarg('mach').&param_to_cgiarg('mode').
		&param_to_cgiarg('input').&param_to_cgiarg('check_input').
		&param_to_cgiarg('linkwith').
		&param_to_cgiarg('foo_ret').&param_to_cgiarg('foo_arg0').
		&param_to_cgiarg('orun'). &param_to_cgiarg('ocompile');
	if ($rel_url eq "runt") { # Lecture-notes copy-and-pastable version
		use HTML::Entities; # <- needed for printable HTML
		print '<hr>Code as run:<pre>'.encode_entities($q->param('code')).'</pre><p><a href="',$url,'">(Try this in NetRun now!)</a>';
	} elsif (length($url) < 8000) { # Fits in one URL 
		print '<p><a href="',$url,'">Bookmarkable Code URL (as run)</a>';
	}
}

print
	$config::end_page;

# Convert a CGI parameter to a CGI argument.
sub param_to_cgiarg {
	use URI::Escape;
	my $argname=shift;
	my $ret="";
	foreach my $arg ($q->param($argname)) { 
		$ret .= "&$argname=" . uri_escape($arg); 
	}
	return $ret;
}


########################### main_form ##############################
sub print_main_form {
	print
		'<TABLE BORDER=1><TR><TD VALIGN=top COLSPAN=2>',"\n";

	print
		'<DIV STYLE="width: 95vw">',
		'<TABLE BORDER=0 WIDTH=100%><TR><TD>Run name:    ',
		$q->textfield(-name=>'name',-default=>"Testing"),
		"\n</TD><TD align=right>UAF CS NetRun</TD></TR></TABLE> \n";

	if ($rel_url eq "runh") { # Homework prep:
		print "Homework: ",$q->textfield(-name=>'hwnum',-default=>"2010_CSXXX/tHWY/Z"),"<br>\n";

		my $hwfile="instructor/".checkhwnum();
		my $hwdir=$hwfile; 
		$hwdir =~ m/(.*)\/[0-9]+/; $hwdir=$1;
		print "Checking for homework dir '$hwdir'...<br>\n";
		if (-d $hwdir) { # Dump homework info to files:
			open(HTML,">$hwfile.html") or err("Cannot create $hwfile.html");
                        print HTML $q->param("hwhtml");
                        close HTML;

			open(SAV,">$hwfile.sav") or err("Cannot create $hwfile.sav");
			$q->save(*SAV);
                        close SAV;

			open(GRD,">$hwfile.grd") or err("Cannot create $hwfile.grd");
			print GRD "#!/bin/sh\n. netrun/grade_util.sh\n\n";
                        my $i=1;
			while ($q->param("gradeout$i") && length($q->param("gradeout$i"))>0) {
				print GRD "in='".untaint_singlequote($q->param("gradein$i"))."'\n";
				print GRD "out='".untaint_singlequote($q->param("gradeout$i"))."'\n";
				print GRD "grade_prog\n\n";
				$i=$i+1;
			} 
			print GRD "grade_done\n";
                        close GRD;
			chmod 0755,"$hwfile.grd"
		}
	}

	
	if ($q->param('hwnum')) {
	# They're trying to solve a homework here.
		my $hwnum=checkhwnum();
		if (-e "class/$hwnum.html") {
		# Print out the homework description
		print "$hwnum Homework Instructions: ".my_cat("class/$hwnum.html")."<br>\n";
		}
	}

	if ($rel_url eq "runh") { # Homework prep:
		print "Problem Instructions (HTML):<br></DIV>\n",
			$q->textarea(-name=>'hwhtml',-columns=>85,-rows=>4,
				-onkeydown=>"javascript:return InsertTab(this,event);", 
				-onkeypress=>"javascript:return EatTab(this,event);", 
				-default=>"For this problem, you should...");
		print "<br>Grading prefix code:<br>\n",
			$q->textarea(-name=>'gradecode',-columns=>85,-rows=>3,
				-onkeydown=>"javascript:return InsertTab(this,event);", 
				-onkeypress=>"javascript:return EatTab(this,event);");
		print "<br>Student's default code:<br>\n ",
			$q->textarea(-name=>'code',-columns=>85,-rows=>3,
				-onkeydown=>"javascript:return InsertTab(this,event);", 
				-onkeypress=>"javascript:return EatTab(this,event);" 
			),"\n";
		print "<br>Grading suffix code:<br>\n",
			$q->textarea(-name=>'gradepost',-columns=>85,-rows=>3,
				-onkeydown=>"javascript:return InsertTab(this,event);", 
				-onkeypress=>"javascript:return EatTab(this,event);");
		
	} else {
		# Preserve the hidden homework fields...
		if ($q->param('hwnum')) {print $q->hidden('hwnum',$q->param('hwnum'));}
		if ($q->param('gradecode')) {print $q->hidden('gradecode',$q->param('gradecode'));}
		if ($q->param('gradepost')) {print $q->hidden('gradepost',$q->param('gradepost'));}
		
		# Format the code area
		my $numrows=3;
		if ($q->param('code')) {
			my @rows=split("\n",$q->param('code'));
			$numrows=1+scalar(@rows);
			my $numrowsmax=30;
			if ($numrows>$numrowsmax) {$numrows=$numrowsmax;}
		}
		
		print	"Code to run:<br></DIV>\n ",
			$q->textarea(-name=>'code',-columns=>85,-rows=>$numrows,
				-onkeydown=>"javascript:return InsertTab(this,event);", 
				-onkeypress=>"javascript:return EatTab(this,event);" 
			),"\n";
	}
	my $ace_support=<<'END_ACE';
<!-- Ace editor support: -->

<div id="ace_editor_resize">
    <div id="ace_editor"></div>
</div>

<script src="./ui/ace-min/ace.js" type="text/javascript" charset="utf-8"></script>
<script>
  if (ace) {
    // Hides textbox if Javascript is enabled
    var old_codebox=document.getElementsByName("code")[0];
    old_codebox.style.display = "none";

    var editor = ace.edit("ace_editor");
    editor.setOptions({fontSize:"100%"});
    if (window.localStorage.theme == 'light') {
      editor.setTheme("ace/theme/github");
    } else {
      editor.setTheme("ace/theme/twilight");
    }
    editor.setShowPrintMargin(false);
    editor.setShowInvisibles(false);
    //editor.getSession().setMode("ace/mode/c_cpp");
    editor.getSession().setUseWrapMode(false);
    editor.getSession().setUseWorker(false);
    editor.getSession().setUseSoftTabs(false);
    editor.$blockScrolling=Infinity; // stupid warning
    editor.getSession().setValue(old_codebox.value);

    // Save/restore editor size using browser localStorage
    var div_resize=$("#ace_editor_resize"); // document.getElementById("ace_editor_resize");
    var store=window.localStorage;
    var editsize=null;
    if (store) { // try to load editor size from storage
    	try { 
	    	var str=store.getItem("editsize");
	    	if (str) { // set initial size of div
	    	   var editsize=JSON.parse(str);
	    	   if (editsize.w && editsize.h) {
		    	   div_resize.css({
		    	   	width:editsize.w+"px",
		    	   	height:editsize.h+"px"
		    	   });
		    	   editor.resize();
	    	   }
	    	}
    	} catch (e) {
    		console.log("Ignoring error restoring editor size: "+e);
    	}
    }
    
    div_resize.resizable({
        resize: function( event, ui ) {
          editor.resize();
          if (store) { // try to save editor size to storage
            var editsize={w:div_resize.width(),h:div_resize.height()};
            var str=JSON.stringify(editsize);
            console.log("Storing editor size as "+str);
            store.setItem("editsize",str);
          }
        }
    });
  }
</script>
END_ACE

	print $ace_support,
		"</TD></TR></DIV><TR><TD VALIGN=top>\n";
	print '<div align=left><input type="submit" onclick="document.getElementsByName(\'code\')[0].value=editor.getSession().getValue();" name="Run It!" value="Run It!" title="[alt-shift-r]" accesskey="r"  /></div>';
	
	print
		"<p>",
		$q->checkbox_group(-name=>'check_input',
			-values=>["Input"],
			-id=>"check_input",
			-onClick=>"updateButton('input')",
			-attributes=>{-title=>"[alt-shift-i]",-accesskey=>"i"});

	print '<div id="div_input" style="display:none;padding-left:1em;">',
		$q->textarea(-name=>"input",-rows=>4,-columns=>30),
		'</div>';
	
	
	print
		"<p>",
		$q->checkbox_group(-name=>'check_options',
			-values=>["Options"],
			-id=>"check_options",
			-onClick=>"updateButton('options')");
	print '<div id="div_options" style="display:none;padding-left:1em;">';
	
	print
		"<p>Language:",
		$q->popup_menu(-name=>'lang',
			-values=>[
			'C++',
			'C++0x',
			'C++14',
			'C++17',
			'C',
			'Swift',
			'Rust',
			'Assembly-NASM',
			'Assembly',			
#			'Fortran 77',
			'OpenMP',
			'MPI',
			'CUDA',
			'GPGPU',
#			'OpenCL',
			'CBMC',
			'Python',
			'Python3',
			'Perl',
			'Postscript',
			'PHP',
			'Scheme',
			'JavaScript',
			'Ruby',
			'Bash',
			'Prolog',
#			'glfp',
#			'glsl',
#			'funk_emu',
#			'spice',
			'vhdl'],
			-labels=>{'Assembly' => 'Assembly-GNU',
				'glfp' => 'OpenGL Fragment Program',
				'glsl' => 'OpenGL Shader Language (GLSL)',
				'spice' => 'SPICE Analog Circuit',
				'C++0x' => 'C++11',
				'vhdl' => 'VHDL Digital Circuit'},
			-default=>['C++0x']),"\n";

	print
		"<p>Mode:",
		$q->popup_menu(-name=>'mode',
			-values=>['frag','file','main'],
			-labels=>{
				'frag' => 'Inside a Function',
				'file' => 'Whole Functions (file)',
				'main' => 'Whole Program (main)'
			},
			-default=>['frag']),"\n";

	print   "<p>Function: ",
		$q->popup_menu(-name=>'foo_ret',
			-values=>['void','long','int','double','float','std::string','sse_float4','sse_double2'],
			-default=>['long']),
		" foo(",
		$q->popup_menu(-name=>'foo_arg0',
			-values=>['void','long','int','double','float','std::string'],
			-default=>['void']),
		")\n";
	print
		"<p>&nbsp;&nbsp;",
		$q->checkbox_group(-name=>'repeat',
			-values=>['Re-run foo until input ends'],
			-defaults=>[]),"\n";

	print
		"<p>Machine:",
		$q->popup_menu(-name=>'mach',
			-values=>[
				'threadripper',
				'skylake64',
			#	'sandy64',
			#	'phenom64',
			#	'x64',
				'x86',
			#	'x86_2core',
			#	'x86_atom',
			#	'x86_4core',
			#	'ia64',
			#	'win32',
			#	'x86_2',
			#	'486',
			#	'Alpha',
				'ARM',
				'ARMpi4',
				'RISCV',
			#	'SPARC',
			#	'MIPS',
			#	'PPC',
			#	'PPC_EMU',
			#	'PIC'
			],
			-labels=>{
				'threadripper' => 'x86_64 Threadripper x64',
				'skylake64' => 'x86_64 Skylake x4',
				'x86' => 'x86_32 Skylake x4',
			#	'sandy64' => 'x86_64 Sandy Bridge x4',
			#	'phenom64' => 'x86_64 Phenom II x6',
			#	'x64' => 'x86_64 Q6600 x4',
			#	'x86' => 'x86 P4 x2',
			#	'x86_atom' => 'x86 Atom x1',
			#	'x86_2core' => 'x86 Core2 x2',
			#	'x86_4core' => 'x86  (Linux)',
			#	'ia64' => 'ia64 (Itanium Linux)',
			#	'win32' => 'x86 (Windows) EMULATED',
			#	'x86_2' => 'x86 dual P3 (Linux)',
			#	'486' => '486 (Ancient Linux)',
			#	'Alpha' => 'DEC Alpha (NetBSD)',
				'ARM' => 'ARM (Raspberry Pi 3)',
				'ARMpi4' => 'ARM64 (Pi 4)',
				'RISCV' => 'RISC-V (VisionFive 2)',
			#	'SPARC' => 'SPARC (Sun Ultra5 Linux)',
			#	'MIPS' => 'MIPS (SGI IRIX)',
			#	'PPC' => 'PowerPC (OS X)',
			#	'PPC_EMU' => 'PowerPC (Linux) EMULATED',
			#	'PIC' => 'PIC Microcontroller'
			},
			-default=>['threadripper']),"\n";

	print
		"<p>Compile options:",
		$q->checkbox_group(-name=>'ocompile',
			-values=>['TraceASM','Optimize','Debug','Warnings','Verbose','Execstack'],
			-defaults=>['Optimize','Warnings']),"<br>\n";

	print
		"<p>Actions:",
		$q->checkbox_group(-name=>'orun',
			-values=>['Run','Disassemble','Grade','Time','Profile'],
			-defaults=>['Run','Grade']),"\n";

	print '<p>Link with: ',
		$q->textfield(-name=>"linkwith");

	if ($q->param('hwnum')) { # add reset link
		print '<p><a href="?hw='.$q->param('hwnum').'&reset=1">Clean reset</a> this homework problem (discards your work)';
	}

    my $lang=$q->param('lang');
	if (defined($lang) && $lang eq "MPI") {
		print '<p>MPI Processes (1-20): ',
			$q->textfield(-name=>"numprocs");
	}

	print "<hr>";
	print "Version 2024-10-22 Removed Disqus (privacy leak)";
	print "</div>";

	if ($rel_url eq "runh") { # Homework prep: store correct inputs and outputs
		my $i=0;
		do {
			$i=$i+1;
			print "<hr>Grading input $i:<br>",
				$q->textarea(-name=>"gradein$i",-rows=>2,-columns=>30);
			print "<br>Expected output $i:<br>",
				$q->textarea(-name=>"gradeout$i",-rows=>4,-columns=>30);
		} while ($q->param("gradeout$i") && length($q->param("gradeout$i"))>0);
	}

	print "</TD><TD VALIGN=TOP>";

	print $q->end_form;
}


sub print_end_form {
	print  $q->hr,"Finished.",
		'</TD></TR></TABLE>';
}



########################### compile_and_run ############################
## Compile and execute code.
sub compile_and_run {
	# Create project() file
	my $proj=create_project_directory();
	
	# Create a tarball
	system("tar cf project.tar project");

	# Run project remotely and write output to a file:
	my_start_redir('log'); 
	system("$config::run_dir/bin/sandsend",
		"-f","$userdir/project.tar",
		"-u","$user",
		"$proj->{sr_host}:$proj->{sr_port}") 
		and print("<h2>ERROR!</h2> Cannot send off project (machine may be down)\n<br>");
	my_end_redir();

	my $res=my_cat("log");	

	# if ($lang eq "glfp") { 
		separate_text_binary('log','last.jpg');
	# } else {
	# Print file to screen
	#	print $res;
	# }

	# Check for grading-OK message (FIXME: multi-submit race condition here!)
	my $hwnum = checkhwnum();
	if ( $hwnum && $res =~ m/GRADEVAL="@<YES!>&">/ ) {
		my $hw_und=slash_to_underscore($hwnum);
		journal("hwok $hw_und");
		system("cp","$proj->{src}","hw/$hw_und");
	}
}


## Untaint this string, and make it like a project name
sub untaint_name {
	my $name=shift;
	if (!$name) { $name="Testing"; }

	# 'today' is the one writeable subdirectory (must be manually inserted in run/user/save)
	if ($name =~ /^today\/(\w[\w. +-]+)$/ ) {
		$name='today/'.$1; # untaint and put back
	} elsif ($name =~ /^(\w[\w. +-]+)$/ ) {
		$name = $1; # Untaint
	} else {
		my_security("Run name '$name' contains invalid characters");
	}

	# Replace spaces with underscores
	$name =~ s/ /_/g;
	return $name;
}

## Untaint this C/C++ type name, and return it
sub untaint_typename {
	my $t=$q->param(shift);
	
	if ($t =~ /^(\w[\w:._<>]*)$/ ) {
		$t = $1; # Untaint
	} else {
		my_security("Type name '$t' contains invalid characters");
	}

	return $t;
}

########################### create_project_directory ############################
## Build a Makefile and surrounding stuff for this project
sub create_project_directory {

# Extract out and security-check everything we need:
	my $code = $q->param('code');
	if (!$code) {print("You'll have to enter some code...\n"); return;}
	
	# Replace Microsoft curly quotes in code with normal ASCII quotes:
	$code =~ s/[\x93\x94]/\"/gs;
	# Replace /r/n with just /n:
	$code =~ s/[\r]//g;  # UNIX-ify newlines
	
	if ($code =~ /^([\w\s~'"!#\$@%\^&*\(\)\{\}_\-+=\]\[\|\`\\\/:;<>,\.?]+)$/) {
		$code=$1; # Looks OK-- untaint it.
	} else {
		my_security("Code '$code' contains invalid characters");
	}

	my $gradecode = $q->param('gradecode');
	if (!$gradecode) { $gradecode=""; }
	elsif ($gradecode =~ /^([\w\s~'"!#\$@%\^&*\(\)\{\}_\-+=\]\[\|\\\/:;<>,\.?]+)$/) {
		$gradecode=$1; # Looks OK-- untaint it.
	} else {
		my_security("Grade code '$gradecode' contains invalid characters");
	}
	my $gradepost = $q->param('gradepost');
	if (!$gradepost) { $gradepost=""; }
	elsif ($gradepost =~ /^([\w\s~'"!#\$@%\^&*\(\)\{\}_\-+=\]\[\|\\\/:;<>,\.?]*)$/) {
		$gradepost=$1; # Looks OK-- untaint it.
	} else {
		my_security("Grade post '$gradepost' contains invalid characters");
	}
	
	my $name=untaint_name($q->param('name'));
	
	my $mode=$q->param('mode');
	if (!$mode) { $mode="frag"; }
	if ($mode =~ /^([\w]+)$/ ) {
		$mode = $1; # Untaint
	} else {
		my_security("Run mode '$mode' contains invalid characters");
	}
	
	# lang and mach are checked for string match only
	my $lang=$q->param('lang');
	if (!$lang) { $lang="Assembly"; }
	my $mach=$q->param('mach');
	if (!$mach) { $mach="x86"; }
	if ( $mach eq "x64" ) { $mach="threadripper"; }
	
	my @ocompile=$q->param('ocompile');
	my @orun=$q->param('orun');
	#if (!@orun) { @orun=("Disassemble", "Run"); }		
# Done checking-- log and run the thing
	my $short_code;  # Syslog chokes on huge logs
	if (length($code)>=500) { $short_code=substr($code,0,500) . "...";}
	else {$short_code=$code;}
	journal("run $name len=".length($code)." hash=".perlhash($code));
	my_log("Running","user '$user' mach/lang '$mach/$lang' code '$short_code'");
	# print p,"User '$user' mach/lang '$mach/$lang' mode '$mode' run '$name'<br>";
	# print p,"Code: ",pre($code);
	
	my $srcext=""; # Extension of source code file
	my $srcpre=""; # Stuff to put before the user's code 
	my $srcpost=""; # Stuff to put after the user's code
	my $srcadd=""; # Stuff to link with the user's code
	my $toolpre=""; # Path to compiler & disassembler (& compiler prefix)
	my $compiler=""; # Name of tool used to create output
	my @cflags=(); # Flags passed to compiler
	my $linker="g++ \$(CFLAGS) "; # Used to link output
	my @lflags=(); # Linker flags
	my $disassembler="objdump -drC -M intel"; # Disassembler
	my $main="main.obj"; # Prebuilt copy of main routine (for speed)
	my $sr_host="";  # Network target for build (needed outside)
	my $sr_port="2983";
	my $saferun="netrun/safe_run.sh";
	my $srcflag="-c";
	my $outflag="-o";
	my $netrun="netrun/obj";
	my $scriptrun='/home/netrun_gpu/scriptrun ';

	# Prepare build subdirectory
	system("rm","-fr","project");
	system("cp","-r","$config::run_dir/support/project","project");
	my $hwnum=checkhwnum();
	if ($hwnum) {
	# Is a homework-- try grading as well...
		system("cp","class/$hwnum.grd","project/netrun/grade.sh");
	}
	
	# my $gpu_host="powerwall10"; # GTX 670
	my $gpu_host="skylake"; # GTX 980 Ti

	my $ret="int";
	my $arg0="void";
	my $proto="int foo(void)";
	if ($q->param('foo_ret')) {
		$ret=untaint_typename('foo_ret');
		$arg0=untaint_typename('foo_arg0');
		if ($arg0 ne "void") { $proto="$ret foo($arg0 bar)"; }
		else { $proto="$ret foo($arg0)"; }
		push(@cflags,"-DNETRUN_FOO_DECL='".$proto."'");
	}

	if ($mode eq 'main') { $main=""; } # User will write main routine

	if (grep(/^Optimize$/, @ocompile)==1) {push(@cflags,"-O1");}
	if (grep(/^Debug$/, @ocompile)==1) {push(@cflags,"-g");}
	if (grep(/^Warnings$/, @ocompile)==1) {push(@cflags,"-Wall");}
	if (grep(/^Shared$/, @ocompile)==1) {push(@cflags,"-fPIC");}
	
	if (grep(/^Run$/, @orun)==1) {$netrun="$netrun netrun/run";}
	if (grep(/^Grade$/, @orun)==1) {$netrun="$netrun netrun/grade";}
	if (grep(/^Time$/, @orun)==1) {
		if ($mode eq 'main') {
			print "Sorry, cannot time a Whole Program; ";
			print "for the Time checkbox to work, you need to run inside a function, or write a foo function. <br>\n";
		}
		else {
			push(@lflags,"-DTIME_FOO=1");
			$main="include/lib/main.cpp";
		}
	}
	if (grep(/^Re/,$q->param('repeat'))==1) {
		push(@lflags,"-DREPEAT_FOO=1");
		$main="include/lib/main.cpp";
	}
	if (grep(/^Profile$/, @orun)==1) {
		push(@cflags,"-pg");
		push(@lflags,"-pg");
		$netrun="$netrun netrun/profile";
	}
	if (grep(/^Disassemble$/, @orun)==1) {$netrun="$netrun netrun/dis";}

###############################################	
# Language switch
	if ( $lang eq "Swift") {
		@cflags=();
		push(@cflags,"-I platform/swift ");
		if (grep(/^Optimize$/, @ocompile)==1) {push(@cflags,"-O");}
		@lflags=@cflags;

		$compiler='swiftc -emit-object -parse-as-library  -module-name NetRun  $(CFLAGS) ';
		$linker="swiftc ";

		$main="platform/swift/NetRun/NetRun.swift platform/swift/main.swift ";
		$srcext="swift";
		# $netrun="netrun/swift";
		
		my $src="project/platform/swift/foo/foo.h";
		open(SRC,">$src") or err("Cannot create source file $src");
		print SRC "#define NETRUN_FOO_DECL $proto\n";
		print SRC "NETRUN_FOO_DECL;\n";
		close(SRC);

		my $swarg = "";
		if ($arg0 eq "int" or $arg0 eq "long") { $swarg = "_ bar : Int"; }
		elsif ($arg0 eq "std::string") { $swarg = "_ bar : String"; }
		elsif ($arg0 eq "float") { $swarg = "_ bar : Float"; }
		elsif ($arg0 eq "double") { $swarg = "_ bar : Double"; }
		elsif ($arg0 eq "void") { $swarg = ""; }
		else { print("Ignoring argument type $arg0<br>"); }
		
		my $swret = "";
		if ($ret eq "int" or $ret eq "long") { $swret = "-> Int"; }
		elsif ($ret eq "std::string") { $swret = "-> String"; }
		elsif ($ret eq "float") { $swret = "-> Float"; }
		elsif ($ret eq "double") { $swret = "-> Double"; }
		elsif ($ret eq "sse_float4") { $swret = "-> simd_float4"; }
		elsif ($ret eq "void") { $swret = ""; }
		else { print("Ignoring return type $ret<br>"); }
		
		$srcpre = $gradecode;
		if ($mode eq 'frag') { # Function fragment
			$srcpre=$srcpre . "import Foundation\n";
			$srcpre=$srcpre . '@_cdecl("foo")' . "\npublic func foo($swarg) $swret {\n";
			$srcpost .= "}\n" . $gradepost;

		#	$srcpost = "\n";
		#	$srcpost .= $gradepost;
		}
	}
	elsif ( $lang eq "Rust") {
		$compiler="echo ";
		$linker="echo ";
		$srcext="rs";
		$netrun="netrun/rust";
		$srcpre = $gradecode;
		if ($mode eq 'frag') { # Function fragment
			$srcpre=$srcpre . "pub fn foo() -> i64 {\n";
			$srcpost="\n";
			$srcpost .= "}\n" . $gradepost;
		}
	}
	elsif ( $lang eq "C++" or $lang eq "C++0x" or $lang eq "C++14" or $lang eq "C++17" or $lang eq "OpenMP" or $lang eq "CUDA") {  ############# C++
		$compiler='g++ $(CFLAGS)';
		if ($lang ne "CUDA" ) {
			push(@cflags,"-fopenmp"); # accept pragma omp by default
			if (substr($mach,0,3) ne "ARM" and substr($mach,0,4) ne "RISC") {
				push(@cflags,"-fcf-protection=none"); # avoid "endbr64" noise on x86 (new gcc default)
			}
		}
		if (grep(/^Profile$/, @orun)!=1) { # -pg and -fomit don't work together
			push(@cflags,"-fomit-frame-pointer");
	        }
		if ($lang eq "OpenMP") {$compiler=$linker='g++ -msse3 $(CFLAGS)';}
		if ($lang eq "C++0x") {$compiler=$linker='g++  -std=c++0x $(CFLAGS)';}
		if ($lang eq "C++14") {$compiler=$linker='g++  -std=c++14 $(CFLAGS)';}
		if ($lang eq "C++17") {$compiler=$linker='g++  -std=c++17 $(CFLAGS)';}
		$srcext="cpp";
		$srcpre='/* NetRun C++ Wrapper (Public Domain) */
#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <cstring>
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <iterator>
#include <map>
#include <string>
#include "lib/inc.h"
using std::cout;
using std::cin;

' . $gradecode;
		if ($mode eq 'main') { # whole program, with headers
			$srcpre=$gradecode;
		}
		if ($mode eq 'frag') { # Subroutine fragment
			$srcpre=$srcpre . $proto . " {\n";
			$srcpost="\n;";
		#	if ($ret ne "void") { $srcpost .= "\n  return 0;"; }
			$srcpost .= "\n}\n" . $gradepost;
		}
	}
	elsif ( $lang eq "C") { ############# C
		$compiler='gcc -fomit-frame-pointer $(CFLAGS)'; 
                push(@cflags,"-no-pie"); # avoids overflow in R_X86_64_PC32
		$srcext="c";
		$srcpre='/* NetRun C Wrapper (Public Domain) */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "lib/inc.h"

' . $gradecode;
		if ($mode eq 'frag') { # Subroutine fragment
			$srcpre=$srcpre . $proto ." {\n";
			$srcpost .= "\n}\n" . $gradepost;
		}
	}
	elsif ( $lang eq "Fortran 77") { ############### Fortran 77
		$compiler="g77"; 
		$srcext="f";
		if ($mode eq 'frag') { # Subroutine fragment
		$srcpre='
      FUNCTION foo()
      INTEGER foo;
      ';
		$srcpost='
      END FUNCTION
'. $gradepost;
		}
		# FIXME: fortran name mangling is platform-dependent!
		push(@lflags,"-Dfoo=foo_");
		$main="include/lib/main.cpp";  # Recompile with fortran foo name
	}
	elsif ( $lang eq "Haskell") { # Fails with mkTextEncoding: invalid argument
		$compiler="ghc";
		$linker="ghc";
		$srcext="hs";
		$srcpre="";
		$srcpost="";
		$main="";  # no C++ main
	}
	elsif ( $lang eq "Prolog") { 
		$compiler="gplc";
		$linker="gplc";
		@cflags=();
		@lflags=();
		$srcext="pl";
		$srcpre="";
		$srcpost="";
		$srcflag="-c";
		$main=""; # Disable main
	}
	elsif ( $lang eq "Assembly") { ################# GNU Assembly
		$compiler="as"; 
		$disassembler="objdump -drC"; # Disassemble with GNU syntax...
		$srcext="S";
		$srcpre .='
.section ".text" 
'. $gradecode .'
.globl foo
.type foo,@function
';

		if ($mode eq 'frag') { # Subroutine fragment
			$srcpre .="\nfoo:\n";
			$srcpost=$gradepost . "\nret";
		}
		$srcflag="";
		# @cflags=(); # Get rid of (C-specific) flags

	}
	elsif ( $lang eq "Assembly-NASM") { ################# NASM Assembly
		push(@cflags,"-no-pie"); # avoid relocation warning
		$compiler="nasm -f elf32 ";
		$linker='g++ -fopenmp -no-pie $(CFLAGS) ';
		$srcext="S";
		$srcpre .='
section .text
global foo
';
		if ($mode eq 'frag') { # Subroutine fragment
			$srcpre .="\nfoo:\n" . $gradecode;
			$srcpost=$gradepost . "\n";
		} else {
			$srcpre .=$gradecode;
			$srcpost.=$gradepost;
		}
		$srcflag="";

		# Add macro if we're doing TraceASM
		if (grep(/^TraceASM$/, @ocompile)==1) {
			$srcpre = '

; TraceASM support code:
section .bss align=32

global TraceASM_state
TraceASM_state:
	resq 16 ; times 16 dq 0 ; [0]: general purpose integer registers, rax - r15
	resq 32 ; times 16 dq 0,0 ; [8*16]: xmm0-15
TraceASM_state_flags:
	resq 2 ; times 2 dq 0 ; flags area (big for alignment)
TraceASM_state_end:

TraceASM_stack:
	resq 1024*1024
TraceASM_stack_end:

section .text

;  trace arguments: %1 is line number; %2 is code being executed; %3 is the next line of code.
%macro TraceASM 3

; Basic idea: show differences in machine state after each line of x86 code.
; Need a user-visible copy of all machine state:

%%save_TraceASM:
	; Save all the integer registers:
	mov QWORD[TraceASM_state+8*0],rax
	mov QWORD[TraceASM_state+8*1],rcx
	mov QWORD[TraceASM_state+8*2],rdx
	mov QWORD[TraceASM_state+8*3],rbx
	mov QWORD[TraceASM_state+8*4],rsp
	mov QWORD[TraceASM_state+8*5],rbp
	mov QWORD[TraceASM_state+8*6],rsi
	mov QWORD[TraceASM_state+8*7],rdi
	mov QWORD[TraceASM_state+8*8],r8
	mov QWORD[TraceASM_state+8*9],r9
	mov QWORD[TraceASM_state+8*10],r10
	mov QWORD[TraceASM_state+8*11],r11
	mov QWORD[TraceASM_state+8*12],r12
	mov QWORD[TraceASM_state+8*13],r13
	mov QWORD[TraceASM_state+8*14],r14
	mov QWORD[TraceASM_state+8*15],r15
	
	; Save all the floating point registers
	movaps [TraceASM_state+8*16+16*0],xmm0
	movaps [TraceASM_state+8*16+16*1],xmm1
	movaps [TraceASM_state+8*16+16*2],xmm2
	movaps [TraceASM_state+8*16+16*3],xmm3
	movaps [TraceASM_state+8*16+16*4],xmm4
	movaps [TraceASM_state+8*16+16*5],xmm5
	movaps [TraceASM_state+8*16+16*6],xmm6
	movaps [TraceASM_state+8*16+16*7],xmm7
	movaps [TraceASM_state+8*16+16*8],xmm8
	movaps [TraceASM_state+8*16+16*9],xmm9
	movaps [TraceASM_state+8*16+16*10],xmm10
	movaps [TraceASM_state+8*16+16*11],xmm11
	movaps [TraceASM_state+8*16+16*12],xmm12
	movaps [TraceASM_state+8*16+16*13],xmm13
	movaps [TraceASM_state+8*16+16*14],xmm14
	movaps [TraceASM_state+8*16+16*15],xmm15
	
	
	; Switch stacks to the tracer stack 
	;  (avoids trashing user stack while printing)
	mov rsp,TraceASM_stack_end
	
	; Save flags (mov above does not change flags)
	pushf
	pop rax
	mov QWORD[TraceASM_state_flags],rax

	mov rdi,%1,
	mov rsi,%%codestring
	mov rdx,TraceASM_state
	mov rcx,TraceASM_state_end-TraceASM_state
	mov r8,%%codestring_next
	extern TraceASM_cside
	call TraceASM_cside
	jmp %%restore_TraceASM

%%codestring: db %2,0
%%codestring_next: db %3,0

	
; Restore the machine from the single TraceASM_state:
%%restore_TraceASM:
	; Restore flags
	mov rax,QWORD[TraceASM_state_flags]
	push rax
	popf
	
	; Restore all the integer registers:
	mov rax,QWORD[TraceASM_state+8*0]
	mov rcx,QWORD[TraceASM_state+8*1]
	mov rdx,QWORD[TraceASM_state+8*2]
	mov rbx,QWORD[TraceASM_state+8*3]
	mov rsp,QWORD[TraceASM_state+8*4]
	mov rbp,QWORD[TraceASM_state+8*5]
	mov rsi,QWORD[TraceASM_state+8*6]
	mov rdi,QWORD[TraceASM_state+8*7]
	mov r8,QWORD[TraceASM_state+8*8]
	mov r9,QWORD[TraceASM_state+8*9]
	mov r10,QWORD[TraceASM_state+8*10]
	mov r11,QWORD[TraceASM_state+8*11]
	mov r12,QWORD[TraceASM_state+8*12]
	mov r13,QWORD[TraceASM_state+8*13]
	mov r14,QWORD[TraceASM_state+8*14]
	mov r15,QWORD[TraceASM_state+8*15]
	
	; Restore all the floating point registers
	movaps xmm0, [TraceASM_state+8*16+16*0]
	movaps xmm1, [TraceASM_state+8*16+16*1]
	movaps xmm2, [TraceASM_state+8*16+16*2]
	movaps xmm3, [TraceASM_state+8*16+16*3]
	movaps xmm4, [TraceASM_state+8*16+16*4]
	movaps xmm5, [TraceASM_state+8*16+16*5]
	movaps xmm6, [TraceASM_state+8*16+16*6]
	movaps xmm7, [TraceASM_state+8*16+16*7]
	movaps xmm8, [TraceASM_state+8*16+16*8]
	movaps xmm9, [TraceASM_state+8*16+16*9]
	movaps xmm10, [TraceASM_state+8*16+16*10]
	movaps xmm11, [TraceASM_state+8*16+16*11]
	movaps xmm12, [TraceASM_state+8*16+16*12]
	movaps xmm13, [TraceASM_state+8*16+16*13]
	movaps xmm14, [TraceASM_state+8*16+16*14]
	movaps xmm15, [TraceASM_state+8*16+16*15]

%endmacro
' . $srcpre;

			addTraceASM($code);	
		}

	}
	elsif ( $lang eq "MPI") {
		$sr_host="powerwall0";
		$srcext='cpp';
		$compiler='mpiCC -msse3 $(CFLAGS)';
		$linker=$compiler;

		my $numprocs=2; # Sanity-check the processor count:
		if ($q->param('numprocs') =~ /^([0-9]+)$/) {
			if ($1>=1 && $1<=20) {
				$numprocs=$1;
			}
		}

		$saferun="netrun/safe_MPI.sh $numprocs ";
	}
	elsif ( $lang eq "GPGPU") {
		$sr_host=$gpu_host;
		$srcext='cpp';
		$compiler='g++   $(CFLAGS)';
		$linker="$compiler ";
		push(@lflags,"/usr/lib/libglut_plain/libglut.a"); 
		push(@lflags,"/usr/lib/libGLEW.a");
		push(@lflags,"-lGLU -lGL");
		$compiler="$compiler -c";
		$saferun="netrun/safe_CUDA.sh ";
	}
	elsif ( $lang eq "OpenCL") {
		$sr_host=$gpu_host;
		$srcext='cpp';
		$compiler='g++   $(CFLAGS)';
		$linker="$compiler ";
		system("cp /usr/local/include/epgpu.* project/");
		push(@lflags,"-L/usr/local/cuda/include/"); 
		push(@cflags,"-I/usr/local/cuda/include/");
		push(@lflags,"-L/usr/lib/x86_64-linux-gnu/ -lGL -lGLU -lGLEW -lglut -lOpenCL");
		$compiler="$compiler -c";
		$saferun="netrun/safe_CUDA.sh ";
	}
	elsif ( $lang eq "CBMC") {
		$netrun='netrun/scripting';
		$srcext='cpp';
		$saferun=$scriptrun . '/usr/bin/cbmc '
	}
	elsif ( $lang eq "Python") {
		$netrun='netrun/scripting';
		$srcext='py';
		$saferun=$scriptrun . '/usr/bin/python '
	}
	elsif ( $lang eq "Python3") {
		$netrun='netrun/scripting';
		$srcext='py';
		$saferun=$scriptrun . '/usr/bin/python3 '
	}
	elsif ( $lang eq "Perl") {
		$netrun='netrun/scripting';
		$srcext='pl';
		$saferun=$scriptrun . '/usr/bin/perl '
	}
	elsif ( $lang eq "Postscript") {
		$netrun='netrun/scripting';
		$srcext='ps';
		$saferun=$scriptrun . '/usr/bin/gs_run < '
	}
	elsif ( $lang eq "PHP") {
		$netrun='netrun/scripting';
		$srcext='php';
		$saferun=$scriptrun . '/usr/bin/php '
	}
	elsif ( $lang eq "Scheme") {
		$netrun='netrun/scripting';
		$srcext='scm';
		$srcpre .= '(define (print thing) (display thing) (display "\n"))';
		$saferun=$scriptrun . '/usr/bin/mit-scheme-x86-64 --load '
	}
	elsif ( $lang eq "JavaScript") {
		$srcpre .= "function print() {console.log.apply(null,arguments);}\n";

		$netrun='netrun/scripting';
		$srcext='js';
		$saferun=$scriptrun . '/usr/bin/nodejs  '
	}
	elsif ( $lang eq "Ruby") {
		$netrun='netrun/scripting';
		$srcext='rb';
		$saferun=$scriptrun . '/usr/bin/ruby  '
	}
	elsif ( $lang eq "Bash") {
		$netrun='netrun/scripting';
		$srcext='sh';
		$saferun=$scriptrun . '/bin/bash  '
	}
	elsif ( $lang eq "glfp") {
		$sr_host="powerwall6";
		if ($mode eq 'frag') { # Subroutine fragment
		$srcpre='!!ARBfp1.0
TEMP in,out;
MOV in,fragment.texcoord[0]; # Texture coordinate 0';
		$srcpost=$gradepost . '
MOV result.color,out;
END';
		}
		$srcext='glfp';
		$netrun='netrun/glfp';
	}
	elsif ( $lang eq "glsl") {
		$sr_host="skylake";
#		$sr_host="137.229.25.222";
		$srcpre='// GL Shading Language version 1.0 Fragment Shader
// Vertex/fragment shader interface
varying vec4 color; // Material diffuse color
varying vec3 position; // World-space coordinates of object
varying vec3 normal; // Surface normal (world space)
varying vec2 texcoords; // Texture coordinates
uniform sampler3D tex0; // input texture (noise)
uniform sampler2D tex1, tex3, tex4, tex5; // input textures (from files)
';
		if ($mode eq 'frag') { # Subroutine fragment
		$srcpre .= 'void main(void)
{';
		$srcpost=$gradepost . '
}
';
		}
		$srcext='glsl';
		$netrun='netrun/glsl';
	}
	elsif ( $lang eq "funk_emu") {
		$compiler="g++ $(CFLAGS)"; 
		$srcext="cpp";
		$srcpre='/* Here\'s the user\'s program, with bytes separated by commas... */
const int program[]={';

		# "Source code" is just a set of bytes separated by spaces.  Make them separated by commas.
		$code =~ s/[!#;].*//g;  # Remove comments
		$code =~ s/[\n\r]/ /g;  # Remove newlines
		$code =~ s/^[ 	]+//g;  # Remove leading whitespace
		$code =~ s/[ 	]+$//g;  # Remove trailing whitespace
		$code =~ s/[ 	]+/,/g;  # Whitespace to commas (makes an array of ints)

		$srcpost=my_cat("$config::run_dir/support/funk_emu_netrun.cpp");
	} 
	elsif ( $lang eq "spice") {
		$srcext='cir';
		$netrun='netrun/spice';
	}
	elsif ( $lang eq "vhdl") {
		$srcext='vhdl';
		$netrun='netrun/vhdl';
	}
	else {
		security_err("Invalid language '$lang'");
	}


	if ( $lang eq "CUDA") {
		# $sr_host=$gpu_host;
		if ($mach eq "skylake64") {
			print "NVIDIA GeForce 980 Ti (2015) and ";
		}
		if ($mach eq "threadripper") {
			print "NVIDIA GeForce 2080 Ti (2018) and ";
		}
		
		# add headers
		$srcpre='#include <cuda.h>
#include <cassert>
#include <vector>
#include <iostream>

/* Zero-initialize GPU memory (to avoid weird "output never changes" bugs) */
__global__ void gpu_mem_clear(int *mem) { 
	mem[threadIdx.x+blockIdx.x*blockDim.x]=0; 
}
class gpu_mem_clear_at_startup { public:
	gpu_mem_clear_at_startup() {
		int n=100000000;
		int *gpu_mem=0;
		cudaMalloc(&gpu_mem,n*sizeof(int));
		gpu_mem_clear<<< n/256, 256 >>>(gpu_mem);
		cudaFree(gpu_mem);
	}
} gpu_mem_clear_at_startup_singleton;

' . $srcpre;
		$srcext='cu';
		$compiler='nvcc -std=c++14  -keep $(CFLAGS)';
		$linker="$compiler -Xlinker -R/usr/local/cuda/lib  -lcurand_static   -lculibos  ";
		$disassembler="cat code.ptx; echo ";
		# @cflags=();  # -Wall kills it
		@cflags = grep { $_ ne "-Wall" and $_ ne "-fomit-frame-pointer" } @cflags;
		$srcflag="-c";
		$saferun="netrun/safe_CUDA.sh ";
	}

	if ( $srcpost eq "") {
		$srcpost = $srcpost . $gradepost;
	}

	# Machine switch
	if ( $sr_host ne "" ) { # already set--nothing to do.
		
	} elsif ($netrun eq "netrun/scripting") { # The one scripting host
		$sr_host="skylake"; # $sr_port=2984;
	}
	elsif ($mach eq "x86") {
	print "Intel Skylake i7 6700K at (4.0GHz, 4 cores, 32-bit mode) <br>\n";
		$sr_host="skylake";
		$saferun="netrun/safe_run32.sh";
		push(@cflags,"-m32");
		if ( $lang eq "Assembly" ) { $srcpost='ret' . $gradepost; }
		if ( $lang eq "Assembly-NASM" ) { $srcpost='ret' . $gradepost; }
	} elsif ($mach eq "skylake64") {
	print "Intel Skylake i7 6700K at (4.0GHz, 4 cores) <br>\n";
		
		$sr_host="skylake";
		if ( $lang eq "Assembly" ) { $srcpost='ret'; }
		if ( $lang eq "Assembly-NASM") { 
			$compiler="nasm -f elf64 ";
			$srcpost='section .text
netrun_ran_off_end:
	mov rdi,netrun_ran_off_end_string
	extern puts
	call puts
	mov rax,0
	ret ; added by netrun

section .data
netrun_ran_off_end_string: db "Your assembly needs a ret at the end."
section .text
			' . $gradepost;
		}
		if ( $lang eq "C" || $lang eq "C++" || $lang eq "C++0x" || $lang eq "C++14" || $lang eq "C++17" || $lang eq "OpenMP" ) { push(@cflags,"-no-pie -msse4.2 -mavx2 -msse2avx"); }
		if ($lang eq "OpenMP") {$compiler=$linker='g++ -fopenmp $(CFLAGS)';}
	} elsif ($mach eq "threadripper") {
	print "AMD Threadripper 3990X (64 cores)<br>\n";
		
		$sr_host="aurora"; # SSH forward
		if ( $lang eq "Assembly" ) { $srcpost='ret'; }
		if ( $lang eq "Assembly-NASM") { 
			$compiler="nasm -f elf64 ";
			$srcpost='section .text
netrun_ran_off_end:
	mov rdi,netrun_ran_off_end_string
	extern puts
	call puts wrt ..plt
	mov rax,0
	ret ; added by netrun

section .data
netrun_ran_off_end_string: db "Your assembly needs a ret at the end."
section .text
';
			if (grep(/^Execstack$/, @ocompile)!=1) {
				$srcpost = $srcpost . '
				; Linux needs this to make the stack not be executable:
				section .note.GNU-stack noalloc noexec nowrite progbits
section .text

';
			}

			$srcpost = $srcpost . $gradepost;
		}
		if ( $lang eq "C" || $lang eq "C++" || $lang eq "C++0x" || $lang eq "C++14" || $lang eq "C++17" || $lang eq "OpenMP" ) { push(@cflags,"-no-pie -msse4.2 -mavx2 -msse2avx"); }
		if ($lang eq "OpenMP") {$compiler=$linker='g++ -fopenmp $(CFLAGS)';}
	} elsif ($mach eq "sandy64") {
	print "Intel Sandy Bridge i5 2400 (3.1GHz, 4 cores)<br>\n";
		$sr_host="sandy";
		if ( $lang eq "Assembly-NASM") { $compiler="nasm -f elf64 ";}
		if ( $lang eq "Assembly" ) { $srcpost='ret'; }
		if ( $lang eq "C" || $lang eq "C++" || $lang eq "OpenMP" ) { push(@cflags,"-msse4.2 -mavx -msse2avx"); }
		if ($lang eq "OpenMP") {$compiler=$linker='g++ -fopenmp $(CFLAGS)';}
	} elsif ($mach eq "phenom64") {
	print "FYI-- This is a six-core AMD Phenom II.<br>\n";
		$sr_host="phenom";
		if ( $lang eq "Assembly-NASM") { $compiler="nasm -f elf64 ";}
		if ( $lang eq "Assembly" ) { $srcpost='ret'; }
		if ( $lang eq "C" || $lang eq "C++" ) { push(@cflags,"-msse4a -m3dnow"); }
		if ($lang eq "OpenMP") {$compiler=$linker='g++ -fopenmp $(CFLAGS)';}
	} elsif ( $mach eq "x86_atom") {
	print "FYI-- This is a 1.6Ghz Intel Atom 330 dual-core machine.<br>\n";
		if ( $lang eq "Assembly" ) { $srcpost='ret'; }
		if ( $lang eq "C" || $lang eq "C++" ) { push(@cflags,"-msse3"); }
		$sr_host="atomic";
	} elsif ( $mach eq "x86_2") {
	print "FYI-- This is a two CPU 1133MHz Intel Pentium III Xeon machine.<br>\n";
		if ( $lang eq "Assembly" ) { $srcpost='ret'; }
		if ( $lang eq "C" || $lang eq "C++" ) { push(@cflags,"-msse2"); }
		$sr_host="poweredge";
	} elsif ( $mach eq "x86_2core") {
	print "FYI-- This is a dual-core 1.8GHz Intel Core2 Duo machine.<br>\n";
		if ( $lang eq "Assembly" ) { $srcpost='ret'; }
		if ( $lang eq "C" || $lang eq "C++" ) { push(@cflags,"-msse3"); }
		$sr_host="powerwall8";
	} elsif ( $mach eq "x86_4core") {
	print "FYI-- This is a quad-core 2.4GHz Intel Core2 Q6600 machine.<br>\n";
		if ( $lang eq "Assembly" ) { $srcpost='ret'; }
		if ( $lang eq "C" || $lang eq "C++" ) { push(@cflags,"-msse3"); }
		$sr_host="viz1";
	} elsif ( $mach eq "x64") {
#	print "WARNING-- x64 machine is emulated with QEMU, and may be slow (10 seconds)<br>\n";
#		$sr_host="localhost";
#		$sr_port="9923";
		if ( $lang eq "Assembly-NASM") { $compiler="nasm -f elf64 ";}
		if ( $lang eq "Assembly" ) { $srcpost='ret'; }
		if ( $lang eq "C" || $lang eq "C++" ) { push(@cflags,"-msse3"); }
		$sr_host="viz1";
	} elsif ( $mach eq "ia64") {
	print "FYI-- This is a two CPU 800MHz Intel Itanium machine.<br>\n";
		$sr_host="itanium1";
	} elsif ( $mach eq "486") {
	print "FYI-- This is a 50MHz Intel 486 with 12MB RAM.  Please be patient!<br>\n";
		$sr_host="orlando";
		if ( $lang eq "C++") {print "Note that C compiles faster on this slow machine.<br>\n";}
		$disassembler="objdump -drC";   # -M freaks out this version
		# Compiling full set of headers takes *forever*, so strip down.
#		$srcpre='/* NetRun Minimal C/C++ Wrapper (Public Domain) */
#include <stdio.h>
#include "lib/inc.h"
#';
	} elsif ( $mach eq "ARM" || $mach eq "ARMpi2" ) {
	print "FYI-- This is an 1.2GHz Raspberry Pi 3 (ARMv71)<br>\n";
		$sr_host="lawpi3";
		$sr_port=2983;
		push(@cflags,"-fopenmp");
		push(@cflags,"-marm");
		push(@cflags,"-mfloat-abi=hard");
		push(@cflags,"-mfpu=neon");
		if ( $lang eq "Assembly") {
			$compiler="as -mcpu=cortex-a7 -mfpu=neon "; 
		}
		$disassembler="objdump -drC";   # -M freaks out this version
		if ( $lang eq "Assembly") { ################# GNU Assembly
			$srcpre='
.syntax unified  @ no #constants
.align 2
.code 32
.text
'. $gradecode .'
.global foo
';
			if ($mode eq 'frag') { # Subroutine fragment
				$srcpre .="\nfoo:\n";
				$srcpost='bx lr'; 		
			}
		}
	} elsif ( $mach eq "ARMpi4" ) {
	print "FYI-- This is an 1.5GHz Raspberry Pi 4 (ARMv8 in 64-bit mode)<br>\n";
		$sr_host="137.229.25.24";
		$sr_port=2983;
		push(@cflags,"-fPIC");
		if ( $lang eq "Assembly") {
			$compiler="as "; 
		}
		$disassembler="objdump -drC";   # -M freaks out this version
		if ( $lang eq "Assembly") { ################# GNU Assembly
			$srcpre='
.text
'. $gradecode .'
.global foo
';

		# Add macro if we're doing ARM64 TraceASM
		# (generated by ChatGPT, then cleaned up with local labels)
		# This version trimmed to only save q0..q7
		if (grep(/^TraceASM$/, @ocompile)==1) {
			$srcpre = '
.section .bss
    .balign 32
    .global TraceASM_state
TraceASM_state:
    /* General regs area: x0..x30, sp slot (32 * 8 = 256 bytes) */
    .skip 256
    /* Vector regs area: v0..v31 (32 * 16 = 512 bytes) */
    .skip 512
TraceASM_state_flags:
    .skip 8          /* one 8-byte slot for NZCV */
TraceASM_state_end:

    .balign 16
    .global TraceASM_stack
TraceASM_stack:
    /* match your x86 size: resq 1024*1024 => 1024*1024 qwords -> 8,388,608 bytes (8 MiB) */
    .skip 8388608
TraceASM_stack_end:
    .text
    .p2align 4

.altmacro

/* Macro: TraceASM line, code-string, next-code-string */
    .macro TraceASM line, code, codenext
LOCAL Lcodestring, Lcodestring_next

    /* Compute base address of TraceASM_state into x16 */
    adrp    x16, TraceASM_state
    add     x16, x16, :lo12:TraceASM_state

    /* ---------- Save general registers x0..x30 and SP ---------- */
    /* store 8-byte slots at offsets 0,8,16,... */
    /* x0..x30 (31 regs) */
    stp     x0,  x1,  [x16, #0]        /* offset 0 */
    stp     x2,  x3,  [x16, #16]       /* offset 16 */
    stp     x4,  x5,  [x16, #32]
    stp     x6,  x7,  [x16, #48]
    stp     x8,  x9,  [x16, #64]       
    stp     x10, x11, [x16, #80]
    stp     x12, x13, [x16, #96]
    stp     x14, x15, [x16, #112]
    stp     x16, x17, [x16, #128]    /* note: saves new x16 */
    stp     x18, x19, [x16, #144]
    stp     x20, x21, [x16, #160]
    stp     x22, x23, [x16, #176]
    stp     x24, x25, [x16, #192]
    stp     x26, x27, [x16, #208]
    stp     x28, x29, [x16, #224]
    str     x30,        [x16, #240]   /* x30 (LR) at offset 240 */
    /* store SP into offset 248 */
    mov     x10, sp
    str     x10,        [x16, #248]

    /* ---------- Save vector registers v0..v31 (q0..q31, 128-bit each) ---------- */
    /* vector base offset = 256 */
    /* use q-register stores: str qN, [x16, #offset] */
    str     q0,  [x16, #256]
    str     q1,  [x16, #272]
    str     q2,  [x16, #288]
    str     q3,  [x16, #304]
    str     q4,  [x16, #320]
    str     q5,  [x16, #336]
    str     q6,  [x16, #352]
    str     q7,  [x16, #368]

    /* ---------- Save flags (NZCV) ---------- */
    mrs     x11, nzcv
    str     x11, [x16, #768]     /* flags at offset 768 */

    /* ---------- Switch to tracer stack ---------- */
    adrp    x12, TraceASM_stack_end
    add     x12, x12, :lo12:TraceASM_stack_end
    mov     sp, x12

    /* ---------- Prepare arguments and call TraceASM_cside ---------- */
    /* x0: line number
       x1: pointer to code-string
       x2: pointer to TraceASM_state
       x3: size of state (TraceASM_state_end - TraceASM_state)
       x4: pointer to code-next-string
    */
    mov     x0, \line

    /* codestring addr */
    adrp    x13, Lcodestring
    add     x13, x13, :lo12:Lcodestring
    mov     x1, x13

    /* state pointer */
    mov     x2, x16

    /* compute state size into x3: x3 = TraceASM_state_end - TraceASM_state */
    adrp    x14, TraceASM_state_end
    add     x14, x14, :lo12:TraceASM_state_end
    sub     x3, x14, x16

    /* codestring_next */
    adrp    x15, Lcodestring_next
    add     x15, x15, :lo12:Lcodestring_next
    mov     x4, x15

    /* call C helper */
    bl      TraceASM_cside

.section .rodata
    /* put the string constants in the text/rodata area (unique per macro invocation) */
Lcodestring: .asciz \code
Lcodestring_next: .asciz \codenext
.text

    /* ---------- Restore flags and registers from saved state ---------- */
    /* base address again in x16 */
    adrp    x16, TraceASM_state
    add     x16, x16, :lo12:TraceASM_state

    /* restore NZCV */
    ldr     x11, [x16, #768]
    msr     nzcv, x11

    /* restore vectors (q0..q31) */
    ldr     q0,  [x16, #256]
    ldr     q1,  [x16, #272]
    ldr     q2,  [x16, #288]
    ldr     q3,  [x16, #304]
    ldr     q4,  [x16, #320]
    ldr     q5,  [x16, #336]
    ldr     q6,  [x16, #352]
    ldr     q7,  [x16, #368]

    /* restore general registers x0..x30 and SP.
       load using ldp pairs */
    ldp     x0,  x1,  [x16, #0]
    ldp     x2,  x3,  [x16, #16]
    ldp     x4,  x5,  [x16, #32]
    ldp     x6,  x7,  [x16, #48]
    ldp     x8,  x9,  [x16, #64]  
    ldp     x10, x11, [x16, #80]
    ldp     x12, x13, [x16, #96]
    ldp     x14, x15, [x16, #112]
    ldp     x16, x17, [x16, #128]  /* this will clobber x16 with saved value */
    ldp     x18, x19, [x16, #144]
    ldp     x20, x21, [x16, #160]
    ldp     x22, x23, [x16, #176]
    ldp     x24, x25, [x16, #192]
    ldp     x26, x27, [x16, #208]
    ldp     x28, x29, [x16, #224]
    ldr     x30,       [x16, #240]
    ldr     x10,       [x16, #248]
    mov     sp, x10

    /* done; fall through to the next instruction (no explicit jump) */
    .endm

' . $srcpre;
				addTraceASM($code);
			}


			if ($mode eq 'frag') { # Subroutine fragment
				$srcpre .="\nfoo:\n";
				$srcpost='ret'; 		
			}
		}
	} elsif ( $mach eq "RISCV" ) {
	print "FYI-- This is a VisionFive 2 (JH7110) RV64GC<br>\n";
		$sr_host="starfive";
		$sr_port=2983;
		push(@cflags,"-fopenmp");
		if ( $lang eq "Assembly") {
			$compiler="as "; 
		}
		$disassembler="objdump -drC";   # -M freaks out this version
		if ( $lang eq "Assembly") { ###### GNU Assembly
			$srcpre='
.text
'. $gradecode .'
.global foo
';
			if ($mode eq 'frag') { # Subroutine fragment
				$srcpre .="\nfoo:\n";
				$srcpost='ret'; 		
			}
		}
	} elsif ( $mach eq "win32") {
	print "WARNING-- Win32 machine is emulated with QEMU, and may be slow (half a minute!)<br>\n";
		@cflags=("/EHsc","/DWIN32=1");
		if (grep(/^Optimize$/, @ocompile)==1) {push(@cflags,"/O2");}
		if (grep(/^Time$/, @orun)==1) {push(@lflags,"/DTIME_FOO=1");}
		$srcflag="/c";
		$outflag="/o";
		# Run under WINE (slow, dangerous...)
		# $compiler=$linker="'/dos/Program Files/OrionDev/bin/cl.exe'";
		# $disassembler="/opt/cross/win32/bin/i386-mingw32msvc-objdump -drC";
		# Run on real or emulated windows box.
		$compiler=$linker='cl /nologo $(CFLAGS)';
		
		if ( $lang eq "C" ) {
		} elsif ( $lang eq "C++" ) {
		} else { err("Win32 $lang not supported yet--use inline assembly"); }
		$sr_host="olawlor";
		$sr_port="9943";
	} elsif ( $mach eq "MIPS") {
	print "FYI-- This is a 180MHz MIPS R5000 with 256MB RAM.<br>\n";
		$sr_host="sgi1";
		# Linking with no-abi causes error: "cannot mix PIC and non-PIC"
		if (grep(/^Run$/, @orun)==0 and grep(/^Grade$/, @orun)==0 and ($lang eq "C" or $lang eq "C++") ) {
		   print("Disassemble only: enabling less-cluttered compile mode \"-mno-abicalls\"<br>\n");
		   push(@cflags,"-mno-abicalls"); # Eliminate position-independent code clutter
		}
		if ( $lang eq "C++") {print "Note that namespaces aren't supported by this ancient g++ compiler.<br>\n";}
		if ( $lang eq "Assembly") {
			if (grep(/^Shared$/, @ocompile)==1) {
			    $compiler = "as -KPIC ";
			}
			$srcpre = "$srcpre\n.set noreorder\n";
			$srcpost = "jr \$31\nnop";
		}
		$disassembler="dis -Ch "   # IRIX disassembler
	} elsif ( $mach eq "PPC") { 
	print "FYI-- This is a 2GHz PowerPC G5 Mac.<br>\n";
		if ( $lang eq "Assembly" ) 
		{ # Special startup syntax for OS X assembler 
			$srcpost='blr'; 
			$srcpre='
.text
'. $gradecode .'
.globl _foo
';
			if ($mode eq 'frag') { # Subroutine fragment
				$srcpre .="\n_foo:\n";
				$srcpost="\nblr";
			}
		}
		$sr_host="ppc1";
	} elsif ( $mach eq "PPC_G4") { 
	print "FYI-- This is a 768MHz PowerPC G4 Mac running Linux.<br>\n";
		if ( $lang eq "Assembly" ) { $srcpost='blr'; }
		$sr_host="ppc2";
	} elsif ( $mach eq "PPC_EMU") {
	print "WARNING-- PowerPC machine is emulated with QEMU, and may be slow (10 seconds)<br>\n";
		if ( $lang eq "Assembly" ) { $srcpost='blr'; }
		$sr_host="olawlor";
		$sr_port="9933";
	}
	elsif ( $mach eq "PIC") {
	print "Patience: it takes about 10 seconds to upload a program to the PIC microcontroller...<br>\n";
		if ( $lang ne "C" ) { 
			print "Sorry, only C is supported";
			security_err("Invalid language for PIC controller");
		}
		# It's such a weird environment, almost nothing works:
		$netrun="netrun/pic";
		$disassembler='cat run/main.lst #';
		if (grep(/^Disassemble$/, @orun)==1) {$netrun="$netrun netrun/dis";}


		$srcpre="";
		$srcpost="";
		if ($lang eq "C") {
			$srcpre='#include "pic_setup.h"'
		}
		$sr_host="phenom";
		$sr_port="2883";
	}
	else {
		security_err("Invalid machine '$mach'");
	}
	
# Run it!
	# Write user's source code to a file.
	my $orig_name="$name";
	$name="code";
	my $src="project/$name.$srcext";
	open(SRC,">$src") or err("Cannot create source file $src");
	print SRC $srcpre,"\n\n",$code,"\n\n",$srcpost,"\n\n";
	close(SRC);
	
	# Write all their parameters to a .sav file
	my_mkdir("saved");
	open(SFILE,">$userdir/saved/$orig_name.sav") or err("Cannot create save file in $userdir");
	if ($orig_name =~ /^today\/(.*)/) {
		$q->param('name',$1); # remove "today/" from the name
	}
	$q->save(*SFILE);
	close(SFILE);
	system("/bin/cp","$src","$userdir/saved/$orig_name.txt");
	
	# Write their input data
	my $input="";
	if (length $q->param('input')) {
		$input="< input.txt";
		open(INPUT,">project/input.txt");
		my $inputdata=$q->param('input');
		$inputdata =~ s/\r//g; # get rid of annoying carriage-returns...
		print INPUT $inputdata;
		close(INPUT);
	}
	
	# Linkwith support:
	my $linkwith_targets="";
	my $linkwith_build="";
	my $linkwith_param=$q->param('linkwith');
	my $linkwith="";
	if ($linkwith_param and $linkwith_param =~ /^(-l[a-zA-Z0-9_.]*)$/ ) { # -lfoo
		push(@lflags,$1);
	} else { # Link with some other project
		$linkwith=untaint_name($linkwith_param);
	}
	my $linkwith_file="saved/$linkwith.sav";
	if (! $linkwith || $linkwith eq "Testing") { # Nothing to link (untaint does this...)
	}
	elsif ($linkwith eq $orig_name) {
		print "Skipping bogus self-link to '$linkwith'<br>";
	} elsif (! -r $linkwith_file) {
		print "Skipping missing link-with named '$linkwith'<br>";
	} else {# Actually linking with a real project
		print "Will link with project ".saved_file_link($linkwith)."<br>";
		$linkwith_targets='linkwith/code.obj';
		$linkwith_build='
linkwith/code.obj:
	@ echo -n "'.$linkwith.': " && cd linkwith && make -s netrun/obj
';
		# Save the old query, load and create the linkwith project:
		my $old_q=$q;
		load_file($linkwith_file);
		$q->param("linkwith",''); # Prevent infinite recursion: only one layer of links!
		$q->param("mach",$old_q->param("mach")); # Run on same machine
		chdir('project');
		create_project_directory(); # Recursively build project dir!
		system("mv project linkwith"); # Sub-project directory name
		chdir('..');
		$q=$old_q;
	}
	
	# Create a Makefile
	open(MAKEFILE,">project/Makefile") or err("Cannot create Makefile in $userdir");

	print MAKEFILE "#
# Makefile for small user program.
#    generated by NetRun, http://lawlor.cs.uaf.edu/netrun/
# NetRun by Orion Sky Lawlor, olawlor\@acm.org, 2005/09/23 (Public Domain)
#
NAME=$name
USER_CODE=\$\(NAME).$srcext
COMPILER=$compiler
CFLAGS=".join(" ",@cflags)."
LINKER=$linker
LFLAGS=".join(" ",@lflags)."
DISASSEMBLER=$disassembler
INPUT=$input
NETRUN=$netrun
MAIN=$main
HWNUM=$hwnum
STUDENT=$user
SAFERUN=$saferun
SRCFLAG=$srcflag
OUTFLAG=$outflag
LINKWITH=$linkwith_targets

all:

$linkwith_build

# All the targets are inside Makefile.post:
include Makefile.post
";
	close(MAKEFILE);
	
	# Return a hash reference to the run host, port, and source code
	my $proj={};
	$proj->{sr_host}=$sr_host;
	$proj->{sr_port}=$sr_port;
	$proj->{src}=$src;
	return $proj;
}

################ Utility Routines

# Usage: call it to add TraceASM macro invocations to each line of assembly code.
sub addTraceASM {
	my $code = $_[0];
	
	# Add TraceASM macro calls to each line of the code
	#  FIXME: don't trace section, data, strings, globals, etc.
	#   and trim comments.
	my $newcode = ""; # "TraceASM 0,\"Startup\",\"\"\n\n";
	my $lastline = "";
	my $lastlineno = 0;
	my $lineno=0;
	for my $line (split /^/, $code) {
		$lineno+=1;
		chomp($line); # trim newline

		# Save original code:
		my $nextcode = "$line\n";
		
		if ($line =~ /^([^;]*);.*/) { 
			$line=$1; # strip comments
		}
		if ($line =~ /^([^\/]*)\/\/.*/) { 
			$line=$1; # strip comments
		}
		if ($line =~ /^\s*[a-zA-Z0-9_]*\s*:(.*)/) { 
			# $line=$1; # strip labels
		}
		my $skip = 0;
		if ($line =~ /section /i) {
			$skip=1; # skip section markers
		}
		if ($line =~ /[.]/i) {
			$skip=1; # skip dotted stuff
		}
		if ($line =~ /times /i) {
			$skip=1; # skip repeated stuff
		}
		if ($line =~ /^\s*d[bwdq] /i) {
			$skip=1; # skip data declarations
		}
		if ($line =~ /['"`]/) {
			$skip=1; # skip lines with quote chars
		}				
		if ($line =~ /^\s*$/) {
			$skip=1; # skip blank lines
		}
		
		if ($skip == 0) 
		{
			$newcode .= "TraceASM $lastlineno,\"$lastline\",\"$line\"\n";
		}
		
		$newcode .= $nextcode;
		
		if ($skip == 0) 
		{
			$lastline = $line;
			$lastlineno = $lineno;
		}

	}
	$_[0] = $newcode;
}

# Usage: separate_text_binary(input_text_and_binary,output_just_binary)
sub separate_text_binary {
	my $INFILE=shift;
	my $outbin=shift;
	my $b=0;
	open(INFILE,"$INFILE") or err("Could not open input file $INFILE.");
	binmode(INFILE);
	open(BINFILE,">$outbin") or err("Could not create output file $outbin.");
	binmode(BINFILE);
	foreach my $line (<INFILE>) {
	   if ($b) { # binary mode
	       print BINFILE $line;
	   } else { # ASCII Mode
	       chomp($line);              # remove the newline from $line.
	       if ($line eq '<&@SNIP@&>') {
        	   $b=1;
	       } else { 
		   $line=~ s/\xe2\x80./\'/g; #<- Fix up silly gcc curly-quotes
        	   print "$line\n";
	       }
	   }
	}
	close(BINFILE);
}


# Debugging status:
sub my_stat {
	my $what=shift;
	#print "$what<br>\n";
}

# Start redirecting all output to this file
sub my_start_redir {
	my $outfile=shift;
	
	# (I/O redirection from: http://www.unix.org.ua/orelly/perl/cookbook/ch07_21.htm)
	# take copies of the file descriptors
	open(OLDOUT, ">&STDOUT");
	open(OLDERR, ">&STDERR");

	# redirect stdout and stderr
	open(STDOUT, ">$outfile")  or die "Can't redirect stdout: $!";
	open(STDERR, ">&STDOUT")  or die "Can't dup stdout: $!";
}

# Stop redirecting all output to the file
sub my_end_redir {
	# close the redirected filehandles
	close(STDOUT)                       or die "Can't close STDOUT: $!";
	close(STDERR)                       or die "Can't close STDERR: $!";

	# restore stdout and stderr
	open(STDERR, ">&OLDERR")            or die "Can't restore stderr: $!";
	open(STDOUT, ">&OLDOUT")            or die "Can't restore stdout: $!";

	# avoid leaks by closing the independent copies
	close(OLDOUT)                       or die "Can't close OLDOUT: $!";
	close(OLDERR)                       or die "Can't close OLDERR: $!";
}

# Run this command, telling the user about it.
#  Usage: <what to call command> <file to print on error> <command & args>
sub my_sysuser {
	my $what=shift;
	my $src=shift;
	my $cmd=join(" ",@_);
	print "Executing $what: '$cmd'";
	
	my $outfile="output";
	
	my_start_redir($outfile);

	# run the program
	my $res=system(@_);

	my_end_redir();
	
	if ($res != 0) {
		print $q->hr,$q->h2("$what Error:");
		my_prefile($outfile,'-bg','error');
		print "While running command '$cmd' with input file '$src':";
		my_prefile($src,'-line');
		exit(0);
	} else {
		( -z $outfile ) or my_prefile($outfile);
	}
	# my_stat "Back from: '$cmd'";
}

# Exec this command, redirecting our output to "output"
sub my_exec {
	my_start_redir("output");
	exec(@_);
}

# Cat this file into a preformatted & escaped HTML <pre> block.
sub my_prefile {
	my $src=shift;
	my $linePrint=0;
	my $lineNo=0;
	my $bgColor="default";
	while (@_) {
		my $arg=shift;
		if ($arg eq "-line") {$linePrint=1;}
		if ($arg eq "-bg") {$bgColor=shift;}
	}
	
	# Escape less-than and ampersand for HTML.
	my %escapes=();
	$escapes{'<'}="&lt;";
	$escapes{'>'}="&gt;";
	$escapes{'&'}="&amp;";
	$escapes{'"'}="&quot;";
	
	print"<TABLE><TR>";
	if ($linePrint) { # Add a narrow column of line numbers
		my $lineTot=`wc -l $src`;
		print "<TD><PRE>";
		for (my $i=0;$i<$lineTot;$i++) {print((1+$i)."\n");}
		print"</PRE></TD><TD>";
	}
	
	print '<TD CLASS="'.$bgColor.'"><PRE>';
	my $line;
	open(F,"<$src") or err("Could not open file '$src'");
	foreach $line (<F>) {
		$line =~ s/([<&])/$escapes{$1}/eg;
		print $line;
	}
	print"</PRE>";
	
	print"</TD></TR></TABLE>";
}
