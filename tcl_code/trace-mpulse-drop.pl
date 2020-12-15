#!/usr/bin/perl

print "trace filename (default \"mpulse/tr-mpulse.tr\") : ";
$input = <STDIN>;
chop($input);
print "output filename (default \"output/out-mpulse-drop.tr\") : ";
$output = <STDIN>;
chop($output);

if($input eq "") { $input = "mpulse/tr-mpulse.tr"; }
if($output eq "") { $output = "output/out-mpulse-drop.tr"; }

open (TRACE, "<$input") || die ("Can't open trace file!");
open (OUTPUT, ">$output") || die ("Can't open output file!");

while($line = <TRACE>)
{
	@attr = split(/ +/, $line);
#	if($attr[6] eq "cbr" && $attr[2] ne "_100_" && $attr[2] le "_40_") {
	if($attr[6] eq "cbr") {
		if( $attr[0] eq "D" && $attr[3] eq "RTR") {
			print OUTPUT "$attr[1] $attr[2] $attr[5] \n"; 
		}
	}
}

close(TRACE);
close(OUTPUT);

