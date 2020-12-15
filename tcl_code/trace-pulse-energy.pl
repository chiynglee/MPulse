#!/usr/bin/perl

print "trace filename (default \"pulse/tr-pulse.tr\") : ";
$input = <STDIN>;
chop($input);
print "output filename (default \"output/out-pulse-energy.tr\") : ";
$output = <STDIN>;
chop($output);

if($input eq "") { $input = "pulse/tr-pulse.tr"; }
if($output eq "") { $output = "output/out-pulse-energy.tr"; }

open (TRACE, "<$input") || die ("Can't open trace file!");
open (OUTPUT, ">$output") || die ("Can't open output file!");

while($line = <TRACE>)
{
	@attr = split(/ +/, $line);
	if( $attr[0] eq "N" ) {
		@char = split(/[.]/, $attr[2]);
		if ( $char[0] ne "0" && $char[1] eq "000000" ) {
			$remain = $char[0] % 100;
			if ($remain == 0) {
				print OUTPUT "$attr[2] $attr[6]";
			}
		}
	}
}

close(TRACE);
close(OUTPUT);

