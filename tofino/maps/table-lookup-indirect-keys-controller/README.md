# Compile

```
$ g++ -std=c++11 -DSDE_INSTALL=$SDE_INSTALL -I$SDE_INSTALL/include -I$SDE_INSTALL/include/tofino/pdfixed tofino.cpp -o tofino -L$SDE_INSTALL/lib -LSDE_INSTALL/lib/tofinopd/tofino -Wl,--start-group -ldriver -lbfsys -Wl,--end-group -lm -pthread -lpcap -ldl -levent -Wl,-rpath,$SDE_INSTALL/lib
```

# Conclusions

There is something strange going on here. The exact matches on key bytes don't seem to work.

Table matching rule:

```
key = {
	key_byte_0: exact;
	key_byte_1: exact;
}
```

Key byte filling:

```
key_byte_0  = hdr.ipv4.src_addr[7:0];
key_byte_1  = hdr.ipv4.src_addr[15:8];
```

Model report:

```
:03-09 11:24:05.262721:    :0x2:-:<0,0,->:------------ Stage 0 ------------                   
:03-09 11:24:05.263083:    :0x2:-:<0,0,0>:Ingress : Table Ingress.map is miss                 
:03-09 11:24:05.263113:        :0x2:-:<0,0,0>:Key:                                            
:03-09 11:24:05.263156:        :0x2:-:<0,0,0>:  hdr.ipv4.src_addr[15:0] = 0x5277              
:03-09 11:24:05.263177:        :0x2:-:<0,0,0>:  hdr.ipv4.src_addr[31:16] = 0x0                
:03-09 11:24:05.263194:        :0x2:-:<0,0,0>:  hdr.ipv4.src_addr[15:0] = 0x5277              
:03-09 11:24:05.263212:        :0x2:-:<0,0,0>:  hdr.ipv4.src_addr[31:16] = 0x0
:03-09 11:24:05.263236:        :0x2:-:<0,0,0>:Execute Default Action: NoAction                
:03-09 11:24:05.263260:        :0x2:-:<0,0,0>:Action Results:                                 
:03-09 11:24:05.263280:        :0x2:-:<0,0,0>:Next Table = tbl_send_to_cpu
```