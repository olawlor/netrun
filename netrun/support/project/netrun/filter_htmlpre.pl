#!/usr/bin/perl
# Read text on stdin.
# Write HTML-escaped <pre> block on stdout.
# Orion Sky Lawlor, olawlor@acm.org, 2005/09/23 (Public Domain)
# Annoying: IRIX 6.3 Perl doesn't support "my".

%escapes=();
$escapes{'<'}="&lt;";
$escapes{'>'}="&gt;";
$escapes{'&'}="&amp;";
$escapes{'"'}="&quot;";

print '<pre style="code">'."\n";
$count=1;
foreach $line (<>) {
        $line =~ s/([<>&\"])/$escapes{$1}/eg;
        print "$line";
	$count=$count+1;
	if ($count>1000) {goto end;}
}
end:
print "</pre>\n";

