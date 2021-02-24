int round_to_multiple_of_16(int n) {
    if ( (n & 15) == 0 ) {
        return n;
    } else {
        return ((n>>4) + 1) << 4; // (i/16+1)*16
    }
}
