use strict;
use warnings;
use t::Util;
use Test::More;
use LWP::UserAgent;

my $nginx = t::Util->nginx_start;

my $ua = LWP::UserAgent->new;

my $res = $ua->get('http://localhost:8000/small_light(p=msize,q=20)/img/mikan.jpg');
is($res->code, 200, "test code");

done_testing();
