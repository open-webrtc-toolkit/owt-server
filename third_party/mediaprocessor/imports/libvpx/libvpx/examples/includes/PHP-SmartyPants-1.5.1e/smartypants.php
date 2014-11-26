<?php

#
# SmartyPants  -  Smart punctuation for web sites
#
# by John Gruber
# <http://daringfireball.net>
#
# PHP port by Michel Fortin
# <http://www.michelf.com/>
#
# Copyright (c) 2003-2004 John Gruber
# Copyright (c) 2004-2005 Michel Fortin
#


global  $SmartyPantsPHPVersion, $SmartyPantsSyntaxVersion,
        $smartypants_attr, $sp_tags_to_skip;

$SmartyPantsPHPVersion    = '1.5.1e'; # Fru 9 Dec 2005
$SmartyPantsSyntaxVersion = '1.5.1';  # Fri 12 Mar 2004


# Configurable variables:
$smartypants_attr = "1";  # Change this to configure.
                          #  1 =>  "--" for em-dashes; no en-dash support
                          #  2 =>  "---" for em-dashes; "--" for en-dashes
                          #  3 =>  "--" for em-dashes; "---" for en-dashes
                          #  See docs for more configuration options.

# Globals:
$sp_tags_to_skip = '<(/?)(?:pre|code|kbd|script|math)[\s>]';


# -- WordPress plugin interface -----------------------------------------------
/*
Plugin Name: SmartyPants
Plugin URI: http://www.michelf.com/projects/php-smartypants/
Description: SmartyPants is a web publishing utility that translates plain ASCII punctuation characters into &#8220;smart&#8221; typographic punctuation HTML entities. This plugin <strong>replace the default WordPress Texturize algorithm</strong> for the content and the title of your posts, the comments body and author name, and everywhere else Texturize normally apply. Based on the original Perl version by <a href="http://daringfireball.net/">John Gruber</a>.
Version: 1.5.1e
Author: Michel Fortin
Author URI: http://www.michelf.com/
*/
if (isset($wp_version)) {
    # Remove default Texturize filter that would conflict with SmartyPants.
    remove_filter('category_description', 'wptexturize');
    remove_filter('list_cats', 'wptexturize');
    remove_filter('comment_author', 'wptexturize');
    remove_filter('comment_text', 'wptexturize');
    remove_filter('single_post_title', 'wptexturize');
    remove_filter('the_title', 'wptexturize');
    remove_filter('the_content', 'wptexturize');
    remove_filter('the_excerpt', 'wptexturize');
    # Add SmartyPants filter with priority 10 (same as Texturize).
    add_filter('category_description', 'SmartyPants', 10);
    add_filter('list_cats', 'SmartyPants', 10);
    add_filter('comment_author', 'SmartyPants', 10);
    add_filter('comment_text', 'SmartyPants', 10);
    add_filter('single_post_title', 'SmartyPants', 10);
    add_filter('the_title', 'SmartyPants', 10);
    add_filter('the_content', 'SmartyPants', 10);
    add_filter('the_excerpt', 'SmartyPants', 10);
}

# -- Smarty Modifier Interface ------------------------------------------------
function smarty_modifier_smartypants($text, $attr = NULL) {
    return SmartyPants($text, $attr);
}



function SmartyPants($text, $attr = NULL, $ctx = NULL) {
    global $smartypants_attr, $sp_tags_to_skip;
    # Paramaters:
    $text;   # text to be parsed
    $attr;   # value of the smart_quotes="" attribute
    $ctx;    # MT context object (unused)
    if ($attr == NULL) $attr = $smartypants_attr;

    # Options to specify which transformations to make:
    $do_stupefy = FALSE;
    $convert_quot = 0;  # should we translate &quot; entities into normal quotes?

    # Parse attributes:
    # 0 : do nothing
    # 1 : set all
    # 2 : set all, using old school en- and em- dash shortcuts
    # 3 : set all, using inverted old school en and em- dash shortcuts
    #
    # q : quotes
    # b : backtick quotes (``double'' only)
    # B : backtick quotes (``double'' and `single')
    # d : dashes
    # D : old school dashes
    # i : inverted old school dashes
    # e : ellipses
    # w : convert &quot; entities to " for Dreamweaver users

    if ($attr == "0") {
        # Do nothing.
        return $text;
    }
    else if ($attr == "1") {
        # Do everything, turn all options on.
        $do_quotes    = 1;
        $do_backticks = 1;
        $do_dashes    = 1;
        $do_ellipses  = 1;
    }
    else if ($attr == "2") {
        # Do everything, turn all options on, use old school dash shorthand.
        $do_quotes    = 1;
        $do_backticks = 1;
        $do_dashes    = 2;
        $do_ellipses  = 1;
    }
    else if ($attr == "3") {
        # Do everything, turn all options on, use inverted old school dash shorthand.
        $do_quotes    = 1;
        $do_backticks = 1;
        $do_dashes    = 3;
        $do_ellipses  = 1;
    }
    else if ($attr == "-1") {
        # Special "stupefy" mode.
        $do_stupefy   = 1;
    }
    else {
        $chars = preg_split('//', $attr);
        foreach ($chars as $c){
            if      ($c == "q") { $do_quotes    = 1; }
            else if ($c == "b") { $do_backticks = 1; }
            else if ($c == "B") { $do_backticks = 2; }
            else if ($c == "d") { $do_dashes    = 1; }
            else if ($c == "D") { $do_dashes    = 2; }
            else if ($c == "i") { $do_dashes    = 3; }
            else if ($c == "e") { $do_ellipses  = 1; }
            else if ($c == "w") { $convert_quot = 1; }
            else {
                # Unknown attribute option, ignore.
            }
        }
    }

    $tokens = _TokenizeHTML($text);
    $result = '';
    $in_pre = 0;  # Keep track of when we're inside <pre> or <code> tags.

    $prev_token_last_char = "";     # This is a cheat, used to get some context
                                    # for one-character tokens that consist of
                                    # just a quote char. What we do is remember
                                    # the last character of the previous text
                                    # token, to use as context to curl single-
                                    # character quote tokens correctly.

    foreach ($tokens as $cur_token) {
        if ($cur_token[0] == "tag") {
            # Don't mess with quotes inside tags.
            $result .= $cur_token[1];
            if (preg_match("@$sp_tags_to_skip@", $cur_token[1], $matches)) {
                $in_pre = isset($matches[1]) && $matches[1] == '/' ? 0 : 1;
            }
        } else {
            $t = $cur_token[1];
            $last_char = substr($t, -1); # Remember last char of this token before processing.
            if (! $in_pre) {
                $t = ProcessEscapes($t);

                if ($convert_quot) {
                    $t = preg_replace('/&quot;/', '"', $t);
                }

                if ($do_dashes) {
                    if ($do_dashes == 1) $t = EducateDashes($t);
                    if ($do_dashes == 2) $t = EducateDashesOldSchool($t);
                    if ($do_dashes == 3) $t = EducateDashesOldSchoolInverted($t);
                }

                if ($do_ellipses) $t = EducateEllipses($t);

                # Note: backticks need to be processed before quotes.
                if ($do_backticks) {
                    $t = EducateBackticks($t);
                    if ($do_backticks == 2) $t = EducateSingleBackticks($t);
                }

                if ($do_quotes) {
                    if ($t == "'") {
                        # Special case: single-character ' token
                        if (preg_match('/\S/', $prev_token_last_char)) {
                            $t = "&#8217;";
                        }
                        else {
                            $t = "&#8216;";
                        }
                    }
                    else if ($t == '"') {
                        # Special case: single-character " token
                        if (preg_match('/\S/', $prev_token_last_char)) {
                            $t = "&#8221;";
                        }
                        else {
                            $t = "&#8220;";
                        }
                    }
                    else {
                        # Normal case:
                        $t = EducateQuotes($t);
                    }
                }

                if ($do_stupefy) $t = StupefyEntities($t);
            }
            $prev_token_last_char = $last_char;
            $result .= $t;
        }
    }

    return $result;
}


function SmartQuotes($text, $attr = NULL, $ctx = NULL) {
    global $smartypants_attr, $sp_tags_to_skip;
    # Paramaters:
    $text;   # text to be parsed
    $attr;   # value of the smart_quotes="" attribute
    $ctx;    # MT context object (unused)
    if ($attr == NULL) $attr = $smartypants_attr;

    $do_backticks;   # should we educate ``backticks'' -style quotes?

    if ($attr == 0) {
        # do nothing;
        return $text;
    }
    else if ($attr == 2) {
        # smarten ``backticks'' -style quotes
        $do_backticks = 1;
    }
    else {
        $do_backticks = 0;
    }

    # Special case to handle quotes at the very end of $text when preceded by
    # an HTML tag. Add a space to give the quote education algorithm a bit of
    # context, so that it can guess correctly that it's a closing quote:
    $add_extra_space = 0;
    if (preg_match("/>['\"]\\z/", $text)) {
        $add_extra_space = 1; # Remember, so we can trim the extra space later.
        $text .= " ";
    }

    $tokens = _TokenizeHTML($text);
    $result = '';
    $in_pre = 0;  # Keep track of when we're inside <pre> or <code> tags

    $prev_token_last_char = "";     # This is a cheat, used to get some context
                                    # for one-character tokens that consist of
                                    # just a quote char. What we do is remember
                                    # the last character of the previous text
                                    # token, to use as context to curl single-
                                    # character quote tokens correctly.

    foreach ($tokens as $cur_token) {
        if ($cur_token[0] == "tag") {
            # Don't mess with quotes inside tags
            $result .= $cur_token[1];
            if (preg_match("@$sp_tags_to_skip@", $cur_token[1], $matches)) {
                $in_pre = isset($matches[1]) && $matches[1] == '/' ? 0 : 1;
            }
        } else {
            $t = $cur_token[1];
            $last_char = substr($t, -1); # Remember last char of this token before processing.
            if (! $in_pre) {
                $t = ProcessEscapes($t);
                if ($do_backticks) {
                    $t = EducateBackticks($t);
                }

                if ($t == "'") {
                    # Special case: single-character ' token
                    if (preg_match('/\S/', $prev_token_last_char)) {
                        $t = "&#8217;";
                    }
                    else {
                        $t = "&#8216;";
                    }
                }
                else if ($t == '"') {
                    # Special case: single-character " token
                    if (preg_match('/\S/', $prev_token_last_char)) {
                        $t = "&#8221;";
                    }
                    else {
                        $t = "&#8220;";
                    }
                }
                else {
                    # Normal case:
                    $t = EducateQuotes($t);
                }

            }
            $prev_token_last_char = $last_char;
            $result .= $t;
        }
    }

    if ($add_extra_space) {
        preg_replace('/ \z/', '', $result);  # Trim trailing space if we added one earlier.
    }
    return $result;
}


function SmartDashes($text, $attr = NULL, $ctx = NULL) {
    global $smartypants_attr, $sp_tags_to_skip;
    # Paramaters:
    $text;   # text to be parsed
    $attr;   # value of the smart_dashes="" attribute
    $ctx;    # MT context object (unused)
    if ($attr == NULL) $attr = $smartypants_attr;

    # reference to the subroutine to use for dash education, default to EducateDashes:
    $dash_sub_ref = 'EducateDashes';

    if ($attr == 0) {
        # do nothing;
        return $text;
    }
    else if ($attr == 2) {
        # use old smart dash shortcuts, "--" for en, "---" for em
        $dash_sub_ref = 'EducateDashesOldSchool';
    }
    else if ($attr == 3) {
        # inverse of 2, "--" for em, "---" for en
        $dash_sub_ref = 'EducateDashesOldSchoolInverted';
    }

    $tokens;
    $tokens = _TokenizeHTML($text);

    $result = '';
    $in_pre = 0;  # Keep track of when we're inside <pre> or <code> tags
    foreach ($tokens as $cur_token) {
        if ($cur_token[0] == "tag") {
            # Don't mess with quotes inside tags
            $result .= $cur_token[1];
            if (preg_match("@$sp_tags_to_skip@", $cur_token[1], $matches)) {
                $in_pre = isset($matches[1]) && $matches[1] == '/' ? 0 : 1;
            }
        } else {
            $t = $cur_token[1];
            if (! $in_pre) {
                $t = ProcessEscapes($t);
                $t = $dash_sub_ref($t);
            }
            $result .= $t;
        }
    }
    return $result;
}


function SmartEllipses($text, $attr = NULL, $ctx = NULL) {
    # Paramaters:
    $text;   # text to be parsed
    $attr;   # value of the smart_ellipses="" attribute
    $ctx;    # MT context object (unused)
    if ($attr == NULL) $attr = $smartypants_attr;

    if ($attr == 0) {
        # do nothing;
        return $text;
    }

    $tokens;
    $tokens = _TokenizeHTML($text);

    $result = '';
    $in_pre = 0;  # Keep track of when we're inside <pre> or <code> tags
    foreach ($tokens as $cur_token) {
        if ($cur_token[0] == "tag") {
            # Don't mess with quotes inside tags
            $result .= $cur_token[1];
            if (preg_match("@$sp_tags_to_skip@", $cur_token[1], $matches)) {
                $in_pre = isset($matches[1]) && $matches[1] == '/' ? 0 : 1;
            }
        } else {
            $t = $cur_token[1];
            if (! $in_pre) {
                $t = ProcessEscapes($t);
                $t = EducateEllipses($t);
            }
            $result .= $t;
        }
    }
    return $result;
}


function EducateQuotes($_) {
#
#   Parameter:  String.
#
#   Returns:    The string, with "educated" curly quote HTML entities.
#
#   Example input:  "Isn't this fun?"
#   Example output: &#8220;Isn&#8217;t this fun?&#8221;
#
    # Make our own "punctuation" character class, because the POSIX-style
    # [:PUNCT:] is only available in Perl 5.6 or later:
    $punct_class = "[!\"#\\$\\%'()*+,-.\\/:;<=>?\\@\\[\\\\\]\\^_`{|}~]";

    # Special case if the very first character is a quote
    # followed by punctuation at a non-word-break. Close the quotes by brute force:
    $_ = preg_replace(
        array("/^'(?=$punct_class\\B)/", "/^\"(?=$punct_class\\B)/"),
        array('&#8217;',                 '&#8221;'), $_);


    # Special case for double sets of quotes, e.g.:
    #   <p>He said, "'Quoted' words in a larger quote."</p>
    $_ = preg_replace(
        array("/\"'(?=\w)/",    "/'\"(?=\w)/"),
        array('&#8220;&#8216;', '&#8216;&#8220;'), $_);

    # Special case for decade abbreviations (the '80s):
    $_ = preg_replace("/'(?=\\d{2}s)/", '&#8217;', $_);

    $close_class = '[^\ \t\r\n\[\{\(\-]';
    $dec_dashes = '&\#8211;|&\#8212;';

    # Get most opening single quotes:
    $_ = preg_replace("{
        (
            \\s          |   # a whitespace char, or
            &nbsp;      |   # a non-breaking space entity, or
            --          |   # dashes, or
            &[mn]dash;  |   # named dash entities
            $dec_dashes |   # or decimal entities
            &\\#x201[34];    # or hex
        )
        '                   # the quote
        (?=\\w)              # followed by a word character
        }x", '\1&#8216;', $_);
    # Single closing quotes:
    $_ = preg_replace("{
        ($close_class)?
        '
        (?(1)|          # If $1 captured, then do nothing;
          (?=\\s | s\\b)  # otherwise, positive lookahead for a whitespace
        )               # char or an 's' at a word ending position. This
                        # is a special case to handle something like:
                        # \"<i>Custer</i>'s Last Stand.\"
        }xi", '\1&#8217;', $_);

    # Any remaining single quotes should be opening ones:
    $_ = str_replace("'", '&#8216;', $_);


    # Get most opening double quotes:
    $_ = preg_replace("{
        (
            \\s          |   # a whitespace char, or
            &nbsp;      |   # a non-breaking space entity, or
            --          |   # dashes, or
            &[mn]dash;  |   # named dash entities
            $dec_dashes |   # or decimal entities
            &\\#x201[34];    # or hex
        )
        \"                   # the quote
        (?=\\w)              # followed by a word character
        }x", '\1&#8220;', $_);

    # Double closing quotes:
    $_ = preg_replace("{
        ($close_class)?
        \"
        (?(1)|(?=\\s))   # If $1 captured, then do nothing;
                           # if not, then make sure the next char is whitespace.
        }x", '\1&#8221;', $_);

    # Any remaining quotes should be opening ones.
    $_ = str_replace('"', '&#8220;', $_);

    return $_;
}


function EducateBackticks($_) {
#
#   Parameter:  String.
#   Returns:    The string, with ``backticks'' -style double quotes
#               translated into HTML curly quote entities.
#
#   Example input:  ``Isn't this fun?''
#   Example output: &#8220;Isn't this fun?&#8221;
#

    $_ = str_replace(array("``",       "''",),
                     array('&#8220;', '&#8221;'), $_);
    return $_;
}


function EducateSingleBackticks($_) {
#
#   Parameter:  String.
#   Returns:    The string, with `backticks' -style single quotes
#               translated into HTML curly quote entities.
#
#   Example input:  `Isn't this fun?'
#   Example output: &#8216;Isn&#8217;t this fun?&#8217;
#

    $_ = str_replace(array("`",       "'",),
                     array('&#8216;', '&#8217;'), $_);
    return $_;
}


function EducateDashes($_) {
#
#   Parameter:  String.
#
#   Returns:    The string, with each instance of "--" translated to
#               an em-dash HTML entity.
#

    $_ = str_replace('--', '&#8212;', $_);
    return $_;
}


function EducateDashesOldSchool($_) {
#
#   Parameter:  String.
#
#   Returns:    The string, with each instance of "--" translated to
#               an en-dash HTML entity, and each "---" translated to
#               an em-dash HTML entity.
#

    #                      em         en
    $_ = str_replace(array("---",     "--",),
                     array('&#8212;', '&#8211;'), $_);
    return $_;
}


function EducateDashesOldSchoolInverted($_) {
#
#   Parameter:  String.
#
#   Returns:    The string, with each instance of "--" translated to
#               an em-dash HTML entity, and each "---" translated to
#               an en-dash HTML entity. Two reasons why: First, unlike the
#               en- and em-dash syntax supported by
#               EducateDashesOldSchool(), it's compatible with existing
#               entries written before SmartyPants 1.1, back when "--" was
#               only used for em-dashes.  Second, em-dashes are more
#               common than en-dashes, and so it sort of makes sense that
#               the shortcut should be shorter to type. (Thanks to Aaron
#               Swartz for the idea.)
#

    #                      en         em
    $_ = str_replace(array("---",     "--",),
                     array('&#8211;', '&#8212;'), $_);
    return $_;
}


function EducateEllipses($_) {
#
#   Parameter:  String.
#   Returns:    The string, with each instance of "..." translated to
#               an ellipsis HTML entity. Also converts the case where
#               there are spaces between the dots.
#
#   Example input:  Huh...?
#   Example output: Huh&#8230;?
#

    $_ = str_replace(array("...",     ". . .",), '&#8230;', $_);
    return $_;
}


function StupefyEntities($_) {
#
#   Parameter:  String.
#   Returns:    The string, with each SmartyPants HTML entity translated to
#               its ASCII counterpart.
#
#   Example input:  &#8220;Hello &#8212; world.&#8221;
#   Example output: "Hello -- world."
#

                        #  en-dash    em-dash
    $_ = str_replace(array('&#8211;', '&#8212;'),
                     array('-',       '--'), $_);

    # single quote         open       close
    $_ = str_replace(array('&#8216;', '&#8217;'), "'", $_);

    # double quote         open       close
    $_ = str_replace(array('&#8220;', '&#8221;'), '"', $_);

    $_ = str_replace('&#8230;', '...', $_); # ellipsis

    return $_;
}


function ProcessEscapes($_) {
#
#   Parameter:  String.
#   Returns:    The string, with after processing the following backslash
#               escape sequences. This is useful if you want to force a "dumb"
#               quote or other character to appear.
#
#               Escape  Value
#               ------  -----
#               \\      &#92;
#               \"      &#34;
#               \'      &#39;
#               \.      &#46;
#               \-      &#45;
#               \`      &#96;
#
    $_ = str_replace(
        array('\\\\',  '\"',    "\'",    '\.',    '\-',    '\`'),
        array('&#92;', '&#34;', '&#39;', '&#46;', '&#45;', '&#96;'), $_);

    return $_;
}


# _TokenizeHTML is shared between PHP SmartyPants and PHP Markdown.
# We only define it if it is not already defined.
if (!function_exists('_TokenizeHTML')) :
function _TokenizeHTML($str) {
#
#   Parameter:  String containing HTML markup.
#   Returns:    An array of the tokens comprising the input
#               string. Each token is either a tag (possibly with nested,
#               tags contained therein, such as <a href="<MTFoo>">, or a
#               run of text between tags. Each element of the array is a
#               two-element array; the first is either 'tag' or 'text';
#               the second is the actual value.
#
#
#   Regular expression derived from the _tokenize() subroutine in
#   Brad Choate's MTRegex plugin.
#   <http://www.bradchoate.com/past/mtregex.php>
#
    $index = 0;
    $tokens = array();

    $match = '(?s:<!(?:--.*?--\s*)+>)|'.    # comment
             '(?s:<\?.*?\?>)|'.             # processing instruction
                                            # regular tags
             '(?:<[/!$]?[-a-zA-Z0-9:]+\b(?>[^"\'>]+|"[^"]*"|\'[^\']*\')*>)';

    $parts = preg_split("{($match)}", $str, -1, PREG_SPLIT_DELIM_CAPTURE);

    foreach ($parts as $part) {
        if (++$index % 2 && $part != '')
            $tokens[] = array('text', $part);
        else
            $tokens[] = array('tag', $part);
    }
    return $tokens;
}
endif;


/*

PHP SmartyPants
===============

Description
-----------

This is a PHP translation of the original SmartyPants quote educator written in
Perl by John Gruber.

SmartyPants is a web publishing utility that translates plain ASCII
punctuation characters into "smart" typographic punctuation HTML
entities. SmartyPants can perform the following transformations:

*   Straight quotes (`"` and `'`) into "curly" quote HTML entities
*   Backticks-style quotes (` ``like this'' `) into "curly" quote HTML
    entities
*   Dashes (`--` and `---`) into en- and em-dash entities
*   Three consecutive dots (`...`) into an ellipsis entity

SmartyPants does not modify characters within `<pre>`, `<code>`, `<kbd>`,
`<script>`, or `<math>` tag blocks. Typically, these tags are used to
display text where smart quotes and other "smart punctuation" would not
be appropriate, such as source code or example markup.


### Backslash Escapes ###

If you need to use literal straight quotes (or plain hyphens and
periods), SmartyPants accepts the following backslash escape sequences
to force non-smart punctuation. It does so by transforming the escape
sequence into a decimal-encoded HTML entity:

    Escape  Value  Character
    ------  -----  ---------
      \\    &#92;    \
      \"    &#34;    "
      \'    &#39;    '
      \.    &#46;    .
      \-    &#45;    -
      \`    &#96;    `

This is useful, for example, when you want to use straight quotes as
foot and inch marks: 6'2" tall; a 17" iMac.


Bugs
----

To file bug reports or feature requests (other than topics listed in the
Caveats section above) please send email to:

<michel.fortin@michelf.com>

If the bug involves quotes being curled the wrong way, please send example
text to illustrate.


### Algorithmic Shortcomings ###

One situation in which quotes will get curled the wrong way is when
apostrophes are used at the start of leading contractions. For example:

    'Twas the night before Christmas.

In the case above, SmartyPants will turn the apostrophe into an opening
single-quote, when in fact it should be a closing one. I don't think
this problem can be solved in the general case -- every word processor
I've tried gets this wrong as well. In such cases, it's best to use the
proper HTML entity for closing single-quotes (`&#8217;`) by hand.


Version History
---------------

1.5.1e (9 Dec 2005)

*   Corrected a bug that prevented special characters from being
    escaped.


1.5.1d (25 May 2005)

*   Corrected a small bug in `_TokenizeHTML` where a Doctype declaration
    was not seen as HTML (smart quotes where applied inside).


1.5.1c (13 Dec 2004)

*   Changed a regular expression in `_TokenizeHTML` that could lead to
    a segmentation fault with PHP 4.3.8 on Linux.


1.5.1b (6 Sep 2004)

*   Corrected a problem with quotes immediately following a dash
    with no space between: `Text--"quoted text"--text.`

*   PHP SmartyPants can now be used as a modifier by the Smarty
    template engine. Rename the file to "modifier.smartypants.php"
    and put it in your smarty plugins folder.

*   Replaced a lot of space characters by tabs, saving about 4 KB.


1.5.1a (30 Jun 2004)

*   PHP Markdown and PHP Smartypants now share the same `_TokenizeHTML`
    function when loaded simultanously.

*   Changed the internals of `_TokenizeHTML` to lower the PHP version
    requirement to PHP 4.0.5.


1.5.1 (6 Jun 2004)

*   Initial release of PHP SmartyPants, based on version 1.5.1 of the
    original SmartyPants written in Perl.


Author
------

John Gruber
<http://daringfireball.net/>

Ported to PHP by Michel Fortin
<http://www.michelf.com/>


Additional Credits
------------------

Portions of this plug-in are based on Brad Choate's nifty MTRegex plug-in.
Brad Choate also contributed a few bits of source code to this plug-in.
Brad Choate is a fine hacker indeed. (<http://bradchoate.com/>)

Jeremy Hedley (<http://antipixel.com/>) and Charles Wiltgen
(<http://playbacktime.com/>) deserve mention for exemplary beta testing.


Copyright and License
---------------------

Copyright (c) 2003 John Gruber
<http://daringfireball.net/>
All rights reserved.

Copyright (c) 2004-2005 Michel Fortin
<http://www.michelf.com>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

*   Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

*   Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

*   Neither the name "SmartyPants" nor the names of its contributors may
    be used to endorse or promote products derived from this software
    without specific prior written permission.

This software is provided by the copyright holders and contributors "as is"
and any express or implied warranties, including, but not limited to, the
implied warranties of merchantability and fitness for a particular purpose
are disclaimed. In no event shall the copyright owner or contributors be
liable for any direct, indirect, incidental, special, exemplary, or
consequential damages (including, but not limited to, procurement of
substitute goods or services; loss of use, data, or profits; or business
interruption) however caused and on any theory of liability, whether in
contract, strict liability, or tort (including negligence or otherwise)
arising in any way out of the use of this software, even if advised of the
possibility of such damage.

*/
?>