Note: Since doxygen may employ a different markdown parser from other markdown editors, you'd better follow the following syntax or the descriptions on doxygen online guide:https://www.stack.nl/~dimitri/doxygen/manual/markdown.html. And after you change the .md files, you should check whether the format is ok for you by running the gendoc.sh script.

Format Supported:
==================
1. Document title:

++++++++++++++++++EXAMPLE+++++++++++++++++
<TITLE_CONTENT>
----------------------
++++++++++++++++++EXAMPLE+++++++++++++++++

2. Header under different levels:

++++++++++++++++++EXAMPLE+++++++++++++++++
# 1 HEADER1 {#Header1}
## 1.1 HEADER1.1 {#Header1_1}
++++++++++++++++++EXAMPLE+++++++++++++++++

The number of '#' indicates the header level, and the section label in the brace is important to enable a hyper link to this section and generate TOC(table of content) automaticlly in index.

And the section label should not duplicated, or it may fail to enable hyper link or TOC.

3. Paragraphs

You should add an extra empty line to indicate a new paragraph(i.e. two CRLFs) or it will concat your contents regardless of your CRLF.

4. Emphasis

++++++++++++++++++EXAMPLE+++++++++++++++++
**double asterisks**
__double underscores__
`backticks`
++++++++++++++++++EXAMPLE+++++++++++++++++

5. Block quotes
(a space is needed after >)

++++++++++++++++++EXAMPLE+++++++++++++++++
> This is a block quote
> spanning multiple lines
++++++++++++++++++EXAMPLE+++++++++++++++++

For us, we usually use this in notes as follows:

++++++++++++++++++EXAMPLE+++++++++++++++++
> **Note**: NOTE_CONTENT...
++++++++++++++++++EXAMPLE+++++++++++++++++

6. Lists

Simple bullet lists can be made by starting a line with -, +, or *.
List items can span multiple paragraphs (if each paragraph starts with the proper indentation) and lists can be nested.

++++++++++++++++++EXAMPLE+++++++++++++++++
- Item 1

  More text for this item.

- Item 2
  + nested list item.
  + another nested item.
- Item 3

1. First item.
2. Second item.
++++++++++++++++++EXAMPLE+++++++++++++++++

Make sure to check the output html if you use a nested list!

7. Table

Simple format
++++++++++++++++++EXAMPLE+++++++++++++++++
**Table 1-1. Table caption**
| HEADER1 |HEADER2 |
|--------|--------|
| ROW1COL1 | ROW1COL2 |
| ROW2COL1 | ROW2COL2 |
| ROW3COL1 | ROW3COL2 |
++++++++++++++++++EXAMPLE+++++++++++++++++

html table: when you need complex table you can define a html table in @htmlonly block and class
++++++++++++++++++EXAMPLE+++++++++++++++++
@htmlonly
<table class="doxtable">
<caption><b>Table 2-2. Test environment</b></caption>
<tbody>
<tr>
<th>HEADER1</th>
<th>HEADER2</th>
</tr>
<tr>
<td>ROW1COL1</td>
<td>ROW1COL2</td>
</tr>
<tr>
<td>ROW2COL1</td>
<td>ROW2COL2</td>
</tr>
</tbody>
</table>
@endhtmlonly

8. Link
++++++++++++++++++EXAMPLE+++++++++++++++++
[The link text](#labelid)
[Link text](@ref labelid)
[The link text](/relative/path/to/index.html "Link title")
[The link text](http://example.net/)
[The link text](@ref MyClass)
++++++++++++++++++EXAMPLE+++++++++++++++++

image link: if you want to define the width or other properties, define in the html way.
++++++++++++++++++EXAMPLE+++++++++++++++++
![Figurecaption](/relative/path/to/graph.png)
<img src="/relative/path/to/graph.png" alt="Figurecaption" style="width: 600px;">
++++++++++++++++++EXAMPLE+++++++++++++++++

9. Code block
++++++++++++++++++EXAMPLE+++++++++++++++++
~~~~~~~~~~~~~~~{.c}
int func(int a,int b) { return a*b; }
~~~~~~~~~~~~~~~

```
also a fenced code block
```
Preformatted verbatim blocks can be created by indenting each line in a block of text by at least 4 extra spaces
++++++++++++++++++EXAMPLE+++++++++++++++++
This a normal paragraph

    This is a code block

We continue with a normal paragraph again.
++++++++++++++++++EXAMPLE+++++++++++++++++
Note that you cannot start a code block in the middle of a paragraph (i.e. the line preceding the code block must be empty).
