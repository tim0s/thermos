"""
Embedded Python Blocks:

Each time this file is saved, GRC will instantiate the first class it finds
to get ports and parameters of your block. The arguments to __init__  will
be the parameters. All of them are required to have default values!
"""

import sys
import numpy as np
from gnuradio import gr


class blk(gr.basic_block):  # other base classes are basic_block, decim_block, interp_block

    def __init__(self):  # only default arguments here
        """arguments to this function show up as parameters in GRC"""
        gr.basic_block.__init__(
            self,
            name='Print Numbers',   # will show up in GRC
            in_sig=[np.int32],
            out_sig=[np.int32]
        )

    def general_work(self, input_items, output_items):
	items = min( len(input_items[0]), len(output_items[0]));
        for x in range(items):
            sys.stdout.write(str(input_items[0][x]));
            output_items[0][x] = input_items[0][x];
        sys.stdout.flush();
        self.consume(0, items);
        return items;
