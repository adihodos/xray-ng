import gdb
import gdb.printing
import traceback

# ideas from:
# https://github.com/ruediger/Boost-Pretty-Printer/tree/master
# https://wolf.nereid.pl/posts/simd-debugging/

class PrinterControl(gdb.printing.PrettyPrinter):
    def __init__(self, type_name, printer):
        super().__init__(type_name)
        self.printer = printer

    def __call__(self, val):
        return self.printer(val) if val.type.name == self.name else None

def type_printer(type_name):
    # decorator
    def _register_printer(printer):
        gdb.printing.register_pretty_printer(None, PrinterControl(type_name, printer))

    return _register_printer

# @type_printer('__m128')

class M128Printer(gdb.ValuePrinter):
    """ __m128 vector type printer """
    def __init__(self, value):
        self.m128 = value

    def to_string_impl(self):
        s = '__m128 vector {'

        s += f'\nF32x4 = ([0] = {self.m128[0]}, [1] = {self.m128[1]}, [2] = {self.m128[2]}, [3] = {self.m128[3]})'

        uint32_t = gdb.lookup_type('uint32_t')
        u32vals = [gdb.Value(self.m128[x].bytes, uint32_t) for x in range(4)]
        s += f'\nU32x4 = ([0] = {u32vals[0]}, [1] = {u32vals[1]}, [2] = {u32vals[2]}, [3] = {u32vals[3]})'

        int32_t = gdb.lookup_type('int32_t')
        i32vals = [gdb.Value(self.m128[x].bytes, int32_t) for x in range(4)]
        s += f'\nI32x4 = ([0] = {i32vals[0]}, [1] = {i32vals[1]}, [2] = {i32vals[2]}, [3] = {i32vals[3]})'

        uint16_t = gdb.lookup_type('uint16_t')
        u16vals = []
        for i in range(4):
            u32 = gdb.Value(self.m128[i].bytes, uint32_t)
            u16vals.append((u32 & 0x0000FFFF).cast(uint16_t))
            u16vals.append((u32 >> 16 & 0x0000FFFF).cast(uint16_t));

        s += f'\nU16x8 = ([0] = {u16vals[0]}, [1] = {u16vals[1]}, [2] = {u16vals[2]}, [3] = {u16vals[3]},\n\t[4] = {u16vals[4]}, [5] = {u16vals[5]}, [6] = {u16vals[6]}, [7] = {u16vals[7]})'

        int16_t = gdb.lookup_type('int16_t')
        i16vals = []
        for i in range(4):
            i32 = gdb.Value(self.m128[i].bytes, int32_t)
            i16vals.append((i32 & 0x0000FFFF).cast(int16_t))
            i16vals.append((i32 >> 16 & 0x0000FFFF).cast(int16_t))

        s += f'\nI16x8 = ([0] = {i16vals[0]}, [1] = {i16vals[1]}, [2] = {i16vals[2]}, [3] = {i16vals[3]},\n\t[4] = {i16vals[4]}, [5] = {i16vals[5]}, [6] = {i16vals[6]}, [7] = {i16vals[7]})'

        uint8_t = gdb.lookup_type('uint8_t')

        def extract_byte(m128, ty0, ty, i, fmt):
            u32 = gdb.Value(m128[i].bytes, ty0)
            for j in range(4):
                yield f'\t[{i * 4 + j}] = {((u32 >> (j * 8)) & 0xFF).cast(ty).format_string(format=fmt)},\n'
                
        s += '\nU8x16 = (\n'
        for i in range(4):
            s += '\n'.join(extract_byte(self.m128, uint32_t, uint8_t, i, 'u'))
        s += '\n}'

        int8_t = gdb.lookup_type('signed char')
        s += '\nI8x16 = (\n'
        for i in range(4):
            s += '\n'.join(extract_byte(self.m128, int32_t, int8_t, i, 'd'))
        s += '\n}'
        
        return s
        
    def to_string(self):
        try:
            return self.to_string_impl()
        except Exception:
            print(traceback.format_exc())
            return "<<error>>"

    def display_hint(self):
        return 'array'

class M128IPrinter(gdb.ValuePrinter):
    def __init__(self, value):
        self.low = value[0]
        self.high = value[1]

    def to_string_impl(self):
        s = '__m128i vector = {\n'

        values = [self.low, self.high]

        def fmt_x_components(short_name, ty_name, bits, fmt):
            components = int(128 / bits)
            ty = gdb.lookup_type(ty_name)
            s = f'\t.{short_name}x{components} = {{ '
            
            # sp = '\n\t' if components > 8 else ''
            and_val = (1 << bits) - 1

            elements = int(components / 2)
            for i in range(2):
                s += ''.join(f' [{i * elements + j}] = {gdb.Value((values[i] >> (j * bits) & and_val).bytes, ty).format_string(format=fmt)}, ' for j in range(elements))

            s +=' }\n'
            return s

        types = [ 
            ('U8', 'unsigned char', 8, 'u'),
            ('I8', 'char', 8, 'd'),
            ('U16', 'uint16_t', 16, 'u'),
            ('I16', 'int16_t', 16, 'd'),
            ('U32', 'unsigned long', 32, 'u'),
            ('I32', 'int32_t', 32, 'd'),
            ('U64', 'unsigned long long', 64, 'u'),
            ('I64', 'long long', 64, 'd')
        ]

        s += ''.join(fmt_x_components(*a) for a in types) + '}'
        return s
    
    def to_string(self):
        try:
            return self.to_string_impl()
        except Exception:
            print(traceback.format_exc())
            return "<<error>>"

    def display_hint(self):
        return 'array'

def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("footmade.hero")
    pp.add_printer('m128', '^__m128$', M128Printer)
    pp.add_printer('m128i', '^__m128i$', M128IPrinter)
    return pp
