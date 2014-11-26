#=== HTML::Toc ================================================================
# function: HTML Table of Contents


package HTML::Toc;


use strict;


BEGIN {
	use vars qw($VERSION);

	$VERSION = '0.91';
}


use constant FILE_FILTER             => '.*';
use constant GROUP_ID_H              => 'h';
use constant LEVEL_1                 => 1;
use constant NUMBERING_STYLE_DECIMAL => 'decimal';

	# Templates

	# Anchor templates
use constant TEMPLATE_ANCHOR_NAME       => '$groupId."-".$node';
use constant TEMPLATE_ANCHOR_HREF_BEGIN       => 
					'"<a href=#$anchorName>"';
use constant TEMPLATE_ANCHOR_HREF_BEGIN_FILE  => 
					'"<a href=$file#$anchorName>"';
use constant TEMPLATE_ANCHOR_HREF_END         => '"</a>"';
use constant TEMPLATE_ANCHOR_NAME_BEGIN => 
					'"<a name=$anchorName>"';
use constant TEMPLATE_ANCHOR_NAME_END   => '"</a>"';
use constant TOKEN_UPDATE_BEGIN_OF_ANCHOR_NAME_BEGIN => 
					'<!-- #BeginTocAnchorNameBegin -->';
use constant TOKEN_UPDATE_END_OF_ANCHOR_NAME_BEGIN   => 
					'<!-- #EndTocAnchorNameBegin -->';
use constant TOKEN_UPDATE_BEGIN_OF_ANCHOR_NAME_END => 
					'<!-- #BeginTocAnchorNameEnd -->';
use constant TOKEN_UPDATE_END_OF_ANCHOR_NAME_END   => 
					'<!-- #EndTocAnchorNameEnd -->';
use constant TOKEN_UPDATE_BEGIN_NUMBER      => 
					'<!-- #BeginTocNumber -->';
use constant TOKEN_UPDATE_END_NUMBER        => 
					'<!-- #EndTocNumber -->';
use constant TOKEN_UPDATE_BEGIN_TOC         => 
					'<!-- #BeginToc -->';
use constant TOKEN_UPDATE_END_TOC           => 
					'<!-- #EndToc -->';

use constant TEMPLATE_TOKEN_NUMBER      => '"$node &nbsp;"';

	# Level templates
use constant TEMPLATE_LEVEL             => '"<li>$text\n"';
use constant TEMPLATE_LEVEL_BEGIN       => '"<ul>\n"';
use constant TEMPLATE_LEVEL_END         => '"</ul>\n"';


END {}


#--- HTML::Toc::new() ---------------------------------------------------------
# function: Constructor

sub new {
		# Get arguments
	my ($aType) = @_;
		# Local variables
	my $self;

	$self = bless({}, $aType);
		# Default to empty 'options' array
	$self->{options} = {};
		# Empty toc
	$self->{_toc} = "";
		# Hash reference to array for each groupId, each array element
		# referring to the group of the level indicated by the array index.
		# For example, with the default 'tokenGroups', '_levelGroups' would
		# look like: 
		#
		# {'h'} => [\$group1, \$group2, \$group3, \$group4, \$group5, \$group6];
		#
	$self->{_levelGroups} = undef;
		# Set default options
	$self->_setDefaults();
	return $self;
}  # new()


#--- HTML::Toc::_compareLevels() ----------------------------------------------
# function: Compare levels.
# args:     - $aLevel: pointer to level
#           - $aGroupLevel
#           - $aPreviousLevel
#           - $aPreviousGroupLevel
# returns:  0 if new level equals previous level, 1 if new level exceeds
#           previous level, -1 if new level is smaller then previous level.

sub _compareLevels {
		# Get arguments
	my (
		$self, $aLevel, $aPreviousLevel, $aGroupLevel, $aPreviousGroupLevel 
	) = @_;
		# Local variables
	my ($result);
		# Levels equals?
	if (
		($aLevel == $aPreviousLevel) &&
		($aGroupLevel == $aPreviousGroupLevel)
	) {
		# Yes, levels are equals;
			# Indicate so
		$result = 0;
	}
	else {
		# No, levels differ;
			# Bias to new level being smaller than previous level;
		$result = -1;
			# Must groups not be nested and do group levels differ?
		if (
			($self->{options}{'doNestGroup'} == 0) &&
			($aGroupLevel != $aPreviousGroupLevel)
		) {
			# Yes, groups must be kept apart and the group levels differ;
				# Level is greater than previous level?
			if (
				($aLevel > $aPreviousLevel)
			) {
				# Yes, level is greater than previous level;
					# Indicate so
				$result = 1;
			}
		}
		else {
			# No, group must be nested;
				# Level is greater than previous level?
			if (
				($aLevel > $aPreviousLevel) ||
				($aGroupLevel > $aPreviousGroupLevel)
			) {
				# Yes, level is greater than previous level;
					# Indicate so
				$result = 1;
			}
		}
	}
		# Return value
	return $result;
}  # _compareLevels()


#--- HTML::TocGenerator::_formatLevelIndent() ---------------------------------
# function: Format indent.
# args:     - $aText: text to indent
#           - $aLevel: Level.
#           - $aGroupLevel: Group level.
#           - $aAdd
#           - $aGlobalLevel

sub _formatLevelIndent {
		# Get arguments
	my ($self, $aText, $aAdd, $aGlobalLevel) = @_;
		# Local variables
	my ($levelIndent, $indent, $nrOfIndents);
		# Alias indentation option
	$levelIndent = $self->{options}{'levelIndent'}; #=~ s/[0-9]+/&/;
		# Calculate number of indents
	$nrOfIndents = ($aGlobalLevel + $aAdd) * $levelIndent;
		# Assemble indents
	$indent = pack("A$nrOfIndents");
		# Return value
	return $indent . $aText;
}  # _formatLevelIndent()


#--- HTML::Toc::_formatToc() --------------------------------------------------
# function: Format ToC.
# args:     - aPreviousLevel
#           - aPreviousGroupLevel
#           - aToc: ToC to format.
#           - aHeaderLines
# note:     Recursive function this is.

sub _formatToc {
		# Get arguments
	my (
		$self, $aPreviousLevel, $aPreviousGroupLevel, $aToc, $aHeaderLines, 
		$aGlobalLevel
	) = @_;
		# Local variables
	my ($level, $groupLevel, $line, $groupId, $text, $compareStatus);
	my ($anchorName, $globalLevel, $node, $sequenceNr);

	LOOP: {
			# Lines need processing?
		while (scalar(@$aHeaderLines) > 0) {
			# Yes, lines need processing;
				# Get line
			$line = shift @$aHeaderLines;
				
				# Determine levels
			($level, $groupLevel, $groupId, $node, $sequenceNr, 
			$anchorName, $text) = split(
				/ /, $line, 7
			);
				# Must level and group be processed?
			if (
				($level =~ m/$self->{options}{'levelToToc'}/) &&
				($groupId =~ m/$self->{options}{'groupToToc'}/)
			) {
				# Yes, level must be processed;
					# Compare levels
				$compareStatus = $self->_compareLevels(
					$level, $aPreviousLevel, $groupLevel, $aPreviousGroupLevel
				);

				COMPARE_LEVELS: {

						# Equals?
					if ($compareStatus == 0) {
						# Yes, levels are equal;
							# Format level
						$$aToc .= $self->_formatLevelIndent(
							ref($self->{_templateLevel}) eq "CODE" ?
								&{$self->{_templateLevel}}(
									$level, $groupId, $node, $sequenceNr, $text
								) :
								eval($self->{_templateLevel}),
							0, $aGlobalLevel
						);
					}

						# Greater?
					if ($compareStatus > 0) {
						# Yes, new level is greater than previous level;
							# Must level be single-stepped?
						if (
							$self->{options}{'doSingleStepLevel'} && 
							($aPreviousLevel) && 
							($level > $aPreviousLevel)
						) {
							# Yes, level must be single-stepped;
								# Make sure, new level is increased one step only
							$level = $aPreviousLevel + 1;
						}
							# Increase global level
						$aGlobalLevel++;
							# Format begin of level
						$$aToc .= $self->_formatLevelIndent(
							eval($self->{_templateLevelBegin}), -1, $aGlobalLevel
						);
							# Process line again
						unshift @$aHeaderLines, $line;
							# Assemble TOC (recursive) for next level
						$self->_formatToc(
							$level, $groupLevel, $aToc, $aHeaderLines, $aGlobalLevel
						);
							# Format end of level
						$$aToc .= $self->_formatLevelIndent(
							eval($self->{_templateLevelEnd}), -1, $aGlobalLevel
						);
							# Decrease global level
						$aGlobalLevel--;
							# Exit loop
						last COMPARE_LEVELS;
					}

						# Smaller?
					if ($compareStatus < 0) {
						# Yes, new level is smaller than previous level;
							# Process line again
						unshift @$aHeaderLines, $line;
							# End loop
						last LOOP;
					}
				}
			}
		}
	}
}	# _formatToc()


#--- HTML::Toc::_parseTokenGroups() -------------------------------------------
# function: Parse token groups

sub _parseTokenGroups {
		# Get arguments
	my ($self) = @_;
		# Local variables
	my ($group, $levelGroups, $numberingStyle);

		# Clear any previous 'levelGroups'
	$self->{_levelGroups} = undef;
		# Determine default 'numberingStyle'
	$numberingStyle = defined($self->{options}{'numberingStyle'}) ?
		$self->{options}{'numberingStyle'} : NUMBERING_STYLE_DECIMAL;

		# Loop through groups
	foreach $group (@{$self->{options}{'tokenToToc'}}) {
			# 'groupId' is specified?
		if (! defined($group->{'groupId'})) {
			# No, 'groupId' isn't specified;
				# Set default groupId
			$group->{'groupId'} = GROUP_ID_H;
		}
			# 'level' is specified?
		if (! defined($group->{'level'})) {
			# No, 'level' isn't specified;
				# Set default level
			$group->{'level'} = LEVEL_1;
		}
			# 'numberingStyle' is specified?
		if (! defined($group->{'numberingStyle'})) {
			# No, 'numberingStyle' isn't specified;
				# Set default numberingStyle
			$group->{'numberingStyle'} = $numberingStyle;
		}
			# Add group to '_levelGroups' variabele
		$self->{_levelGroups}{$group->{'groupId'}}[$group->{'level'} - 1] = 
			$group;
	}
}  # _parseTokenGroups()


#--- HTML::Toc::_setDefaults() ------------------------------------------------
# function: Set default options.

sub _setDefaults {
		# Get arguments
	my ($self) = @_;
		# Set default options
	$self->setOptions(
		{
			'attributeToExcludeToken' => '-',
			'attributeToTocToken'     => '@',
			'insertionPoint'          => 'after <body>',
			'levelToToc'              => '.*',
			'groupToToc'              => '.*',
			'doNumberToken'           => 0,
			'doLinkToFile'            => 0,
			'doLinkToToken'           => 1,
			'doLinkToId'              => 0,
			'doSingleStepLevel'       => 1,
			'linkUri'                 => '',
			'levelIndent'             => 3,
			'doNestGroup'             => 0,
			'doUseExistingAnchors'    => 1,
			'doUseExistingIds'        => 1,
			'tokenToToc'              => [
				{
					'level'  => 1,
					'tokenBegin' => '<h1>'
				}, {
					'level'  => 2,
					'tokenBegin' => '<h2>'
				}, {
					'level'  => 3,
					'tokenBegin' => '<h3>'
				}, {
					'level'  => 4,
					'tokenBegin' => '<h4>'
				}, {
					'level'  => 5,
					'tokenBegin' => '<h5>'
				}, {
					'level'  => 6,
					'tokenBegin' => '<h6>'
				}
			],
			'header'            =>
				"\n<!-- Table of Contents generated by Perl - HTML::Toc -->\n",
			'footer'            =>
				"\n<!-- End of generated Table of Contents -->\n",
		}
	);
}  # _setDefaults()


#--- HTML::Toc::clear() -------------------------------------------------------
# function: Clear ToC.

sub clear {
		# Get arguments
	my ($self) = @_;
		# Clear ToC
	$self->{_toc}          = "";
	$self->{toc}           = "";
	$self->{groupIdLevels} = undef;
	$self->{levels}        = undef;
}   # clear()


#--- HTML::Toc::format() ------------------------------------------------------
# function: Format ToC.
# returns:  Formatted ToC.

sub format {
		# Get arguments
	my ($self) = @_;
		# Local variables;
	my $toc = "";
	my @tocLines = split(/\r\n|\n/, $self->{_toc});
		# Format table of contents
	$self->_formatToc("0", "0", \$toc, \@tocLines, 0);
		# Remove last newline
	$toc =~ s/\n$//m;
		# Add header & footer
	$toc = $self->{options}{'header'} . $toc . $self->{options}{'footer'};
		# Return value
	return $toc;
}	# format()


#--- HTML::Toc::parseOptions() ------------------------------------------------
# function: Parse options.

sub parseOptions {
		# Get arguments
	my ($self) = @_;
		# Alias options
	my $options = $self->{options};

		# Parse token groups
	$self->_parseTokenGroups();

		# Link ToC to tokens?
	if ($self->{options}{'doLinkToToken'}) {
		# Yes, link ToC to tokens;
			# Determine anchor href template begin
		$self->{_templateAnchorHrefBegin} =
			defined($options->{'templateAnchorHrefBegin'}) ?
				$options->{'templateAnchorHrefBegin'} :
				$options->{'doLinkToFile'} ? 
					TEMPLATE_ANCHOR_HREF_BEGIN_FILE : TEMPLATE_ANCHOR_HREF_BEGIN;

			# Determine anchor href template end
		$self->{_templateAnchorHrefEnd} =
			defined($options->{'templateAnchorHrefEnd'}) ?
				$options->{'templateAnchorHrefEnd'} :
				TEMPLATE_ANCHOR_HREF_END;

			# Determine anchor name template
		$self->{_templateAnchorName} =
			defined($options->{'templateAnchorName'}) ?
				$options->{'templateAnchorName'} :
				TEMPLATE_ANCHOR_NAME;

			# Determine anchor name template begin
		$self->{_templateAnchorNameBegin} =
			defined($options->{'templateAnchorNameBegin'}) ?
				$options->{'templateAnchorNameBegin'} :
				TEMPLATE_ANCHOR_NAME_BEGIN;

			# Determine anchor name template end
		$self->{_templateAnchorNameEnd} =
			defined($options->{'templateAnchorNameEnd'}) ?
				$options->{'templateAnchorNameEnd'} :
				TEMPLATE_ANCHOR_NAME_END;
	}

		# Determine token number template
	$self->{_templateTokenNumber} = 
		defined($options->{'templateTokenNumber'}) ?
			$options->{'templateTokenNumber'} :
			TEMPLATE_TOKEN_NUMBER;

		# Determine level template
	$self->{_templateLevel} =
		defined($options->{'templateLevel'}) ?
			$options->{'templateLevel'} :
			TEMPLATE_LEVEL;

		# Determine level begin template
	$self->{_templateLevelBegin} =
		defined($options->{'templateLevelBegin'}) ?
			$options->{'templateLevelBegin'} :
			TEMPLATE_LEVEL_BEGIN;

		# Determine level end template
	$self->{_templateLevelEnd} =
		defined($options->{'templateLevelEnd'}) ?
			$options->{'templateLevelEnd'} :
			TEMPLATE_LEVEL_END;

		# Determine 'anchor name begin' begin update token
	$self->{_tokenUpdateBeginOfAnchorNameBegin} =
		defined($options->{'tokenUpdateBeginOfAnchorNameBegin'}) ?
			$options->{'tokenUpdateBeginOfAnchorNameBegin'} :
			TOKEN_UPDATE_BEGIN_OF_ANCHOR_NAME_BEGIN;

		# Determine 'anchor name begin' end update token
	$self->{_tokenUpdateEndOfAnchorNameBegin} =
		defined($options->{'tokenUpdateEndOfAnchorNameBegin'}) ?
			$options->{'tokenUpdateEndOfAnchorNameBegin'} :
			TOKEN_UPDATE_END_OF_ANCHOR_NAME_BEGIN;

		# Determine 'anchor name end' begin update token
	$self->{_tokenUpdateBeginOfAnchorNameEnd} =
		defined($options->{'tokenUpdateBeginOfAnchorNameEnd'}) ?
			$options->{'tokenUpdateBeginOfAnchorNameEnd'} :
			TOKEN_UPDATE_BEGIN_OF_ANCHOR_NAME_END;

		# Determine 'anchor name end' end update token
	$self->{_tokenUpdateEndOfAnchorNameEnd} =
		defined($options->{'tokenUpdateEndOfAnchorNameEnd'}) ?
			$options->{'tokenUpdateEndOfAnchorNameEnd'} :
			TOKEN_UPDATE_END_OF_ANCHOR_NAME_END;

		# Determine number begin update token
	$self->{_tokenUpdateBeginNumber} =
		defined($options->{'tokenUpdateBeginNumber'}) ?
			$options->{'tokenUpdateBeginNumber'} :
			TOKEN_UPDATE_BEGIN_NUMBER;

		# Determine number end update token
	$self->{_tokenUpdateEndNumber} =
		defined($options->{'tokenUpdateEndNumber'}) ?
			$options->{'tokenUpdateEndNumber'} :
			TOKEN_UPDATE_END_NUMBER;

		# Determine toc begin update token
	$self->{_tokenUpdateBeginToc} =
		defined($options->{'tokenUpdateBeginToc'}) ?
			$options->{'tokenUpdateBeginToc'} :
			TOKEN_UPDATE_BEGIN_TOC;

		# Determine toc end update token
	$self->{_tokenUpdateEndToc} =
		defined($options->{'tokenUpdateEndToc'}) ?
			$options->{'tokenUpdateEndToc'} :
			TOKEN_UPDATE_END_TOC;

}  # parseOptions()


#--- HTML::Toc::setOptions() --------------------------------------------------
# function: Set options.
# args:     - aOptions: Reference to hash containing options.

sub setOptions {
		# Get arguments
	my ($self, $aOptions) = @_;
		# Add options
	%{$self->{options}} = (%{$self->{options}}, %$aOptions);
}  # setOptions()


1;
