#!/bin/bash

pdfdetach -save 1 simplepdf_f8004a3ad0acde31c40267b9856e63fc.pdf

for(( count=10; count>=0; count=count-1 ))
do
	pdfdetach -save 1 "pdf${count}.pdf"
#	echo -e "pdf${count}.pdf has been processed !"
done
