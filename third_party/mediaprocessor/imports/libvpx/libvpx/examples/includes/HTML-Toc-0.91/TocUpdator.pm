#==== HTML::TocUpdator ========================================================
# function: Update 'HTML::Toc' table of contents.
# note:     - 'TUT' is an abbreviation of 'Toc Update Token'.


package HTML::TocUpdator;


use strict;
use HTML::TocInsertor;


BEGIN {
	use vars qw(@ISA $VERSION);

	$VERSION = '0.91';

	@ISA = qw(HTML::TocInsertor);
}


use constant TUT_TOKENTYPE_START    => 0;
use constant TUT_TOKENTYPE_END      => 1;
use constant TUT_TOKENTYPE_TEXT     => 2;
use constant TUT_TOKENTYPE_COMMENT  => 3;

use constant MODE_DO_NOTHING   => 0;	# 0b00
use constant MODE_DO_INSERT    => 1;	# 0b01
use constant MODE_DO_UPDATE    => 3;	# 0b11


END {}


#--- HTML::TocUpdator::new() --------------------------------------------------
# function: Constructor.

sub new {
		# Get arguments
	my ($aType) = @_;
	my $self = $aType->SUPER::new;
		# Bias to not update ToC
	$self->{htu__Mode} = MODE_DO_NOTHING;
		# Bias to not delete tokens
	$self->{_doDeleteTokens} = 0;
		# Reset batch variables
	#$self->_resetBatchVariables;

	$self->{options} = {};
		
		# TODO: Initialize output

	return $self;
}  # new()


#--- HTML::TocUpdator::_deinitializeUpdatorBatch() --------------------------
# function: Deinitialize updator batch.
# args:     - $aTocs: Reference to array of tocs.

sub _deinitializeUpdatorBatch {
		# Get arguments
	my ($self, $aTocs) = @_;
		# Indicate end of ToC updating
	$self->{htu__Mode} = MODE_DO_NOTHING;
		# Deinitialize insertor batch
	$self->_deinitializeInsertorBatch();
}  # _deinitializeUpdatorBatch()


#--- HTML::TokenUpdator::_doesHashEqualHash() ---------------------------------
# function: Determines whether hash1 equals hash2.
# args:     - $aHash1
#           - $aHash2
# returns:  True (1) if hash1 equals hash2, 0 if not.  For example, with the
#           following hashes:
#
#              %hash1 = {                     %hash2 = {
#                 'class' => 'header',          'class' => 'header',
#                 'id'    => 'intro1'           'id'    => 'intro2'
#              }                             }
#
#           the routine will return 0, cause the hash fields 'id' differ.
# note:     Class function.

sub _doesHashEqualHash {
		# Get arguments
	my ($aHash1, $aHash2) = @_;
		# Local variables
	my ($key1, $value1, $key2, $value2, $result);
		# Bias to success
	$result = 1;
		# Loop through hash1 while values available
	HASH1: while (($key1, $value1) = each %$aHash1) {
		# Yes, values are available;
			# Value1 differs from value2?
		if ($value1 ne $aHash2->{$key1}) {
			# Yes, hashes differ;
				# Indicate condition fails
			$result = 0;
				# Reset 'each' iterator which we're going to break
			keys %$aHash2;
				# Break loop
			last HASH1;
		}
	}
		# Return value
	return $result;
}  # _doesHashEqualHash()


#--- HTML::TokenUpdator::_doesTagExistInArray() -------------------------------
# function: Check whether tag & attributes matches any of the tags & attributes
#           in the specified array.  The array must consist of elements with 
#           format:
#
#              [$tag, \%attributes]
#
# args:     - $aTag: tag to search for
#           - $aAttributes: tag attributes to search for
#           - $aArray: Array to search in.
# returns:  1 if tag does exist in array, 0 if not.
# note:     Class function.

sub _doesTagExistInArray {
		# Get arguments
	my ($aTag, $aAttributes, $aArray) = @_;
		# Local variables
	my ($tag, $result);
		# Bias to non-existing tag
	$result = 0;
		# Loop through existing tags
	TAG: foreach $tag (@{$aArray}) {
		if (defined(@{$tag}[0])) {
				# Does tag equals any existing tag?
			if ($aTag eq @{$tag}[0]) {
				# Yes, tag equals existing tag;
					# Do hashes equal?
				if (HTML::TocUpdator::_doesHashEqualHash(
					$aAttributes, @{$tag}[1]
				)) {
					# Yes, hashes are the same;
						# Indicate tag exists in array
					$result = 1;
						# Break loop
					last TAG;
				}
			}
		}
	}
		# Return value
	return $result;
}  # _doesTagExistInArray()


#--- HTML::TocUpdator::_initializeUpdatorBatch() ----------------------------
# function: Initialize insertor batch.
# args:     - $aMode: Mode.  Can be either MODE_DO_INSERT or MODE_DO_UPDATE
#           - $aTocs: Reference to array of tocs.
#           - $aOptions: optional options
# note:     Updating actually means: deleting the old ToC and inserting a new
#           ToC.  That's why we're calling 'insertor' methods here.

sub _initializeUpdatorBatch {
		# Get arguments
	my ($self, $aMode, $aTocs, $aOptions) = @_;
		# Initialize insertor batch
	$self->_initializeInsertorBatch($aTocs, $aOptions);
		# Parse ToC update templates
	$self->_parseTocUpdateTokens();
		# Indicate start of ToC updating
	$self->{htu__Mode} = $aMode;
}  # _initializeUpdatorBatch()


#--- HTML::TocUpdator::_parseTocUpdateTokens() --------------------------------
# function: Parse ToC insertion point specifier.

sub _parseTocUpdateTokens {
		# Get arguments
	my ($self) = @_;
		# Local variables
	my ($toc, $tokenType, $tokenPreposition, $token);
	my ($tocInsertionPoint, $tocInsertionPointTokenAttributes);
		# Create parser for update begin tokens
	my $tokenUpdateBeginParser = HTML::_TokenUpdateParser->new(
		$self->{_tokensUpdateBegin}
	);
		# Create parser for update end tokens
	my $tokenUpdateEndParser = HTML::_TokenUpdateParser->new(
		$self->{_tokensUpdateEnd}
	);

		# Loop through ToCs
	foreach $toc (@{$self->{_tocs}}) {
			# Parse update tokens
		$tokenUpdateBeginParser->parse(
			$toc->{_tokenUpdateBeginOfAnchorNameBegin}
		);
		$tokenUpdateBeginParser->parse($toc->{_tokenUpdateBeginOfAnchorNameEnd});
		$tokenUpdateBeginParser->parse($toc->{_tokenUpdateBeginNumber});
		$tokenUpdateBeginParser->parse($toc->{_tokenUpdateBeginToc});

		$tokenUpdateEndParser->parse($toc->{_tokenUpdateEndOfAnchorNameBegin});
		$tokenUpdateEndParser->parse($toc->{_tokenUpdateEndOfAnchorNameEnd});
		$tokenUpdateEndParser->parse($toc->{_tokenUpdateEndNumber});
		$tokenUpdateEndParser->parse($toc->{_tokenUpdateEndToc});
	}
}  # _parseTocUpdateTokens()


#--- HTML::TocUpdator::_resetBatchVariables() ---------------------------------
# function: Reset batch variables

sub _resetBatchVariables {
		# Get arguments
	my ($self) = @_;
		# Call ancestor
	$self->SUPER::_resetBatchVariables();
		# Arrays containing start, end, comment & text tokens which indicate
		# the begin of ToC tokens.  The tokens are stored in keys of hashes to 
		# avoid storing duplicates as an array would.
	$self->{_tokensUpdateBegin} = [
		[],	# ['<start tag>', <attributes>]
		{},	# {'<end tag>' => ''}
		{},	# {'<text>'    => ''}
		{}		# {'<comment>' => ''}
	];
		# Arrays containing start, end, comment & text tokens which indicate
		# the end of ToC tokens.  The tokens are stored in keys of hashes to 
		# avoid storing duplicates as an array would.
	$self->{_tokensUpdateEnd} = [
		[],	# ['<start tag>', <attributes>]
		{},	# {'<end tag>' => ''}
		{},	# {'<text>'    => ''}
		{}		# {'<comment>' => ''}
	];
}  # _resetBatchVariables()


#--- HTML::TocUpdator::_setActiveAnchorName() ---------------------------------
# function: Set active anchor name.
# args:     - aAnchorName: Name of anchor name to set active.

sub _setActiveAnchorName {
		# Get arguments
	my ($self, $aAnchorName) = @_;
		# Are tokens being deleted?
	if (! $self->{_doDeleteTokens}) {
		# No, tokens aren't being deleted;
			# Call ancestor to set anchor name
		$self->SUPER::_setActiveAnchorName($aAnchorName);
	}
}  # _setActiveAnchorName()


#--- HTML::TocUpdator::_update() ----------------------------------------------
# function: Update ToC in string.
# args:     - $aMode: Mode.  Can be either MODE_DO_UPDATE or MODE_DO_INSERT.
#           - $aToc: (reference to array of) ToC object to update
#           - $aString: string to update ToC of
#           - $aOptions: optional updator options
# note:     Used internally.

sub _update {
		# Get arguments
	my ($self, $aMode, $aToc, $aString, $aOptions) = @_;
		# Initialize TocUpdator batch
	$self->_initializeUpdatorBatch($aMode, $aToc, $aOptions);
		# Start updating ToC by starting ToC insertion
	$self->_insert($aString);
		# Deinitialize TocUpdator batch
	$self->_deinitializeUpdatorBatch();
}  # update()


#--- HTML::TocUpdator::_updateFile() ------------------------------------------
# function: Update ToCs in file.
# args:     - $aMode: Mode.  Can be either MODE_DO_UPDATE or MODE_DO_INSERT.
#           - $aToc: (reference to array of) ToC object to update
#           - $aFile: (reference to array of) file to parse for updating.
#           - $aOptions: optional updator options
# note:     Used internally.

sub _updateFile {
		# Get arguments
	my ($self, $aMode, $aToc, $aFile, $aOptions) = @_;
		# Initialize TocUpdator batch
	$self->_initializeUpdatorBatch($aMode, $aToc, $aOptions);
		# Start updating ToC by starting ToC insertion
	$self->_insertIntoFile($aFile);
		# Deinitialize TocUpdator batch
	$self->_deinitializeUpdatorBatch();
}  # _updateFile()


#--- HTML::TocUpdator::_writeOrBufferOutput() ---------------------------------
# function: Write processed HTML to output device(s).
# args:     - aOutput: scalar to write

sub _writeOrBufferOutput {
		# Get arguments
	my ($self, $aOutput) = @_;
		# Delete output?
	if (! $self->{_doDeleteTokens}) {
		# No, don't delete output;
			# Call ancestor
		$self->SUPER::_writeOrBufferOutput($aOutput);
	}
}  # _writeOrBufferOutput()


#--- HTML::TocUpdator::anchorNameBegin() --------------------------------------
# function: Process 'anchor name begin' generated by HTML::Toc.
# args:     - $aAnchorName: Anchor name begin tag to output.
#           - $aToc: Reference to ToC to which anchorname belongs.

sub anchorNameBegin {
		# Get arguments
	my ($self, $aAnchorNameBegin, $aToc) = @_;
		# Call ancestor
	$self->SUPER::anchorNameBegin($aAnchorNameBegin);
		# Must ToC be inserted or updated?
	if ($self->{htu__Mode} != MODE_DO_NOTHING) {
		# Yes, ToC must be inserted or updated;
			# Surround anchor name with update tags
		$self->{_outputPrefix} = 
			$aToc->{_tokenUpdateBeginOfAnchorNameBegin} .
			$self->{_outputPrefix} . 
			$aToc->{_tokenUpdateEndOfAnchorNameBegin};
	}
}	# anchorNameBegin()


#--- HTML::TocUpdator::anchorNameEnd() ----------------------------------------
# function: Process 'anchor name end' generated by HTML::Toc.
# args:     - $aAnchorNameEnd: Anchor name end tag to output.
#           - $aToc: Reference to ToC to which anchorname belongs.

sub anchorNameEnd {
		# Get arguments
	my ($self, $aAnchorNameEnd, $aToc) = @_;
		# Call ancestor
	$self->SUPER::anchorNameEnd($aAnchorNameEnd);
		# Must ToC be inserted or updated?
	if ($self->{htu__Mode} != MODE_DO_NOTHING) {
		# Yes, ToC must be inserted or updated;
			# Surround anchor name with update tags
		$self->{_outputSuffix} = 
			$aToc->{_tokenUpdateBeginOfAnchorNameEnd} .
			$self->{_outputSuffix} . 
			$aToc->{_tokenUpdateEndOfAnchorNameEnd};
	}
}	# anchorNameEnd()


#--- HTML::TocUpdator::comment() ----------------------------------------------
# function: Process comment.
# args:     - $aComment: comment text with '<!--' and '-->' tags stripped off.

sub comment {
		# Get arguments
	my ($self, $aComment) = @_;
		# Must ToC be updated?
	if ($self->{htu__Mode} == MODE_DO_UPDATE) {
		# Yes, ToC must be updated;
			# Updator is currently deleting tokens?
		if ($self->{_doDeleteTokens}) {
			# Yes, tokens must be deleted;
				# Call ancestor
			$self->SUPER::comment($aComment);

				# Look for update end token

				# Does comment matches update end token?
			if (defined(
				$self->{_tokensUpdateEnd}[TUT_TOKENTYPE_COMMENT]{$aComment}
			)) {
				# Yes, comment matches update end token;
					# Indicate to stop deleting tokens
				$self->{_doDeleteTokens} = 0;
			}
		}
		else {
			# No, tokens mustn't be deleted;

				# Look for update begin token

				# Does comment matches update begin token?
			if (defined(
				$self->{_tokensUpdateBegin}[TUT_TOKENTYPE_COMMENT]{$aComment}
			)) {
				# Yes, comment matches update begin token;
					# Indicate to start deleting tokens
				$self->{_doDeleteTokens} = 1;
			}
				# Call ancestor
			$self->SUPER::comment($aComment);
		}
	}
	else {
		# No, ToC mustn't be updated;
			# Call ancestor
		$self->SUPER::comment($aComment);
	}
}  # comment()


#--- HTML::TocUpdator::end() --------------------------------------------------
# function: This function is called every time a closing tag is encountered.
# args:     - $aTag: tag name (in lower case).
#           - $aOrigText: tag name including brackets.

sub end {
		# Get arguments
	my ($self, $aTag, $aOrigText) = @_;
		# Call ancestor
	$self->SUPER::end($aTag, $aOrigText);
		# Must ToC be updated?
	if ($self->{htu__Mode} == MODE_DO_UPDATE) {
		# Yes, ToC must be updated;
			# Updator is currently deleting tokens?
		if ($self->{_doDeleteTokens}) {
			# Yes, tokens must be deleted;
				# Does end tag matches update end token?
			if (defined(
				$self->{_tokensUpdateEnd}[TUT_TOKENTYPE_END]{$aTag}
			)) {
				# Yes, end tag matches update end token;
					# Indicate to stop deleting tokens
				$self->{_doDeleteTokens} = 0;
			}
		}
	}
}  # end()


#--- HTML::TocUpdator::insert() -----------------------------------------------
# function: Insert ToC in string.
# args:     - $aToc: (reference to array of) ToC object to update
#           - $aString: string to insert ToC in.
#           - $aOptions: optional updator options

sub insert {
		# Get arguments
	my ($self, $aToc, $aString, $aOptions) = @_;
		# Do start insert
	$self->_update(MODE_DO_INSERT, $aToc, $aString, $aOptions);
}  # insert()


#--- HTML::TocUpdator::insertIntoFile() --------------------------------------
# function: Insert ToC in file.
# args:     - $aToc: (reference to array of) ToC object to update
#           - $aFile: File to insert ToC in.
#           - $aOptions: optional updator options

sub insertIntoFile {
		# Get arguments
	my ($self, $aToc, $aFile, $aOptions) = @_;
		# Do start insert
	$self->_updateFile(MODE_DO_INSERT, $aToc, $aFile, $aOptions);
}  # insertIntoFile()


#--- HTML::TocUpdator::number() -----------------------------------------------
# function: Process heading number generated by HTML::Toc.
# args:     - $aNumber
#           - $aToc: Reference to ToC to which anchorname belongs.

sub number {
		# Get arguments
	my ($self, $aNumber, $aToc) = @_;
		# Call ancestor
	$self->SUPER::number($aNumber);
		# Must ToC be inserted or updated?
	if ($self->{htu__Mode} != MODE_DO_NOTHING) {
		# Yes, ToC must be inserted or updated;
			# Surround number with update tags
		$self->{_outputSuffix} = 
			$aToc->{_tokenUpdateBeginNumber} .
			$self->{_outputSuffix} . 
			$aToc->{_tokenUpdateEndNumber};
	}
}	# number()


#--- HTML::TocUpdator::start() ------------------------------------------------
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
		# Must ToC be updated?
	if ($self->{htu__Mode} == MODE_DO_UPDATE) {
		# Yes, ToC must be updated;
			# Does start tag matches token update begin tag?
		if (HTML::TocUpdator::_doesTagExistInArray(
			$aTag, $aAttr, $self->{_tokensUpdateBegin}[TUT_TOKENTYPE_START]
		)) {
			# Yes, start tag matches token update tag;
				# Indicate to delete tokens
			$self->{_doDeleteTokens} = 1;
		}
	}
		# Let ancestor process the start tag
	$self->SUPER::start($aTag, $aAttr, $aAttrSeq, $aOrigText);
}  # start()


#--- HTML::TocUpdator::toc() --------------------------------------------------
# function: Toc processing method.  Add toc reference to scenario.
# args:     - $aScenario: Scenario to add ToC reference to.
#           - $aToc: Reference to ToC to insert.
# note:     The ToC hasn't been build yet; only a reference to the ToC to be
#           build is inserted.

sub toc {
		# Get arguments
	my ($self, $aScenario, $aToc) = @_;

		# Surround toc with update tokens

		# Add update begin token
	push(@$aScenario, \$aToc->{_tokenUpdateBeginToc});
		# Call ancestor
	$self->SUPER::toc($aScenario, $aToc);
		# Add update end token
	push(@$aScenario, \$aToc->{_tokenUpdateEndToc});
}  # toc()


#--- HTML::TocUpdator::_processTocText() --------------------------------------
# function: Toc text processing function.
# args:     - $aText: Text to add to ToC.
#           - $aToc: ToC to add text to.

sub _processTocText {
		# Get arguments
	my ($self, $aText, $aToc) = @_;
		# Delete output?
	if (! $self->{_doDeleteTokens}) {
		# No, don't delete output;
			# Call ancestor
		$self->SUPER::_processTocText($aText, $aToc);
	}
}  # _processTocText()


#--- HTML::TocUpdator::update() -----------------------------------------------
# function: Update ToC in string.
# args:     - $aToc: (reference to array of) ToC object to update
#           - $aString: string to update ToC of
#           - $aOptions: optional updator options

sub update {
		# Get arguments
	my ($self, $aToc, $aString, $aOptions) = @_;
		# Do start update
	$self->_update(MODE_DO_UPDATE, $aToc, $aString, $aOptions);
}  # update()


#--- HTML::TocUpdator::updateFile() -------------------------------------------
# function: Update ToC of file.
# args:     - $aToc: (reference to array of) ToC object to update
#           - $aFile: (reference to array of) file to parse for updating.
#           - $aOptions: optional updator options

sub updateFile {
		# Get arguments
	my ($self, $aToc, $aFile, $aOptions) = @_;
		# Do start update
	$self->_updateFile(MODE_DO_UPDATE, $aToc, $aFile, $aOptions);
}  # update()




#=== HTML::_TokenUpdateParser =================================================
# function: Parse 'update tokens'.  'Update tokens' mark HTML code which is
#           inserted by 'HTML::TocInsertor'.
# note:     Used internally.

package HTML::_TokenUpdateParser;


BEGIN {
	use vars qw(@ISA);

	@ISA = qw(HTML::Parser);
}

END {}


#--- HTML::_TokenUpdateParser::new() ------------------------------------------
# function: Constructor

sub new {
		# Get arguments
	my ($aType, $aTokenArray) = @_;
		# Create instance
	my $self = $aType->SUPER::new;
		# Reference token array
	$self->{tokens} = $aTokenArray;
		# Return instance
	return $self;
}  # new()


#--- HTML::_TokenUpdateParser::comment() --------------------------------------
# function: Process comment.
# args:     - $aComment: comment text with '<!--' and '-->' tags stripped off.

sub comment {
		# Get arguments
	my ($self, $aComment) = @_;
		# Add token to array of update tokens
	$self->{tokens}[HTML::TocUpdator::TUT_TOKENTYPE_COMMENT]{$aComment} = '';
}  # comment()


#--- HTML::_TokenUpdateParser::end() ------------------------------------------
# function: This function is called every time a closing tag is encountered
#           by HTML::Parser.
# args:     - $aTag: tag name (in lower case).

sub end {
		# Get arguments
	my ($self, $aTag, $aOrigText) = @_;
		# Add token to array of update tokens
	$self->{tokens}[HTML::TocUpdator::TUT_TOKENTYPE_END]{$aTag} = '';
}  # end()


#--- HTML::_TokenUpdateParser::parse() ----------------------------------------
# function: Parse token.
# args:     - $aToken: 'update token' to parse

sub parse {
		# Get arguments
	my ($self, $aString) = @_;
		# Call ancestor
	$self->SUPER::parse($aString);
}  # parse()


#--- HTML::_TokenUpdateParser::start() ----------------------------------------
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
		# Does token exist in array?
	if (! HTML::TocUpdator::_doesTagExistInArray(
		$aTag, $aAttr, $self->{tokens}[HTML::TocUpdator::TUT_TOKENTYPE_START]
	)) {
		# No, token doesn't exist in array;
			# Add token to array of update tokens
		push(
			@{$self->{tokens}[HTML::TocUpdator::TUT_TOKENTYPE_START]},
			[$aTag, $aAttr]
		);
	}
}  # start()


#--- HTML::_TokenUpdateParser::text() -----------------------------------------
# function: This function is called every time plain text is encountered.
# args:     - @_: array containing data.

sub text {
		# Get arguments
	my ($self, $aText) = @_;
		# Add token to array of update tokens
	$self->{tokens}[HTML::TocUpdator::TUT_TOKENTYPE_TEXT]{$aText} = '';
}  # text()


1;
