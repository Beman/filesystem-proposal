@echo off
del tr2.html 2>nul
mmp TARGET=TR2 source.html temp.html
html_section_numbers <temp.html >filesystem-proposal.html
del reference.html 2>nul
mmp TARGET=BOOST source.html reference.html
