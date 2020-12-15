#!/usr/bin/perl

print "trace filename (default \"pulse/tr-pulse.tr\") : ";
$input = <STDIN>;
chop($input);
print "output filename (default \"output/out-pulse-drop.tr\") : ";
$output = <STDIN>;
chop($output);

if($input eq "") { $input = "pulse/tr-pulse.tr"; }
if($output eq "") { $output = "output/out-pulse-drop.tr"; }

open (TRACE, "<$input") || die ("Can't open trace file!");
open (OUTPUT, ">$output") || die ("Can't open output file!");

while($line = <TRACE>)
{
	@attr = split(/ +/, $line);
	if($attr[6] eq "cbr") {
		if( $attr[0] eq "D" && $attr[3] eq "RTR" && $attr[4] eq "CBK") {
			print OUTPUT "$attr[1] $attr[2] $attr[5] \n"; 
		}
	}
}

close(TRACE);
close(OUTPUT);

