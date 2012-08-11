@echo off
rem Copyright Beman Dawes 2012
rem Distributed under the Boost Software License, Version 1.0.

echo Generating LWG filesystem-proposal.html...
del filesystem-proposal.html 2>nul

rem ' dir="ltr"' messes up the section number generation
chg boost_snippets.html " dir=\qltr\q" ""
chg tr2_snippets.html " dir=\qltr\q" ""
chg source.html " dir=\qltr\q" ""

mmp TARGET=TR2 source.html temp.html
html_section_numbers <temp.html >filesystem-proposal.html
toc -x filesystem-proposal.html filesystem-proposal.toc
echo Fresh table-of-contents generated in filesystem-proposal.toc.
echo Hand edit into source files if desired.
type \bgd\util\crnl.txt

echo Generating Boost reference.html...
del reference.html 2>nul
mmp TARGET=BOOST source.html reference.html
toc -x reference.html reference.toc
echo Fresh table-of-contents generated in reference.toc.
echo Hand edit into source files if desired.
type \bgd\util\crnl.txt

echo Run "hoist" to hoist reference.html to doc directory
