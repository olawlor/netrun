#!/bin/sh
# Shell script provides utilities for grading a homework assignment.
# Orion Sky Lawlor, olawlor@acm.org, 2005/10/14

echo "<hr><h2>Grading</h2>"
prog="$@"
desc="CS Problem $HWNUM"

# Run "$prog" for input "$in".
#  Exits on errors.
run_prog() {
echo "$in" | $prog > $0.out.orig 2>&1
grep -a -v TraceASM $0.out.orig > $0.out
}


# Exit while complaining about $0.diffs
bad_diffs() {
	echo '<TABLE><TR><TD CLASS="error">'
	echo "Your output for $desc is not quite right.  For the input:"
	echo "$in" | netrun/filter_htmlpre.pl 
	echo "I expected the output:"
	if [ ! -z "$outsecret" ]
	then
		echo "$outsecret" | netrun/filter_htmlpre.pl
	else
		echo "$out" | netrun/filter_htmlpre.pl
	fi
	echo "but your program returned:"
	cat $0.out | netrun/filter_htmlpre.pl
	if [ -z "$outsecret" ]
	then
		echo "Differences: (yours &lt;) (mine &gt;)"
		cat $0.diffs | netrun/filter_htmlpre.pl
	fi
	echo '</TD></TR></TABLE>'
	exit 1
}

# Run "$prog" for input "$in" and compare to "$out".
#  Exits on errors or differences.
grade_prog() {
# Run the program on this input:
run_prog

# Diff the program's output with the known-good output:
echo "$out" | diff -a $0.out - > $0.diffs
# if [ $? -ne 0 ]
if [ "$out" != "`cat $0.out`" ]
then
	bad_diffs
else
	if [ ! -z "$in" ]
	then
		echo "<p>Program works correctly for input:"
		echo "$in" | netrun/filter_htmlpre.pl		
	fi 
fi

}

# Like grade_prog, but adds read_input blather to expected output
grade_io_prog() {
	precrap=""
	for i in $in
	do
		precrap="$precrap"'Please enter an input value:
read_input> Returning '`echo $i | gawk -M '{
	b_and=$1;
	# Unfortunately, awk negative numbers are not the same as C...
        if (b_and<0) {b_and=xor(0xffFFffFFffFFffFF,-b_and)+1;}
	printf("%d (0x%X)\n",$1,b_and);
}'`'
'
	done
	out="$precrap$out"
	grade_prog
}

# Abort with message $1
bad() {
	echo "$1"
	exit 1
}

# Abort with message $1 if $2 is empty.
empty_bad() {
	[ -z "$2" ] && bad "$1"
}

# Abort with message $1 if $2 is not empty.
nonempty_bad() {
	if [ ! -z "$2" ]
	then
		echo "$1"
		echo "$2" | netrun/filter_htmlpre.pl
		exit 1
	fi
}

time_sub="foo";

# Check subroutine foo's time (less than $1 ns) and answer (exactly $2)
grade_perf() {
	t="$1"
	a="$2"
	echo "$in" | $prog > $0.out
	grep -v $time_sub $0.out > $0.out.strip
	ans=`cat $0.out.strip`
	if [ "$ans" != "$a" ]
	then
		echo "Your program gave incorrect output.  I expected<br>"
		echo "$a" | netrun/filter_htmlpre.pl
		echo " but your program returned<br>"
		echo "$ans" | netrun/filter_htmlpre.pl
		echo "$a" > correct
		echo "$ans" > yours
		echo "Differences:<br>"
		diff yours correct | netrun/filter_htmlpre.pl

		exit 1
	fi
	tperf=`grep "$time_sub:" $0.out | awk '{print $2}'`
	perf=`grep "$time_sub:" $0.out | awk '{if ($2>'$t') print("slow");  else { if (!done) { print("fast"); done=1;} } }'`
	if [ ! "$perf" = "fast" ]
	then
		echo "Sorry, $time_sub is still too slow-- $time_sub should run in $t ns/call or less; but it actually ran in $tperf ns/call.<br>"
		exit 1
	fi
}


# These two strings must match exactly; error if they don't match.
string_match() {
	if [ "$1" != "$2" ]
	then
	 echo "You seem to have modified the $3 code.  Mine:"
	 echo "$1" | netrun/filter_htmlpre.pl
	 echo "Yours:"
	 echo "$2" | netrun/filter_htmlpre.pl
	 if false
	 then  # Debugging help for me!
		echo "Characters:"
		echo "$1" | od -t c | netrun/filter_htmlpre.pl
		echo "$2" | od -t c | netrun/filter_htmlpre.pl
		echo "Differences:"
		echo "$1" > m
		echo "$2" > u
		diff -b m u | netrun/filter_htmlpre.pl
	 fi
	 bad "Sorry!"
	fi
}

# If grep for $1 in file $2 returns true, that's bad (reason $3)
grep_bad() {
	[ -r "$2" ] || bad "Can't find file '$2'--did you change languages?"
	
	if [ "$2" = "code.S" ]
	then
		# Strip out assembly comments
		matches=`grep -v netrun "$2" | sed -e 's/;.*//' | grep "$1"`
	elif [ "$2" = "code.cpp" ]
	then
		# Strip out C++ comments
		matches=`sed -e 'sX//.*XX' "$2" | grep "$1"`
	else
		matches=`grep "$1" "$2"`
	fi
	if [ "$?" -eq 0 ]
	then
		echo "Sorry, found bad values in your $2:"
		echo "$matches" | netrun/filter_htmlpre.pl
		bad "$3"
	fi
}

# Actual output is double secret, stored in fixed file
#  double_secret 493hw1_0 2019
double_secret() {
	outsecret="Program complete.  Return 'REDACTED'"
	out=`grep "$1" /netrun_answers/"$2" | awk -F: '{print $2}'`
	if [ -z "$out" ]
	then
		echo 'Correct answers not found--email <a href="mailto:lawlor@alaska.edu">Dr. Lawlor</a> to fix this problem.'
		exit 1
	fi
}


# Finished grading--all tests passed!
grade_done() {
	# The good "GRADEVAL" argument is found by netrun, and stored for grading.
	# Subtle: student code can't just printf this, because netrun/filter_htmlpre escapes it.
	echo '<TABLE><TR><TD CLASS="success" STYLE="color:#FFFFFF" GRADEVAL="@<YES!>&">'
	echo "$desc output looks OK!"
	echo '</TD></TR></TABLE>'
}
