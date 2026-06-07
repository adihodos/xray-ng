python

import gdb.printing
import sys
sys.path.insert(1, './')
import pretty_printers
# pretty_printers.type_printer('__m128')
gdb.printing.register_pretty_printer(gdb.current_objfile(), pretty_printers.build_pretty_printer())
end
