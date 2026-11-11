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
    imports=[];functions={}
    import_rva,import_size=struct.unpack_from('<II',data,directory+8)
    if import_rva:
        d=pos(import_rva)
        while any(data[d:d+20]):
            dll=cstring(pos(struct.unpack_from('<I',data,d+12)[0]));imports.append(dll)
            thunk=pos(struct.unpack_from('<I',data,d)[0]or struct.unpack_from('<I',data,d+16)[0]);names=[]
            width=8 if magic==0x20b else 4
            while True:
                value=int.from_bytes(data[thunk:thunk+width],'little')
                if not value:break
                names.append('#'+str(value&65535)if value>>(width*8-1)else cstring(pos(value)+2));thunk+=width
            functions[dll]=names
            d+=20
    return dict(path=str(p),bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),architecture=hex(machine),exports=exports,imported_dlls=imports,imported_functions=functions,os_version=list(struct.unpack_from('<HH',data,opt+40)),subsystem_version=list(struct.unpack_from('<HH',data,opt+48)))

def resources(p):
    """Return raw PE resources without loading or executing the plug-in."""
    data=p.read_bytes();pe=struct.unpack_from('<I',data,0x3c)[0];opt=pe+24
    assert struct.unpack_from('<H',data,opt)[0]==0x20b,'x64 PE expected'
    count=struct.unpack_from('<H',data,pe+6)[0];size=struct.unpack_from('<H',data,pe+20)[0]
    sections=[struct.unpack_from('<IIII',data,opt+size+i*40+8)for i in range(count)]
    def pos(rva):
        for vsize,va,rsize,offset in sections:
            if va<=rva<va+max(vsize,rsize):return offset+rva-va
        raise ValueError('resource RVA outside sections')
    base=pos(struct.unpack_from('<I',data,opt+112+16)[0]);result={}
    def visit(relative,path):
        at=base+relative;named,number=struct.unpack_from('<HH',data,at+12)
        for i in range(named+number):
            name,entry=struct.unpack_from('<II',data,at+16+i*8)
            if name>>31:
                s=base+(name&0x7fffffff);length=struct.unpack_from('<H',data,s)[0];name=data[s+2:s+2+length*2].decode('utf-16-le')
            if entry>>31:visit(entry&0x7fffffff,path+(name,))
            else:
                rva,length=struct.unpack_from('<II',data,base+entry);offset=pos(rva);result[path+(name,)]=data[offset:offset+length]
    visit(0,());return result
