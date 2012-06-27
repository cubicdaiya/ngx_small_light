use strict;
use warnings;
use t::Util;
use Test::More;
use HTTP::Request;
use Image::Size;

my $nginx = t::Util->nginx_start;

t::Util->test_nginx(sub {
    my ($ua, $do_request) = @_;
    subtest "msize", sub {
        my $req = HTTP::Request->new(GET => '/small_light(p=msize,q=20)/img/mikan.jpg');
        my $res = $do_request->($req);
        is($res->code, 200, "test code");
        my ($width, $height, $format) = Image::Size::imgsize(\$res->content);
        is($format, "JPG", 'format ok');
        is($width, 1000, "width ok");
        is($height, 1000, "height ok");
    };
    subtest "ssize", sub {
        my $req = HTTP::Request->new(GET => '/small_light(p=ssize,q=20)/img/mikan.jpg');
        my $res = $do_request->($req);
        is($res->code, 200, "test code");
        my ($width, $height, $format) = Image::Size::imgsize(\$res->content);
        is($format, "JPG", 'format ok');
        is($width, 20, "width ok");
        is($height, 20, "height ok");
    };
});

done_testing();
