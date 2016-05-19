use strict;
use warnings;
use Module::Build;

my $build = Module::Build->new(
    module_name    => 'ngx_small_light',
    dist_version   => '0.7.3',
    dist_author    => "Tatsuhiko Kubo",
    dist_abstract  => "nginx module for dynamic thumbnail",
    build_requires => {
        'Test::More'  => '0.98',
        'Test::TCP'   => 0,
        'Image::Size' => 0,
        'Proc::Guard' => 0,
    },
    configure_requires => { 'Module::Build' => '0.38' },
    test_files => 't/',
    recursive_test_files => 1,

);

$build->create_build_script();
