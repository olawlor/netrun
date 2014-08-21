#!/usr/bin/perl
# Configuration & Utility file
#   Use like: require 'util.pl'

$ENV{'PATH'}='/bin:/usr/bin';  # Security paranoia

# Log this situation
sub my_log {
	my $what=shift;
	my $why=shift;
	my @param;
	if ($q) {@param=$q->param();} # New object style
	else {@param=param();} # Old procedural style
	my $log="$what $why: user=$ENV{'REMOTE_USER'}; param='".	
		join("/",@param).
		"' ip=$ENV{'REMOTE_ADDR'}; port=$ENV{'REMOTE_PORT'}; browser=$ENV{'HTTP_USER_AGENT'};";
	$log =~ s/%/%%/g;
	syslog('notice', $log);
}

# Log this problem to the error log and screen.
sub my_errlog {
	my $what=shift;
	my $why=shift;
	my_log($what,$why);
	print("$what: $why\n");
}

# Stop running, log the problem, and leave.
sub my_err {
	my $why=shift;
	my_errlog("Error",$why);
	exit(1);
}
sub err {
	my_err(shift);
}

# Stop running, log the problem, and leave.
sub my_security {
	my $why=shift;
	my_errlog("Security",$why);
	print("<h1>SECURITY ERROR: $why.</h1>  Please don't do that.");
	exit(1);
}

# Execute this command, checking for errors
sub my_system {
        my $cmd=join(" ",@_);
	# print "Executing> $cmd\n";
	system(@_) and my_err("executing command $cmd");
}

# Make this directory.  Don't worry if it already exists.  Die if the make fails.
sub my_mkdir {
        my $dir=shift;
	if ( -r $dir ) { return; }
	mkdir $dir or my_err("Cannot create directory '$dir'");
}

# Return the non-directory part of this string
sub endname {
	my $n=shift;
	$n =~ m/.*\/([^\/]*)/;
	return $1;
}

# Return the contents of this file as a string
sub my_cat {
	my $f=shift;
	if ( -r $f ) { return `cat $f`; }
	else { return '';}
}	

# Replace slashes with underscores
sub slash_to_underscore {
	my $n=shift;
	$n =~ s/\//_/g;
	return $n;
}

# "use strict" requires this--
return 1;

