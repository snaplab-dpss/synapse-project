# CloNE


## Input Format

```
# Device Declarations
# device <device-name>

device device-0
device device-1
...
device device-n

# NF Declarations
# nf <nf-name>

nf nf-0
nf nf-1
...
nf nf-n

# Link Declarations
# link <device-0> <port-0> <device-1> <port-1>

link device-0 0 nf-0
link nf-0 device-0

link nf-0 nf-1
link nf-1 nf-0
...
link nf-1 server-1
```

