#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3dcompiler.h>
#include "psui/imgui_dx11_shader_blobs.hpp"
#include <atomic>
#include <cstring>
#include <new>
#include <vector>
namespace {
class Blob final:public ID3DBlob {
 std::atomic<ULONG> refs{1};std::vector<unsigned char> data;
public:
 Blob(const unsigned char* first,SIZE_T size):data(first,first+size){}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)override{
  if(!out)return E_POINTER;*out=nullptr;
  if(IsEqualIID(id,__uuidof(IUnknown))||IsEqualIID(id,__uuidof(ID3DBlob))){*out=static_cast<ID3DBlob*>(this);AddRef();return S_OK;}return E_NOINTERFACE;
 }
 ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}
 ULONG STDMETHODCALLTYPE Release()override{const auto value=--refs;if(!value)delete this;return value;}
 LPVOID STDMETHODCALLTYPE GetBufferPointer()override{return data.data();}
 SIZE_T STDMETHODCALLTYPE GetBufferSize()override{return data.size();}
};
}
// Only the pinned ImGui backend is compiled with D3DCompile redirected here.
// No process hooks or system DLL replacements are used.
extern "C" HRESULT WINAPI aeps_imgui_shader_blob(LPCVOID source,SIZE_T size,LPCSTR,
 const D3D_SHADER_MACRO* defines,ID3DInclude* include,LPCSTR entry,LPCSTR target,
 UINT flags1,UINT flags2,ID3DBlob** code,ID3DBlob** errors){
 if(errors)*errors=nullptr;if(!code)return E_POINTER;*code=nullptr;
 if(!source||!entry||!target||defines||include||flags1||flags2||std::strcmp(entry,"main"))return E_INVALIDARG;
 using namespace psui::dx11_shaders;const unsigned char* bytes=nullptr;SIZE_T length=0;
 if(!std::strcmp(target,"vs_4_0")&&size==sizeof(vertex_source)&&!std::memcmp(source,vertex_source,size)){bytes=vertex_code;length=sizeof(vertex_code);}
 if(!std::strcmp(target,"ps_4_0")&&size==sizeof(pixel_source)&&!std::memcmp(source,pixel_source,size)){bytes=pixel_code;length=sizeof(pixel_code);}
 if(!bytes)return E_INVALIDARG;
 try{*code=new Blob(bytes,length);return S_OK;}catch(const std::bad_alloc&){return E_OUTOFMEMORY;}
}
