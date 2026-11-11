"""Read PE architecture, exports and imports without executing the binary."""
import struct,hashlib

def inspect(p):
    data = p.read_bytes()
    pe = struct.unpack_from('<I',data,0x3c)[0]
    assert data[pe:pe+4] == b'PE\0\0'
    machine, count = struct.unpack_from('<HH',data,pe+4)
    opt_size = struct.unpack_from('<H',data,pe+20)[0]
    opt = pe + 24
    magic = struct.unpack_from('<H',data,opt)[0]
    directory = opt + (112 if magic == 0x20b else 96)
    sections = []
    for i in range(count):
        s=opt+opt_size+i*40
        vsize,va,rsize,offset=struct.unpack_from('<IIII',data,s+8)
        sections.append((va,max(vsize,rsize),offset))
    def pos(rva):
        for va,size,offset in sections:
            if va <= rva < va+size:
                return offset+rva-va
        return rva
    def cstring(offset):
        return data[offset:data.index(0,offset)].decode('ascii','replace')
    exports=[]
    export_rva,export_size=struct.unpack_from('<II',data,directory)
    if export_rva:
        e=pos(export_rva)
        n=struct.unpack_from('<I',data,e+24)[0]
        names=pos(struct.unpack_from('<I',data,e+32)[0])
        for i in range(n): exports.append(cstring(pos(struct.unpack_from('<I',data,names+i*4)[0])))
    imports=[]
    import_rva,import_size=struct.unpack_from('<II',data,directory+8)
    if import_rva:
        d=pos(import_rva)
        while any(data[d:d+20]):
            imports.append(cstring(pos(struct.unpack_from('<I',data,d+12)[0])))
            d+=20
    return dict(path=str(p),bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),architecture=hex(machine),exports=exports,imported_dlls=imports)
