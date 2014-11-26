#--- generate.t ---------------------------------------------------------------
# function: Test ToC generation.

use strict;
use Test;

BEGIN { plan tests => 4; }

use HTML::Toc;
use HTML::TocGenerator;

my ($filename);
my $toc          = HTML::Toc->new;
my $tocGenerator = HTML::TocGenerator->new;

$toc->setOptions({
	'doLinkToToken' => 0,
	'levelIndent'       => 0,
	'header'            => '',
	'footer'            => '',
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


#--- 1. extend ----------------------------------------------------------------

	# Generate ToC
$tocGenerator->generate($toc, "<h1>Header</h1>");
	# Extend ToC
$tocGenerator->extend($toc, "<h1>Header</h1>");
	# Test ToC
ok($toc->format(), "<ul>\n<li>Header\n<li>Header\n</ul>");


#--- 2. extendFromFile --------------------------------------------------------

	# Generate ToC
$tocGenerator->generateFromFile($toc, $filename);
	# Extend ToC
$tocGenerator->extendFromFile($toc, $filename);
	# Test ToC
ok($toc->format(), "<ul>\n<li>Header\n<li>Header\n</ul>");


#--- 3. extendFromFiles -------------------------------------------------------

	# Generate ToC
$tocGenerator->generateFromFile($toc, $filename);
	# Extend ToC
$tocGenerator->extendFromFile($toc, [$filename, $filename]);
	# Test ToC
ok($toc->format(), "<ul>\n<li>Header\n<li>Header\n<li>Header\n</ul>");


#--- 4. linkTocToToken --------------------------------------------------------

$toc->setOptions({
	'doLinkToToken' => 1,
});
	# Generate ToC
$tocGenerator->generate($toc, "<h1>Header</h1>");
	# Extend ToC
$tocGenerator->extend($toc, "<h1>Header</h1>");
	# Test ToC
ok($toc->format() . "\n", <<'EOT');
<ul>
<li><a href=#h-1>Header</a>
<li><a href=#h-2>Header</a>
</ul>
EOT
