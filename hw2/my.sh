#!/bin/sh
diff test_in test_sigalrm
if [ $? -ne 0 ];then
		echo $?
    echo "${RED}***FAIL***${NC}"
else
#FILESIZE=$(stat -c%s "./select.pcap")
#SIZE=$(numfmt --to=iec-i --suffix=B --format="%.5f" $FILESIZE) 
#echo "Size of select.pcap = $SIZE."
    echo "${GREEN}***PASS***${NC}"
fi

diff test_in test_select
if [ $? -ne 0 ];then
    echo "${RED}***FAIL***${NC}"
else
#FILESIZE=$(stat -c%s "./select.pcap")
#SIZE=$(numfmt --to=iec-i --suffix=B --format="%.5f" $FILESIZE) 
#echo "Size of select.pcap = $SIZE."
    echo "${GREEN}***PASS***${NC}"
fi

diff test_in test_sockopt
if [ $? -ne 0 ];then
    echo "${RED}***FAIL***${NC}"
else
#FILESIZE=$(stat -c%s "./sockopt.pcap")
#SIZE=$(numfmt --to=iec-i --suffix=B --format="%.5f" $FILESIZE) 
#echo "Size of sockopt.pcap = $SIZE ."
    echo "${GREEN}***PASS***${NC}"
fi