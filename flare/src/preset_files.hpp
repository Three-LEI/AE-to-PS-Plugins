#pragma once
#include "flare_design.hpp"
#include <windows.h>
#include <commdlg.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
namespace flare::files {
inline std::filesystem::path test_directory;
inline std::filesystem::path directory(){
 if(!test_directory.empty())return test_directory;wchar_t path[32768]{};const auto n=GetEnvironmentVariableW(L"APPDATA",path,32768);return n&&n<32768?std::filesystem::path(path)/L"AE-to-PS-Plugins"/L"LensFlare"/L"Presets":std::filesystem::path{};
}
inline std::filesystem::path path_from_utf8(const std::string& s){
 const int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),int(s.size()),nullptr,0);if(n<=0)return {};std::wstring wide(n,L'\0');MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),int(s.size()),wide.data(),n);return wide;
}
inline std::string utf8_name(const std::filesystem::path& p){const auto s=p.stem().wstring();const int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,s.data(),int(s.size()),nullptr,0,nullptr,nullptr);if(n<=0)return {};std::string text(n,'\0');WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,s.data(),int(s.size()),text.data(),n,nullptr,nullptr);return text;}
inline bool read(const std::filesystem::path& path,FlarePreset& out){try{const auto size=std::filesystem::file_size(path);if(size>max_design_bytes+256)return false;std::vector<std::uint8_t> bytes(size);std::ifstream f(path,std::ios::binary);if(!f.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(size)))return false;return decode_preset(bytes,out);}catch(...){return false;}}
inline bool write(const std::filesystem::path& path,const FlarePreset& preset,bool replace=false){
 const auto bytes=encode_preset(preset);if(path.empty()||bytes.empty())return false;std::filesystem::path temp;
 try{if(!replace&&std::filesystem::exists(path))return false;std::filesystem::create_directories(path.parent_path());static unsigned counter=0;
  do{temp=path;temp+=L".aeps-tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(++counter);}while(std::filesystem::exists(temp));
  {std::ofstream f(temp,std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));f.close();if(!f){std::filesystem::remove(temp);return false;}}
  if(!MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH|(replace?MOVEFILE_REPLACE_EXISTING:0))){std::filesystem::remove(temp);return false;}return true;
 }catch(...){if(!temp.empty()){std::error_code error;std::filesystem::remove(temp,error);}return false;}
}
inline std::vector<std::filesystem::path> list(){std::vector<std::filesystem::path> result;try{const auto folder=directory();if(!folder.empty()&&std::filesystem::exists(folder))for(const auto& e:std::filesystem::directory_iterator(folder))if(e.is_regular_file()&&e.path().extension()==L".aepsflare")result.push_back(e.path());std::sort(result.begin(),result.end());}catch(...){}return result;}
inline std::filesystem::path save_new(const std::string& name,const FlarePreset& preset){
 try{
 const auto folder=directory();auto basename=path_from_utf8(name).filename();if(folder.empty()||basename.empty()||basename==L"."||basename==L"..")return {};
 auto text=basename.wstring();for(auto& c:text)if(c<32||std::wstring(L"<>:\"/\\|?*").find(c)!=std::wstring::npos)c=L'_';while(!text.empty()&&(text.back()==L'.'||text.back()==L' '))text.pop_back();if(text.empty())return {};
 for(int i=0;i<1000;i++){auto file=folder/(text+(i?L" ("+std::to_wstring(i)+L")":L"")+L".aepsflare");if(!std::filesystem::exists(file))return write(file,preset)?file:std::filesystem::path{};}return {};
 }catch(...){return {};}
}
inline std::filesystem::path choose(HWND owner,bool save){
 wchar_t file[32768]{};OPENFILENAMEW info{};info.lStructSize=sizeof(info);info.hwndOwner=owner;info.lpstrFile=file;info.nMaxFile=32768;info.lpstrFilter=L"AEPS Flare preset (*.aepsflare)\0*.aepsflare\0\0";info.lpstrDefExt=L"aepsflare";info.Flags=OFN_EXPLORER|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
 return (save?GetSaveFileNameW(&info):GetOpenFileNameW(&info))?std::filesystem::path(file):std::filesystem::path{};
}
}
