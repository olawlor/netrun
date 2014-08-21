#!/usr/bin/perl
# Read text on stdin.
# Write out line numbers for that text on stdout.
# Orion Sky Lawlor, olawlor@acm.org, 2005/09/23 (Public Domain)

$count=1;
foreach $line (<>) { print $count++ . "\n"; }
