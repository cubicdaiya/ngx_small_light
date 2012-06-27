use strict;
use warnings;
use t::Util;
use Test::More;
use HTTP::Request;
use Image::Size;

my $nginx = t::Util->nginx_start;

t::Util->test_nginx(sub {
    my ($ua, $do_request) = @_;
    subtest "normal case", sub {
        my $req = HTTP::Request->new(GET => '/small_light(p=msize,q=20)/img/mikan.jpg');
        my $res = $do_request->($req);
        is($res->code, 200, "test code");
        my ($width, $height, $format) = Image::Size::imgsize(\$res->content);
        is($format, "JPG", 'format ok');
        is($width, 128, "width ok");
        is($height, 128, "height ok");
    };
});

done_testing();
