This is a simple test tool for TCP co-current connections performance test.

cctest < -n numConn(max=40960)> <-x speed [1,256] > <-i remoteIP> <-p remotePort>

"speed" actually means the number of 1KB block which TCP socket will send at once.

