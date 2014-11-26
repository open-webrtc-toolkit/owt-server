use ExtUtils::MakeMaker;

WriteMakefile(
    'NAME'		      => 'HTML::Toc',
    'VERSION_FROM'	=> 'Toc.pm',
    'PREREQ_PM'		=> {'HTML::Parser' => 0},
	 'MAN3PODS'       => {},
);
