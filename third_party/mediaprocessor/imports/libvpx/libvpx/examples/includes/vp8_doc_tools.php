#!/usr/bin/env php
<?php
/**
 * vp8_doc_tools.php - Functions used when generating the
 *   On2 VP8 user documentation.
 *
 * Requirements
 *
 *   PHP Markdown Extra
 *   http://michelf.com/projects/php-markdown/extra/
 *
 *   PHP SmartyPants
 *   http://michelf.com/projects/php-smartypants/
 *
 *   GeSHI
 *   http://qbnz.com/highlighter/
 *
 *   ASCIIMathPHP
 *   http://tinyurl.com/asciimathphp
 *
 *   HTML::Toc
 *   http://search.cpan.org/~fvulto/HTML-Toc-0.91/Toc.pod
 *
 *
 * April 2009 - Lou Quillio <lou.quillio@on2.com>
 *
 **********************************************************/


// Includes
include_once('PHP-Markdown-Extra-1.2.3/markdown.php');
include_once('PHP-SmartyPants-1.5.1e/smartypants.php');
include_once('ASCIIMathPHP-2.0/ASCIIMathPHP-2.0.cfg.php');
require_once('ASCIIMathPHP-2.0/ASCIIMathPHP-2.0.class.php');
include_once('geshi/geshi.php');


// Paths and Scripts
$geshi_lang   = 'geshi/geshi/';       // GeSHi language files
$toc_script     = './do_toc.pl';



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

/**
 * do_geshi() - Performs GeSHi transforms on XHTML blobs
 *
 * @param $blob str  - The blob to transform
 * @param $open str  - Opening expression to match
 * @param $close str - Closing expression to match
 * @param $lang str  - Language file to use
 *
 **********************************************************/

function do_geshi($blob, $open = '<pre>',
                    $close = '</pre>', $lang = 'c')
{
  $out = FALSE;
  $regexp = '|' . $open . '(.*?)' . $close . '|si';
  echo $regexp . "\n\n";

  while (preg_match($regexp, $blob, $matches))
  {
    $geshi = new GeSHi($matches[1], $lang);
    $geshi->set_language_path($geshi_lang);
    $blob_new = $geshi->parse_code();
    // Strip annoying final <br />
    $blob_new  = preg_replace('/\n&nbsp;<\/pre>/', '</pre>' , $blob_new);
    // Fix annoying GeSHI-injected attributes
    $blob_new  = preg_replace('/<pre.*>/i', '<pre>' , $blob_new);
    $blob  = preg_replace($regexp, $blob_new, $blob, 1);
    unset($geshi);
  }

  return $out;

}




/**
 * prep_dd_codeblocks()
 *
 * I'm _so_ not proud of this, but don't have time to
 * write a proper regex.
 *
 * @TODO - Write that regex.
 *
 **********************************************************/
/*
function prep_dd_codeblocks($page_body)
{
  $out = FALSE;
  $toggle = 0;
  $regexp = '/~{3,}/';

  while (preg_match($regexp, $page_body))
  {
    if ($toggle == 0)
    {
      $regexp = '/:\s*~{3,}\s*\n/';
      $page_body = preg_replace($regexp, ': <pre><code>', $page_body, 1);
      $toggle = 1;
    }
    else
    {
      $regexp = '/\n\s*~{3,}/';
      $page_body = preg_replace($regexp, '</code></pre>', $page_body, 1);
      $toggle = 0;
    }
  }

  // One more time
  $regexp = '/\n\s*~{3,}/';
  $page_body = preg_replace($regexp, '</code></pre>', $page_body, 1);
  $out = $page_body;

  return $out;
}
*/
