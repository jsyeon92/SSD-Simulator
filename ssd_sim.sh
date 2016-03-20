#!/bin/bash
ssd_size=8
page_size=4
block_size=64
free_block_limit=30
gc_size=20
path="$1"
trace_size=`cat $path | wc -l`
if [ "$#" -ne 1 ]; then
	echo "usage:$0 [-i 4] [-p 4] [-b 64] [-o 30] [-t 10]"
fi

while [ -n "$2" ]; do
	case $2 in
		-i)shift 1; ssd_size="$2"; shift 1;;
		-p)shift 1; page_size="$2"; shift 1;;
		-b)shift 1; block_size="$2"; shift 1;;
		-o)shift 1; free_block_limit="$2"; shift 1;;
		-t)shift 1; gc_size="$2"; shift 1;;
		*)echo "Unrecognized value"
	esac	
done

foot_print=`cat $path | sort | uniq | wc -l`
temp=$(($ssd_size * 1024 * 1024 / $page_size))

if [ "$foot_print" -gt "$temp" ]; then
	echo "footprint over!"
	exit 1
fi
error=$(($ssd_size * 1024 * 1024 / $page_size * (100-30)/100))

#if [ "$trace_size" -gt "$error" ]; then
#	echo "valid page not enough!"
#	exit 1
#fi

f_size=$(($foot_print * 4 / 1024 / 1024))

echo "SSD size          : $ssd_size G" 
echo "foot print        : $f_size K"
echo "Page size         : $page_size KB"
echo "Block size        : $block_size Pages"
echo "dirty block limit : $free_block_limit %"
echo "Garbage Size      : $gc_size %"

gcc -g -o ssd_sim ssd_sim.c -Db_in_page=$block_size

./ssd_sim $path $ssd_size $page_size $free_block_limit $gc_size > $path.result 

echo "#!/usr/bin/gnuplot"       >  plot.config
echo "set terminal wxt persist" >> plot.config
echo "set xlabel 'time'"        >> plot.config
echo "set ylabel 'block num'"   >> plot.config
echo "set title 'SSD Simulator'">> plot.config
echo "plot '$path.result' lt rgb 'black' title '' with l" >> plot.config

#gnuplot plot.config
