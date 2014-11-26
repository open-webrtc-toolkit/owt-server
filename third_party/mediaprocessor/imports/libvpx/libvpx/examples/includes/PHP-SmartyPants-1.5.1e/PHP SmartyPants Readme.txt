PHP SmartyPants
===============

Version 1.5.1e - Fri 9 Dec 2005

by Michel Fortin
<http://www.michelf.com/>

based on work by John Gruber
<http://daringfireball.net/>


Introduction
------------

PHP SmartyPants is a port to PHP of the original SmartyPants written
in Perl by John Gruber.

PHP SmartyPants is a free web publishing plug-in for WordPress and
Smarty template engine that easily translates plain ASCII punctuation
characters into "smart" typographic punctuation HTML entities.
SmartyPants can also be invoked as a standalone PHP function.

SmartyPants can perform the following transformations:

*   Straight quotes (`"` and `'`) into "curly" quote HTML entities
*   Backtick-style quotes (` ``like this'' `) into "curly" quote HTML
    entities
*   Dashes (`--` and `---`) into en- and em-dash entities
*   Three consecutive dots (`...`) into an ellipsis entity

This means you can write, edit, and save using plain old ASCII straight
quotes, plain dashes, and plain dots, but your published posts (and
final HTML output) will appear with smart quotes, em-dashes, and proper
ellipses.

SmartyPants does not modify characters within `<pre>`, `<code>`,
`<kbd>`, or `<script>` tag blocks. Typically, these tags are used to
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
foot and inch marks:

    6\'2\" tall

translates into:

    6&#39;2&#34; tall

in SmartyPants's HTML output. Which, when rendered by a web browser,
looks like:

    6'2" tall


Installation and Requirement
----------------------------

PHP SmartyPants require PHP version 4.0.5 or later.


### WordPress ###

WordPress already include a filter called "Texturize" with the same
goal as SmartyPants. You could still find some usefulness to
PHP SmartyPants if you are not happy enough with the standard algorithm.

PHP SmartyPants works with [WordPress][wp], version 1.2 or later.

[wp]: http://wordpress.org/

1.  To use PHP SmartyPants with WordPress, place the "smartypants.php"
    file in the "plugins" folder. This folder is hidden inside
    "wp-content" at the root of your site:

        (site home)/wp-content/plugins/smartypants.php

2.  Activate the plugin with the administrative interface of WordPress.
    In the "Plugins" section you will now find SmartyPants. To activate
    the plugin, click on the "Activate" button on the same line than
    SmartyPants. Your entries will now be filtered by PHP SmartyPants.

Note: It is not possible at this time to apply a different set of
filters to different entries. All your entries will be filtered by
PHP SmartyPants if the plugin is active. This is currently a limitation
of WordPress.


### Blosxom ###

SmartyPants works with Blosxom version 2.0 or later.

1.  Rename the "SmartyPants.pl" plug-in to "SmartyPants" (case is
    important). Movable Type requires plug-ins to have a ".pl"
    extension; Blosxom forbids it (at least as of this writing).

2.  Copy the "SmartyPants" plug-in file to your Blosxom plug-ins folder.
    If you're not sure where your Blosxom plug-ins folder is, see the
    Blosxom documentation for information.

3.  That's it. The entries in your weblog should now automatically have
    SmartyPants's default transformations applied.

4.  If you wish to configure SmartyPants's behavior, open the
    "SmartyPants" plug-in, and edit the value of the `$smartypants_attr`
    configuration variable, located near the top of the script. The
    default value is 1; see "Options", below, for the full list of
    supported values.


### In your programs ###

You can use PHP SmartyPants easily in your current PHP program. Simply
include the file and then call the `SmartyPants` function on the text
you want to convert:

	include_once "smartypants.php";
	$my_text = SmartyPants($my_text);


### With Smarty ###

If your program use the [Smarty][sm] template engine, PHP SmartyPants
can now be used as a modifier for your templates. Rename
"smartypants.php" to "modifier.smartypants.php" and put it in your
smarty plugins folder.

[sm]: http://smarty.php.net/


Options and Configuration
-------------------------

Settings are specified by editing the value of the `$smartypants_attr`
variable in the "smartypants.php" file. For users of the Smarty template
engine, the "smartypants" modifier also takes an optional attribute where
you can specify configuration options, like this:
`{$var|smartypants:1}` (where "1" is the configuration option).

Numeric values are the easiest way to configure SmartyPants's behavior:

"0"
    Suppress all transformations. (Do nothing.)

"1"
    Performs default SmartyPants transformations: quotes (including
    backticks-style), em-dashes, and ellipses. `--` (dash dash) is
    used to signify an em-dash; there is no support for en-dashes.

"2"
    Same as smarty_pants="1", except that it uses the old-school
    typewriter shorthand for dashes: `--` (dash dash) for en-dashes,
    `---` (dash dash dash) for em-dashes.

"3"
    Same as smarty_pants="2", but inverts the shorthand for dashes: `--`
    (dash dash) for em-dashes, and `---` (dash dash dash) for en-dashes.

"-1"
    Stupefy mode. Reverses the SmartyPants transformation process,
    turning the HTML entities produced by SmartyPants into their ASCII
    equivalents. E.g. `&#8220;` is turned into a simple double-quote
    (`"`), `&#8212;` is turned into two dashes, etc. This is useful if you
    wish to suppress smart punctuation in specific pages, such as
    RSS feeds.

The following single-character attribute values can be combined to
toggle individual transformations from within the smarty_pants
attribute. For example, to educate normal quotes and em-dashes, but not
ellipses or backticks-style quotes:

    $smartypants_attr = "qd";

Or inside a Smarty template:

    {$var|smartypants:"qd"}

"q"
    Educates normal quote characters: (`"`) and (`'`).

"b"
    Educates ` ``backticks'' ` double quotes.

"B"
    Educates backticks-style double quotes and ` `single' ` quotes.

"d"
    Educates em-dashes.

"D"
    Educates em-dashes and en-dashes, using old-school typewriter
    shorthand: (dash dash) for en-dashes, (dash dash dash) for
    em-dashes.

"i"
    Educates em-dashes and en-dashes, using inverted old-school
    typewriter shorthand: (dash dash) for em-dashes, (dash dash dash)
    for en-dashes.

"e"
    Educates ellipses.

"w"
    Translates any instance of `&quot;` into a normal double-quote
    character. This should be of no interest to most people, but of
    particular interest to anyone who writes their posts using
    Dreamweaver, as Dreamweaver inexplicably uses this entity to
    represent a literal double-quote character. SmartyPants only
    educates normal quotes, not entities (because ordinarily, entities
    are used for the explicit purpose of representing the specific
    character they represent). The "w" option must be used in
    conjunction with one (or both) of the other quote options ("q" or
    "b"). Thus, if you wish to apply all SmartyPants transformations
    (quotes, en- and em-dashes, and ellipses) and also translate
    `&quot;` entities into regular quotes so SmartyPants can educate
    them, you should pass the following to the smarty_pants attribute:

        $smartypants_attr = "qDew";

    Inside a Smarty template, this will be:

        {$var|smartypants:"qDew"}


Caveats
-------

### Why You Might Not Want to Use Smart Quotes in Your Weblog ###

For one thing, you might not care.

Most normal, mentally stable individuals do not take notice of proper
typographic punctuation. Many design and typography nerds, however,
break out in a nasty rash when they encounter, say, a restaurant sign
that uses a straight apostrophe to spell "Joe's".

If you're the sort of person who just doesn't care, you might well want
to continue not caring. Using straight quotes -- and sticking to the
7-bit ASCII character set in general -- is certainly a simpler way to
live.

Even if you *do* care about accurate typography, you still might want to
think twice before educating the quote characters in your weblog. One
side effect of publishing curly quote HTML entities is that it makes
your weblog a bit harder for others to quote from using copy-and-paste.
What happens is that when someone copies text from your blog, the copied
text contains the 8-bit curly quote characters (as well as the 8-bit
characters for em-dashes and ellipses, if you use these options). These
characters are not standard across different text encoding methods,
which is why they need to be encoded as HTML entities.

People copying text from your weblog, however, may not notice that
you're using curly quotes, and they'll go ahead and paste the unencoded
8-bit characters copied from their browser into an email message or
their own weblog. When pasted as raw "smart quotes", these characters
are likely to get mangled beyond recognition.

That said, my own opinion is that any decent text editor or email client
makes it easy to stupefy smart quote characters into their 7-bit
equivalents, and I don't consider it my problem if you're using an
indecent text editor or email client.

### Algorithmic Shortcomings ###

One situation in which quotes will get curled the wrong way is when
apostrophes are used at the start of leading contractions. For example:

    'Twas the night before Christmas.

In the case above, SmartyPants will turn the apostrophe into an opening
single-quote, when in fact it should be a closing one. I don't think
this problem can be solved in the general case -- every word processor
I've tried gets this wrong as well. In such cases, it's best to use the
proper HTML entity for closing single-quotes (`&#8217;` or `&rsquo;`) by
hand.


Bugs
----

To file bug reports or feature requests (other than topics listed in the
Caveats section above) please send email to:

<michel.fortin@michelf.com>

If the bug involves quotes being curled the wrong way, please send
example text to illustrate.


Version History
---------------

1.5.1e (9 Dec 2005)

*	Corrected a bug that prevented special characters from being
    escaped.


1.5.1d (6 Jun 2005)

*	Correct a small bug in `_TokenizeHTML` where a Doctype declaration
	was not seen as HTML, making curly quotes inside it.


1.5.1c (13 Dec 2004)

*	Changed a regular expression in `_TokenizeHTML` that could lead
	to a segmentation fault with PHP 4.3.8 on Linux.


1.5.1b (6 Sep 2004)

*	Corrected a problem with quotes immediately following a dash
	with no space between: `Text--"quoted text"--text.`

*	PHP SmartyPants can now be used as a modifier by the Smarty
	template engine. Rename the file to "modifier.smartypants.php"
	and put it in your smarty plugins folder.

*	Replaced a lot of spaces characters by tabs, saving about 4 KB.


1.5.1a (30 Jun 2004)

*	PHP Markdown and PHP Smartypants now share the same `_TokenizeHTML`
	function when loaded simultanously.

*	Changed the internals of `_TokenizeHTML` to lower the PHP version
	requirement to PHP 4.0.5.


1.5.1 (6 Jun 2004)

*	Initial release of PHP SmartyPants, based on version 1.5.1 of the
	original SmartyPants written in Perl.


Copyright and License
---------------------

Copyright (c) 2005 Michel Fortin
<http://www.michelf.com/>
All rights reserved.

Copyright (c) 2003-2004 John Gruber
<http://daringfireball.net/>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

*   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

*   Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

*   Neither the name "SmartyPants" nor the names of its contributors may
    be used to endorse or promote products derived from this software
    without specific prior written permission.

This software is provided by the copyright holders and contributors "as
is" and any express or implied warranties, including, but not limited
to, the implied warranties of merchantability and fitness for a
particular purpose are disclaimed. In no event shall the copyright owner
or contributors be liable for any direct, indirect, incidental, special,
exemplary, or consequential damages (including, but not limited to,
procurement of substitute goods or services; loss of use, data, or
profits; or business interruption) however caused and on any theory of
liability, whether in contract, strict liability, or tort (including
negligence or otherwise) arising in any way out of the use of this
software, even if advised of the possibility of such damage.
