#!/usr/bin/perl -w

use strict;

use Socket qw(PF_INET SOCK_STREAM pack_sockaddr_in inet_aton);
use Fcntl qw(F_GETFL F_SETFL O_NONBLOCK);

sub readall {
  my ($socket) = @_;
  my $oldflags = fcntl($socket, F_GETFL, 0) or die "Can't get flags for the socket: $!\n";
  my $flags = fcntl($socket, F_SETFL, $oldflags | O_NONBLOCK) or die "Can't set flags for the socket: $!\n";
  my $nread = 1;
  my $BUF;
  while(defined($nread=sysread $socket,$BUF,4096) && $nread > 0){}

  $flags = fcntl($socket, F_SETFL, $oldflags) or die "Can't set flags for the socket: $!\n";
}

sub recvall {
  my($sock,$count) = @_;
  my $buf="";
  while($count) {
    my $nbuf="";
    my $nread = sysread $sock,$nbuf,$count;
    if($nread > 0) {
      $count -= $nread;
      $buf .= $nbuf;
    } else {
      exit 2;
    }
  }
  return $buf;
}

sub send_string {
  my($sock,$string) = @_;
  syswrite $sock,$string,length($string);
}

$|=1;

socket(my $s, PF_INET, SOCK_STREAM, 0) or die "socket: $!";
my $port = getservbyname "newtonwars", "tcp";
$port=3490 unless(defined($port));

connect($s, pack_sockaddr_in($port, inet_aton("127.0.0.1"))) or die "connect: $!";

send_string($s,"n Stuplbot\n");

# wait before we slurp the socket
sleep 1;
readall($s);

send_string($s, "b\n");

my $angle = rand(360) - 180;
my $speed = 10;
my $mypid = undef;

send_string($s,sprintf("v %f\n%f\n",$speed, $angle));

while(1) {
   my $data = recvall($s, 8);
   my($typ,$pid) = unpack("LL",$data);
   if($typ == 1) {
      $mypid = $pid;
      printf "My id: %d\n",$mypid;
  } elsif($typ == 2) {
      printf "Player %d left\n",$pid;
      send_string($s,"r\n");
  } elsif($typ == 3) {
      $data = recvall($s, 8);
      my($fx,$fy) = unpack("ff",$data);
      printf "Player %d at %f, %f\n",$pid,$fx,$fy;
      send_string($s,"r\n");
  } elsif($typ == 4) {
      $data = recvall($s, 4);
      my $n = unpack("L", $data);
      printf "Player %d finished shot with %d segments\n",$pid, $n;
      while($n--) {
         recvall($s, 8);
      }
      if($pid == $mypid) {
         $angle += 361 / 3.0;
	 $angle -= 360 if($angle > 180);
         send_string($s,sprintf("v %f\n%f\n",$speed,$angle));
      }
  } else {
     print "Unexpected data: ";
     for(my $l=0;$l < length($data);$l++) {
       printf "%02x",ord(substr($data,$l,1));
     }
     print "\n";
  }
}


