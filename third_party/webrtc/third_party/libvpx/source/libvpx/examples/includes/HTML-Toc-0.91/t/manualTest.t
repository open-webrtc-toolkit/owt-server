#--- manual.t -----------------------------------------------------------------
# function: Test HTML::ToC generating a manual.

use strict;
use Test;

BEGIN { plan tests => 3; }

use HTML::Toc;
use HTML::TocGenerator;
use HTML::TocInsertor;
use HTML::TocUpdator;


#--- AssembleTocLine() --------------------------------------------------------
# function: Assemble ToC line.

sub AssembleTocLine {
		# Get arguments
	my ($aLevel, $aGroupId, $aNode, $aSequenceNr, $aText) = @_;
		# Local variables
	my ($result);

		# Assemble ToC line
	SWITCH: {
		if ($aGroupId eq "prelude") {
			$result = "<li>$aText\n";
			last SWITCH;
		}
		if ($aGroupId eq "part") {
			$result = "<li>Part $aNode &nbsp;$aText\n";
			last SWITCH;
		}
		if ($aGroupId eq "h") {
			$result = "<li>$aSequenceNr. &nbsp;$aText\n";
			last SWITCH;
		}
		else {
			$result = "<li>$aNode &nbsp;$aText\n";
			last SWITCH;
		}
	}

		# Return value
	return $result;
}  # AssembleTocLine()


#--- AssembleTokenNumber() ----------------------------------------------------
# function: Assemble token number.

sub AssembleTokenNumber {
		# Get arguments
	my ($aNode, $aGroupId, $aFile, $aGroupLevel, $aLevel, $aToc) = @_;
		# Local variables
	my ($result);
		# Assemble token number
	SWITCH: {
		if ($aGroupId eq "part") {
			$result = "Part $aNode &nbsp;";
			last SWITCH;
		}
		else {
			$result = "$aNode &nbsp;";
			last SWITCH;
		}
	}
		# Return value
	return $result;
}  # AssembleTokenNumber()


#--- TestInsertManualToc ------------------------------------------------------
# function: Test inserting ToC into manual.

sub TestInsertManualToc {
	my $output;
		# Create objects
	my $toc          = new HTML::Toc;
	my $tocOfFigures = new HTML::Toc;
	my $tocOfTables  = new HTML::Toc;
	my $tocInsertor  = new HTML::TocInsertor;
	
		# Set ToC options
	$toc->setOptions({
		'doNestGroup'          => 1,
		'doNumberToken'        => 1,
		'insertionPoint'       => "replace <!-- Table of Contents -->",
		'templateLevel'        => \&AssembleTocLine,
      'templateLevelBegin'   => '"<ul class=toc_$groupId$level>\n"',
      'templateLevelEnd'     => '"</ul>\n"',
		'templateTokenNumber'  => \&AssembleTokenNumber,
      'tokenToToc'           => [{
            'groupId'        => 'part',
			   'doNumberToken'  => 1,
            'level'          => 1,
            'tokenBegin'     => '<h1 class=part>',
         }, {
            'tokenBegin'     => '<h1 class=-[appendix|prelude|hidden|part]>'
         }, {
            'tokenBegin'     => '<h2>',
            'level'          => 2
         }, {
            'tokenBegin'     => '<h3>',
            'level'          => 3
         }, {
            'groupId'        => 'appendix',
            'tokenBegin'     => '<h1 class=appendix>',
			   'numberingStyle' => 'upper-alpha',
         }, {
            'groupId'        => 'appendix',
            'tokenBegin'     => '<h2 class=appendix>',
            'level'          => 2
         }, {
            'groupId'        => 'prelude',
            'tokenBegin'     => '<h1 class=prelude>',
            'level'          => 1,
			   'doNumberToken'  => 0,
         }],
	});
	$tocOfFigures->setOptions({
		'doNumberToken'        => 1,
		'insertionPoint'       => "replace <!-- Table of Figures -->",
		'templateLevelBegin'   => '"<ol>\n"',
		'templateLevelEnd'     => '"</ol>\n"',
		'templateTokenNumber'  => '"Figure $node: &nbsp;"',
		'tokenToToc'           => [{
				'groupId'        => 'Figure',
				'tokenBegin'     => '<p class=captionFigure>'
			}]
	});
	$tocOfTables->setOptions({
		'doNumberToken'        => 1,
		'insertionPoint'       => "replace <!-- Table of Tables -->",
		'templateLevelBegin'   => '"<ol>\n"',
		'templateLevelEnd'     => '"</ol>\n"',
		'templateTokenNumber'  => '"Table $node: &nbsp;"',
		'tokenToToc'           => [{
				'groupId'        => 'Table',
				'tokenBegin'     => '<p class=captionTable>'
			}]
	});
		# Insert ToC
	$tocInsertor->insertIntoFile(
		[$toc, $tocOfFigures, $tocOfTables], 
		't/ManualTest/manualTest1.htm', {
			 'doUseGroupsGlobal' => 1,
			 'output'            => \$output,
			 'outputFile'        => 't/ManualTest/manualTest2.htm'
		}
	);
	ok($output, <<EOT);
<html>
<head>
   <title>Manual</title>
    <style type="text/css">
       ul.toc_appendix1 { 
         list-style-type: none;
         margin-left: 0;
         margin-top: 1em;
         margin-bottom: 1em;
       }
       ul.toc_h1 {
         list-style-type: none;
         margin-left: 1;
         margin-top: 1em;
         margin-bottom: 1em;
       }
       ul.toc_h2 {
         list-style-type: none;
       }
       ul.toc_h3 {
         list-style-type: none;
       }
       ul.toc_part1 {
         list-style-type: none;
         margin-left: 1;
         margin-top: 1em;
         margin-bottom: 1em;
       }
       ul.toc_prelude1 {
         list-style: none;
       }
       p.captionFigure {
         font-style: italic;
         font-weight: bold;
       }
       p.captionTable {
         font-style: italic;
         font-weight: bold;
       }
    </style>
</head>
<body>

<a name=prelude-1><h1 class=prelude>Preface</h1></a>
Better C than never.

<h1 class=hidden>Table of Contents</h1>

<!-- Table of Contents generated by Perl - HTML::Toc -->
<ul class=toc_prelude1>
   <li><a href=#prelude-1>Preface</a>
   <li><a href=#prelude-2>Table of Figures</a>
   <li><a href=#prelude-3>Table of Tables</a>
   <li><a href=#prelude-4>Introduction</a>
   <ul class=toc_part1>
      <li>Part 1 &nbsp;<a href=#part-1>Disks</a>
      <ul class=toc_h1>
         <li>1. &nbsp;<a href=#h-1>Compiler Disk v1</a>
         <ul class=toc_h2>
            <li>1. &nbsp;<a href=#h-1.1>System</a>
            <li>2. &nbsp;<a href=#h-1.2>Standard Library</a>
         </ul>
         <li>2. &nbsp;<a href=#h-2>Compiler Disk v2</a>
         <ul class=toc_h2>
            <li>1. &nbsp;<a href=#h-2.1>System</a>
            <ul class=toc_h3>
               <li>1. &nbsp;<a href=#h-2.1.1>parser.com</a>
               <li>2. &nbsp;<a href=#h-2.1.2>compiler.com</a>
               <li>3. &nbsp;<a href=#h-2.1.3>linker.com</a>
            </ul>
            <li>2. &nbsp;<a href=#h-2.2>Standard Library</a>
         </ul>
         <li>3. &nbsp;<a href=#h-3>Library System Disk</a>
      </ul>
      <li>Part 2 &nbsp;<a href=#part-2>Personal</a>
      <ul class=toc_h1>
         <li>4. &nbsp;<a href=#h-4>Tips & Tricks</a>
      </ul>
      <li>Part 3 &nbsp;<a href=#part-3>Appendixes</a>
      <ul class=toc_appendix1>
         <li>A &nbsp;<a href=#appendix-A>Functions Standard Library v1</a>
         <li>B &nbsp;<a href=#appendix-B>Functions Standard Library v2</a>
         <li>C &nbsp;<a href=#appendix-C>Functions Graphic Library</a>
      </ul>
   </ul>
   <li><a href=#prelude-5>Bibliography</a>
</ul>
<!-- End of generated Table of Contents -->


<a name=prelude-2><h1 class=prelude>Table of Figures</h1></a>

<!-- Table of Contents generated by Perl - HTML::Toc -->
<ol>
   <li><a href=#Figure-1>Contents Compiler Disk v1</a>
   <li><a href=#Figure-2>Contents Compiler Disk v2</a>
</ol>
<!-- End of generated Table of Contents -->


<a name=prelude-3><h1 class=prelude>Table of Tables</h1></a>

<!-- Table of Contents generated by Perl - HTML::Toc -->
<ol>
   <li><a href=#Table-1>Compile Steps</a>
</ol>
<!-- End of generated Table of Contents -->


<a name=prelude-4><h1 class=prelude>Introduction</h1></a>
Thanks to standardisation and the excellent work of the QWERTY corporation it is possible to learn C with almost any C manual.
<a name=Table-1><p class=captionTable>Table 1: &nbsp;Compile Steps</p></a>
<ul><pre>
   Parser
   Compiler
   Linker
</pre></ul>

<a name=part-1><h1 class=part>Part 1 &nbsp;Disks</h1></a>
<a name=h-1><h1>1 &nbsp;Compiler Disk v1</h1></a>
<img src=img.gif alt="Contents Compiler Disk v1">
<a name=Figure-1><p class=captionFigure>Figure 1: &nbsp;Contents Compiler Disk v1</p></a>

<a name=h-1.1><h2>1.1 &nbsp;System</h2></a>
<a name=h-1.2><h2>1.2 &nbsp;Standard Library</h2></a>

<a name=h-2><h1>2 &nbsp;Compiler Disk v2</h1></a>
<img src=img.gif alt="Contents Compiler Disk v2">
<a name=Figure-2><p class=captionFigure>Figure 2: &nbsp;Contents Compiler Disk v2</p></a>

<a name=h-2.1><h2>2.1 &nbsp;System</h2></a>
<a name=h-2.1.1><h3>2.1.1 &nbsp;parser.com</h3></a>
<a name=h-2.1.2><h3>2.1.2 &nbsp;compiler.com</h3></a>
<a name=h-2.1.3><h3>2.1.3 &nbsp;linker.com</h3></a>
<a name=h-2.2><h2>2.2 &nbsp;Standard Library</h2></a>

<a name=h-3><h1>3 &nbsp;Library System Disk</h1></a>
<a name=part-2><h1 class=part>Part 2 &nbsp;Personal</h1></a>
<a name=h-4><h1>4 &nbsp;Tips & Tricks</h1></a>
<a name=part-3><h1 class=part>Part 3 &nbsp;Appendixes</h1></a>
<a name=appendix-A><h1 class=appendix>A &nbsp;Functions Standard Library v1</h1></a>
<a name=appendix-B><h1 class=appendix>B &nbsp;Functions Standard Library v2</h1></a>
<a name=appendix-C><h1 class=appendix>C &nbsp;Functions Graphic Library</h1></a>
<a name=prelude-5><h1 class=prelude>Bibliography</h1></a>
</body>
</html>
EOT
}  # TestInsertManualToc()


#--- TestInsertManualForUpdating() --------------------------------------------
# function: Test inserting ToC into manual.

sub TestInsertManualForUpdating {
	my $output;
		# Create objects
	my $toc          = new HTML::Toc;
	my $tocOfFigures = new HTML::Toc;
	my $tocOfTables  = new HTML::Toc;
	my $tocUpdator   = new HTML::TocUpdator;
	
		# Set ToC options
	$toc->setOptions({
		'doNestGroup'          => 1,
		'doNumberToken'        => 1,
		'insertionPoint'       => "after <!-- Table of Contents -->",
		'templateLevel'        => \&AssembleTocLine,
      'templateLevelBegin'   => '"<ul class=toc_$groupId$level>\n"',
      'templateLevelEnd'     => '"</ul>\n"',
		'templateTokenNumber'  => \&AssembleTokenNumber,
      'tokenToToc'           => [{
            'groupId'        => 'part',
			   'doNumberToken'  => 1,
            'level'          => 1,
            'tokenBegin'     => '<h1 class=part>',
         }, {
            'tokenBegin'     => '<h1 class=-[appendix|prelude|hidden|part]>'
         }, {
            'tokenBegin'     => '<h2>',
            'level'          => 2
         }, {
            'tokenBegin'     => '<h3>',
            'level'          => 3
         }, {
            'groupId'        => 'appendix',
            'tokenBegin'     => '<h1 class=appendix>',
			   'numberingStyle' => 'upper-alpha',
         }, {
            'groupId'        => 'appendix',
            'tokenBegin'     => '<h2 class=appendix>',
            'level'          => 2
         }, {
            'groupId'        => 'prelude',
            'tokenBegin'     => '<h1 class=prelude>',
            'level'          => 1,
			   'doNumberToken'  => 0,
         }],
	});
	$tocOfFigures->setOptions({
		'doNumberToken'        => 1,
		'insertionPoint'       => "after <!-- Table of Figures -->",
		'templateLevelBegin'   => '"<ol>\n"',
		'templateLevelEnd'     => '"</ol>\n"',
		'templateTokenNumber'  => '"Figure $node: &nbsp;"',
		'tokenToToc'           => [{
				'groupId'        => 'Figure',
				'tokenBegin'     => '<p class=captionFigure>'
			}]
	});
	$tocOfTables->setOptions({
		'doNumberToken'        => 1,
		'insertionPoint'       => "after <!-- Table of Tables -->",
		'templateLevelBegin'   => '"<ol>\n"',
		'templateLevelEnd'     => '"</ol>\n"',
		'templateTokenNumber'  => '"Table $node: &nbsp;"',
		'tokenToToc'           => [{
				'groupId'        => 'Table',
				'tokenBegin'     => '<p class=captionTable>'
			}]
	});
		# Insert ToC
	$tocUpdator->updateFile(
		[$toc, $tocOfFigures, $tocOfTables], 
		't/ManualTest/manualTest1.htm', {
			 'doUseGroupsGlobal' => 1,
			 'output'            => \$output,
			 'outputFile'        => 't/ManualTest/manualTest3.htm'
		}
	);
	ok($output, <<EOT);
<html>
<head>
   <title>Manual</title>
    <style type="text/css">
       ul.toc_appendix1 { 
         list-style-type: none;
         margin-left: 0;
         margin-top: 1em;
         margin-bottom: 1em;
       }
       ul.toc_h1 {
         list-style-type: none;
         margin-left: 1;
         margin-top: 1em;
         margin-bottom: 1em;
       }
       ul.toc_h2 {
         list-style-type: none;
       }
       ul.toc_h3 {
         list-style-type: none;
       }
       ul.toc_part1 {
         list-style-type: none;
         margin-left: 1;
         margin-top: 1em;
         margin-bottom: 1em;
       }
       ul.toc_prelude1 {
         list-style: none;
       }
       p.captionFigure {
         font-style: italic;
         font-weight: bold;
       }
       p.captionTable {
         font-style: italic;
         font-weight: bold;
       }
    </style>
</head>
<body>

<!-- #BeginTocAnchorNameBegin --><a name=prelude-1><!-- #EndTocAnchorNameBegin --><h1 class=prelude>Preface</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
Better C than never.

<h1 class=hidden>Table of Contents</h1>
<!-- Table of Contents --><!-- #BeginToc -->
<!-- Table of Contents generated by Perl - HTML::Toc -->
<ul class=toc_prelude1>
   <li><a href=#prelude-1>Preface</a>
   <li><a href=#prelude-2>Table of Figures</a>
   <li><a href=#prelude-3>Table of Tables</a>
   <li><a href=#prelude-4>Introduction</a>
   <ul class=toc_part1>
      <li>Part 1 &nbsp;<a href=#part-1>Disks</a>
      <ul class=toc_h1>
         <li>1. &nbsp;<a href=#h-1>Compiler Disk v1</a>
         <ul class=toc_h2>
            <li>1. &nbsp;<a href=#h-1.1>System</a>
            <li>2. &nbsp;<a href=#h-1.2>Standard Library</a>
         </ul>
         <li>2. &nbsp;<a href=#h-2>Compiler Disk v2</a>
         <ul class=toc_h2>
            <li>1. &nbsp;<a href=#h-2.1>System</a>
            <ul class=toc_h3>
               <li>1. &nbsp;<a href=#h-2.1.1>parser.com</a>
               <li>2. &nbsp;<a href=#h-2.1.2>compiler.com</a>
               <li>3. &nbsp;<a href=#h-2.1.3>linker.com</a>
            </ul>
            <li>2. &nbsp;<a href=#h-2.2>Standard Library</a>
         </ul>
         <li>3. &nbsp;<a href=#h-3>Library System Disk</a>
      </ul>
      <li>Part 2 &nbsp;<a href=#part-2>Personal</a>
      <ul class=toc_h1>
         <li>4. &nbsp;<a href=#h-4>Tips & Tricks</a>
      </ul>
      <li>Part 3 &nbsp;<a href=#part-3>Appendixes</a>
      <ul class=toc_appendix1>
         <li>A &nbsp;<a href=#appendix-A>Functions Standard Library v1</a>
         <li>B &nbsp;<a href=#appendix-B>Functions Standard Library v2</a>
         <li>C &nbsp;<a href=#appendix-C>Functions Graphic Library</a>
      </ul>
   </ul>
   <li><a href=#prelude-5>Bibliography</a>
</ul>
<!-- End of generated Table of Contents -->
<!-- #EndToc -->

<!-- #BeginTocAnchorNameBegin --><a name=prelude-2><!-- #EndTocAnchorNameBegin --><h1 class=prelude>Table of Figures</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- Table of Figures --><!-- #BeginToc -->
<!-- Table of Contents generated by Perl - HTML::Toc -->
<ol>
   <li><a href=#Figure-1>Contents Compiler Disk v1</a>
   <li><a href=#Figure-2>Contents Compiler Disk v2</a>
</ol>
<!-- End of generated Table of Contents -->
<!-- #EndToc -->

<!-- #BeginTocAnchorNameBegin --><a name=prelude-3><!-- #EndTocAnchorNameBegin --><h1 class=prelude>Table of Tables</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- Table of Tables --><!-- #BeginToc -->
<!-- Table of Contents generated by Perl - HTML::Toc -->
<ol>
   <li><a href=#Table-1>Compile Steps</a>
</ol>
<!-- End of generated Table of Contents -->
<!-- #EndToc -->

<!-- #BeginTocAnchorNameBegin --><a name=prelude-4><!-- #EndTocAnchorNameBegin --><h1 class=prelude>Introduction</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
Thanks to standardisation and the excellent work of the QWERTY corporation it is possible to learn C with almost any C manual.
<!-- #BeginTocAnchorNameBegin --><a name=Table-1><!-- #EndTocAnchorNameBegin --><p class=captionTable><!-- #BeginTocNumber -->Table 1: &nbsp;<!-- #EndTocNumber -->Compile Steps</p><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<ul><pre>
   Parser
   Compiler
   Linker
</pre></ul>

<!-- #BeginTocAnchorNameBegin --><a name=part-1><!-- #EndTocAnchorNameBegin --><h1 class=part><!-- #BeginTocNumber -->Part 1 &nbsp;<!-- #EndTocNumber -->Disks</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=h-1><!-- #EndTocAnchorNameBegin --><h1><!-- #BeginTocNumber -->1 &nbsp;<!-- #EndTocNumber -->Compiler Disk v1</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<img src=img.gif alt="Contents Compiler Disk v1">
<!-- #BeginTocAnchorNameBegin --><a name=Figure-1><!-- #EndTocAnchorNameBegin --><p class=captionFigure><!-- #BeginTocNumber -->Figure 1: &nbsp;<!-- #EndTocNumber -->Contents Compiler Disk v1</p><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->

<!-- #BeginTocAnchorNameBegin --><a name=h-1.1><!-- #EndTocAnchorNameBegin --><h2><!-- #BeginTocNumber -->1.1 &nbsp;<!-- #EndTocNumber -->System</h2><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=h-1.2><!-- #EndTocAnchorNameBegin --><h2><!-- #BeginTocNumber -->1.2 &nbsp;<!-- #EndTocNumber -->Standard Library</h2><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->

<!-- #BeginTocAnchorNameBegin --><a name=h-2><!-- #EndTocAnchorNameBegin --><h1><!-- #BeginTocNumber -->2 &nbsp;<!-- #EndTocNumber -->Compiler Disk v2</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<img src=img.gif alt="Contents Compiler Disk v2">
<!-- #BeginTocAnchorNameBegin --><a name=Figure-2><!-- #EndTocAnchorNameBegin --><p class=captionFigure><!-- #BeginTocNumber -->Figure 2: &nbsp;<!-- #EndTocNumber -->Contents Compiler Disk v2</p><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->

<!-- #BeginTocAnchorNameBegin --><a name=h-2.1><!-- #EndTocAnchorNameBegin --><h2><!-- #BeginTocNumber -->2.1 &nbsp;<!-- #EndTocNumber -->System</h2><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=h-2.1.1><!-- #EndTocAnchorNameBegin --><h3><!-- #BeginTocNumber -->2.1.1 &nbsp;<!-- #EndTocNumber -->parser.com</h3><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=h-2.1.2><!-- #EndTocAnchorNameBegin --><h3><!-- #BeginTocNumber -->2.1.2 &nbsp;<!-- #EndTocNumber -->compiler.com</h3><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=h-2.1.3><!-- #EndTocAnchorNameBegin --><h3><!-- #BeginTocNumber -->2.1.3 &nbsp;<!-- #EndTocNumber -->linker.com</h3><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=h-2.2><!-- #EndTocAnchorNameBegin --><h2><!-- #BeginTocNumber -->2.2 &nbsp;<!-- #EndTocNumber -->Standard Library</h2><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->

<!-- #BeginTocAnchorNameBegin --><a name=h-3><!-- #EndTocAnchorNameBegin --><h1><!-- #BeginTocNumber -->3 &nbsp;<!-- #EndTocNumber -->Library System Disk</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=part-2><!-- #EndTocAnchorNameBegin --><h1 class=part><!-- #BeginTocNumber -->Part 2 &nbsp;<!-- #EndTocNumber -->Personal</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=h-4><!-- #EndTocAnchorNameBegin --><h1><!-- #BeginTocNumber -->4 &nbsp;<!-- #EndTocNumber -->Tips & Tricks</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=part-3><!-- #EndTocAnchorNameBegin --><h1 class=part><!-- #BeginTocNumber -->Part 3 &nbsp;<!-- #EndTocNumber -->Appendixes</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=appendix-A><!-- #EndTocAnchorNameBegin --><h1 class=appendix><!-- #BeginTocNumber -->A &nbsp;<!-- #EndTocNumber -->Functions Standard Library v1</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=appendix-B><!-- #EndTocAnchorNameBegin --><h1 class=appendix><!-- #BeginTocNumber -->B &nbsp;<!-- #EndTocNumber -->Functions Standard Library v2</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=appendix-C><!-- #EndTocAnchorNameBegin --><h1 class=appendix><!-- #BeginTocNumber -->C &nbsp;<!-- #EndTocNumber -->Functions Graphic Library</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=prelude-5><!-- #EndTocAnchorNameBegin --><h1 class=prelude>Bibliography</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
</body>
</html>
EOT
}  # TestInsertManualForUpdating()


#--- TestUpdateManual() -------------------------------------------------------
# function: Test inserting ToC into manual.

sub TestUpdateManual {
	my $output;
		# Create objects
	my $toc          = new HTML::Toc;
	my $tocOfFigures = new HTML::Toc;
	my $tocOfTables  = new HTML::Toc;
	my $tocUpdator   = new HTML::TocUpdator;
	
		# Set ToC options
	$toc->setOptions({
		'doNestGroup'          => 1,
		'doNumberToken'        => 1,
		'insertionPoint'       => "after <!-- Table of Contents -->",
		'templateLevel'        => \&AssembleTocLine,
      'templateLevelBegin'   => '"<ul class=toc_$groupId$level>\n"',
      'templateLevelEnd'     => '"</ul>\n"',
		'templateTokenNumber'  => \&AssembleTokenNumber,
      'tokenToToc'           => [{
            'groupId'        => 'part',
			   'doNumberToken'  => 1,
            'level'          => 1,
            'tokenBegin'     => '<h1 class=part>',
         }, {
            'tokenBegin'     => '<h1 class=-[appendix|prelude|hidden|part]>'
         }, {
            'tokenBegin'     => '<h2>',
            'level'          => 2
         }, {
            'tokenBegin'     => '<h3>',
            'level'          => 3
         }, {
            'groupId'        => 'appendix',
            'tokenBegin'     => '<h1 class=appendix>',
			   'numberingStyle' => 'upper-alpha',
         }, {
            'groupId'        => 'appendix',
            'tokenBegin'     => '<h2 class=appendix>',
            'level'          => 2
         }, {
            'groupId'        => 'prelude',
            'tokenBegin'     => '<h1 class=prelude>',
            'level'          => 1,
			   'doNumberToken'  => 0,
         }],
	});
	$tocOfFigures->setOptions({
		'doNumberToken'        => 1,
		'insertionPoint'       => "after <!-- Table of Figures -->",
		'templateLevelBegin'   => '"<ol>\n"',
		'templateLevelEnd'     => '"</ol>\n"',
		'templateTokenNumber'  => '"Figure $node: &nbsp;"',
		'tokenToToc'           => [{
				'groupId'        => 'Figure',
				'tokenBegin'     => '<p class=captionFigure>'
			}]
	});
	$tocOfTables->setOptions({
		'doNumberToken'        => 1,
		'insertionPoint'       => "after <!-- Table of Tables -->",
		'templateLevelBegin'   => '"<ol>\n"',
		'templateLevelEnd'     => '"</ol>\n"',
		'templateTokenNumber'  => '"Table $node: &nbsp;"',
		'tokenToToc'           => [{
				'groupId'        => 'Table',
				'tokenBegin'     => '<p class=captionTable>'
			}]
	});
		# Insert ToC
	$tocUpdator->updateFile(
		[$toc, $tocOfFigures, $tocOfTables], 
		't/ManualTest/manualTest3.htm', {
			 'doUseGroupsGlobal' => 1,
			 'output'            => \$output,
			 'outputFile'        => 't/ManualTest/manualTest4.htm'
		}
	);
	ok($output, <<EOT);
<html>
<head>
   <title>Manual</title>
    <style type="text/css">
       ul.toc_appendix1 { 
         list-style-type: none;
         margin-left: 0;
         margin-top: 1em;
         margin-bottom: 1em;
       }
       ul.toc_h1 {
         list-style-type: none;
         margin-left: 1;
         margin-top: 1em;
         margin-bottom: 1em;
       }
       ul.toc_h2 {
         list-style-type: none;
       }
       ul.toc_h3 {
         list-style-type: none;
       }
       ul.toc_part1 {
         list-style-type: none;
         margin-left: 1;
         margin-top: 1em;
         margin-bottom: 1em;
       }
       ul.toc_prelude1 {
         list-style: none;
       }
       p.captionFigure {
         font-style: italic;
         font-weight: bold;
       }
       p.captionTable {
         font-style: italic;
         font-weight: bold;
       }
    </style>
</head>
<body>

<!-- #BeginTocAnchorNameBegin --><a name=prelude-1><!-- #EndTocAnchorNameBegin --><h1 class=prelude>Preface</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
Better C than never.

<h1 class=hidden>Table of Contents</h1>
<!-- Table of Contents --><!-- #BeginToc -->
<!-- Table of Contents generated by Perl - HTML::Toc -->
<ul class=toc_prelude1>
   <li><a href=#prelude-1>Preface</a>
   <li><a href=#prelude-2>Table of Figures</a>
   <li><a href=#prelude-3>Table of Tables</a>
   <li><a href=#prelude-4>Introduction</a>
   <ul class=toc_part1>
      <li>Part 1 &nbsp;<a href=#part-1>Disks</a>
      <ul class=toc_h1>
         <li>1. &nbsp;<a href=#h-1>Compiler Disk v1</a>
         <ul class=toc_h2>
            <li>1. &nbsp;<a href=#h-1.1>System</a>
            <li>2. &nbsp;<a href=#h-1.2>Standard Library</a>
         </ul>
         <li>2. &nbsp;<a href=#h-2>Compiler Disk v2</a>
         <ul class=toc_h2>
            <li>1. &nbsp;<a href=#h-2.1>System</a>
            <ul class=toc_h3>
               <li>1. &nbsp;<a href=#h-2.1.1>parser.com</a>
               <li>2. &nbsp;<a href=#h-2.1.2>compiler.com</a>
               <li>3. &nbsp;<a href=#h-2.1.3>linker.com</a>
            </ul>
            <li>2. &nbsp;<a href=#h-2.2>Standard Library</a>
         </ul>
         <li>3. &nbsp;<a href=#h-3>Library System Disk</a>
      </ul>
      <li>Part 2 &nbsp;<a href=#part-2>Personal</a>
      <ul class=toc_h1>
         <li>4. &nbsp;<a href=#h-4>Tips & Tricks</a>
      </ul>
      <li>Part 3 &nbsp;<a href=#part-3>Appendixes</a>
      <ul class=toc_appendix1>
         <li>A &nbsp;<a href=#appendix-A>Functions Standard Library v1</a>
         <li>B &nbsp;<a href=#appendix-B>Functions Standard Library v2</a>
         <li>C &nbsp;<a href=#appendix-C>Functions Graphic Library</a>
      </ul>
   </ul>
   <li><a href=#prelude-5>Bibliography</a>
</ul>
<!-- End of generated Table of Contents -->
<!-- #EndToc -->

<!-- #BeginTocAnchorNameBegin --><a name=prelude-2><!-- #EndTocAnchorNameBegin --><h1 class=prelude>Table of Figures</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- Table of Figures --><!-- #BeginToc -->
<!-- Table of Contents generated by Perl - HTML::Toc -->
<ol>
   <li><a href=#Figure-1>Contents Compiler Disk v1</a>
   <li><a href=#Figure-2>Contents Compiler Disk v2</a>
</ol>
<!-- End of generated Table of Contents -->
<!-- #EndToc -->

<!-- #BeginTocAnchorNameBegin --><a name=prelude-3><!-- #EndTocAnchorNameBegin --><h1 class=prelude>Table of Tables</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- Table of Tables --><!-- #BeginToc -->
<!-- Table of Contents generated by Perl - HTML::Toc -->
<ol>
   <li><a href=#Table-1>Compile Steps</a>
</ol>
<!-- End of generated Table of Contents -->
<!-- #EndToc -->

<!-- #BeginTocAnchorNameBegin --><a name=prelude-4><!-- #EndTocAnchorNameBegin --><h1 class=prelude>Introduction</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
Thanks to standardisation and the excellent work of the QWERTY corporation it is possible to learn C with almost any C manual.
<!-- #BeginTocAnchorNameBegin --><a name=Table-1><!-- #EndTocAnchorNameBegin --><p class=captionTable><!-- #BeginTocNumber -->Table 1: &nbsp;<!-- #EndTocNumber -->Compile Steps</p><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<ul><pre>
   Parser
   Compiler
   Linker
</pre></ul>

<!-- #BeginTocAnchorNameBegin --><a name=part-1><!-- #EndTocAnchorNameBegin --><h1 class=part><!-- #BeginTocNumber -->Part 1 &nbsp;<!-- #EndTocNumber -->Disks</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=h-1><!-- #EndTocAnchorNameBegin --><h1><!-- #BeginTocNumber -->1 &nbsp;<!-- #EndTocNumber -->Compiler Disk v1</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<img src=img.gif alt="Contents Compiler Disk v1">
<!-- #BeginTocAnchorNameBegin --><a name=Figure-1><!-- #EndTocAnchorNameBegin --><p class=captionFigure><!-- #BeginTocNumber -->Figure 1: &nbsp;<!-- #EndTocNumber -->Contents Compiler Disk v1</p><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->

<!-- #BeginTocAnchorNameBegin --><a name=h-1.1><!-- #EndTocAnchorNameBegin --><h2><!-- #BeginTocNumber -->1.1 &nbsp;<!-- #EndTocNumber -->System</h2><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=h-1.2><!-- #EndTocAnchorNameBegin --><h2><!-- #BeginTocNumber -->1.2 &nbsp;<!-- #EndTocNumber -->Standard Library</h2><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->

<!-- #BeginTocAnchorNameBegin --><a name=h-2><!-- #EndTocAnchorNameBegin --><h1><!-- #BeginTocNumber -->2 &nbsp;<!-- #EndTocNumber -->Compiler Disk v2</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<img src=img.gif alt="Contents Compiler Disk v2">
<!-- #BeginTocAnchorNameBegin --><a name=Figure-2><!-- #EndTocAnchorNameBegin --><p class=captionFigure><!-- #BeginTocNumber -->Figure 2: &nbsp;<!-- #EndTocNumber -->Contents Compiler Disk v2</p><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->

<!-- #BeginTocAnchorNameBegin --><a name=h-2.1><!-- #EndTocAnchorNameBegin --><h2><!-- #BeginTocNumber -->2.1 &nbsp;<!-- #EndTocNumber -->System</h2><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=h-2.1.1><!-- #EndTocAnchorNameBegin --><h3><!-- #BeginTocNumber -->2.1.1 &nbsp;<!-- #EndTocNumber -->parser.com</h3><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=h-2.1.2><!-- #EndTocAnchorNameBegin --><h3><!-- #BeginTocNumber -->2.1.2 &nbsp;<!-- #EndTocNumber -->compiler.com</h3><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=h-2.1.3><!-- #EndTocAnchorNameBegin --><h3><!-- #BeginTocNumber -->2.1.3 &nbsp;<!-- #EndTocNumber -->linker.com</h3><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=h-2.2><!-- #EndTocAnchorNameBegin --><h2><!-- #BeginTocNumber -->2.2 &nbsp;<!-- #EndTocNumber -->Standard Library</h2><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->

<!-- #BeginTocAnchorNameBegin --><a name=h-3><!-- #EndTocAnchorNameBegin --><h1><!-- #BeginTocNumber -->3 &nbsp;<!-- #EndTocNumber -->Library System Disk</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=part-2><!-- #EndTocAnchorNameBegin --><h1 class=part><!-- #BeginTocNumber -->Part 2 &nbsp;<!-- #EndTocNumber -->Personal</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=h-4><!-- #EndTocAnchorNameBegin --><h1><!-- #BeginTocNumber -->4 &nbsp;<!-- #EndTocNumber -->Tips & Tricks</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=part-3><!-- #EndTocAnchorNameBegin --><h1 class=part><!-- #BeginTocNumber -->Part 3 &nbsp;<!-- #EndTocNumber -->Appendixes</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=appendix-A><!-- #EndTocAnchorNameBegin --><h1 class=appendix><!-- #BeginTocNumber -->A &nbsp;<!-- #EndTocNumber -->Functions Standard Library v1</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=appendix-B><!-- #EndTocAnchorNameBegin --><h1 class=appendix><!-- #BeginTocNumber -->B &nbsp;<!-- #EndTocNumber -->Functions Standard Library v2</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=appendix-C><!-- #EndTocAnchorNameBegin --><h1 class=appendix><!-- #BeginTocNumber -->C &nbsp;<!-- #EndTocNumber -->Functions Graphic Library</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
<!-- #BeginTocAnchorNameBegin --><a name=prelude-5><!-- #EndTocAnchorNameBegin --><h1 class=prelude>Bibliography</h1><!-- #BeginTocAnchorNameEnd --></a><!-- #EndTocAnchorNameEnd -->
</body>
</html>
EOT
}  # TestUpdateManual()


	# Test inserting ToC into manual
TestInsertManualToc();
	# Test inserting ToC with update tokens into manual
TestInsertManualForUpdating();
	# Test updating ToC
TestUpdateManual();
