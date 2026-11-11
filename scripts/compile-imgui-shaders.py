"""Regenerate pinned ImGui SM4 DXBC at development time, not in the plug-in.

The HLSL is from Dear ImGui 1.91.9b (MIT); vendor files remain unmodified.
Windows runtime needs D3D11, not a separately installed D3DCompiler DLL.
"""
from pathlib import Path
import ast,ctypes,hashlib,re
root=Path(__file__).resolve().parents[1];backend=root/'third_party/imgui/backends/imgui_impl_dx11.cpp'
text=backend.read_text(encoding='utf-8');directory=ctypes.create_unicode_buffer(32768);ctypes.windll.kernel32.GetSystemDirectoryW(directory,len(directory))
compiler=ctypes.WinDLL(str(Path(directory.value)/'D3DCompiler_47.dll'));compile=compiler.D3DCompile
compile.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_char_p,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_char_p,ctypes.c_char_p,ctypes.c_uint,ctypes.c_uint,ctypes.POINTER(ctypes.c_void_p),ctypes.POINTER(ctypes.c_void_p)];compile.restype=ctypes.c_long
def blob_data(blob):
 table=ctypes.cast(blob,ctypes.POINTER(ctypes.POINTER(ctypes.c_void_p))).contents
 get=ctypes.WINFUNCTYPE(ctypes.c_void_p,ctypes.c_void_p)(table[3]);size=ctypes.WINFUNCTYPE(ctypes.c_size_t,ctypes.c_void_p)(table[4]);release=ctypes.WINFUNCTYPE(ctypes.c_ulong,ctypes.c_void_p)(table[2])
 data=ctypes.string_at(get(blob),size(blob));release(blob);return data
def array(name,data):
 rows=[', '.join('0x'+data[i+j:i+j+1].hex()for j in range(min(16,len(data)-i)))for i in range(0,len(data),16)]
 return 'inline constexpr unsigned char '+name+'[] = {\n '+',\n '.join(rows)+'\n};\n'
out='#pragma once\n// Generated from Dear ImGui 1.91.9b HLSL; copyright Omar Cornut and contributors, MIT.\n// Regenerate with scripts/compile-imgui-shaders.py on a development machine.\nnamespace psui::dx11_shaders {\n'
for name,target in [('vertex','vs_4_0'),('pixel','ps_4_0')]:
 match=re.search(r'static const char\* '+name+r'Shader\s*=\s*(".*?");',text,re.S);assert match,name
 literal=re.sub(r'\\\r?\n','',match[1]);source=ast.literal_eval(literal).encode('ascii');code=ctypes.c_void_p();error=ctypes.c_void_p();buffer=ctypes.create_string_buffer(source)
 result=compile(buffer,len(source),None,None,None,b'main',target.encode('ascii'),0,0,ctypes.byref(code),ctypes.byref(error))
 if result<0:raise RuntimeError(blob_data(error).decode('ascii','replace')if error else'D3DCompile failed')
 if error:blob_data(error)
 data=blob_data(code);assert data.startswith(b'DXBC')
 out+='// '+name+' source SHA256 '+hashlib.sha256(source).hexdigest()+'\n'+array(name+'_source',source)+array(name+'_code',data)
out+='}\n';(root/'native/ui/include/psui/imgui_dx11_shader_blobs.hpp').write_text(out,encoding='utf-8');print('Generated pinned SM4 vertex/pixel shader blobs; no vendor file changes')
