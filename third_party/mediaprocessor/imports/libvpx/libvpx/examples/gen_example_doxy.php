#!/usr/bin/env php
/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


<?php

/* This script converts markdown to doxygen htmlonly syntax, nesting the
 * content inside a \page. It expects input on stdin and outputs on stdout.
 *
 * Usage: gen_example_doxy.php <page_identifier> "<page description>"
 */


$geshi_path = dirname($argv[0])."/includes/geshi/geshi/"; // Language files
$tmp_token  = '<!-- I wanna rock you, Chaka Khan -->';

// Include prerequisites or exit
if(!include_once('includes/PHP-Markdown-Extra-1.2.3/markdown.php'))
  die("Cannot load Markdown transformer.\n");
if(!include_once('includes/PHP-SmartyPants-1.5.1e/smartypants.php'))
  die("Cannot load SmartyPants transformer.\n");
if(!include_once('includes/geshi/geshi.php'))
  die("Cannot load GeSHi transformer.\n");
// ASCIIMathPHP?
// HTML::Toc?
// Tidy?
// Prince?

/**
 *  Generate XHTML body
 *
 */

$page_body = file_get_contents('php://stdin');

// Transform any MathML expressions in the body text
$regexp = '/\[\[(.*?)\]\]/'; // Double square bracket delimiters
$page_body = preg_replace_callback($regexp, 'ASCIIMathPHPCallback', $page_body);

// Fix ASCIIMathPHP's output
$page_body = fix_asciiMath($page_body);

// Wrap block-style <math> elements in <p>, since Markdown doesn't.
$page_body = preg_replace('/\n(<math.*<\/math>)\n/', '<p class="eq_para">$1</p>', $page_body);

// Transform the body text to HTML
$page_body = Markdown($page_body);

// Preprocess code blocks
// Decode XML entities. GeSHi doesn't anticipate that
// Markdown has already done this.
$regexp = '|<pre><code>(.*?)<\/code><\/pre>|si';
while (preg_match($regexp, $page_body, $matches) > 0)
{
  // Replace 1st match with token
  $page_body = preg_replace($regexp, $tmp_token, $page_body, 1);
  $block_new = $matches[1];
  // Un-encode ampersand entities
  $block_new = decode_markdown($block_new);
  // Replace token with revised string
  $page_body = preg_replace("|$tmp_token|", '<div class="codeblock">'.$block_new.'</div>', $page_body);
}

// Run GeSHi over code blocks
$regexp   = '|<div class="codeblock">(.*?)<\/div>|si';
$language = 'c';

while (preg_match($regexp, $page_body, $matches))
{
  $geshi = new GeSHi($matches[1], $language);
  $geshi->set_language_path($geshi_path);
  $block_new = $geshi->parse_code();
  // Strip annoying final newline
  $block_new = preg_replace('|\n&nbsp;<\/pre>|', '</pre>' , $block_new);
  // Remove style attribute (TODO: Research this in GeSHi)
  $block_new = preg_replace('| style="font-family:monospace;"|', '' , $block_new);
  $page_body = preg_replace($regexp, $block_new, $page_body, 1);
  unset($geshi);    // Clean up
}
unset($block_new);  // Clean up

// Apply typographic flourishes
$page_body = SmartyPants($page_body);


/**
 *  Generate Doxygen Body
 *
 */
$page_id=(isset($argv[1]))?$argv[1]:"";
$page_desc=(isset($argv[2]))?$argv[2]:"";
print "/*!\\page ".$page_id." ".$page_desc."\n\\htmlonly\n";
print $page_body;
print "\\endhtmlonly\n*/\n";

// ---------------------------------------------------------

/**
 * decode_markdown()
 *
 * Markdown encodes '&', '<' and '>' in detected code
 * blocks, as a convenience. This will restore the
 * encoded entities to ordinary characters, since a
 * downstream transformer (like GeSHi) may not
 * anticipate this.
 *
 **********************************************************/

function decode_markdown($input)
{
  $out = FALSE;

  $entities   = array ('|&amp;|'
                      ,'|&lt;|'
                      ,'|&gt;|'
                      );
  $characters = array ('&'
                      ,'<'
                      ,'>'
                      );
  $input = preg_replace($entities, $characters, $input);
  $out = $input;

  return $out;
}


/**
 * ASCIIMathML parser
 * http://tinyurl.com/ASCIIMathPHP
 *
 * @PARAM mtch_arr array - Array of ASCIIMath expressions
 *   as returned by preg_replace_callback([pattern]). First
 *   dimension is the full matched string (with delimiter);
 *   2nd dimension is the undelimited contents (typically
 *   a capture group).
 *
 **********************************************************/

function ASCIIMathPHPCallback($mtch_arr)
{
  $txt = trim($mtch_arr[1]);

  include('includes/ASCIIMathPHP-2.0/ASCIIMathPHP-2.0.cfg.php');
  require_once('includes/ASCIIMathPHP-2.0/ASCIIMathPHP-2.0.class.php');

  static $asciimath;

  if (!isset($asciimath)) $asciimath = new ASCIIMathPHP($symbol_arr);

  $math_attr_arr = array('displaystyle' => 'true');

  $asciimath->setExpr($txt);
  $asciimath->genMathML($math_attr_arr);

  return($asciimath->getMathML());
}

/**
 * fix_asciiMath()
 *
 * ASCIIMath pretty-prints its output, with linefeeds
 * and tabs. Causes unexpected behavior in some renderers.
 * This flattens <math> blocks.
 *
 * @PARAM page_body str - The <body> element of an
 * XHTML page to transform.
 *
 **********************************************************/

function fix_asciiMath($page_body)
{
  $out = FALSE;

  // Remove linefeeds and whitespace in <math> elements
  $tags_bad  = array('/(<math.*?>)\n*\s*/'
                    , '/(<mstyle.*?>)\n*\s*/'
                    , '/(<\/mstyle>)\n*\s*/'
                    , '/(<mrow.*?>)\n*\s*/'
                    , '/(<\/mrow>)\n*\s*/'
                    , '/(<mo.*?>)\n*\s*/'
                    , '/(<\/mo>)\n*\s*/'
                    , '/(<mi.*?>)\n*\s*/'
                    , '/(<\/mi>)\n*\s*/'
                    , '/(<mn.*?>)\n*\s*/'
                    , '/(<\/mn>)\n*\s*/'
                    , '/(<mtext.*?>)\n*\s*/'
                    , '/(<\/mtext>)\n*\s*/'
                    , '/(<msqrt.*?>)\n*\s*/'
                    , '/(<\/msqrt>)\n*\s*/'
                    , '/(<mfrac.*?>)\n*\s*/'
                    , '/(<\/mfrac>)\n*\s*/'
                    );
  $tags_good = array( '$1'
                    , '$1'
                    , '$1'
                    , '$1'
                    , '$1'
                    , '$1'
                    , '$1'
                    , '$1'
                    , '$1'
                    , '$1'
                    , '$1'
                    , '$1'
                    , '$1'
                    , '$1'
                    , '$1'
                    , '$1'
                    , '$1'
                    );
  $out = preg_replace($tags_bad, $tags_good, $page_body);

  return $out;

}
