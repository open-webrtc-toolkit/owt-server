#=== HTML::TocGenerator =======================================================
# function: Generate 'HTML::Toc' table of contents.
# note:     - 'TT' is an abbrevation of 'TocToken'.


package HTML::TocGenerator;


use strict;
use HTML::Parser;


BEGIN {
	use vars qw(@ISA $VERSION);

	$VERSION = '0.91';

	@ISA = qw(HTML::Parser);
}


	# Warnings
use constant WARNING_NESTED_ANCHOR_PS_WITHIN_PS               => 1;
use constant WARNING_TOC_ATTRIBUTE_PS_NOT_AVAILABLE_WITHIN_PS => 2;


use constant TOC_TOKEN_ID       => 0;
use constant TOC_TOKEN_INCLUDE  => 1;
use constant TOC_TOKEN_EXCLUDE  => 2;
use constant TOC_TOKEN_TOKENS   => 3;
use constant TOC_TOKEN_GROUP    => 4;
use constant TOC_TOKEN_TOC      => 5;

	# Token types
use constant TT_TAG_BEGIN                => 0;
use constant TT_TAG_END                  => 1;
use constant TT_TAG_TYPE_END             => 2;
use constant TT_INCLUDE_ATTRIBUTES_BEGIN => 3;
use constant TT_EXCLUDE_ATTRIBUTES_BEGIN => 4;
use constant TT_INCLUDE_ATTRIBUTES_END   => 5;
use constant TT_EXCLUDE_ATTRIBUTES_END   => 6;
use constant TT_GROUP                    => 7;
use constant TT_TOC                      => 8;
use constant TT_ATTRIBUTES_TOC           => 9;


use constant CONTAINMENT_INCLUDE => 0;
use constant CONTAINMENT_EXCLUDE => 1;

use constant TEMPLATE_ANCHOR            => '$groupId."-".$node';
use constant TEMPLATE_ANCHOR_HREF       => 
					'"<a href=#".' . TEMPLATE_ANCHOR . '.">"';
use constant TEMPLATE_ANCHOR_HREF_FILE  => 
					'"<a href=".$file."#".' . TEMPLATE_ANCHOR . '.">"';
use constant TEMPLATE_ANCHOR_NAME       => 
					'"<a name=".' . TEMPLATE_ANCHOR . '.">"';

use constant TEMPLATE_TOKEN_NUMBER      => '"$node &nbsp"';


use constant TT_TOKENTYPE_START        => 0;
use constant TT_TOKENTYPE_END          => 1;
use constant TT_TOKENTYPE_TEXT         => 2;
use constant TT_TOKENTYPE_COMMENT      => 3;
use constant TT_TOKENTYPE_DECLARATION  => 4;


END {}


#--- HTML::TocGenerator::new() ------------------------------------------------
# function: Constructor

sub new {
		# Get arguments
	my ($aType) = @_;
	my $self = $aType->SUPER::new;
		# Bias to not generate ToC
	$self->{_doGenerateToc} = 0;
		# Bias to not use global groups
	$self->{_doUseGroupsGlobal} = 0;
		# Output
	$self->{output} = "";
		# Reset internal variables
	$self->_resetBatchVariables();

	$self->{options} = {};

	return $self;
}  # new()


#--- HTML::TocGenerator::_deinitializeBatch() ---------------------------------

sub _deinitializeBatch() {
		# Get arguments
	my ($self) = @_;
}  # _deinitializeBatch()


#--- HTML::TocGenerator::_deinitializeExtenderBatch() -------------------------

sub _deinitializeExtenderBatch() {
		# Get arguments
	my ($self) = @_;
		# Do general batch deinitialization
	$self->_deinitializeBatch();
		# Indicate end of ToC generation
	$self->{_doGenerateToc} = 0;
		# Reset batch variables
	$self->_resetBatchVariables();
}  # _deinitializeExtenderBatch()


#--- HTML::TocGenerator::_deinitializeGeneratorBatch() ------------------------

sub _deinitializeGeneratorBatch() {
		# Get arguments
	my ($self) = @_;
		# Do 'extender' batch deinitialization
	$self->_deinitializeExtenderBatch();
}  # _deinitializeBatchGenerator()


#--- HTML::TocGenerator::_doesHashContainHash() -------------------------------
# function: Determines whether hash1 matches regular expressions of hash2.
# args:     - $aHash1
#           - $aHash2
#           - $aContainmentType: 0 (include) or 1 (exclude)
# returns:  True (1) if hash1 satisfies hash2, 0 if not.  For example, with the
#           following hashes:
#
#              %hash1 = {							%hash2 = {
#                 'class' => 'header'				'class' => '^h'
#                 'id'    => 'intro'         }
#              }
#
#           the routine will return 1 if 'aContainmentType' equals 0, cause
#           'hash1' satisfies the conditions of 'hash2'.  The routine will
#           return 0 if 'aContainmentType' equals 1, cause 'hash1' doesn't
#           exclude the conditions of 'hash2'.
# note:     Class function.

sub _doesHashContainHash {
		# Get arguments
	my ($aHash1, $aHash2, $aContainmentType) = @_;
		# Local variables
	my ($key1, $value1, $key2, $value2, $result);
		# Bias to success
	$result = 1;
		# Loop through hash2
	HASH2: while (($key2, $value2) = each %$aHash2) {
		# Yes, values are available;
			# Get value1
		$value1 = $aHash1->{$key2};
			# Does value1 match criteria of value2?
		if (defined($value1) && $value1 =~ m/$value2/) {
			# Yes, value1 matches criteria of value2;
				# Containment type was exclude?
			if ($aContainmentType == CONTAINMENT_EXCLUDE) {
				# Yes, containment type was exclude;
					# Indicate condition fails
				$result = 0;
					# Reset 'each' iterator which we're going to break
				keys %$aHash2;
					# Break loop
				last HASH2;
			}
		}
		else {
			# No, value1 didn't match criteria of value2;
				# Containment type was include?
			if ($aContainmentType == CONTAINMENT_INCLUDE) {
				# Yes, containment type was include;
					# Indicate condition fails
				$result = 0;
					# Reset 'each' iterator which we're going to break
				keys %$aHash2;
					# Break loop
				last HASH2;
			}
		}
	}
		# Return value
	return $result;
}  # _doesHashContainHash()


#--- HTML::TocGenerator::_extend() --------------------------------------------
# function: Extend ToC.
#           - $aString: String to parse.

sub _extend {
		# Get arguments
	my ($self, $aFile) = @_;
		# Local variables
	my ($file);
		# Parse string
	$self->parse($aFile);
		# Flush remaining buffered text
	$self->eof();
}  # _extend()


#--- HTML::TocGenerator::_extendFromFile() ------------------------------------
# function: Extend ToC.
#           - $aFile: (reference to array of) file to parse.

sub _extendFromFile {
		# Get arguments
	my ($self, $aFile) = @_;
		# Local variables
	my ($file, @files);
		# Dereference array reference or make array of file specification
	@files = (ref($aFile) =~ m/ARRAY/) ? @$aFile : ($aFile);
		# Loop through files
	foreach $file (@files) {
			# Store filename
		$self->{_currentFile} = $file;
			# Parse file
		$self->parse_file($file);
			# Flush remaining buffered text
		$self->eof();
	}
}  # _extendFromFile()


#--- HTML::TocGenerator::_formatHeadingLevel() --------------------------------
# function: Format heading level.
# args:     - $aLevel: Level of current heading
#           - $aClass: Class of current heading
#           - $aGroup: Group of current heading
#           - $aToc: Toc of current heading

sub _formatHeadingLevel {
		# Get arguments
	my ($self, $aLevel, $aClass, $aGroup, $aToc) = @_;
		# Local variables
	my ($result, $headingNumber, $numberingStyle);

	$headingNumber = $self->_getGroupIdManager($aToc)->
		{levels}{$aClass}[$aLevel - 1] || 0;

		# Alias numbering style of current group
	$numberingStyle = $aGroup->{numberingStyle};

	SWITCH: {
		if ($numberingStyle eq "decimal") {
			$result = $headingNumber;
			last SWITCH;
		}
		if ($numberingStyle eq "lower-alpha") {
			$result = chr($headingNumber + ord('a') - 1);
			last SWITCH;
		}
		if ($numberingStyle eq "upper-alpha") {
			$result = chr($headingNumber + ord('A') - 1);
			last SWITCH;
		}
		if ($numberingStyle eq "lower-roman") {
			require Roman;
			$result = Roman::roman($headingNumber);
			last SWITCH;
		}
		if ($numberingStyle eq "upper-roman") {
			require Roman;
			$result = Roman::Roman($headingNumber);
			last SWITCH;
		}
		die "Unknown case: $numberingStyle";
	}
		# Return value
	return $result;
}	# _formatHeadingLevel()


#--- HTML::TocGenerator::_formatTocNode() -------------------------------------
# function: Format heading node.
# args:     - $aLevel: Level of current heading
#           - $aClass: Class of current heading
#           - $aGroup: Group of current heading
#           - $aToc: Toc of current heading

sub _formatTocNode {
		# Get arguments
	my ($self, $aLevel, $aClass, $aGroup, $aToc) = @_;
		# Local variables
	my ($result, $level, $levelGroups);

		# Alias 'levelGroups' of right 'groupId'
	$levelGroups = $aToc->{_levelGroups}{$aGroup->{'groupId'}};
		# Loop through levels
	for ($level = 1; $level <= $aLevel; $level++) {
			# If not first level, add dot
		$result = ($result ? $result . "." : $result);
			# Format heading level using argument group
		$result .= $self->_formatHeadingLevel(
			$level, $aClass, @{$levelGroups}[$level - 1], $aToc
		);
	}
		# Return value
	return $result;
}  # _formatTocNode()
     	
     	
#--- HTML::TocGenerator::_generate() ------------------------------------------
# function: Generate ToC.
# args:     - $aString: Reference to string to parse

sub _generate {
		# Get arguments
	my ($self, $aString) = @_;
		# Local variables
	my ($toc);
		# Loop through ToCs
	foreach $toc (@{$self->{_tocs}}) {
			# Clear ToC
		$toc->clear();
	}
		# Extend ToCs
	$self->_extend($aString);
}  # _generate()


#--- HTML::TocGenerator::_generateFromFile() ----------------------------------
# function: Generate ToC.
# args:     - $aFile: (reference to array of) file to parse.

sub _generateFromFile {
		# Get arguments
	my ($self, $aFile) = @_;
		# Local variables
	my ($toc);
		# Loop through ToCs
	foreach $toc (@{$self->{_tocs}}) {
			# Clear ToC
		$toc->clear();
	}
		# Extend ToCs
	$self->_extendFromFile($aFile);
}  # _generateFromFile()


#--- HTML::TocGenerator::_getGroupIdManager() ---------------------------------
# function: Get group id manager.
# args:     - $aToc: Active ToC.
# returns:  Group id levels.

sub _getGroupIdManager {
		# Get arguments
	my ($self, $aToc) = @_;
		# Local variables
	my ($result);
		# Global groups?
	if ($self->{options}{'doUseGroupsGlobal'}) {
		# Yes, global groups;
		$result = $self;
	}
	else {
		# No, local groups;
		$result = $aToc;
	}
		# Return value
	return $result;
}  # _getGroupIdManager()


#--- HTML::TocGenerator::_initializeBatch() -----------------------------------
# function: Initialize batch.  This function is called once when a parse batch
#           is started.
# args:     - $aTocs: Reference to array of tocs.

sub _initializeBatch {
		# Get arguments
	my ($self, $aTocs) = @_;
		# Local variables
	my ($toc);

		# Store reference to tocs
		
		# Is ToC specification reference to array?
	if (ref($aTocs) =~ m/ARRAY/) {
		# Yes, ToC specification is reference to array;
			# Store array reference
		$self->{_tocs} = $aTocs;
	}
	else {
		# No, ToC specification is reference to ToC object;
			# Wrap reference in array reference, containing only one element
		$self->{_tocs} = [$aTocs];
	}
		# Loop through ToCs
	foreach $toc (@{$self->{_tocs}}) {
			# Parse ToC options
		$toc->parseOptions();
	}
}  # _initializeBatch()


#--- HTML::TocGenerator::_initializeExtenderBatch() --------------------------
# function: Initialize 'extender' batch.  This function is called once when a 
#           parse batch is started.
# args:     - $aTocs: Reference to array of tocs.

sub _initializeExtenderBatch {
		# Get arguments
	my ($self, $aTocs) = @_;
		# Do general batch initialization
	$self->_initializeBatch($aTocs);
		# Parse ToC options
	$self->_parseTocOptions();
		# Indicate start of batch
	$self->{_doGenerateToc} = 1;
}  # _initializeExtenderBatch()


#--- HTML::TocGenerator::_initializeGeneratorBatch() --------------------------
# function: Initialize generator batch.  This function is called once when a 
#           parse batch is started.
# args:     - $aTocs: Reference to array of tocs.
#           - $aOptions: optional options

sub _initializeGeneratorBatch {
		# Get arguments
	my ($self, $aTocs, $aOptions) = @_;
		# Add invocation options
	$self->setOptions($aOptions);
		# Option 'doUseGroupsGlobal' specified?
	if (!defined($self->{options}{'doUseGroupsGlobal'})) {
		# No, options 'doUseGroupsGlobal' not specified;
			# Default to no 'doUseGroupsGlobal'
		$self->{options}{'doUseGroupsGlobal'} = 0;
	}
		# Global groups?
	if ($self->{options}{'doUseGroupsGlobal'}) {
		# Yes, global groups;
			# Reset groups and levels
		$self->_resetStackVariables();
	}
		# Do 'extender' batch initialization
	$self->_initializeExtenderBatch($aTocs);
}  # _initializeGeneratorBatch()


#--- HTML::TocGenerator::_linkTocToToken() ------------------------------------
# function: Link ToC to token.
# args:     - $aToc: ToC to add token to.
#           - $aFile
#           - $aGroupId
#           - $aLevel
#           - $aNode
#           - $aGroupLevel
#           - $aLinkType
#           - $aTokenAttributes: reference to hash containing attributes of 
#                currently parsed token

sub _linkTocToToken {
		# Get arguments
	my (
		$self, $aToc, $aFile, $aGroupId, $aLevel, $aNode, $aGroupLevel, 
		$aDoLinkToId, $aTokenAttributes
	) = @_;
		# Local variables
	my ($file, $groupId, $level, $node, $anchorName);
	my ($doInsertAnchor, $doInsertId);

		# Fill local arguments to be used by templates
	$file    = $aFile;
	$groupId = $aGroupId;
	$level   = $aLevel;
	$node    = $aNode;
	
		# Assemble anchor name
	$anchorName = 
		ref($aToc->{_templateAnchorName}) eq "CODE" ?
			&{$aToc->{_templateAnchorName}}(
				$aFile, $aGroupId, $aLevel, $aNode
			) : 
			eval($aToc->{_templateAnchorName});

		# Bias to insert anchor name
	$doInsertAnchor = 1;
	$doInsertId     = 0;
		# Link to 'id'?
	if ($aDoLinkToId) {
		# Yes, link to 'id';
			# Indicate to insert anchor id
		$doInsertAnchor = 0;
		$doInsertId     = 1;
			# Id attribute is available?
		if (defined($aTokenAttributes->{id})) {
			# Yes, id attribute is available;
				# Use existing ids?
			if ($aToc->{options}{'doUseExistingIds'}) {
				# Yes, use existing ids;
					# Use existing id
				$anchorName = $aTokenAttributes->{id};
					# Indicate to not insert id
				$doInsertId = 0;
			}
		}

	}
	else {
		# No, link to 'name';
			# Anchor name is currently active?
		if (defined($self->{_activeAnchorName})) {
			# Yes, anchor name is currently active;
				# Use existing anchors?
			if ($aToc->{options}{'doUseExistingAnchors'}) {
				# Yes, use existing anchors;
					# Use existing anchor name
				$anchorName = $self->{_activeAnchorName};
					# Indicate to not insert anchor name
				$doInsertAnchor = 0;
			}
			else {
				# No, don't use existing anchors; insert new anchor;
					# 
			}
		}
	}

		# Add reference to ToC
	$aToc->{_toc} .= 
		ref($aToc->{_templateAnchorHrefBegin}) eq "CODE" ?
			&{$aToc->{_templateAnchorHrefBegin}}(
				$aFile, $aGroupId, $aLevel, $aNode, $anchorName
			) : 
			eval($aToc->{_templateAnchorHrefBegin});

		# Bias to not output anchor name end
	$self->{_doOutputAnchorNameEnd} = 0;
		# Must anchor be inserted?
	if ($doInsertAnchor) {
		# Yes, anchor must be inserted;
			# Allow adding of anchor name begin token to text by calling 
			# 'anchorNameBegin' method
		$self->anchorNameBegin(
			ref($aToc->{_templateAnchorNameBegin}) eq "CODE" ?
				&{$aToc->{_templateAnchorNameBegin}}(
					$aFile, $aGroupId, $aLevel, $aNode, $anchorName
				) :
				eval($aToc->{_templateAnchorNameBegin}),
			$aToc
		);
	}

		# Must anchorId attribute be inserted?
	if ($doInsertId) {
		# Yes, anchorId attribute must be inserted;
			# Allow adding of anchorId attribute to text by calling 'anchorId'
			# method
		$self->anchorId($anchorName);
	}
}  # _linkTocToToken()


#--- HTML::TocGenerator::_outputAnchorNameEndConditionally() ------------------
# function: Output 'anchor name end' if necessary
# args:     - $aToc: ToC of which 'anchor name end' must be output.

sub _outputAnchorNameEndConditionally {
		# Get arguments
	my ($self, $aToc) = @_;
		# Must anchor name end be output?
	if ($self->{_doOutputAnchorNameEnd}) {
		# Yes, output anchor name end;
			# Allow adding of anchor to text by calling 'anchorNameEnd' 
			# method
		$self->anchorNameEnd(
			ref($aToc->{_templateAnchorNameEnd}) eq "CODE" ?
				&{$aToc->{_templateAnchorNameEnd}} :
				eval($aToc->{_templateAnchorNameEnd}),
			$aToc
		);
	}
}  # _outputAnchorNameEndConditionally()


#--- HTML::TocGenerator::_parseTocOptions() -----------------------------------
# function: Parse ToC options.

sub _parseTocOptions {
		# Get arguments
	my ($self) = @_;
		# Local variables
	my ($toc, $group, $tokens, $tokenType, $i);
		# Create parsers for ToC tokens
	$self->{_tokensTocBegin} = [];
	my $tokenTocBeginParser = HTML::_TokenTocBeginParser->new(
		$self->{_tokensTocBegin}
	);
	my $tokenTocEndParser = HTML::_TokenTocEndParser->new();
		# Loop through ToCs
	foreach $toc (@{$self->{_tocs}}) {
			# Reference parser ToC to current ToC
		$tokenTocBeginParser->setToc($toc);
			# Loop through 'tokenToToc' groups
		foreach $group (@{$toc->{options}{'tokenToToc'}}) {
				# Reference parser group to current group
			$tokenTocBeginParser->setGroup($group);
				# Parse 'tokenToToc' group
			$tokenTocBeginParser->parse($group->{'tokenBegin'});
				# Flush remaining buffered text
			$tokenTocBeginParser->eof();
			$tokenTocEndParser->parse(
				$group->{'tokenEnd'}, 
				$tokenTocBeginParser->{_lastAddedToken},
				$tokenTocBeginParser->{_lastAddedTokenType}
			);
				# Flush remaining buffered text
			$tokenTocEndParser->eof();
		}
	}
}  # _parseTocOptions()


#--- HTML::TocGenerator::_processTocEndingToken() -----------------------------
# function: Process ToC-ending-token.
# args:     - $aTocToken: token which acts as ToC-ending-token.

sub _processTocEndingToken {
		# Get arguments
	my ($self, $aTocToken) = @_;
		# Local variables
	my ($toc);
		# Aliases
	$toc = $aTocToken->[TT_TOC];
		# Link ToC to tokens?
	if ($toc->{options}{'doLinkToToken'}) {
		# Yes, link ToC to tokens;
			# Add anchor href end
		$toc->{_toc} .= 
			(ref($toc->{_templateAnchorHrefEnd}) eq "CODE") ?
				&{$toc->{_templateAnchorHrefEnd}} : 
				eval($toc->{_templateAnchorHrefEnd});

			# Output anchor name end only if necessary
		$self->_outputAnchorNameEndConditionally($toc);
	}
}  # _processTocEndingToken()


#--- HTML::TocGenerator::_processTocStartingToken() ---------------------------
# function: Process ToC-starting-token.
# args:     - $aTocToken: token which acts as ToC-starting-token.
#           - $aTokenType: type of token.  Can be either TT_TOKENTYPE_START,
#                _END, _TEXT, _COMMENT or _DECLARATION.
#           - $aTokenAttributes: reference to hash containing attributes of 
#                currently parsed token
#           - $aTokenOrigText: reference to original token text

sub _processTocStartingToken {
		# Get arguments
	my ($self, $aTocToken, $aTokenType, $aTokenAttributes, $aTokenOrigText) = @_;
		# Local variables
	my ($i, $level, $doLinkToId, $node, $groupLevel);
	my ($file, $tocTokenId, $groupId, $toc, $attribute);
		# Aliases
	$file        = $self->{_currentFile};
	$toc		    = $aTocToken->[TT_TOC];
	$level	    = $aTocToken->[TT_GROUP]{'level'};
	$groupId	    = $aTocToken->[TT_GROUP]{'groupId'};

		# Retrieve 'doLinkToId' setting from either group options or toc options
	$doLinkToId = (defined($aTocToken->[TT_GROUP]{'doLinkToId'})) ?
		$aTocToken->[TT_GROUP]{'doLinkToId'} : $toc->{options}{'doLinkToId'}; 
	
		# Link to 'id' and tokenType isn't 'start'?
	if (($doLinkToId) && ($aTokenType != TT_TOKENTYPE_START)) {
		# Yes, link to 'id' and tokenType isn't 'start';
			# Indicate to *not* link to 'id'
		$doLinkToId = 0;
	}

	if (ref($level) eq "CODE") {
		$level = &$level($self->{_currentFile}, $node);
	}
	if (ref($groupId) eq "CODE") {
		$groupId = &$groupId($self->{_currentFile}, $node);
	}

		# Determine class level

	my $groupIdManager = $self->_getGroupIdManager($toc);
		# Known group?
	if (!exists($groupIdManager->{groupIdLevels}{$groupId})) {
		# No, unknown group;
			# Add group
		$groupIdManager->{groupIdLevels}{$groupId} = keys(
			%{$groupIdManager->{groupIdLevels}}
		) + 1;
	}
	$groupLevel = $groupIdManager->{groupIdLevels}{$groupId};

		# Temporarily allow symbolic references
	#no strict qw(refs);
		# Increase level
	$groupIdManager->{levels}{$groupId}[$level - 1] += 1;
		# Reset remaining levels of same group
	for ($i = $level; $i < @{$groupIdManager->{levels}{$groupId}}; $i++) {
		$groupIdManager->{levels}{$groupId}[$i] = 0;
	}

		# Assemble numeric string indicating current level
	$node = $self->_formatTocNode(
		$level, $groupId, $aTocToken->[TT_GROUP], $toc
	);

		# Add newline if _toc not empty
	if ($toc->{_toc}) { 
		$toc->{_toc} .= "\n";
	}

		# Add toc item info
	$toc->{_toc} .= "$level $groupLevel $groupId $node " .
		$groupIdManager->{levels}{$groupId}[$level - 1] . " ";

		# Add value of 'id' attribute if available
	if (defined($aTokenAttributes->{id})) {
		$toc->{_toc} .= $aTokenAttributes->{id};
	}
	$toc->{_toc} .= " ";
		# Link ToC to tokens?
	if ($toc->{options}{'doLinkToToken'}) {
		# Yes, link ToC to tokens;
			# Link ToC to token
		$self->_linkTocToToken(
			$toc, $file, $groupId, $level, $node, $groupLevel, $doLinkToId,
			$aTokenAttributes
		);
	}

		# Number tokens?
	if (
		$aTocToken->[TT_GROUP]{'doNumberToken'} || 
		(
			! defined($aTocToken->[TT_GROUP]{'doNumberToken'}) && 
			$toc->{options}{'doNumberToken'}
		)
	) {
		# Yes, number tokens;
			# Add number by calling 'number' method
		$self->number(
			ref($toc->{_templateTokenNumber}) eq "CODE" ?
				&{$toc->{_templateTokenNumber}}(
					$node, $groupId, $file, $groupLevel, $level, $toc
				) : 
				eval($toc->{_templateTokenNumber}),
			$toc
		);
	}

		# Must attribute be used as ToC text?
	if (defined($aTocToken->[TT_ATTRIBUTES_TOC])) {
		# Yes, attribute must be used as ToC text;
			# Loop through attributes
		foreach $attribute (@{$aTocToken->[TT_ATTRIBUTES_TOC]}) {
				# Attribute is available?
			if (defined($$aTokenAttributes{$attribute})) {
				# Yes, attribute is available;
					# Add attribute value to ToC
				$self->_processTocText($$aTokenAttributes{$attribute}, $toc);
			}
			else {
				# No, attribute isn't available;
					# Show warning
				$self->_showWarning(
					WARNING_TOC_ATTRIBUTE_PS_NOT_AVAILABLE_WITHIN_PS,
					[$attribute, $$aTokenOrigText]
				);
			}
				# Output anchor name end only if necessary
			#$self->_outputAnchorNameEndConditionally($toc);
				# End attribute
			$self->_processTocEndingToken($aTocToken);
		}
	}
	else {
		# No, attribute mustn't be used as ToC text;
			# Add end token to 'end token array'
		push(
			@{$self->{_tokensTocEnd}[$aTocToken->[TT_TAG_TYPE_END]]}, $aTocToken
		);
	}
}  # _processTocStartingToken()


#--- HTML::TocGenerator::_processTocText() ------------------------------------
# function: This function processes text which must be added to the preliminary
#           ToC.
# args:     - $aText: Text to add to ToC.
#           - $aToc: ToC to add text to.

sub _processTocText {
		# Get arguments
	my ($self, $aText, $aToc) = @_;
		# Add text to ToC
	$aToc->{_toc} .= $aText;
}  # _processTocText()


#--- HTML::TocGenerator::_processTokenAsTocEndingToken() ----------------------
# function: Check for token being a token to use for triggering the end of
#           a ToC line and process it accordingly.
# args:     - $aTokenType: type of token: 'start', 'end', 'comment' or 'text'.
#           - $aTokenId: token id of currently parsed token

sub _processTokenAsTocEndingToken {
		# Get arguments
	my ($self, $aTokenType, $aTokenId) = @_;
		# Local variables
	my ($i, $tokenId, $toc, $tokens);
		# Loop through dirty start tokens
	$i = 0;

		# Alias token array of right type
	$tokens = $self->{_tokensTocEnd}[$aTokenType];
		# Loop through token array
	while ($i < scalar @$tokens) {
			# Aliases
		$tokenId = $tokens->[$i][TT_TAG_END];
			# Does current end tag equals dirty tag?
		if ($aTokenId eq $tokenId) {
			# Yes, current end tag equals dirty tag;
				# Process ToC-ending-token
			$self->_processTocEndingToken($tokens->[$i]);
				# Remove dirty tag from array, automatically advancing to
				# next token
			splice(@$tokens, $i, 1);
		}
		else {
			# No, current end tag doesn't equal dirty tag;
				# Advance to next token
			$i++;
		}
	}
}  # _processTokenAsTocEndingToken()


#--- HTML::TocGenerator::_processTokenAsTocStartingToken() --------------------
# function: Check for token being a ToC-starting-token and process it 
#           accordingly.
# args:     - $aTokenType: type of token.  Can be either TT_TOKENTYPE_START,
#                _END, _TEXT, _COMMENT or _DECLARATION.
#           - $aTokenId: token id of currently parsed token
#           - $aTokenAttributes: reference to hash containing attributes of 
#                currently parsed token
#           - $aTokenOrigText: reference to original text of token
# returns:  1 if successful, i.e. token is processed as ToC-starting-token, 0
#           if not.

sub _processTokenAsTocStartingToken {
		# Get arguments
	my ($self, $aTokenType, $aTokenId, $aTokenAttributes, $aTokenOrigText) = @_;
		# Local variables
	my ($level, $levelToToc, $groupId, $groupToToc);
	my ($result, $tocToken, $tagBegin, @tokensTocBegin, $fileSpec);
		# Bias to token not functioning as ToC-starting-token
	$result = 0;
		# Loop through start tokens of right type
	foreach $tocToken (@{$self->{_tokensTocBegin}[$aTokenType]}) {
			# Alias file filter
		$fileSpec = $tocToken->[TT_GROUP]{'fileSpec'};
			# File matches?
		if (!defined($fileSpec) || (
			defined($fileSpec) &&
			($self->{_currentFile} =~ m/$fileSpec/)
		)) {
			# Yes, file matches;
				# Alias tag begin
			$tagBegin = $tocToken->[TT_TAG_BEGIN];
				# Tag and attributes match?
			if (
				defined($tagBegin) && 
				($aTokenId =~ m/$tagBegin/) && 
				HTML::TocGenerator::_doesHashContainHash(
					$aTokenAttributes, $tocToken->[TT_INCLUDE_ATTRIBUTES_BEGIN], 0
				) &&
				HTML::TocGenerator::_doesHashContainHash(
					$aTokenAttributes, $tocToken->[TT_EXCLUDE_ATTRIBUTES_BEGIN], 1
				)
			) {
				# Yes, tag and attributes match;
					# Aliases
				$level	    = $tocToken->[TT_GROUP]{'level'};
				$levelToToc = $tocToken->[TT_TOC]{options}{'levelToToc'};
				$groupId     = $tocToken->[TT_GROUP]{'groupId'}; 
				$groupToToc = $tocToken->[TT_TOC]{options}{'groupToToc'};
					# Must level and group be processed?
				if (
					($level =~ m/$levelToToc/) &&
					($groupId =~ m/$groupToToc/)
				) {
					# Yes, level and group must be processed;
						# Indicate token acts as ToC-starting-token
					$result = 1;
						# Process ToC-starting-token
					$self->_processTocStartingToken(
						$tocToken, $aTokenType, $aTokenAttributes, $aTokenOrigText
					);
				}
			}
		}
	}
		# Return value
	return $result;
}  # _processTokenAsTocStartingToken()


#--- HTML::TocGenerator::_resetBatchVariables() -------------------------------
# function: Reset variables which are set because of batch invocation.

sub _resetBatchVariables {
		# Get arguments
	my ($self) = @_;

		# Filename of current file being parsed, empty string if not available
	$self->{_currentFile} = "";
		# Arrays containing start, end, comment, text & declaration tokens which 
		# must trigger the ToC assembling.  Each array element may contain a 
		# reference to an array containing the following elements:
		#
      #    TT_TAG_BEGIN                => 0;
      #    TT_TAG_END                  => 1;
      #    TT_TAG_TYPE_END             => 2;
      #    TT_INCLUDE_ATTRIBUTES_BEGIN => 3;
      #    TT_EXCLUDE_ATTRIBUTES_BEGIN => 4;
      #    TT_INCLUDE_ATTRIBUTES_END   => 5;
      #    TT_EXCLUDE_ATTRIBUTES_END   => 6;
      #    TT_GROUP                    => 7;
      #    TT_TOC                      => 8;
		#    TT_ATTRIBUTES_TOC           => 9;
		#
	$self->{_tokensTocBegin} = [
		[],  # TT_TOKENTYPE_START      
		[],  # TT_TOKENTYPE_END        
		[],  # TT_TOKENTYPE_COMMENT    
		[],  # TT_TOKENTYPE_TEXT       
		[]   # TT_TOKENTYPE_DECLARATION
	];
	$self->{_tokensTocEnd} = [
		[],  # TT_TOKENTYPE_START      
		[],  # TT_TOKENTYPE_END        
		[],  # TT_TOKENTYPE_COMMENT    
		[],  # TT_TOKENTYPE_TEXT       
		[]   # TT_TOKENTYPE_DECLARATION
	];
		# TRUE if ToCs have been initialized, FALSE if not.
	$self->{_doneInitializeTocs} = 0;
		# Array of ToCs to process
	$self->{_tocs} = [];
		# Active anchor name
	$self->{_activeAnchorName} = undef;
}  # _resetBatchVariables()


#--- HTML::TocGenerator::_resetStackVariables() -------------------------------
# function: Reset variables which cumulate during ToC generation.

sub _resetStackVariables {
		# Get arguments
	my ($self) = @_;
		# Reset variables
	$self->{levels}        = undef;
	$self->{groupIdLevels} = undef;
}  # _resetStackVariables()


#--- HTML::TocGenerator::_setActiveAnchorName() -------------------------------
# function: Set active anchor name.
# args:     - aAnchorName: Name of anchor name to set active.

sub _setActiveAnchorName {
		# Get arguments
	my ($self, $aAnchorName) = @_;
		# Set active anchor name
	$self->{_activeAnchorName} = $aAnchorName;
}  # _setActiveAnchorName()


#--- HTML::TocGenerator::_showWarning() ---------------------------------------
# function: Show warning.
# args:     - aWarningNr: Number of warning to show.
#           - aWarningArgs: Arguments to display within the warning.

sub _showWarning {
		# Get arguments
	my ($self, $aWarningNr, $aWarningArgs) = @_;
		# Local variables
	my (%warnings);
		# Set warnings
	%warnings = (
		WARNING_NESTED_ANCHOR_PS_WITHIN_PS()               => 
			"Nested anchor '%s' within anchor '%s'.", 
		WARNING_TOC_ATTRIBUTE_PS_NOT_AVAILABLE_WITHIN_PS() =>
			"ToC attribute '%s' not available within token '%s'.",
	);
		# Show warning
	print STDERR "warning ($aWarningNr): " . sprintf($warnings{"$aWarningNr"}, @$aWarningArgs) . "\n";
}  # _showWarning()


#--- HTML::TocGenerator::anchorId() -------------------------------------------
# function: Anchor id processing method.  Leave it up to the descendant to do 
#           something useful with it.
# args:     - $aAnchorId
#           - $aToc: Reference to ToC to which anchorId belongs.

sub anchorId {
}  # anchorId()


#--- HTML::TocGenerator::anchorNameBegin() ------------------------------------
# function: Anchor name begin processing method.  Leave it up to the descendant
#           to do something useful with it.
# args:     - $aAnchorName
#           - $aToc: Reference to ToC to which anchorname belongs.

sub anchorNameBegin {
}  # anchorNameBegin()


#--- HTML::TocGenerator::anchorNameEnd() --------------------------------------
# function: Anchor name end processing method.  Leave it up to the descendant
#           to do something useful with it.
# args:     - $aAnchorName
#           - $aToc: Reference to ToC to which anchorname belongs.

sub anchorNameEnd {
}  # anchorNameEnd()


#--- HTML::TocGenerator::comment() --------------------------------------------
# function: Process comment.
# args:     - $aComment: comment text with '<!--' and '-->' tags stripped off.

sub comment {
		# Get arguments
	my ($self, $aComment) = @_;
		# Must a ToC be generated?
	if ($self->{_doGenerateToc}) {
		# Yes, a ToC must be generated
			# Process end tag as ToC-starting-token
		$self->_processTokenAsTocStartingToken(
			TT_TOKENTYPE_COMMENT, $aComment, undef, \$aComment
		);
			# Process end tag as token which ends ToC registration
		$self->_processTokenAsTocEndingToken(
			TT_TOKENTYPE_COMMENT, $aComment
		);
	}
}  # comment()


#--- HTML::TocGenerator::end() ------------------------------------------------
# function: This function is called every time a closing tag is encountered.
# args:     - $aTag: tag name (in lower case).
#           - $aOrigText: tag name including brackets.

sub end {
		# Get arguments
	my ($self, $aTag, $aOrigText) = @_;
		# Local variables
	my ($tag, $toc, $i);
		# Must a ToC be generated?
	if ($self->{_doGenerateToc}) {
		# Yes, a ToC must be generated
			# Process end tag as ToC-starting-token
		$self->_processTokenAsTocStartingToken(
			TT_TOKENTYPE_END, $aTag, undef, \$aOrigText
		);
			# Process end tag as ToC-ending-token
		$self->_processTokenAsTocEndingToken(
			TT_TOKENTYPE_END, $aTag
		);
			# Tag is of type 'anchor'?
		if (defined($self->{_activeAnchorName}) && ($aTag eq "a")) {
			# Yes, tag is of type 'anchor';
				# Reset dirty anchor
			$self->{_activeAnchorName} = undef;
		}
	}
}  # end()


#--- HTML::TocGenerator::extend() ---------------------------------------------
# function: Extend ToCs.
# args:     - $aTocs: Reference to array of ToC objects
#           - $aString: String to parse.

sub extend {
		# Get arguments
	my ($self, $aTocs, $aString) = @_;
		# Initialize TocGenerator batch
	$self->_initializeExtenderBatch($aTocs);
		# Extend ToCs
	$self->_extend($aString);
		# Deinitialize TocGenerator batch
	$self->_deinitializeExtenderBatch();
}  # extend()


#--- HTML::TocGenerator::extendFromFile() -------------------------------------
# function: Extend ToCs.
# args:     - @aTocs: Reference to array of ToC objects
#           - @aFiles: Reference to array of files to parse.

sub extendFromFile {
		# Get arguments
	my ($self, $aTocs, $aFiles) = @_;
		# Initialize TocGenerator batch
	$self->_initializeExtenderBatch($aTocs);
		# Extend ToCs
	$self->_extendFromFile($aFiles);
		# Deinitialize TocGenerator batch
	$self->_deinitializeExtenderBatch();
}  # extendFromFile()


#--- HTML::TocGenerator::generate() -------------------------------------------
# function: Generate ToC.
# args:     - $aToc: Reference to (array of) ToC object(s)
#           - $aString: Reference to string to parse
#           - $aOptions: optional options

sub generate {
		# Get arguments
	my ($self, $aToc, $aString, $aOptions) = @_;
		# Initialize TocGenerator batch
	$self->_initializeGeneratorBatch($aToc, $aOptions);
		# Do generate ToC
	$self->_generate($aString);
		# Deinitialize TocGenerator batch
	$self->_deinitializeGeneratorBatch();
}  # generate()


#--- HTML::TocGenerator::generateFromFile() -----------------------------------
# function: Generate ToC.
# args:     - $aToc: Reference to (array of) ToC object(s)
#           - $aFile: (reference to array of) file to parse.
#           - $aOptions: optional options

sub generateFromFile {
		# Get arguments
	my ($self, $aToc, $aFile, $aOptions) = @_;
		# Initialize TocGenerator batch
	$self->_initializeGeneratorBatch($aToc, $aOptions);
		# Do generate ToC
	$self->_generateFromFile($aFile);
		# Deinitialize TocGenerator batch
	$self->_deinitializeGeneratorBatch();
}  # generateFromFile()


#--- HTML::TocGenerator::number() ---------------------------------------------
# function: Heading number processing method.  Leave it up to the descendant
#           to do something useful with it.
# args:     - $aNumber
#           - $aToc: Reference to ToC to which anchorname belongs.

sub number {
		# Get arguments
	my ($self, $aNumber, $aToc) = @_;
}  # number()


#--- HTML::TocGenerator::parse() ----------------------------------------------
# function: Parse scalar.
# args:     - $aString: string to parse

sub parse {
		# Get arguments
	my ($self, $aString) = @_;
		# Call ancestor
	$self->SUPER::parse($aString);
}  # parse()


#--- HTML::TocGenerator::parse_file() -----------------------------------------
# function: Parse file.

sub parse_file {
		# Get arguments
	my ($self, $aFile) = @_;
		# Call ancestor
	$self->SUPER::parse_file($aFile);
}  # parse_file()


#--- HTML::TocGenerator::setOptions() -----------------------------------------
# function: Set options.
# args:     - aOptions: Reference to hash containing options.

sub setOptions {
		# Get arguments
	my ($self, $aOptions) = @_;
		# Options are defined?
	if (defined($aOptions)) {
		# Yes, options are defined; add to options
		%{$self->{options}} = (%{$self->{options}}, %$aOptions);
	}
}  # setOptions()


#--- HTML::TocGenerator::start() ----------------------------------------------
# function: This function is called every time an opening tag is encountered.
# args:     - $aTag: tag name (in lower case).
#           - $aAttr: reference to hash containing all tag attributes (in lower
#                case).
#           - $aAttrSeq: reference to array containing all tag attributes (in 
#                lower case) in the original order
#           - $aOrigText: the original HTML text

sub start {
		# Get arguments
	my ($self, $aTag, $aAttr, $aAttrSeq, $aOrigText) = @_;
	$self->{isTocToken} = 0;
		# Start tag is of type 'anchor name'?
	if ($aTag eq "a" && defined($aAttr->{name})) {
		# Yes, start tag is of type 'anchor name';
			# Is another anchor already active?
		if (defined($self->{_activeAnchorName})) {
			# Yes, another anchor is already active;
				# Is the first anchor inserted by 'TocGenerator'?
			if ($self->{_doOutputAnchorNameEnd}) {
				# Yes, the first anchor is inserted by 'TocGenerator';
					# Show warning
				$self->_showWarning(
					WARNING_NESTED_ANCHOR_PS_WITHIN_PS,
					[$aOrigText, $self->{_activeAnchorName}]
				);
			}
		}
			# Set active anchor name
		$self->_setActiveAnchorName($aAttr->{name});
	}
		# Must a ToC be generated?
	if ($self->{_doGenerateToc}) {
		# Yes, a ToC must be generated
			# Process start tag as ToC token
		$self->{isTocToken} = $self->_processTokenAsTocStartingToken(
			TT_TOKENTYPE_START, $aTag, $aAttr, \$aOrigText
		);
			# Process end tag as ToC-ending-token
		$self->_processTokenAsTocEndingToken(
			TT_TOKENTYPE_START, $aTag
		);
	}
}  # start()


#--- HTML::TocGenerator::text() -----------------------------------------------
# function: This function is called every time plain text is encountered.
# args:     - @_: array containing data.

sub text {
		# Get arguments
	my ($self, $aText) = @_;
		# Local variables
	my ($text, $toc, $i, $token, $tokens);
		# Must a ToC be generated?
	if ($self->{_doGenerateToc}) {
		# Yes, a ToC must be generated
			# Are there dirty start tags?

			# Loop through token types
		foreach $tokens (@{$self->{_tokensTocEnd}}) {
				# Loop though tokens
			foreach $token (@$tokens) {
					# Add text to toc

					# Alias
				$toc = $token->[TT_TOC];
					# Remove possible newlines from text
				($text = $aText) =~ s/\s*\n\s*/ /g;
					# Add text to toc
				$self->_processTocText($text, $toc);
			}
		}
	}
}  # text()




#=== HTML::_TokenTocParser ====================================================
# function: Parse 'toc tokens'.  'Toc tokens' mark HTML code which is to be
#           inserted into the ToC.
# note:     Used internally.

package HTML::_TokenTocParser;


BEGIN {
	use vars qw(@ISA);

	@ISA = qw(HTML::Parser);
}


END {}


#--- HTML::_TokenTocParser::new() ---------------------------------------------
# function: Constructor

sub new {
		# Get arguments
	my ($aType) = @_;
		# Create instance
	my $self = $aType->SUPER::new;

		# Return instance
	return $self;
}  # new()


#--- HTML::_TokenTocParser::_parseAttributes() --------------------------------
# function: Parse attributes.
# args:     - $aAttr: Reference to hash containing all tag attributes (in lower
#                case).
#           - $aIncludeAttributes: Reference to hash to which 'include
#                attributes' must be added.
#           - $aExcludeAttributes: Reference to hash to which 'exclude
#                attributes' must be added.
#           - $aTocAttributes: Reference to hash to which 'ToC attributes' 
#                must be added.

sub _parseAttributes {
		# Get arguments
	my (
		$self, $aAttr, $aIncludeAttributes, $aExcludeAttributes,
		$aTocAttributes
	) = @_;
		# Local variables
	my ($key, $value);
	my ($attributeToExcludeToken, $attributeToTocToken);
		# Get token which marks attributes which must be excluded
	$attributeToExcludeToken = $self->{_toc}{options}{'attributeToExcludeToken'};
	$attributeToTocToken     = $self->{_toc}{options}{'attributeToTocToken'};
		# Loop through attributes
	while (($key, $value) = each %$aAttr) {
			# Attribute value equals 'ToC token'?
		if ($value =~ m/$attributeToTocToken/) {
			# Yes, attribute value equals 'ToC token';
				# Add attribute to 'ToC attributes'
			push @$aTocAttributes, $key;
		}
		else {
			# No, attribute isn't 'ToC' token;
				# Attribute value starts with 'exclude token'?
			if ($value =~ m/^$attributeToExcludeToken(.*)/) {
				# Yes, attribute value starts with 'exclude token';
					# Add attribute to 'exclude attributes'
				$$aExcludeAttributes{$key} = "$1";
			}
			else {
				# No, attribute key doesn't start with '-';
					# Add attribute to 'include attributes'
				$$aIncludeAttributes{$key} = $value;
			}
		}
	}
}  # _parseAttributes()




#=== HTML::_TokenTocBeginParser ===============================================
# function: Parse 'toc tokens'.  'Toc tokens' mark HTML code which is to be
#           inserted into the ToC.
# note:     Used internally.

package HTML::_TokenTocBeginParser;


BEGIN {
	use vars qw(@ISA);

	@ISA = qw(HTML::_TokenTocParser);
}

END {}


#--- HTML::_TokenTocBeginParser::new() ----------------------------------------
# function: Constructor

sub new {
		# Get arguments
	my ($aType, $aTokenArray) = @_;
		# Create instance
	my $self = $aType->SUPER::new;
		# Reference token array
	$self->{tokens} = $aTokenArray;
		# Reference to last added token
	$self->{_lastAddedToken}     = undef;
	$self->{_lastAddedTokenType} = undef;
		# Return instance
	return $self;
}  # new()


#--- HTML::_TokenTocBeginParser::_processAttributes() -------------------------
# function: Process attributes.
# args:     - $aAttributes: Attributes to parse.

sub _processAttributes {
		# Get arguments
	my ($self, $aAttributes) = @_;
		# Local variables
	my (%includeAttributes, %excludeAttributes, @tocAttributes);

		# Parse attributes
	$self->_parseAttributes(
		$aAttributes, \%includeAttributes, \%excludeAttributes, \@tocAttributes
	);
		# Include attributes are specified?
	if (keys(%includeAttributes) > 0) {
		# Yes, include attributes are specified;
			# Store include attributes
		@${$self->{_lastAddedToken}}[
			HTML::TocGenerator::TT_INCLUDE_ATTRIBUTES_BEGIN
		] = \%includeAttributes;
	}
		# Exclude attributes are specified?
	if (keys(%excludeAttributes) > 0) {
		# Yes, exclude attributes are specified;
			# Store exclude attributes
		@${$self->{_lastAddedToken}}[
			HTML::TocGenerator::TT_EXCLUDE_ATTRIBUTES_BEGIN
		] = \%excludeAttributes;
	}
		# Toc attributes are specified?
	if (@tocAttributes > 0) {
		# Yes, toc attributes are specified;
			# Store toc attributes
		@${$self->{_lastAddedToken}}[
			HTML::TocGenerator::TT_ATTRIBUTES_TOC
		] = \@tocAttributes;
	}
}  # _processAttributes()


#--- HTML::_TokenTocBeginParser::_processToken() ------------------------------
# function: Process token.
# args:     - $aTokenType: Type of token to process.
#           - $aTag: Tag of token.

sub _processToken {
		# Get arguments
	my ($self, $aTokenType, $aTag) = @_;
		# Local variables
	my ($tokenArray, $index);
		# Push element on array of update tokens
	$index = push(@{$self->{tokens}[$aTokenType]}, []) - 1;
		# Alias token array to add element to
	$tokenArray = $self->{tokens}[$aTokenType];
		# Indicate last updated token array element
	$self->{_lastAddedTokenType} = $aTokenType;
	$self->{_lastAddedToken}     = \$$tokenArray[$index];
		# Add fields
	$$tokenArray[$index][HTML::TocGenerator::TT_TAG_BEGIN] = $aTag;
	$$tokenArray[$index][HTML::TocGenerator::TT_GROUP]     = $self->{_group};
	$$tokenArray[$index][HTML::TocGenerator::TT_TOC]       = $self->{_toc};
}  # _processToken()


#--- HTML::_TokenTocBeginParser::comment() ------------------------------------
# function: Process comment.
# args:     - $aComment: comment text with '<!--' and '-->' tags stripped off.

sub comment {
		# Get arguments
	my ($self, $aComment) = @_;
		# Process token
	$self->_processToken(HTML::TocGenerator::TT_TOKENTYPE_COMMENT, $aComment);
}  # comment()


#--- HTML::_TokenTocBeginParser::declaration() --------------------------------
# function: This function is called every time a markup declaration is
#           encountered by HTML::Parser.
# args:     - $aDeclaration: Markup declaration.

sub declaration {
		# Get arguments
	my ($self, $aDeclaration) = @_;
		# Process token
	$self->_processToken(
		HTML::TocGenerator::TT_TOKENTYPE_DECLARATION, $aDeclaration
	);
}  # declaration()

	
#--- HTML::_TokenTocBeginParser::end() ----------------------------------------
# function: This function is called every time a closing tag is encountered
#           by HTML::Parser.
# args:     - $aTag: tag name (in lower case).

sub end {
		# Get arguments
	my ($self, $aTag, $aOrigText) = @_;
		# Process token
	$self->_processToken(HTML::TocGenerator::TT_TOKENTYPE_END, $aTag);
}  # end()


#--- HTML::_TokenTocBeginParser::parse() --------------------------------------
# function: Parse begin token.
# args:     - $aToken: 'toc token' to parse

sub parse {
		# Get arguments
	my ($self, $aString) = @_;
		# Call ancestor
	$self->SUPER::parse($aString);
}  # parse()


#--- HTML::_TokenTocBeginParser->setGroup() -----------------------------------
# function: Set current 'tokenToToc' group.

sub setGroup {
		# Get arguments
	my ($self, $aGroup) = @_;
		# Set current 'tokenToToc' group
	$self->{_group} = $aGroup;
}  # setGroup()


#--- HTML::_TokenTocBeginParser->setToc() -------------------------------------
# function: Set current ToC.

sub setToc {
		# Get arguments
	my ($self, $aToc) = @_;
		# Set current ToC
	$self->{_toc} = $aToc;
}  # setToc()


#--- HTML::_TokenTocBeginParser::start() --------------------------------------
# function: This function is called every time an opening tag is encountered.
# args:     - $aTag: tag name (in lower case).
#           - $aAttr: reference to hash containing all tag attributes (in lower
#                case).
#           - $aAttrSeq: reference to array containing all attribute keys (in 
#                lower case) in the original order
#           - $aOrigText: the original HTML text

sub start {
		# Get arguments
	my ($self, $aTag, $aAttr, $aAttrSeq, $aOrigText) = @_;
		# Process token
	$self->_processToken(HTML::TocGenerator::TT_TOKENTYPE_START, $aTag);
		# Process attributes
	$self->_processAttributes($aAttr);
}  # start()


#--- HTML::_TokenTocBeginParser::text() ---------------------------------------
# function: This function is called every time plain text is encountered.
# args:     - @_: array containing data.

sub text {
		# Get arguments
	my ($self, $aText) = @_;
		# Was token already created and is last added token of type 'text'?
	if (
		defined($self->{_lastAddedToken}) && 
		$self->{_lastAddedTokenType} == HTML::TocGenerator::TT_TOKENTYPE_TEXT
	) {
		# Yes, token is already created;
			# Add tag to existing token
		@${$self->{_lastAddedToken}}[HTML::TocGenerator::TT_TAG_BEGIN] .= $aText;
	}
	else {
		# No, token isn't created;
			# Process token
		$self->_processToken(HTML::TocGenerator::TT_TOKENTYPE_TEXT, $aText);
	}
}  # text()




#=== HTML::_TokenTocEndParser =================================================
# function: Parse 'toc tokens'.  'Toc tokens' mark HTML code which is to be
#           inserted into the ToC.
# note:     Used internally.

package HTML::_TokenTocEndParser;


BEGIN {
	use vars qw(@ISA);

	@ISA = qw(HTML::_TokenTocParser);
}


END {}


#--- HTML::_TokenTocEndParser::new() ------------------------------------------
# function: Constructor
# args:     - $aType: Class type.

sub new {
		# Get arguments
	my ($aType) = @_;
		# Create instance
	my $self = $aType->SUPER::new;
		# Reference to last added token
	$self->{_lastAddedToken} = undef;
		# Return instance
	return $self;
}  # new()


#--- HTML::_TokenTocEndParser::_processAttributes() ---------------------------
# function: Process attributes.
# args:     - $aAttributes: Attributes to parse.

sub _processAttributes {
		# Get arguments
	my ($self, $aAttributes) = @_;
		# Local variables
	my (%includeAttributes, %excludeAttributes);

		# Parse attributes
	$self->_parseAttributes(
		$aAttributes, \%includeAttributes, \%excludeAttributes
	);
		# Include attributes are specified?
	if (keys(%includeAttributes) > 0) {
		# Yes, include attributes are specified;
			# Store include attributes
		@${$self->{_Token}}[
			HTML::TocGenerator::TT_INCLUDE_ATTRIBUTES_END
		] = \%includeAttributes;
	}
		# Exclude attributes are specified?
	if (keys(%excludeAttributes) > 0) {
		# Yes, exclude attributes are specified;
			# Store exclude attributes
		@${$self->{_Token}}[
			HTML::TocGenerator::TT_EXCLUDE_ATTRIBUTES_END
		] = \%excludeAttributes;
	}
}  # _processAttributes()


#--- HTML::_TokenTocEndParser::_processToken() --------------------------------
# function: Process token.
# args:     - $aTokenType: Type of token to process.
#           - $aTag: Tag of token.

sub _processToken {
		# Get arguments
	my ($self, $aTokenType, $aTag) = @_;
		# Update token
	@${$self->{_token}}[HTML::TocGenerator::TT_TAG_TYPE_END] = $aTokenType;
	@${$self->{_token}}[HTML::TocGenerator::TT_TAG_END]      = $aTag;
		# Indicate token type which has been processed
	$self->{_lastAddedTokenType} = $aTokenType;
}  # _processToken()


#--- HTML::_TokenTocEndParser::comment() --------------------------------------
# function: Process comment.
# args:     - $aComment: comment text with '<!--' and '-->' tags stripped off.

sub comment {
		# Get arguments
	my ($self, $aComment) = @_;
		# Process token
	$self->_processToken(HTML::TocGenerator::TT_TOKENTYPE_COMMENT, $aComment);
}  # comment()


#--- HTML::_TokenTocDeclarationParser::declaration() --------------------------
# function: This function is called every time a markup declaration is
#           encountered by HTML::Parser.
# args:     - $aDeclaration: Markup declaration.

sub declaration {
		# Get arguments
	my ($self, $aDeclaration) = @_;
		# Process token
	$self->_processToken(
		HTML::TocGenerator::TT_TOKENTYPE_DECLARATION, $aDeclaration
	);
}  # declaration()

	
#--- HTML::_TokenTocEndParser::end() ------------------------------------------
# function: This function is called every time a closing tag is encountered
#           by HTML::Parser.
# args:     - $aTag: tag name (in lower case).

sub end {
		# Get arguments
	my ($self, $aTag, $aOrigText) = @_;
		# Process token
	$self->_processToken(HTML::TocGenerator::TT_TOKENTYPE_END, $aTag);
}  # end()


#--- HTML::_TokenTocEndParser::parse() ----------------------------------------
# function: Parse token.
# args:     - $aString: 'toc token' to parse
#           - $aToken: Reference to token
#           - $aTokenTypeBegin: Type of begin token

sub parse {
		# Get arguments
	my ($self, $aString, $aToken, $aTokenTypeBegin) = @_;
		# Token argument specified?
	if (defined($aToken)) {
		# Yes, token argument is specified;
			# Store token reference
		$self->{_token} = $aToken;
	}
		# End tag defined?
	if (! defined($aString)) {
		# No, end tag isn't defined;
			# Last added tokentype was of type 'start'?
		if (
			(defined($aTokenTypeBegin)) &&
			($aTokenTypeBegin == HTML::TocGenerator::TT_TOKENTYPE_START) 
		) {
			# Yes, last added tokentype was of type 'start';
				# Assume end tag
			$self->_processToken(
				HTML::TocGenerator::TT_TAG_END,
				@${$self->{_token}}[HTML::TocGenerator::TT_TAG_BEGIN]
			);
		}
	}
	else {
			# Call ancestor
		$self->SUPER::parse($aString);
	}
}  # parse()


#--- HTML::_TokenTocEndParser::start() ----------------------------------------
# function: This function is called every time an opening tag is encountered.
# args:     - $aTag: tag name (in lower case).
#           - $aAttr: reference to hash containing all tag attributes (in lower
#                case).
#           - $aAttrSeq: reference to array containing all attribute keys (in 
#                lower case) in the original order
#           - $aOrigText: the original HTML text

sub start {
		# Get arguments
	my ($self, $aTag, $aAttr, $aAttrSeq, $aOrigText) = @_;
		# Process token
	$self->_processToken(HTML::TocGenerator::TT_TOKENTYPE_START, $aTag);
		# Process attributes
	$self->_processAttributes($aAttr);
}  # start()


#--- HTML::_TokenTocEndParser::text() -----------------------------------------
# function: This function is called every time plain text is encountered.
# args:     - @_: array containing data.

sub text {
		# Get arguments
	my ($self, $aText) = @_;

		# Is token already created?
	if (defined($self->{_lastAddedTokenType})) {
		# Yes, token is already created;
			# Add tag to existing token
		@${$self->{_token}}[HTML::TocGenerator::TT_TAG_END] .= $aText;
	}
	else {
		# No, token isn't created;
			# Process token
		$self->_processToken(HTML::TocGenerator::TT_TOKENTYPE_TEXT, $aText);
	}
}  # text()


1;
