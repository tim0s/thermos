"""
Embedded Python Blocks:

Each time this file is saved, GRC will instantiate the first class it finds
to get ports and parameters of your block. The arguments to __init__  will
be the parameters. All of them are required to have default values!
"""

import numpy as np
from gnuradio import gr


class blk(gr.basic_block):  # other base classes are basic_block, decim_block, interp_block
    """Embedded Python Block example - a simple multiply const"""

    def __init__(self, max_len=10):  # only default arguments here
        self.value = 0;
        self.length = 0;
        self.maxlen = max_len;
        """arguments to this function show up as parameters in GRC"""
        gr.basic_block.__init__(
            self,
            name='Run Length Decoder',
            in_sig=[np.int32, np.int32],
            out_sig=[np.int32]
        )

    def general_work(self, input_items, output_items):

        outidx = 0;
        inidx = 0;

        while inidx < len(input_items[0]) and outidx < len(output_items[0]):
            if self.length == 0:
                self.value = input_items[0][inidx];
                self.length = input_items[1][inidx];
                if self.length > self.maxlen:
                    self.length = 0; 
                inidx += 1;
            
            while (outidx < len(output_items[0])) and (self.length > 0):
                output_items[0][outidx] = self.value;
                self.length -= 1;
                outidx += 1;

	self.consume(0, inidx);
        self.consume(1, inidx);
 
        return outidx;


