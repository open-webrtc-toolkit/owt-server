<?php

$symbol_arr = array(

// Greek symbols
'alpha' => array( 'input'=>'alpha','tag'=>'mi', 'output'=>'&#' . hexdec('03B1') . ';'),
'beta'  => array( 'input'=>'beta','tag'=>'mi', 'output'=>'&#' . hexdec('03B2') . ';'),
'chi'   => array( 'input'=>'chi','tag'=>'mi', 'output'=>'&#' . hexdec('03C7') . ';'),
'delta' => array( 'input'=>'delta','tag'=>'mi', 'output'=>'&#' . hexdec('03B4') . ';'),
'Delta' => array( 'input'=>'Delta','tag'=>'mo', 'output'=>'&#' . hexdec('0394') . ';'),
'epsi'  => array( 'input'=>'epsi','tag'=>'mi', 'output'=>'&#' . hexdec('03B5') . ';'),
'varepsilon'    => array( 'input'=>'varepsilon','tag'=>'mi', 'output'=>'&#' . hexdec('025B') . ';'),
'eta'   => array( 'input'=>'eta','tag'=>'mi', 'output'=>'&#' . hexdec('03B7') . ';'),
'gamma' => array( 'input'=>'gamma','tag'=>'mi', 'output'=>'&#' . hexdec('03B3') . ';'),
'Gamma' => array( 'input'=>'Gamma','tag'=>'mi', 'output'=>'&#' . hexdec('0393') . ';'),
'iota'  => array( 'input'=>'iota','tag'=>'mi', 'output'=>'&#' . hexdec('03B9') . ';'),
'kappa' => array( 'input'=>'kappa','tag'=>'mi', 'output'=>'&#' . hexdec('03BA') . ';'),
'lambda'    => array( 'input'=>'lambda','tag'=>'mi', 'output'=>'&#' . hexdec('03BB') . ';'),
'Lambda'    => array( 'input'=>'Lambda','tag'=>'mo', 'output'=>'&#' . hexdec('039B') . ';'),
'mu'    => array( 'input'=>'mu','tag'=>'mi', 'output'=>'&#' . hexdec('03BC') . ';'),
'nu'    => array( 'input'=>'nu','tag'=>'mi', 'output'=>'&#' . hexdec('03BD') . ';'),
'omega' => array( 'input'=>'omega','tag'=>'mi', 'output'=>'&#' . hexdec('03C9') . ';'),
'Omega' => array( 'input'=>'Omega','tag'=>'mo', 'output'=>'&#' . hexdec('03A9') . ';'),
'phi'   => array( 'input'=>'phi','tag'=>'mi', 'output'=>'&#' . hexdec('03C6') . ';'),
'varphi'    => array( 'input'=>'varphi','tag'=>'mi', 'output'=>'&#' . hexdec('03D5') . ';'),
'Phi'   => array( 'input'=>'Phi','tag'=>'mo', 'output'=>'&#' . hexdec('03A6') . ';'),
'pi'    => array( 'input'=>'pi','tag'=>'mi', 'output'=>'&#' . hexdec('03C0') . ';'),
'Pi'    => array( 'input'=>'Pi','tag'=>'mo', 'output'=>'&#' . hexdec('03A0') . ';'),
'psi'   => array( 'input'=>'psi','tag'=>'mi', 'output'=>'&#' . hexdec('03C8') . ';'),
'rho'   => array( 'input'=>'rho','tag'=>'mi', 'output'=>'&#' . hexdec('03C1') . ';'),
'sigma' => array( 'input'=>'sigma','tag'=>'mi', 'output'=>'&#' . hexdec('03C3') . ';'),
'Sigma' => array( 'input'=>'Sigma','tag'=>'mo', 'output'=>'&#' . hexdec('03A3') . ';'),
'tau'   => array( 'input'=>'tau','tag'=>'mi', 'output'=>'&#' . hexdec('03C4') . ';'),
'theta' => array( 'input'=>'theta','tag'=>'mi', 'output'=>'&#' . hexdec('03B8') . ';'),
'vartheta'  => array( 'input'=>'vartheta','tag'=>'mi', 'output'=>'&#' . hexdec('03D1') . ';'),
'Theta' => array( 'input'=>'Theta','tag'=>'mo', 'output'=>'&#' . hexdec('0398') . ';'),
'upsilon'   => array( 'input'=>'upsilon','tag'=>'mi', 'output'=>'&#' . hexdec('03C5') . ';'),
'xi'    => array( 'input'=>'xi','tag'=>'mi', 'output'=>'&#' . hexdec('03BE') . ';'),
'Xi'    => array( 'input'=>'alpha','tag'=>'mo', 'output'=>'&#' . hexdec('039E') . ';'),
'zeta'  => array( 'input'=>'zeta','tag'=>'mi', 'output'=>'&#' . hexdec('03B6') . ';'),

// Binary operation symbols
'*'     => array( 'input'=>'*','tag'=>'mo', 'output'=>'&#' . hexdec('22C5') . ';'),
'**'    => array( 'input'=>'**','tag'=>'mo', 'output'=>'&#' . hexdec('22C6') . ';'),
'//'    => array( 'input'=>'//','tag'=>'mo', 'output'=>'/'),
'\\\\'  => array( 'input'=>'\\\\','tag'=>'mo', 'output'=>'\\'),
'xx'    => array( 'input'=>'xx','tag'=>'mo', 'output'=>'&#' . hexdec('00D7') . ';'),
'-:'    => array( 'input'=>'-:','tag'=>'mo', 'output'=>'&#' . hexdec('00F7') . ';'),
'@'     => array( 'input'=>'@','tag'=>'mo', 'output'=>'&#' . hexdec('2218') . ';'),
'o+'    => array( 'input'=>'o+','tag'=>'mo', 'output'=>'&#' . hexdec('2295') . ';'),
'ox'    => array( 'input'=>'ox','tag'=>'mo', 'output'=>'&#' . hexdec('2297') . ';'),
'sum'   => array( 'input'=>'sum','tag'=>'mo', 'output'=>'&#' . hexdec('2211') . ';', 'underover'=>TRUE),
'prod'  => array( 'input'=>'prod','tag'=>'mo', 'output'=>'&#' . hexdec('220F') . ';', 'underover'=>TRUE),
'^^'    => array( 'input'=>'^^','tag'=>'mo', 'output'=>'&#' . hexdec('2227') . ';'),
'^^^'   => array( 'input'=>'^^^','tag'=>'mo', 'output'=>'&#' . hexdec('22C0') . ';', 'underover'=>TRUE),
'vv'    => array( 'input'=>'vv','tag'=>'mo', 'output'=>'&#' . hexdec('2228') . ';'),
'vvv'   => array( 'input'=>'vvv','tag'=>'mo', 'output'=>'&#' . hexdec('22C1') . ';', 'underover'=>TRUE),
'nn'    => array( 'input'=>'nn','tag'=>'mo', 'output'=>'&#' . hexdec('2229') . ';'),
'nnn'   => array( 'input'=>'nnn','tag'=>'mo', 'output'=>'&#' . hexdec('22C5') . ';', 'underover'=>TRUE),
'uu'    => array( 'input'=>'uu','tag'=>'mo', 'output'=>'&#' . hexdec('222A') . ';'),
'uuu'   => array( 'input'=>'uuu','tag'=>'mo', 'output'=>'&#' . hexdec('22C3') . ';', 'underover'=>TRUE),

// Binary relation symbols
'!='    => array( 'input'=>'!=','tag'=>'mo', 'output'=>'&#' . hexdec('2260') . ';'),
':='    => array( 'input'=>':=','tag'=>'mo', 'output'=>':=' ),              /* 2005-06-05 wes */
'<'     => array( 'input'=>'<','tag'=>'mo', 'output'=>'&lt;'),
'lt'    => array( 'input'=>'lt','tag'=>'mo', 'output'=>'&lt;'),             /* 2005-06-05 wes */
'<='    => array( 'input'=>'<=','tag'=>'mo', 'output'=>'&#' . hexdec('2264') . ';'),
'lt='   => array( 'input'=>'lt=','tag'=>'mo', 'output'=>'&#' . hexdec('2264') . ';'),
'le'    => array( 'input'=>'le','tag'=>'mo', 'output'=>'&#' . hexdec('2264') . ';'),    /* 2005-06-05 wes */
'>'     => array( 'input'=>'>','tag'=>'mo', 'output'=>'&gt;'),
'>='    => array( 'input'=>'>=','tag'=>'mo', 'output'=>'&#' . hexdec('2265') . ';'),
'qeq'   => array( 'input'=>'geq','tag'=>'mo', 'output'=>'&#' . hexdec('2265') . ';'),
'-<'    => array( 'input'=>'-<','tag'=>'mo', 'output'=>'&#' . hexdec('227A') . ';'),
'-lt'   => array( 'input'=>'-lt','tag'=>'mo', 'output'=>'&#' . hexdec('227A') . ';'),
'>-'    => array( 'input'=>'>-','tag'=>'mo', 'output'=>'&#' . hexdec('227B') . ';'),
'in'    => array( 'input'=>'in','tag'=>'mo', 'output'=>'&#' . hexdec('2208') . ';'),
'!in'   => array( 'input'=>'!in','tag'=>'mo', 'output'=>'&#' . hexdec('2209') . ';'),
'sub'   => array( 'input'=>'sub','tag'=>'mo', 'output'=>'&#' . hexdec('2282') . ';'),
'sup'   => array( 'input'=>'sup','tag'=>'mo', 'output'=>'&#' . hexdec('2283') . ';'),
'sube'  => array( 'input'=>'sube','tag'=>'mo', 'output'=>'&#' . hexdec('2286') . ';'),
'supe'  => array( 'input'=>'supe','tag'=>'mo', 'output'=>'&#' . hexdec('2287') . ';'),
'-='    => array( 'input'=>'-=','tag'=>'mo', 'output'=>'&#' . hexdec('2261') . ';'),
'~='    => array( 'input'=>'~=','tag'=>'mo', 'output'=>'&#' . hexdec('2245') . ';'),
'~~'    => array( 'input'=>'~~','tag'=>'mo', 'output'=>'&#' . hexdec('2248') . ';'),
'prop'  => array( 'input'=>'prop','tag'=>'mo', 'output'=>'&#' . hexdec('221D') . ';'),

// Logical symbols
'and'   => array( 'input'=>'and','tag'=>'mtext', 'output'=>'and', 'space'=>'1ex'),
'or'    => array( 'input'=>'or','tag'=>'mtext', 'output'=>'or', 'space'=>'1ex'),
'not'   => array( 'input'=>'not','tag'=>'mo', 'output'=>'&#' . hexdec('00AC') . ';'),
'=>'    => array( 'input'=>'=>','tag'=>'mo', 'output'=>'&#' . hexdec('21D2') . ';'),
'if'    => array( 'input'=>'if','tag'=>'mo', 'output'=>'if', 'space'=>'1ex'),
'iff'   => array( 'input'=>'iff','tag'=>'mo', 'output'=>'&#' . hexdec('21D4') . ';'),
'<=>'   => array( 'input'=>'iff','tag'=>'mo', 'output'=>'&#' . hexdec('21D4') . ';'), /* 2005-06-07 wes */
'AA'    => array( 'input'=>'AA','tag'=>'mo', 'output'=>'&#' . hexdec('2200') . ';'),
'EE'    => array( 'input'=>'EE','tag'=>'mo', 'output'=>'&#' . hexdec('2203') . ';'),
'_|_'   => array( 'input'=>'_|_','tag'=>'mo', 'output'=>'&#' . hexdec('22A5') . ';'),
'TT'    => array( 'input'=>'TT','tag'=>'mo', 'output'=>'&#' . hexdec('22A4') . ';'),
'|-'    => array( 'input'=>'|-','tag'=>'mo', 'output'=>'&#' . hexdec('22A2') . ';'),
'|='    => array( 'input'=>'|=','tag'=>'mo', 'output'=>'&#' . hexdec('22A8') . ';'),

// Miscellaneous symbols
'ang'   => array('input'=>'ang','tag'=>'mo','output'=>'&#' . hexdec('2220') . ';'),
'deg'   => array('input'=>'deg','tag'=>'mo','output'=>'&#' . hexdec('00B0') . ';'),
'int'   => array( 'input'=>'int','tag'=>'mo', 'output'=>'&#' . hexdec('222B') . ';'),
'dx'    => array( 'input'=>'dx','tag'=>'mi', 'output'=>'{:d x:}', 'definition'=>TRUE), /* 2005-06-11 wes */
'dy'    => array( 'input'=>'dy','tag'=>'mi', 'output'=>'{:d y:}', 'definition'=>TRUE), /* 2005-06-11 wes */
'dz'    => array( 'input'=>'dz','tag'=>'mi', 'output'=>'{:d z:}', 'definition'=>TRUE), /* 2005-06-11 wes */
'dt'    => array( 'input'=>'dt','tag'=>'mi', 'output'=>'{:d t:}', 'definition'=>TRUE), /* 2005-06-11 wes */
'oint'  => array( 'input'=>'oint','tag'=>'mo', 'output'=>'&#' . hexdec('222E') . ';'),
'del'   => array( 'input'=>'del','tag'=>'mo', 'output'=>'&#' . hexdec('2202') . ';'),
'grad'  => array( 'input'=>'grad','tag'=>'mo', 'output'=>'&#' . hexdec('2207') . ';'),
'+-'    => array( 'input'=>'+-','tag'=>'mo', 'output'=>'&#' . hexdec('00B1') . ';'),
'O/'    => array( 'input'=>'0/','tag'=>'mo', 'output'=>'&#' . hexdec('2205') . ';'),
'oo'    => array( 'input'=>'oo','tag'=>'mo', 'output'=>'&#' . hexdec('221E') . ';'),
'aleph' => array( 'input'=>'aleph','tag'=>'mo', 'output'=>'&#' . hexdec('2135') . ';'),
'...'   => array( 'input'=>'int','tag'=>'mo', 'output'=>'...'),
'~' => array( 'input'=>'!~','tag'=>'mo', 'output'=>'&#' . hexdec('0020') . ';'),
'\\ '   => array( 'input'=>'~','tag'=>'mo', 'output'=>'&#' . hexdec('00A0') . ';'),
'quad'  => array( 'input'=>'quad','tag'=>'mo', 'output'=>'&#' . hexdec('00A0') . ';&#' . hexdec('00A0') . ';'),
'qquad' => array( 'input'=>'qquad','tag'=>'mo', 'output'=>  '&#' . hexdec('00A0') . ';&#' . hexdec('00A0') . ';&#' . hexdec('00A0') . ';'),
'cdots' => array( 'input'=>'cdots','tag'=>'mo', 'output'=>'&#' . hexdec('22EF') . ';'),
'vdots' => array( 'input'=>'vdots','tag'=>'mo', 'output'=>'&#' . hexdec('22EE') . ';'), /* 2005-06-11 wes */
'ddots' => array( 'input'=>'ddots','tag'=>'mo', 'output'=>'&#' . hexdec('22F1') . ';'), /* 2005-06-11 wes */
'diamond'   => array( 'input'=>'diamond','tag'=>'mo', 'output'=>'&#' . hexdec('22C4') . ';'),
'square'    => array( 'input'=>'square','tag'=>'mo', 'output'=>'&#' . hexdec('25A1') . ';'),
'|_'    => array( 'input'=>'|_','tag'=>'mo', 'output'=>'&#' . hexdec('230A') . ';'),
'_|'    => array( 'input'=>'_|','tag'=>'mo', 'output'=>'&#' . hexdec('230B') . ';'),
'|~'    => array( 'input'=>'|~','tag'=>'mo', 'output'=>'&#' . hexdec('2308') . ';'),
'~|'    => array( 'input'=>'~|','tag'=>'mo', 'output'=>'&#' . hexdec('2309') . ';'),
'CC'    => array( 'input'=>'CC','tag'=>'mo', 'output'=>'&#' . hexdec('2102') . ';'),
'NN'    => array( 'input'=>'NN','tag'=>'mo', 'output'=>'&#' . hexdec('2115') . ';'),
'QQ'    => array( 'input'=>'QQ','tag'=>'mo', 'output'=>'&#' . hexdec('211A') . ';'),
'RR'    => array( 'input'=>'RR','tag'=>'mo', 'output'=>'&#' . hexdec('211D') . ';'),
'ZZ'    => array( 'input'=>'ZZ','tag'=>'mo', 'output'=>'&#' . hexdec('2124') . ';'),

// Standard functions
'lim'   => array( 'input'=>'lim','tag'=>'mo', 'output'=>'lim', 'underover'=>TRUE),
'Lim'   => array( 'input'=>'Lim','tag'=>'mo', 'output'=>'Lim', 'underover'=>TRUE), /* 2005-06-11 wes */
'sin'   => array( 'input'=>'sin','tag'=>'mo', 'output'=>'sin', 'unary'=>TRUE, 'func'=>TRUE),
'cos'   => array( 'input'=>'cos', 'tag'=>'mo', 'output'=>'cos', 'unary'=>TRUE, 'func'=>TRUE),
'tan'   => array( 'input'=>'tan', 'tag'=>'mo', 'output'=>'tan', 'unary'=>TRUE, 'func'=>TRUE),
'arcsin'    => array( 'input'=>'arcsin','tag'=>'mo', 'output'=>'arcsin', 'unary'=>TRUE, 'func'=>TRUE), //2006-9-7 DL
'arccos'    => array( 'input'=>'arccos', 'tag'=>'mo', 'output'=>'arccos', 'unary'=>TRUE, 'func'=>TRUE), //2006-9-7 DL
'arctan'    => array( 'input'=>'arctan', 'tag'=>'mo', 'output'=>'arctan', 'unary'=>TRUE, 'func'=>TRUE), //2006-9-7 DL
'sinh'  => array( 'input'=>'sinh','tag'=>'mo', 'output'=>'sinh', 'unary'=>TRUE, 'func'=>TRUE),
'cosh'  => array( 'input'=>'cosh', 'tag'=>'mo', 'output'=>'cosh', 'unary'=>TRUE, 'func'=>TRUE),
'tanh'  => array( 'input'=>'tanh', 'tag'=>'mo', 'output'=>'tanh', 'unary'=>TRUE, 'func'=>TRUE),
'cot'   => array( 'input'=>'cot','tag'=>'mo', 'output'=>'cot', 'unary'=>TRUE, 'func'=>TRUE),
'sec'   => array( 'input'=>'sec', 'tag'=>'mo', 'output'=>'sec', 'unary'=>TRUE, 'func'=>TRUE),
'csc'   => array( 'input'=>'csc', 'tag'=>'mo', 'output'=>'csc', 'unary'=>TRUE, 'func'=>TRUE),
'coth'  => array( 'input'=>'coth','tag'=>'mo', 'output'=>'coth', 'unary'=>TRUE, 'func'=>TRUE),
'sech'  => array( 'input'=>'sech', 'tag'=>'mo', 'output'=>'sech', 'unary'=>TRUE, 'func'=>TRUE),
'csch'  => array( 'input'=>'csch', 'tag'=>'mo', 'output'=>'csch', 'unary'=>TRUE, 'func'=>TRUE),
'log'   => array( 'input'=>'log', 'tag'=>'mo', 'output'=>'log', 'unary'=>TRUE, 'func'=>TRUE),
'ln'    => array( 'input'=>'ln', 'tag'=>'mo', 'output'=>'ln', 'unary'=>TRUE, 'func'=>TRUE),
'det'   => array( 'input'=>'det', 'tag'=>'mo', 'output'=>'det', 'unary'=>TRUE, 'func'=>TRUE),
'dim'   => array( 'input'=>'dim', 'tag'=>'mo', 'output'=>'dim'),
'mod'   => array( 'input'=>'mod', 'tag'=>'mo', 'output'=>'mod'),
'gcd'   => array( 'input'=>'gcd', 'tag'=>'mo', 'output'=>'gcd', 'unary'=>TRUE, 'func'=>TRUE),
'lcm'   => array( 'input'=>'lcm', 'tag'=>'mo', 'output'=>'lcm', 'unary'=>TRUE, 'func'=>TRUE),
'lub'   => array( 'input'=>'lub', 'tag'=>'mo', 'output'=>'lub'), /* 2005-06-11 wes */
'glb'   => array( 'input'=>'glb', 'tag'=>'mo', 'output'=>'glb'), /* 2005-06-11 wes */
'min'   => array( 'input'=>'min', 'tag'=>'mo', 'output'=>'min', 'underover'=>TRUE), /* 2005-06-11 wes */
'max'   => array( 'input'=>'max', 'tag'=>'mo', 'output'=>'max', 'underover'=>TRUE), /* 2005-06-11 wes */
'f' => array( 'input'=>'f','tag'=>'mi', 'output'=>'f', 'unary'=>TRUE, 'func'=>TRUE), //2006-9-7 DL
'g' => array( 'input'=>'g', 'tag'=>'mi', 'output'=>'g', 'unary'=>TRUE, 'func'=>TRUE), //2006-9-7 DL

// Arrows
'uarr'  => array( 'input'=>'uarr', 'tag'=>'mo', 'output'=>'&#' . hexdec('2191') . ';'),
'darr'  => array( 'input'=>'darr', 'tag'=>'mo', 'output'=>'&#' . hexdec('2193') . ';'),
'rarr'  => array( 'input'=>'rarr', 'tag'=>'mo', 'output'=>'&#' . hexdec('2192') . ';'),
'->'    => array( 'input'=>'->', 'tag'=>'mo', 'output'=>'&#' . hexdec('2192') . ';'),
'|->'   => array( 'input'=>'|->', 'tag'=>'mo', 'output'=>'&#' . hexdec('21A6') . ';'), /* 2005-06-11 wes */
'larr'  => array( 'input'=>'larr', 'tag'=>'mo', 'output'=>'&#' . hexdec('2190') . ';'),
'harr'  => array( 'input'=>'harr', 'tag'=>'mo', 'output'=>'&#' . hexdec('2194') . ';'),
'rArr'  => array( 'input'=>'rArr', 'tag'=>'mo', 'output'=>'&#' . hexdec('21D2') . ';'),
'lArr'  => array( 'input'=>'lArr', 'tag'=>'mo', 'output'=>'&#' . hexdec('21D0') . ';'),
'hArr'  => array( 'input'=>'hArr', 'tag'=>'mo', 'output'=>'&#' . hexdec('21D4') . ';'),

// Commands with argument
'sqrt'  => array( 'input'=>'sqrt', 'tag'=>'msqrt', 'output'=>'sqrt', 'unary'=>TRUE ),
'root'  => array( 'input'=>'root', 'tag'=>'mroot', 'output'=>'root', 'binary'=>TRUE ),
'frac'  => array( 'input'=>'frac', 'tag'=>'mfrac', 'output'=>'/', 'binary'=>TRUE),
'/'     => array( 'input'=>'/', 'tag'=>'mfrac', 'output'=>'/', 'infix'=>TRUE),
'_'     => array( 'input'=>'_', 'tag'=>'msub', 'output'=>'_', 'infix'=>TRUE),
'^'     => array( 'input'=>'^', 'tag'=>'msup', 'output'=>'^', 'infix'=>TRUE),
'hat'   => array( 'input'=>'hat', 'tag'=>'mover', 'output'=>'&#' . hexdec('005E') . ';', 'unary'=>TRUE, 'acc'=>TRUE),
'bar'   => array( 'input'=>'bar', 'tag'=>'mover', 'output'=>'&#' . hexdec('00AF') . ';', 'unary'=>TRUE, 'acc'=>TRUE),
'vec'   => array( 'input'=>'vec', 'tag'=>'mover', 'output'=>'&#' . hexdec('2192') . ';', 'unary'=>TRUE, 'acc'=>TRUE),
'dot'   => array( 'input'=>'dot', 'tag'=>'mover', 'output'=>'.', 'unary'=>TRUE, 'acc'=>TRUE),
'ddot'  => array( 'input'=>'ddot', 'tag'=>'mover', 'output'=>'..', 'unary'=>TRUE, 'acc'=>TRUE),
'ul'    => array( 'input'=>'ul', 'tag'=>'munder', 'output'=>'&#' . hexdec('0332') . ';', 'unary'=>TRUE, 'acc'=>TRUE),
'avec'  => array( 'input'=>'avec', 'tag'=>'munder', 'output'=>'~', 'unary'=>TRUE, 'acc'=>TRUE),
'text'  => array( 'input'=>'text', 'tag'=>'mtext', 'output'=>'text', 'unary'=>TRUE),
'mbox'  => array( 'input'=>'mbox', 'tag'=>'mtext', 'output'=>'mbox', 'unary'=>TRUE),
'"' => array( 'input'=>'"', 'tag'=>'mtext','output'=>'mbox', 'unary'=>TRUE),

/* 2005-06-05 wes: added stackrel */
'stackrel' => array( 'input'=>'stackrel', 'tag'=>'mover', 'output'=>'stackrel', 'binary'=>TRUE),

// Grouping brackets
'('     => array( 'input'=>'(', 'tag'=>'mo', 'output'=>'(', 'left_bracket'=>TRUE),
')'     => array( 'input'=>')', 'tag'=>'mo', 'output'=>')', 'right_bracket'=>TRUE),
'['     => array( 'input'=>'[', 'tag'=>'mo', 'output'=>'[', 'left_bracket'=>TRUE),
']'     => array( 'input'=>']', 'tag'=>'mo', 'output'=>']', 'right_bracket'=>TRUE),
'{'     => array( 'input'=>'{', 'tag'=>'mo', 'output'=>'{', 'left_bracket'=>TRUE),
'}'     => array( 'input'=>'}', 'tag'=>'mo', 'output'=>'}', 'right_bracket'=>TRUE),
'(:'    => array( 'input'=>'(:', 'tag'=>'mo', 'output'=>'&#' . hexdec('2329') . ';', 'left_bracket'=>TRUE),
':)'    => array( 'input'=>':)', 'tag'=>'mo', 'output'=>'&#' . hexdec('232A') . ';', 'right_bracket'=>TRUE),
'{:'    => array( 'input'=>'{:', 'tag'=>'mo', 'output'=>'{:', 'left_bracket'=>TRUE, 'invisible'=>TRUE),
':}'    => array( 'input'=>':}', 'tag'=>'mo', 'output'=>':}', 'right_bracket'=>TRUE ,'invisible'=>TRUE),
'<<'    => array( 'input'=>'<<', 'tag'=>'mo', 'output'=>'&#' . hexdec('2329') . ';', 'left_bracket'=>TRUE), // 2005-06-07 wes
'>>'    => array( 'input'=>'>>', 'tag'=>'mo', 'output'=>'&#' . hexdec('232A') . ';', 'right_bracket'=>TRUE) // 2005-06-07 wes
);

?>