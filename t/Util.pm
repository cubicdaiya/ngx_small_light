package t::Util;
use strict;
use warnings;
use Test::More;
use Proc::Guard qw(proc_guard);

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
}

1;
