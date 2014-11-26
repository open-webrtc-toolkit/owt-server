#--- propagate.t --------------------------------------------------------------
# function: Test ToC propagation.

use strict;
use Test;

BEGIN { plan tests => 10; }

use HTML::Toc;
use HTML::TocGenerator;
use HTML::TocInsertor;

my ($output, $content, $filename);
my $toc          = HTML::Toc->new;
my $tocGenerator = HTML::TocGenerator->new;
my $tocInsertor  = HTML::TocInsertor->new;

$toc->setOptions({
	'doLinkToToken'  => 0,
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


#--- 1. propagate -------------------------------------------------------------

$tocInsertor->insert($toc, "<h1>Header</h1>", {'output' => \$output});
ok($output, "<ul>\n<li>Header\n</ul><h1>Header</h1>");


#--- 2. propagateFile ---------------------------------------------------------

$tocInsertor->insertIntoFile($toc, $filename, {'output' => \$output});
ok($output, "<ul>\n<li>Header\n</ul><h1>Header</h1>\n");


#--- 3. doLinkToToken -----------------------------------------------------

$toc->setOptions({'doLinkToToken' => 1});
$tocInsertor->insert($toc, "<h1>Header</h1>", {'output' => \$output});
ok("$output\n", <<'EOT');
<ul>
<li><a href=#h-1>Header</a>
</ul><a name=h-1><h1>Header</h1></a>
EOT


#--- 4. templateAnchorHrefBegin -----------------------------------------------

$toc->setOptions(
	{'templateAnchorHrefBegin' => '"<$node${file}test${groupId}>"'}
);
$tocInsertor->insertIntoFile($toc, $filename, {'output' => \$output});
ok($output, "<ul>\n<li><1${filename}testh>Header</a>\n</ul><a name=h-1><h1>Header</h1></a>\n");
$toc->setOptions({'templateAnchorHrefBegin' => undef});


#--- 5. templateAnchorNameBegin -----------------------------------------------

$toc->setOptions({
	'templateAnchorName'      => '"$node$groupId"',
	'templateAnchorNameBegin' => '"<$anchorName>"'
});
$tocInsertor->insert($toc, "<h1>Header</h1>", {'output' => \$output});
ok($output, "<ul>\n<li><a href=#1h>Header</a>\n</ul><1h><h1>Header</h1></a>");
$toc->setOptions({'templateAnchorName' => undef});


#--- 6. templateAnchorName function -------------------------------------------

sub AssembleAnchorName {
		# Get arguments
	my ($aFile, $aGroupId, $aLevel, $aNode) = @_;
		# Return value
	return $aFile . $aGroupId . $aLevel . $aNode;
}  # AssembleAnchorName()

	# Set options
$toc->setOptions({'templateAnchorNameBegin' => \&AssembleAnchorName});
	# Propagate ToC
$tocInsertor->insert($toc, "<h1>Header</h1>", {'output' => \$output});
	# Test ToC
ok($output, "<ul>\n<li><a href=#h-1>Header</a>\n</ul>h11<h1>Header</h1></a>");
	# Restore options
$toc->setOptions({'templateAnchorNameBegin' => undef});


#--- 7. doNumberToken --------------------------------------------------------

	# Set options
$toc->setOptions({'doNumberToken' => 1});
$tocInsertor->insert($toc, "<h1>Header</h1>", {'output' => \$output});
ok("$output\n", <<'EOT');
<ul>
<li><a href=#h-1>Header</a>
</ul><a name=h-1><h1>1 &nbsp;Header</h1></a>
EOT
	# Reset options
$toc->setOptions({
	'templateTokenNumber' => undef,
	'doNumberToken'      => 0
});


#--- 8. templateTokenNumber ---------------------------------------------------

	# Set options
$toc->setOptions({
	'templateTokenNumber' => '"-$node-"',
	'doNumberToken'      => 1
});
	# Propagate ToC
$tocInsertor->insert($toc, "<h1>Header</h1>", {'output' => \$output});
	# Test ToC
ok("$output\n", <<'EOT');
<ul>
<li><a href=#h-1>Header</a>
</ul><a name=h-1><h1>-1-Header</h1></a>
EOT
	# Reset options
$toc->setOptions({
	'doNumberToken'      => 0,
	'templateTokenNumber' => undef
});


#--- 9. numberingStyle --------------------------------------------------------

	# Set options
$toc->setOptions({
	'doNumberToken' => 1,
	'tokenToToc' => [{
		'level' => 1,
		'tokenBegin' => '<h1>',
		'numberingStyle' => 'lower-alpha'
	}]
});
	# Propagate ToC
$tocInsertor->insert($toc, "<h1>Header</h1>", {'output' => \$output});
	# Test ToC
ok("$output\n", <<'EOT');
<ul>
<li><a href=#h-a>Header</a>
</ul><a name=h-a><h1>a &nbsp;Header</h1></a>
EOT
	# Reset options
$toc->setOptions({
	'doNumberToken' => 0,
	'tokenToToc' => undef,
});


#--- 10. declaration pass through ---------------------------------------------

$tocInsertor->insert($toc, '<! DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"><h1>Header</h1>', {'output' => \$output});
	# Test ToC
ok($output, '<! DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"><h1>Header</h1>');
