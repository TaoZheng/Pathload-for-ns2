#Create a simulator object
set ns [new Simulator]
#debug 1;
#Define different colors for data flows
$ns color 1 Blue
$ns color 2 Red
$ns color 3 Green
$ns color 4 Yellow

#Open the NAM trace file
set nf [open out.nam w]
$ns namtrace-all $nf

#Open a trace file

$ns trace-all [open all.out w]


#Define a 'finish' procedure
proc finish {} {
        global ns 
        $ns flush-trace
#		exec nam out.nam &
        exit 0
}

#Create three nodes
set n0 [$ns node]
set n1 [$ns node]
set n2 [$ns node]
set n3 [$ns node]
set n4 [$ns node]
set n5 [$ns node]



#
#n0-------n1-----n2-----n3
#         |      |
#         |      |
#         n4    n5
#

#Connect the nodes with two links
$ns duplex-link $n0 $n1 1000Mb 10ms DropTail
$ns duplex-link $n1 $n2 100Mb 10ms DropTail
$ns duplex-link $n2 $n3 1000Mb 10ms  DropTail
$ns duplex-link $n4 $n1 1000Mb 10ms  DropTail
$ns duplex-link $n5 $n2 1000Mb 10ms  DropTail

$ns duplex-link-op $n0 $n1 orient right
$ns duplex-link-op $n1 $n2 orient right
$ns duplex-link-op $n2 $n3 orient right
$ns duplex-link-op $n4 $n1 orient up
$ns duplex-link-op $n5 $n2 orient up


#create Pathload sender/receiver
#create full (bidirectional tcp) agent
#use server/client to show who establishes the
#tcp connection
set tcp_server [new Agent/TCP/FullTcp] 
set tcp_client [new Agent/TCP/FullTcp]
$ns attach-agent $n0 $tcp_server 
$ns attach-agent $n3 $tcp_client 
$tcp_server set fid_ 1   
$tcp_client set fid_ 2  
$tcp_server listen ;
$ns connect $tcp_client $tcp_server 

# set up TCP-level connections
#$tcp_server set window_ $TCP_WINDOW;

#create udp agents
set udp_src [new Agent/UDP]
set udp_rcv [new Agent/UDP]
$ns attach-agent $n0 $udp_src
$ns attach-agent $n3 $udp_rcv
$ns connect $udp_src $udp_rcv
$udp_src set fid_ 3
$udp_rcv set fid_ 4


#attach Pathload application to agents

set Pathload_snd [new Application/Pathload]
$Pathload_snd set type_ 1
$Pathload_snd attach-tcp-agent $tcp_server
$Pathload_snd attach-udp-agent $udp_src

set Pathload_rcv [new Application/Pathload]
$Pathload_rcv set type_ 2
$Pathload_rcv set transmission_rate 1000000.0
$Pathload_rcv set bw_resol 0.5#精度,单位Mbps

$Pathload_rcv attach-tcp-agent $tcp_client
$Pathload_rcv attach-udp-agent $udp_rcv

$Pathload_rcv connect $Pathload_snd


###########################################################
#n4 udp4 poi4
set udp4 [new Agent/UDP]
$ns attach-agent $n4 $udp4

set Poi4 [new Application/Traffic/Poisson]
$Poi4 set packetSize_ 1500
$Poi4 set rate_ 10Mb
$Poi4 attach-agent $udp4

#########################################################
#n5 null5
set null5 [new Agent/Null]
$ns attach-agent $n5 $null5

#########################################################
#数据从n4---->n5
$ns connect $udp4 $null5


$ns at 0.0 "$Pathload_snd start"
$ns at 0.0 "$Pathload_rcv start"


$ns at 30.0 "$Pathload_snd stop"
$ns at 30.0 "$Pathload_rcv stop"

$ns at 0.0 "$Poi4 start"
$ns at 30.0 "$Poi4 stop"
$ns at 30.1 "finish"


$ns run
