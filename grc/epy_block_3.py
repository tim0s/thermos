"""
Embedded Python Blocks:

Each time this file is saved, GRC will instantiate the first class it finds
to get ports and parameters of your block. The arguments to __init__  will
be the parameters. All of them are required to have default values!
"""

import numpy as np
from gnuradio import gr


class blk(gr.basic_block):  # other base classes are basic_block, decim_block, interp_block

    def __init__(self, packet_length=130, header=[1,0]*11):  # only default arguments here
        self.packet_len = packet_length;
        self.expected_header = header;
        self.packet_bits = [];

        gr.basic_block.__init__(
            self,
            name='Thermos Decoder',   # will show up in GRC
            in_sig=[np.int32],
            out_sig=[]
        )

    def general_work(self, input_items, output_items):
        inidx = 0;
 
        while inidx < len(input_items[0]):
            in_bit = input_items[0][inidx];
            self.packet_bits.append(in_bit);
            inidx += 1;
            if len(self.packet_bits) > self.packet_len:
                self.packet_bits.pop(0);
            if len(self.packet_bits) == self.packet_len and self.expected_header == self.packet_bits[0:len(self.expected_header)]:
                out = [0] * (self.packet_len / 2);
                outidx = 0;
                for i in range(1,len(self.packet_bits), 2):
                    if self.packet_bits[i-1] == 1 and self.packet_bits[i-0] == 0:
                        out[outidx] = 1;
                        outidx += 1;
                    elif self.packet_bits[i-1] == 0 and self.packet_bits[i-0] == 1:
                        out[outidx] = 0;
                        outidx += 1;
                    else:
                        print "Decoding error"
                print ''.join(map(str,out));
	self.consume(0, inidx);
        return 0;
