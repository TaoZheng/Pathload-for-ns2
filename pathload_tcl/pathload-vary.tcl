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

#Connect the nodes with two links
$ns duplex-link $n0 $n1 1000Mb 10ms DropTail
$ns duplex-link $n3 $n1 1000Mb 10ms DropTail
$ns duplex-link $n1 $n2 100Mb 10ms DropTail

############################################################

$ns duplex-link-op $n0 $n1 orient right
$ns duplex-link-op $n1 $n2 orient right
$ns duplex-link-op $n3 $n1 orient up


#create Pathload sender/receiver
#create full (bidirectional tcp) agent
#use server/client to show who establishes the
#tcp connection
set tcp_server [new Agent/TCP/FullTcp] 
set tcp_client [new Agent/TCP/FullTcp]
$ns attach-agent $n0 $tcp_server 
$ns attach-agent $n2 $tcp_client 
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
$ns attach-agent $n2 $udp_rcv
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
$Pathload_rcv set bw_resol 10.0 #精度

$Pathload_rcv attach-tcp-agent $tcp_client
$Pathload_rcv attach-udp-agent $udp_rcv

$Pathload_rcv connect $Pathload_snd




# CBR CROSS TRAFFIC

set cbr [new Application/Traffic/CBR]
$cbr set packetSize_ 1000
$cbr set rate_ 30Mb #to get 1 percent of bandwidth

set udp [new Agent/UDP]
$ns attach-agent $n3 $udp
$cbr attach-agent $udp
set null0 [new Agent/Null] 
$ns attach-agent $n2 $null0

################################################
$udp set fid_ 3
###############################################
 
$ns connect $udp $null0


$ns at 81.750054 "$Pathload_snd start"
$ns at 81.750054 "$Pathload_rcv start"
$ns at 102.0 "$Pathload_snd stop"
$ns at 102.0 "$Pathload_rcv stop"

$ns at 0.0 "$cbr start"
$ns at 0.0 "$cbr set rate_ 10Mb"
$ns at 2.0 "$cbr set rate_ 25Mb"
$ns at 4.0 "$cbr set rate_ 10Mb"
$ns at 6.0 "$cbr set rate_ 25Mb"
$ns at 8.0 "$cbr set rate_ 15Mb"
$ns at 10.0 "$cbr set rate_ 30Mb"
$ns at 12.0 "$cbr set rate_ 15Mb"
$ns at 14.0 "$cbr set rate_ 35Mb"
$ns at 16.0 "$cbr set rate_ 20Mb"
$ns at 18.0 "$cbr set rate_ 45Mb"
$ns at 20.0 "$cbr set rate_ 5Mb"
$ns at 22.0 "$cbr set rate_ 40Mb"
$ns at 24.0 "$cbr set rate_ 30Mb"
$ns at 26.0 "$cbr set rate_ 55Mb"
$ns at 28.0 "$cbr set rate_ 30Mb"
$ns at 30.0 "$cbr set rate_ 55Mb"
$ns at 32.0 "$cbr set rate_ 35Mb"
$ns at 34.0 "$cbr set rate_ 50Mb"
$ns at 36.0 "$cbr set rate_ 40Mb"
$ns at 38.0 "$cbr set rate_ 70Mb"
$ns at 40.0 "$cbr set rate_ 40Mb"
$ns at 42.0 "$cbr set rate_ 55Mb"
$ns at 44.0 "$cbr set rate_ 30Mb"
$ns at 46.0 "$cbr set rate_ 40Mb"
$ns at 48.0 "$cbr set rate_ 30Mb"
$ns at 50.0 "$cbr set rate_ 55Mb"
$ns at 52.0 "$cbr set rate_ 35Mb"
$ns at 54.0 "$cbr set rate_ 90Mb"
$ns at 56.0 "$cbr set rate_ 50Mb"
$ns at 58.0 "$cbr set rate_ 80Mb"
$ns at 60.0 "$cbr set rate_ 45Mb"
$ns at 62.0 "$cbr set rate_ 65Mb"
$ns at 64.0 "$cbr set rate_ 45Mb"
$ns at 66.0 "$cbr set rate_ 60Mb"
$ns at 68.0 "$cbr set rate_ 50Mb"
$ns at 70.0 "$cbr set rate_ 70Mb"
$ns at 72.0 "$cbr set rate_ 60Mb"
$ns at 74.0 "$cbr set rate_ 80Mb"
$ns at 76.0 "$cbr set rate_ 55Mb"
$ns at 78.0 "$cbr set rate_ 80Mb"
$ns at 80.0 "$cbr set rate_ 35Mb"
$ns at 82.0 "$cbr set rate_ 45Mb"
$ns at 84.0 "$cbr set rate_ 35Mb"
$ns at 86.0 "$cbr set rate_ 50Mb"
$ns at 88.0 "$cbr set rate_ 40Mb"
$ns at 90.0 "$cbr set rate_ 45Mb"
$ns at 92.0 "$cbr set rate_ 30Mb"
$ns at 94.0 "$cbr set rate_ 50Mb"
$ns at 96.0 "$cbr set rate_ 30Mb"
$ns at 98.0 "$cbr set rate_ 40Mb"
$ns at 100.0 "$cbr set rate_ 40Mb"
$ns at 102.0 "$cbr stop"
$ns at 102.0 "finish"


$ns run
