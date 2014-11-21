#--- update.t -----------------------------------------------------------------
# function: Test ToC updating.

use strict;
use Test;

BEGIN { plan tests => 6; }

use HTML::Toc;
use HTML::TocUpdator;

my ($output, $output2, $content, $filename);
my $toc         = HTML::Toc->new;
my $tocUpdator  = HTML::TocUpdator->new;

$toc->setOptions({
	'doLinkToToken'  => 1,
	'doNumberToken'  => 1,
	'levelIndent'    => 0,
	'insertionPoint' => 'before <h1>',
	'header'         => '',
	'footer'         => '',
});


BEGIN {
		# Create test file
	$filename = "file$$.htm";
	die "$filename is already there" if -e $filename;
	open(FILE, ">$filename") || die "Can't create $filename: $!";
	print FILE <<'EOT'; close(FILE);
<h1>Header</h1>
EOT
}


END {
		# Remove test file
	unlink($filename) or warn "Can't unlink $filename: $!";
}


#--- 1. update ----------------------------------------------------------------

$tocUpdator->update($toc, "<h1>Header</h1>", {'output' => \$output});
ok("$output\n", <<'EOT');
<!-- #BeginToc --><ul>
<li><a href=#h-1>Header</a>
</ul><!-- #EndToc --><!-- #BeginTocAnchorNameBegin --><a name=h-1><!-- #EndTocAnchorNameBegin --><h1><!-- #BeginTocNumber -->1 &nbsp;<!-- #EndTocNumber -->Header</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
EOT

#--- 2. updateFile ------------------------------------------------------------

$tocUpdator->updateFile($toc, $filename, {'output' => \$output});
	open(FILE, ">a.out1") || die "Can't create a.out1: $!";
	print FILE $output; close(FILE);
$output2 = <<'EOT';
<!-- #BeginToc --><ul>
<li><a href=#h-1>Header</a>
</ul><!-- #EndToc --><!-- #BeginTocAnchorNameBegin --><a name=h-1><!-- #EndTocAnchorNameBegin --><h1><!-- #BeginTocNumber -->1 &nbsp;<!-- #EndTocNumber -->Header</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
EOT
	open(FILE, ">a.out2") || die "Can't create a.out2: $!";
	print FILE $output2; close(FILE);
ok($output, $output2);


#--- 3. insert ----------------------------------------------------------------

$tocUpdator->insert($toc, "<h1>Header</h1>", {'output' => \$output});
ok("$output\n", <<'EOT');
<!-- #BeginToc --><ul>
<li><a href=#h-1>Header</a>
</ul><!-- #EndToc --><!-- #BeginTocAnchorNameBegin --><a name=h-1><!-- #EndTocAnchorNameBegin --><h1><!-- #BeginTocNumber -->1 &nbsp;<!-- #EndTocNumber -->Header</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
EOT

#--- 4. insertIntoFile --------------------------------------------------------

$tocUpdator->insertIntoFile($toc, $filename, {'output' => \$output});
ok($output, <<'EOT');
<!-- #BeginToc --><ul>
<li><a href=#h-1>Header</a>
</ul><!-- #EndToc --><!-- #BeginTocAnchorNameBegin --><a name=h-1><!-- #EndTocAnchorNameBegin --><h1><!-- #BeginTocNumber -->1 &nbsp;<!-- #EndTocNumber -->Header</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
EOT


#--- 5. update twice ----------------------------------------------------------

$tocUpdator->update($toc, "<h1>Header</h1>", {'output' => \$output});
$tocUpdator->update($toc, $output, {'output' => \$output2});
ok("$output\n", <<'EOT');
<!-- #BeginToc --><ul>
<li><a href=#h-1>Header</a>
</ul><!-- #EndToc --><!-- #BeginTocAnchorNameBegin --><a name=h-1><!-- #EndTocAnchorNameBegin --><h1><!-- #BeginTocNumber -->1 &nbsp;<!-- #EndTocNumber -->Header</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
EOT


#--- 6. tokens update begin & end ---------------------------------------------

$toc->setOptions({
	'tokenUpdateBeginOfAnchorNameBegin' => '<tocAnchorNameBegin>',
	'tokenUpdateEndOfAnchorNameBegin'   => '</tocAnchorNameBegin>',
	'tokenUpdateBeginOfAnchorNameEnd'   => '<tocAnchorNameEnd>',
	'tokenUpdateEndOfAnchorNameEnd'     => '</tocAnchorNameEnd>',
	'tokenUpdateBeginNumber'            => '<tocNumber>',
	'tokenUpdateEndNumber'              => '</tocNumber>',
	'tokenUpdateBeginToc'               => '<toc>',
	'tokenUpdateEndToc',                => '</toc>'
});
$tocUpdator->update($toc, "<h1>Header</h1>", {'output' => \$output});
ok("$output\n", <<'EOT');
<toc><ul>
<li><a href=#h-1>Header</a>
</ul></toc><tocAnchorNameBegin><a name=h-1></tocAnchorNameBegin><h1><tocNumber>1 &nbsp;</tocNumber>Header</h1><tocAnchorNameEnd></a></tocAnchorNameEnd>
EOT
