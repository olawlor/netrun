#!/usr/bin/perl -Tw
# Allows UA students to set or reset their NetRun password
#  Orion Sky Lawlor, olawlor@acm.org, 2005/09/20 (Public domain)

use strict;

require "./util.pl";
BEGIN { require "./config.pl"; }

use Sys::Syslog;
openlog 'netrun_pwreset', '', 'local1';        # don't forget this

use CGI qw/:standard -nosticky/;
print header,
	start_html('NetRun Password Reset'),
	h1('NetRun Password Set/Reset');

param();
my $error = cgi_error;
if ($error) { my_err("Error $error on CGI request"); }

if (param()) {
## Parameters were passed in:
	if (param('name')) {
		&updatePassword;
	}
}

sub updatePassword {
	my $name=param('name');
	my $domain="alaska.edu";
	# print p,"Considering password update\n";
	if ($name =~ /([^@]*)@(.*)/ ) {
		#print p,"Splitting name into $1 and $2";
		#print h2('ERROR: @ character not allowed in username!  The "@alaska.edu" is implicit!');
		#return; # Give user another chance
		$name=$1; # e.g., "lawlor"
		$domain=$2; # e.g., "alaska.edu"
	}

	if (!($domain eq "alaska.edu" )) {
		print h2("ERROR: You must use your alaska.edu email account");
		return; # Give user another chance
	}
	
	if ((!($name =~ /^([a-zA-Z0-9_.]+)$/)) || length($name)>40) 
	{	# Really bogus name-- too long, or wierd characters
		my_security("User name '$name' contains invalid characters.");
	} 
	else { # Untaint name:
		$name=$1;
	}
	# Force name to be lowercase (email is not case sensitive)
	$name=lc $name;

	if ((!($domain =~ /^([a-zA-Z0-9_.]+)$/)) || length($domain)>40) 
	{
		my_security("Domain name '$domain' contains invalid characters.");
	} 
	else { # Untaint domain:
		$domain=$1;
	}

	
	
	my_log("Normal reset","with name='$name'");
	chdir($config::run_dir);
	my $now = localtime time;

	#Generate the random password string
	my $pw=&generate_random_string(5);
	if ($pw =~ /([a-zA-Z0-9_]*)/) { $pw=$1; }
	else { my_err("Why's password so weird? pw=$pw"); }
	
	my $dir="$config::run_dir/pwreset/$name";
	my_mkdir($dir);
	$dir="$dir/$pw";
	my_mkdir($dir);
	
	# Save the password into the .htpasswd file
	my_system("cp","$config::run_dir/.htpasswd","$dir/htpasswd.old");
	my_system("/usr/bin/htpasswd","-b","$config::run_dir/.htpasswd",$name,$pw);
	my_system("cp","$config::run_dir/.htpasswd","$dir/htpasswd.new");
	
	# Prepare an email message describing the new password
	open(EMAIL,">$dir/email") or err("open $dir/email");
	print EMAIL "The NetRun password for $name is '$pw'.
You should just copy-and-paste the password from this email
to log in to NetRun here:
	https://lawlor.cs.uaf.edu/netrun/run

If you're not using IE, and not worried about somebody stealing 
your password from this machine, you can log in using this URL:
	https://$name:$pw\@lawlor.cs.uaf.edu/netrun/run

To eventually reset your password use
	https://lawlor.cs.uaf.edu/netrun/pwreset

If you've never heard of NetRun, or you didn't request the 
password to be reset, the following machine may be trying
to break into your account, and you should forward this 
message to Dr. Lawlor at lawlor\@alaska.edu or call 907 474-7678.

Request IP address & port: $ENV{'REMOTE_ADDR'} / $ENV{'REMOTE_PORT'}
Request browser: $ENV{'HTTP_USER_AGENT'}
Request date: $now
Request email address: $name\@$domain
";
	close(EMAIL);

	open(SEND,"cat '$dir/email' | mail -s 'NetRun Password Reset' '$name\@$domain' -- -r '$config::admin_email' |"); 
	close(SEND);
	
	print p,"OK-- a NetRun account is set up for $name".'@'.$domain.". ",
		"An email will arrive shortly at this address explaining how to log in.",
		$config::end_page;
	
	exit(0);
}

## Default case: no parameters given.

print
	start_form,
	p,"This form allows you to automatically request a NetRun account or reset your NetRun account password.   Any University of Alaska student is welcome to request a NetRun account.";

print
	p,"Enter your University of Alaska email address, and press the button below: ",
	p,textfield(-name=>'name');

print p,"This button will add an account for this username, generate a random 
password, and send an email with the password to this email address.  You must use your UA email account (alaska.edu).";

print
	p,submit('Make NetRun Account');
	
print p,"The password email should arrive in under one minute.  If your
email doesn't arrive, check your spam filter and forwarding settings,
try a test email to yourself, or <a href=\"mailto:lawlor\@alaska.edu\">email
Dr. Lawlor</a>";

print p,"WARNING: Non-UA email accounts are not allowed.
Resetting the password on someone else's account will be considered
a breakin attempt!";
print pre("ip=$ENV{'REMOTE_ADDR'}; port=$ENV{'REMOTE_PORT'}; browser=$ENV{'HTTP_USER_AGENT'}");

print
	end_form,
	$config::end_page;

############### Support Routines ###################

# Return a cryptographically good random number between zero and (arg-1).
#  Only works for arg up to 65000; larger values will overflow.
sub good_rand
{
        my $max_val=shift;
        open(RF,"/dev/urandom") or return rand();
        my $val=256*ord(getc(RF))+ord(getc(RF));
	close(RF);
        # print $val." ".$max_val."\n";
        return $val*$max_val/65536;
}

# This function generates random strings of a given length
# Original written by Guy Malachi http://guymal.com
sub generate_random_string
{
        my $length_of_randomstring=shift;# the length of 
                         # the random string to generate

	# Never generate 0 or O, 1 or l or I:
        my @chars=('a'..'k','m'..'z','A'..'H','J'..'N','P'..'Z','2'..'9','_');
        my $random_string;
        foreach (1..$length_of_randomstring) 
        {
                $random_string.=$chars[good_rand(63)];
        }
        return $random_string;
}
