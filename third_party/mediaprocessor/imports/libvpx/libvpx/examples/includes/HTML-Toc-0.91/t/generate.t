#--- generate.t ---------------------------------------------------------------
# function: Test ToC generation.

use strict;
use Test;

BEGIN { plan tests => 13; }

use HTML::Toc;
use HTML::TocGenerator;

my ($filename);
my $toc          = HTML::Toc->new;
my $tocGenerator = HTML::TocGenerator->new;

$toc->setOptions({
	'doLinkToToken' => 0,
	'levelIndent'   => 0,
	'header'        => '',
	'footer'        => '',
});


BEGIN {
		# Create test file
	$filename = "file$$.htm";
	die "$filename is already there" if -e $filename;
	open(FILE, ">$filename") || die "Can't create $filename: $!";
	print FILE <<'EOT';
<h1>Header</h1>
EOT
	close(FILE);
}


END {
		# Remove test file
	unlink($filename) or warn "Can't unlink $filename: $!";
}


#--- 1. generate --------------------------------------------------------------

$tocGenerator->generate($toc, "<h1>Header</h1>");
ok($toc->format(), "<ul>\n<li>Header\n</ul>");


#--- 2. generateFromFile ------------------------------------------------------

$tocGenerator->generateFromFile($toc, $filename);
ok($toc->format(), "<ul>\n<li>Header\n</ul>");


#--- 3. generateFromFiles -----------------------------------------------------

$tocGenerator->generateFromFile($toc, [$filename, $filename]);
ok($toc->format(), "<ul>\n<li>Header\n<li>Header\n</ul>");
	

#--- 4. doLinkToToken -----------------------------------------------------

$toc->setOptions({'doLinkToToken' => 1});
$tocGenerator->generateFromFile($toc, $filename, {'globalGroups' => 1});
ok($toc->format(), "<ul>\n<li><a href=#h-1>Header</a>\n</ul>");


#--- 5. doLinkToFile -------------------------------------------------------

$toc->setOptions({'doLinkToFile' => 1});
$tocGenerator->generateFromFile($toc, $filename);
ok($toc->format(), "<ul>\n<li><a href=$filename#h-1>Header</a>\n</ul>");


#--- 6. templateAnchorHrefBegin -----------------------------------------------

	# Set options
$toc->setOptions({'templateAnchorHrefBegin' => '"test-$file"'});
	# Generate ToC
$tocGenerator->generateFromFile($toc, $filename);
	# Test ToC
ok($toc->format(), "<ul>\n<li>test-".$filename."Header</a>\n</ul>");
	# Reset options
$toc->setOptions({'templateAnchorHrefBegin' => undef});


#--- 7. templateAnchorHrefBegin function --------------------------------------

sub AssembleAnchorHrefBegin {
		# Get arguments
	my ($aFile, $aGroupId, $aLevel, $aNode) = @_;
		# Return value
	return $aFile . $aGroupId . $aLevel . $aNode;
}  # AssembleAnchorHrefBegin()


	# Set options
$toc->setOptions({'templateAnchorHrefBegin' => \&AssembleAnchorHrefBegin});
	# Generate ToC
$tocGenerator->generateFromFile($toc, $filename);
	# Test ToC
ok($toc->format(), "<ul>\n<li>".$filename."h11Header</a>\n</ul>");
	# Reset options
$toc->setOptions({'templateAnchorHrefBegin' => undef});


#--- 8. levelToToc no levels available ---------------------------------------

$toc->setOptions({'levelToToc' => '2'});
$tocGenerator->generate($toc, "<h1>Header</h1>");
ok($toc->format(), "");


#--- 9. levelToToc level 1 ---------------------------------------------------

	# Set options
$toc->setOptions({
	'levelToToc' => '1',
	'doLinkToToken' => 0,
});
$tocGenerator->generate($toc, "<h1>Header1</h1>\n<h2>Header2</h2>");
ok($toc->format(), "<ul>\n<li>Header1\n</ul>");


#--- 10. levelToToc level 2 --------------------------------------------------

	# Set options
$toc->setOptions({
	'levelToToc' => '2',
	'doLinkToToken' => 0,
});
$tocGenerator->generate($toc, "<h1>Header1</h1>\n<h2>Header2</h2>");
ok($toc->format(), "<ul>\n<li>Header2\n</ul>");
	# Restore options
$toc->setOptions({
	'levelToToc' => '.*',
});


#--- 11. tokenToToc empty array ----------------------------------------------

	# Set options
$toc->setOptions({'tokenToToc' => []});
$tocGenerator->generate($toc, "<h1>Header</h1>");
ok($toc->format(), "");


#--- 12. groups nested --------------------------------------------------------

$toc->setOptions({
	'doNestGroup' => 1,
	'tokenToToc' => [
		{
			'level' => 1,
			'tokenBegin' => '<h1 class=-appendix>'
		}, {
			'groupId' => 'appendix',
			'level' => 1,
			'tokenBegin' => '<h1 class=appendix>'
		}
	]
});
$tocGenerator->generate(
	$toc, "<h1>Header1</h1>\n<h1 class=appendix>Appendix</h1>"
);
ok($toc->format() . "\n", <<'EOT');
<ul>
<li>Header1
<ul>
<li>Appendix
</ul>
</ul>
EOT


#--- 13. groups not nested ----------------------------------------------------

$toc->setOptions({
	'doNestGroup' => 0,
	'tokenToToc' => [
		{
			'level' => 1,
			'tokenBegin' => '<h1 class=-appendix>'
		}, {
			'groupId' => 'appendix',
			'level' => 1,
			'tokenBegin' => '<h1 class=appendix>'
		}
	]
});
$tocGenerator->generate(
	$toc, "<h1>Header1</h1>\n<h1 class=appendix>Appendix</h1>"
);
ok($toc->format() . "\n", <<'EOT');
<ul>
<li>Header1
</ul>
<ul>
<li>Appendix
</ul>
EOT
