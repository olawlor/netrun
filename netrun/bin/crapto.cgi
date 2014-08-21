#!/usr/bin/perl -Tw
# crApto: cryto that's made to be broken.
#  Dr. Orion Sky Lawlor, lawlor@alaska.edu, 2013-01-24 (Public domain)

use strict;

require "./util.pl";
BEGIN { require "./config.pl"; }

use Sys::Syslog;
openlog 'netrun_run', '', 'local1';        # don't forget this

#use CGI qw/:standard -nosticky/;
#use CGI qw/:standard/;
use CGI;

our $q = new CGI;
$q->param(); # Check for CGI errors early...
my $error = $q->cgi_error;
if ($error) { err("Error '$error' in CGI headers!"); }

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
if (length($rel_url)>7) { $rel_url="crapto"; }
my $script_url="$config::url_dir/$rel_url";

my $user="$ENV{'REMOTE_USER'}";
if ($user =~ /^(\w[\w.]+)$/ ) {
	$user = $1; # Untaint
} else {
	my_security("User name '$user' contains invalid characters");
}
my $userdir="$config::run_dir/run/$user/crapto/";
my_mkdir($userdir);
chdir($userdir);

# Download a binary version of the last stream, crapto.bin
my $tarball=$q->param('binary');
if ($tarball) {  
	print $q->header(-type  =>  'application/octet-stream',
		-content_disposition => "attachment; filename=crapto.bin");
	system("cat","crapto.bin");
	journal("downloading crapto.bin");
	exit(0);
}

############# Output HTML
print $q->header;

print $q->start_html(-title=>'crApto');

print $q->start_form(-action=>$script_url);  # Always works, but not bookmarkable
# print $q->start_form("GET"); # Bookmarkable, but doesn't work for long code...

# Howework load
my $hw=$q->param('hw');
if ($hw) {  # "hw" mode: Loading up a homework assignment
	if ($hw =~ /^(\w[\w+-]+)$/ ) {
		$hw = $1; # Untaint
	} else {
		my_security("Homework name '$hw' contains invalid characters");
	}
} else {
	$hw="demo";
}

# Decrypted data load
my $decrypt=$q->param('decrypt');
if (defined $decrypt and length $decrypt) { 
	# Dump all data directly to user "input.bin" file.
	open(INBIN,">input.bin");
	print INBIN $decrypt;
	close(INBIN);
} else {
	$decrypt="";
}


&print_main_form();


########################### main_form ##############################
sub print_main_form {
	print
		'<DIV STYLE="width: 46em"><TABLE BORDER=0 WIDTH=100%><TR><TD>Homework name:    ',
		$q->textfield(-name=>'hw',-default=>"demo"),
		"\n</TD><TD align=right>crApto</TD></TR></TABLE> \n";

	my $ct="/home/crapto/hw";
	if ( -e "$ct/$hw" ) {
		journal "Run [$hw]/[$decrypt]";
		system("$ct/$hw");
	} else {
		print "--Homework not found--<br></DIV>\n";
	}
	print
		$q->textarea(-name=>'decrypt',-columns=>85,-rows=>5),"\n";

	print
		"</TD></TR><TR><TD VALIGN=top>\n";
	print '<div align=left><input type="submit" name="submit" value="Submit" title="[alt-shift-r]" accesskey="r"  /></div>';

	print "<hr>";

# Make list of homework assignments
print "Homeworks & Examples: <ul>\n";
my $prevpre="notreallyaprefix";
foreach my $file (reverse </home/crapto/hw/*>) {
    if ( $file =~ s/(\w[\w.+-]+)//g ) {
        my $f=$1;
	my $pre=$f;
	$pre =~ s/[._].*//g;
        if ($pre ne $prevpre) { # Start a new row with each new prefix
		print "<li>";
		my $info = "/home/crapto/info/$pre.html";
		if (-r $info) {print(" ".`cat $info`."<br>\n");}
		$prevpre = $pre;

		foreach my $hw (</home/crapto/hw/$pre*>) {
		  if ( $hw =~ s/(\w[\w.+-]+)//g ) {
			my $file=$1;
			my $post=$1;	
			$post =~ s/[^_]*_//g;
			print "\n".'<a href="',$rel_url,'?hw=',$file,'">',$post,"</a>";
			if ( -r "hw/$file" ) { print ' OK! &nbsp; '; }
		  }
		}
	}
    }
}
print "</ul>";

	if (1) {
		print "Announcements:
	<UL>
		<li>Homework list & OK (2013-01-24)
		<li>Freshly created! (2013-01-18)
	</UL>
	";
	}
	print "Version 2013-01-18";
	print "</div>";
}

