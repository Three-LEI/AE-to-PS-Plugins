#include "psui/photoshop_host.hpp"
#include "PIProperties.h"
#include "PIUIHooksSuite.h"
#include "PIActions.h"
#include "psui/localization.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace psui {
namespace {
template<class T> struct LocaleSuite {
  SPBasicSuite* basic;const char* name;int32 version;const T* value=nullptr;
  LocaleSuite(SPBasicSuite* b,const char* n,int32 v):basic(b),name(n),version(v){
    if(basic->AcquireSuite(name,version,reinterpret_cast<const void**>(&value))!=0)value=nullptr;
  }
  ~LocaleSuite(){if(value)basic->ReleaseSuite(name,version);}
};
std::string interface_locale(FilterRecord* host) {
  auto* basic=host?host->sSPBasic:nullptr;
  if(!basic||!basic->AcquireSuite||!basic->ReleaseSuite)return {};
  // Basic Action Control is available to filters, including older Photoshop.
  // Read only interfacePrefs.uiLanguageKey: no script execution or preference
  // mutation, and no assumption about the Windows display language.
  LocaleSuite<PSBasicActionControlProcsOld> control(basic,kPSBasicActionControlSuite,kPSBasicActionControlSuitePrevVersion);
  LocaleSuite<PSActionReferenceProcsOld> reference(basic,kPSActionReferenceSuite,kPSActionReferenceSuitePrevVersion);
  LocaleSuite<PSActionDescriptorProcs2> descriptor(basic,kPSActionDescriptorSuite,kPSActionDescriptorSuiteVersion2);
  const auto* c=control.value;const auto* r=reference.value;const auto* d=descriptor.value;
  if(!c||!r||!d||!c->Get||!c->StringIDToTypeID||!r->Make||!r->Free||!r->PutProperty||!r->PutEnumerated||!d->Free||!d->GetObject||!d->GetStringLength||!d->GetString)return {};
  struct Values {
    const PSActionReferenceProcsOld* r;const PSActionDescriptorProcs2* d;
    PIActionReference reference=nullptr;PIActionDescriptor root=nullptr,prefs=nullptr;
    ~Values(){if(prefs)d->Free(prefs);if(root)d->Free(root);if(reference)r->Free(reference);}
  } values{r,d};
  DescriptorTypeID prefs=0,language=0;
  if(c->StringIDToTypeID("interfacePrefs",&prefs)||c->StringIDToTypeID("uiLanguageKey",&language)||r->Make(&values.reference)||!values.reference)return {};
  if(r->PutProperty(values.reference,0x50727072,prefs)||r->PutEnumerated(values.reference,0x63617070,0x4f72646e,0x54726774)||c->Get(&values.root,values.reference)||!values.root)return {};
  DescriptorClassID type=0;uint32 length=0;
  if(d->GetObject(values.root,prefs,&type,&values.prefs)||!values.prefs||d->GetStringLength(values.prefs,language,&length)||!length||length>128)return {};
  std::vector<char> bytes(length+1,0);
  if(d->GetString(values.prefs,language,bytes.data(),static_cast<uint32>(bytes.size())))return {};
  return decode_host_locale({bytes.data(),length});
}
std::string property_locale(FilterRecord *host) {
  if (!host || !host->propertyProcs || !host->propertyProcs->getPropertyProc ||
      !host->handleProcs)
    return {};
  const auto &procs = *host->handleProcs;
  if (!procs.getSizeProc || !procs.lockProc || !procs.unlockProc ||
      !procs.disposeProc)
    return {};
  struct PropertyHandle {
    const HandleProcs &procs;
    Handle value = nullptr;
    bool locked = false;
    ~PropertyHandle() {
      if (locked)
        procs.unlockProc(value);
      if (value)
        procs.disposeProc(value);
    }
  } handle{procs};
  const auto status = host->propertyProcs->getPropertyProc(
      0x3842494d, propLocale, 0, nullptr, &handle.value);
  if (status != noErr || !handle.value)
    return {};
  // The handle is byte-counted. Photoshop 27.7 returns UTF-16LE with a
  // terminator; also accept byte strings used by older compatible hosts.
  const auto size = procs.getSizeProc(handle.value);
  if (size <= 0 || size > 128)
    return {};
  const auto *bytes = procs.lockProc(handle.value, false);
  if (!bytes)
    return {};
  handle.locked = true;
  return decode_host_locale({bytes, static_cast<std::size_t>(size)});
}
} // namespace
std::string host_locale(FilterRecord* host) {
  auto locale=property_locale(host);
  return locale.empty()?interface_locale(host):locale;
}
int host_brightness(FilterRecord *host) noexcept {
  intptr_t value = 3;
  if (host && host->propertyProcs && host->propertyProcs->getPropertyProc &&
      host->propertyProcs->getPropertyProc(0x3842494d, propUIBrightness, 0,
                                           &value, nullptr) == noErr &&
      value >= 0 && value <= 3)
    return static_cast<int>(value);
  return 3;
}
OSErr host_pick_rgb(FilterRecord *host, double color[3]) {
  if (!host || !host->colorServices)
    return paramErr;
  ColorServicesInfo info{};
  info.infoSize = sizeof(info);
  info.selector = plugIncolorServicesChooseColor;
  info.sourceSpace = info.resultSpace = plugIncolorServicesRGBSpace;
  for (int i = 0; i < 3; ++i)
    info.colorComponents[i] = static_cast<int16>(std::lround(color[i] * 255));
  const auto result = host->colorServices(&info);
  if (result != noErr)
    return result;
  if (info.resultSpace != plugIncolorServicesRGBSpace)
    return paramErr;
  for (int i = 0; i < 3; ++i)
    color[i] = std::clamp<int>(info.colorComponents[i], 0, 255) / 255.0;
  return noErr;
}
OSErr host_sample_rgb(FilterRecord *host, POINT screen, double color[3]) {
  if (!host || !host->colorServices || !color)
    return paramErr;
  // Adobe's legacy global Point uses signed 16-bit coordinates; never wrap a
  // large or negative multi-monitor coordinate into a different sample point.
  if (screen.x < std::numeric_limits<short>::min() ||
      screen.x > std::numeric_limits<short>::max() ||
      screen.y < std::numeric_limits<short>::min() ||
      screen.y > std::numeric_limits<short>::max())
    return errInvalidSamplePoint;
  Point point{static_cast<short>(screen.y), static_cast<short>(screen.x)};
  ColorServicesInfo info{};
  info.infoSize=sizeof(info);
  info.selector=plugIncolorServicesSamplePoint;
  info.sourceSpace=info.resultSpace=plugIncolorServicesRGBSpace;
  info.selectorParameter.globalSamplePoint=&point;
  const auto result=host->colorServices(&info);
  if(result!=noErr)return result;
  if(info.resultSpace!=plugIncolorServicesRGBSpace)return paramErr;
  for(int k=0;k<3;++k)color[k]=std::clamp<int>(info.colorComponents[k],0,255)/255.;
  return noErr;
}
bool screen_sample_rgb(POINT screen, double color[3]) {
  if(!color)return false;
  const auto dc=GetDC(nullptr);
  if(!dc)return false;
  const COLORREF pixel=GetPixel(dc,screen.x,screen.y);
  ReleaseDC(nullptr,dc);
  if(pixel==CLR_INVALID)return false;
  color[0]=GetRValue(pixel)/255.;color[1]=GetGValue(pixel)/255.;color[2]=GetBValue(pixel)/255.;
  return true;
}
HCURSOR host_eyedropper_cursor(FilterRecord* host) {
  auto fallback=LoadCursorW(nullptr,IDC_CROSS);
  if(!host||!host->sSPBasic||!host->sSPBasic->AcquireSuite||!host->sSPBasic->ReleaseSuite)return fallback;
  const PSUIHooksSuite1* hooks=nullptr;
  if(host->sSPBasic->AcquireSuite(kPSUIHooksSuite,kPSUIHooksSuiteVersion1,
      reinterpret_cast<const void**>(&hooks))!=0)return fallback;
  const auto previous=GetCursor();
  HCURSOR cursor=fallback;
  if(hooks&&hooks->SetCursor&&hooks->SetCursor(kPICursorEyedropper)==0)
    if(auto native=GetCursor())cursor=native;
  ::SetCursor(previous);
  host->sSPBasic->ReleaseSuite(kPSUIHooksSuite,kPSUIHooksSuiteVersion1);
  return cursor;
}
} // namespace psui
