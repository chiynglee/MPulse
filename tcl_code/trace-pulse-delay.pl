#!/usr/bin/perl

print "trace filename (default \"pulse/tr-pulse.tr\") : ";
$input = <STDIN>;
chop($input);
print "delay output filename (default \"output/out-pulse-delay.tr\") : ";
$output_delay = <STDIN>;
chop($output_delay);

if($input eq "") { $input = "pulse/tr-pulse.tr"; }
if($output_delay eq "") { $output_delay = "output/out-pulse-delay.tr"; }

open (TRACE, "<$input") || die ("Can't open trace file!");

@recv = ();
%send = ();
$idx = 0;
while($line = <TRACE>)
{
	@attr = split(/ +/, $line);
	if($attr[6] eq "cbr") {
		if( $attr[0] eq "r" && $attr[2] eq "_0_" && $attr[3] eq "AGT" ) {
			$recv[$idx][0] = $attr[1];
			$recv[$idx][1] = $attr[5];
			$idx++;
		}
		elsif( $attr[0] eq "s" && $attr[2] eq "_100_" && $attr[3] eq "AGT" ) {
			$send[$attr[5]] = $attr[1];
		}
	}
}

close(TRACE);

open (OUTPUT_DELAY, ">$output_delay") || die ("Can't open trace file!");

for($i = 0; $i < $idx; $i++) {
	$send_time = $send[$recv[$i][1]];
	$delay = $recv[$i][0] - $send_time;
#	print OUTPUT_DELAY "$recv[$i][1] $send_time $recv[$i][0] $delay\n";
	print OUTPUT_DELAY "$recv[$i][1] $delay\n";
}

close (OUTPUT_DELAY);

