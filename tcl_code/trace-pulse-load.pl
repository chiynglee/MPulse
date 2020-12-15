#!/usr/bin/perl

print "trace filename (default \"pulse/tr-pulse.tr\") : ";
$input = <STDIN>;
chop($input);
print "output filename (default \"output/out-pulse-load.tr\") : ";
$output = <STDIN>;
chop($output);

if($input eq "") { $input = "pulse/tr-pulse.tr"; }
if($output eq "") { $output = "output/out-pulse-load.tr"; }

open (TRACE, "<$input") || die ("Can't open trace file!");
open (OUTPUT, ">$output") || die ("Can't open output file!");

while($line = <TRACE>)
{
	@attr = split(/ +/, $line);
	if( $attr[0] eq "s" || $attr[0] eq "f" ) {
		if( $attr[3] eq "RTR" && $attr[6] eq "PULSE" ) {
			@num = split(/([_])/, $attr[2]);
			@dst = split(/[]]/, $attr[26]);
			print OUTPUT "$attr[1] $num[2] $dst[0]\n";
		}
	}
}

close(TRACE);
close(OUTPUT);

