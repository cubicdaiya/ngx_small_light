package t::Util;
use strict;
use warnings;
use Test::More;
use Test::TCP;
use Proc::Guard qw(proc_guard);
use LWP::UserAgent;

sub nginx_start {
    my ($class, $conf_name) = @_;
    $conf_name ||= "basic.nginx.conf";


    my $pwd = `pwd`;
    chomp $pwd;
    my $nginx_bin = $ENV{'NGINX_BIN'}
        or die "please specify ENV NGINX_BIN";

    my $proc = proc_guard($nginx_bin, 
        '-g', 'daemon off;', 
        '-p', $pwd . '/t/ngx_base/',
        '-c', $pwd . "/t/ngx_base/etc/$conf_name"
    );
    Test::TCP::wait_port(8000);
    return $proc;
}

sub test_nginx {
    my ($class, $code) = @_;
    my $ua = LWP::UserAgent->new;

    my $do_request = sub {
        my $req = shift;
        $req->uri->scheme('http');
        $req->uri->host('localhost:8000');
        $ua->request($req);
    };
    return $code->($ua, $do_request);
}

1;
