use strict;
use warnings;
use t::Util;
use Test::More;
use HTTP::Request;
use Image::Size;

my $nginx = t::Util->nginx_start;

t::Util->test_nginx(sub {
    my ($ua, $do_request) = @_;
    subtest "ssize", sub {
        subtest "jpg to jpg", sub {
            my $req = HTTP::Request->new(GET => '/small_light(p=ssize)/img/mikan.jpg');
            my $res = $do_request->($req);
            is($res->code, 200, "test code");
            is($res->content_type, "image/jpeg", "content type ok");
            my ($width, $height, $format) = Image::Size::imgsize(\$res->content);
            is($format, "JPG", 'format ok');
            is($width, 20, "width ok");
            is($height, 20, "height ok");
        };

        subtest "png to jpg", sub {
            my $req = HTTP::Request->new(GET => '/small_light(p=ssize)/img/mikan.png');
            my $res = $do_request->($req);
            is($res->code, 200, "test code");
            is($res->content_type, "image/jpeg", "content type ok");
            my ($width, $height, $format) = Image::Size::imgsize(\$res->content);
            is($format, "JPG", 'format ok');
            is($width, 20, "width ok");
            is($height, 20, "height ok");
        };
    };
    subtest "too_big_image", sub {
        my $req = HTTP::Request->new(GET => '/small_light(p=too_big_image)/img/mikan.jpg');
        my $res = $do_request->($req);
        TODO: {
            todo_skip "It should be raise errr and log", 1;
            is($res->code, 500, "it should raise error");
        }
    };
});

done_testing();
