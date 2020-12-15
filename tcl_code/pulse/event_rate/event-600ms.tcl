# =================================================================
# Define options
#
# =================================================================
set val(chan) Channel/WirelessChannel ;# channel type
set val(prop) Propagation/TwoRayGround ;# radio-propagation model
set val(netif) Phy/WirelessPhy ;# network interface type
set val(mac) Mac/802_11 ; # Mac/SMAC ;# MAC type
set val(ifq) Queue/DropTail/PriQueue ;# interface queue type
set val(ll) LL ;# link layer type
set val(ant) Antenna/OmniAntenna ;# antenna model
set val(ifqlen) 50 ;# max packet in ifq
set val(nn) 101 ;# number of mobilenodes
set val(rp) PULSE ;# routing protocol
set val(x) 4001 ;# grid width
set val(y) 1231 ;# grid hieght
set val(energy) EnergyModel ;# energy model
set val(initeng) 3000 ;# Initial energy
set val(stop) 3600.2

Queue/DropTail/PriQueue set Prefer_Routing_Protocols 1

# specify the transmit power
# (see wireless-shadowing-vis-test.tcl for another example)
Phy/WirelessPhy set Pt_ 0.2818 ;	# 250m
# Phy/WirelessPhy set Pt_ 7.214e-3 ;	# 100m
# Phy/WirelessPhy set Pt_ 8.5872e-4 ;	# 40m
# Phy/WirelessPhy set Pt_ pow(10, 2.05)*1e-3 ;	# 24.5 dbm ~ 281.8 mw

# RTS/CTS diable
Mac/802_11 set RTSThreshold_ 3000

puts "This is a sensor network test program."

# =================================================================
# Main Program
#
# =================================================================

#
# Initialize Global Variables
#

set ns_ [new Simulator]
set tracefd [open tr-p-600ms.tr w]
$ns_ trace-all $tracefd

set namtrace [open nam-p-600ms.nam w]
$ns_ namtrace-all-wireless $namtrace $val(x) $val(y)

# set up topography object
set topo [new Topography]

$topo load_flatgrid $val(x) $val(y)

#
# Create God
#
set god_ [create-god $val(nn)]
$god_ off
$god_ allow_to_stop
$god_ num_data_types 1

#configure data channel
set chan_data [new $val(chan)]

# configure phenomenon node
$ns_ node-config \
 -adhocRouting $val(rp) \
 -llType $val(ll) \
 -macType $val(mac) \
 -ifqType $val(ifq) \
 -ifqLen $val(ifqlen) \
 -antType $val(ant) \
 -propType $val(prop) \
 -phyType $val(netif) \
 -channel $chan_data \
 -topoInstance $topo \
 -agentTrace ON \
 -routerTrace ON \
 -macTrace OFF \
 -movementTrace ON \
 -energyModel $val(energy) \
 -idlePower 0.843 \
 -rxPower 0.966 \
 -txPower 1.327 \
 -sleepPower 0.066 \
 -transitionPower 0.2 \
 -transitionTime 0.005 \
 -initialEnergy $val(initeng)

for {set i 0} {$i < $val(nn) } {incr i} {
	set node_($i) [$ns_ node]
	$node_($i) random-motion 1
	$god_ new_node $node_($i)
	$node_($i) namattach $namtrace
}

#
# Provide initial (X,Y, for now Z=0) co-ordinates for mobilenodes
#
set id 1
for {set y 1} {$y <= 5} {incr y} {
	set posY [expr {($y * 200.0) + 30.0}]
	for {set x 0} {$x < 20} {incr x} {
		set posX [expr { $x * 200.0 }]
		if { $posX == 0 } { set posX 1.0 }
		if { $posY == 0 } { set posY 1.0 }

		$node_($id) set X_ $posX
		$node_($id) set Y_ $posY
		$ns_ at 0.01 "$node_($id) setdest $posX $posY 50.0"

		incr id
	}
}

set sinkid 0

$node_($sinkid) set X_ 1.0
$node_($sinkid) set Y_ 1.0
$ns_ at 0.01 "$node_($sinkid) setdest 1.0 1.0 50.0"

#set dest format is "setdest <x> <y> <speed>"
# node_($sinkid) is the sink.
#$ns_ at 30.0 "$node_($sinkid) setdest 30.0 450.0 300.0"
#$ns_ at 100.0 "$node_($sinkid) setdest 200.0 50.0 300.0"

$ns_ at 2.0 "$node_($sinkid) setdest 2900.0 1.0 1.5"

#############################################################################

# setup UDP connections to data collection point, and attach apps
set sink [new Agent/UDP]
$ns_ attach-agent $node_($sinkid) $sink

for {set i 1} {$i < $val(nn)} {incr i} {
	set src_($i) [new Agent/UDP]
	$ns_ attach-agent $node_($i) $src_($i)
	$ns_ connect $src_($i) $sink
}

# interval = 20 ms
set num [expr $val(nn)-1]
set app_($num) [new Application/Traffic/CBR]
$app_($num) set rate_ 32kbps
$app_($num) set packetSize_ 80
$app_($num) set random_  0
$app_($num) attach-agent $src_($num)

#$ns_ at 0.2 "$app_($num) start"
#$ns_ at [expr $val(stop) - 0.1] "$app_($num) stop"

# event generate interval = 600 ms
# 5 packets per event
set stopTime 0
for {set i 0.2} {$i < $val(stop)} {set i [expr $stopTime + 0.6]} {
	$ns_ at $i "$app_($num) start"
	set stopTime [expr {$i + 0.09}]
	if {$stopTime > $val(stop)} { set stopTime [expr $val(stop) - 0.1] }
	$ns_ at $stopTime "$app_($num) stop"
}

#Tell nodes when the simulation ends

for {set i 0} {$i < $val(nn) } {incr i} {
	$ns_ at $val(stop) "$node_($i) reset";
}

$ns_ at $val(stop) "stop"
$ns_ at [expr $val(stop) + 0.1] "puts \"NS EXITING...\" ; $ns_ halt"

proc stop {} {
	global ns_ tracefd namtrace
	$ns_ flush-trace
	close $tracefd
	close $namtrace
}

#Begin command line parsing

puts "Starting Simulation..."
$ns_ run
