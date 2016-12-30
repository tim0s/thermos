"""
Embedded Python Blocks:

Each time this file is saved, GRC will instantiate the first class it finds
to get ports and parameters of your block. The arguments to __init__  will
be the parameters. All of them are required to have default values!
"""

import numpy as np
from gnuradio import gr


class blk(gr.basic_block):  # other base classes are basic_block, decim_block, interp_block

    def __init__(self):
        self.value = 0;
        self.length = 0;  
        """arguments to this function show up as parameters in GRC"""
        gr.basic_block.__init__(
            self,
            name='Run Length Encoder',
            in_sig=[np.int32],
            out_sig=[np.int32,np.int32]
        )

    def general_work(self, input_items, output_items):

        outidx = 0;
        inidx = 0;
	while outidx < len(output_items[0]) and inidx < len(input_items[0]):
            if (self.length == 0):
                self.value = input_items[0][inidx];
                self.length = 1;
            if input_items[0][inidx] != self.value:
                output_items[0][outidx] = self.value;
                output_items[1][outidx] = self.length;
                #print "value: " + str(self.value) + " length: " + str(self.length); 
                self.length = 1;
                self.value = input_items[0][inidx];
                outidx += 1;
            else:
                self.length += 1;
            inidx += 1;

	self.consume(0, inidx);
        return outidx;
