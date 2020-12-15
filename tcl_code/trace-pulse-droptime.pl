#!/usr/bin/perl

print "trace filename (default \"pulse/tr-pulse.tr\") : ";
$input = <STDIN>;
chop($input);
print "output filename (default \"output/out-pulse-droptime.tr\") : ";
$output = <STDIN>;
chop($output);

if($input eq "") { $input = "pulse/tr-pulse.tr"; }
if($output eq "") { $output = "output/out-pulse-droptime.tr"; }

open (TRACE, "<$input") || die ("Can't open trace file!");
open (OUTPUT, ">$output") || die ("Can't open output file!");

$flag = 0;
while($line = <TRACE>)
{
	@attr = split(/ +/, $line);
	if($attr[6] eq "cbr") {
		if($attr[0] eq "D" && $attr[3] eq "RTR" && $attr[4] eq "CBK") {
			if($flag == 0) {
				$drop_start = $attr[1];
				$flag = 1;
			}
		}
		elsif($attr[0] eq "r" && $attr[2] eq "_0_" && $attr[3] eq "AGT") {
			if($flag == 1) {
				$drop_end = $attr[1];
				$diff = $drop_end - $drop_start;
				print OUTPUT "$drop_start $drop_end $diff\n";
				$flag = 0;
			}
		}
	}
}

close(TRACE);
close(OUTPUT);

