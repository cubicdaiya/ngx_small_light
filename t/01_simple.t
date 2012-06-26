use strict;
use warnings;
use t::Util;
use Test::More;
use HTTP::Request;

my $nginx = t::Util->nginx_start;

t::Util->test_nginx(sub {
    my ($ua, $do_request) = @_;
    subtest "normal case", sub {
        my $req = HTTP::Request->new(GET => '/small_light(p=msize,q=20)/img/mikan.jpg');
        my $res = $do_request->($req);
        is($res->code, 200, "test code");
    };
});

done_testing();
