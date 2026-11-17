// Production editor. Solo, selection and cached cards are view state only.
constexpr const char* element_types[]={"热点 / Hotspot","柔光 / Glow","射线 / Rays","拉丝 / Streak","光环 / Ring","光圈 / Aperture","光弧 / Arc","多重光圈 / Multi Aperture","星耀 / Glint","闪烁光点 / Sparkle","线环 / Hoop","光谱色带 / Spectrum Band"};
constexpr const char* design_presets[]={"电影蓝光 / Cinematic Blue","暖阳 / Warm Sun","复古镜头 / Vintage Lens","彩虹光环 / Rainbow Halo","宽银幕蓝 / Anamorphic Blue","柔和自然光 / Soft Natural","洋红霓光 / Magenta Neon","棱镜光圈 / Prism Aperture","光弧晨曦 / Arc Dawn","电影散景 / Cinema Bokeh","星幕 / Starfield","光谱之翼 / Spectrum Wings"};
constexpr const char* designer_glyphs=" 耀斑设计器 使用元素堆栈 返回参数 撤销 重做 元素编辑 预设库 添加元素 添加 复制 删除 上移 下移 清空 名称 自动名称 启用 独显预览 关闭独显 元素类型 元素亮度 元素尺寸 元素高宽比 轴向位置 水平偏移 垂直偏移 旋转 颜色 射线数量 光圈叶片 线条粗细 衰减长度 边缘柔和 光谱色彩 内置预设 我的预设 保存名称 保存新预设 刷新 导入 导出 已保存 保存失败 加载失败 文件无效 或版本不支持 文件写入失败 已导入 预览独显不改变最终输出 堆栈为空 输出没有光效 热点 柔光 射线 拉丝 光环 光圈 光弧 多重光圈 星耀 闪烁光点 线环 光谱色带 鬼影 副本 鎏金 光弧晨曦 电影散景 星幕 光谱之翼 弧长 环数 环间距 光圈间距 光点数量 散布范围 色带长度 色带宽度 星芒长度 星芒粗细 拖动排序 连续添加 关闭 正在生成预览 预览不可用 整圆比例光圈数量点击可连续添加拖动排序 ×";
void finish_history(Context& c){if(!c.history_pending)return;c.undo.push_back(std::move(c.pending));if(c.undo.size()>48)c.undo.erase(c.undo.begin());c.redo.clear();c.history_pending=false;}
void restore(Context& c,const Context::Snapshot& snapshot){c.worker.stop();c.p=snapshot.p;c.design=snapshot.design;c.solo=-1;c.element=std::clamp(c.element,0,std::max(0,int(c.design.elements.size())-1));c.error=c.read_mask()?noErr:filterBadParameters;}
FlarePreset current_preset(Context& c){return {c.p,c.design.enabled?c.design:design_from_legacy(c.p)};}
bool load_preset(Context& c,const FlarePreset& value){apply_preset(value,c.p,c.design);c.element=0;c.solo=-1;return true;}
bool preset_browser(psui::LibraryPanel& ui,Context& c){
 bool changed=false;c.thumbnails.start();ImGui::TextUnformatted(ui.text("内置预设 / Built-in Presets").c_str());const float scale=ui.scale();const int columns=std::max(1,std::min(3,int(ImGui::GetContentRegionAvail().x/(195*scale))));int ready=0;
 ImGui::BeginChild("builtin-presets",{0,std::clamp(ImGui::GetContentRegionAvail().y*.55f,220*scale,360*scale)},ImGuiChildFlags_None);
 if(ImGui::BeginTable("preset-cards",columns,ImGuiTableFlags_SizingStretchSame)){
  for(int i=0;i<builtin_preset_count;i++){ImGui::TableNextColumn();ImGui::PushID(i);const float width=ImGui::GetContentRegionAvail().x,height=width*.625f;const auto pos=ImGui::GetCursorScreenPos();const bool click=ImGui::InvisibleButton("preset-card",{width,height});ui.mark(("designer.preset/"+std::to_string(i)).c_str());const bool hovered=ImGui::IsItemHovered();auto* list=ImGui::GetWindowDrawList();const auto frame=c.thumbnails.get(i);const auto key="flare-preset-"+std::to_string(i);
   bool drawn=false;if(frame){++ready;drawn=ui.cached_image(key.c_str(),frame->rgba.data(),ThumbnailRenderer::width,ThumbnailRenderer::height,pos,{pos.x+width,pos.y+height});}
   if(!drawn){list->AddRectFilled(pos,{pos.x+width,pos.y+height},IM_COL32(20,23,29,255));const auto label=ui.text(c.thumbnails.error()||frame?"预览不可用 / Preview unavailable":"正在生成预览 / Rendering preview");list->AddText({pos.x+8*scale,pos.y+height*.45f},IM_COL32(210,215,225,255),label.c_str());}
   list->AddRect(pos,{pos.x+width,pos.y+height},hovered?IM_COL32(57,169,255,255):IM_COL32(100,100,100,255));ImGui::TextWrapped("%s",ui.text(design_presets[i]).c_str());
   if(hovered&&frame){ImGui::BeginTooltip();ImGui::TextUnformatted(ui.text(design_presets[i]).c_str());const float w=std::min({640.f,520*scale,ImGui::GetIO().DisplaySize.x-32*scale});const auto at=ImGui::GetCursorScreenPos();ImGui::Dummy({w,w*.625f});ui.cached_image(key.c_str(),frame->rgba.data(),ThumbnailRenderer::width,ThumbnailRenderer::height,at,{at.x+w,at.y+w*.625f});ImGui::EndTooltip();}
   if(click)changed|=load_preset(c,design_preset(i,c.source.width,c.source.height,c.source.left,c.source.top));ImGui::PopID();
  }ImGui::EndTable();
 }
 ImGui::EndChild();
 ui.items["designer.thumbnails"]={float(ThumbnailRenderer::width),float(ThumbnailRenderer::height),float(ready),c.thumbnails.error()?1.f:0.f};
 ImGui::Separator();ImGui::TextUnformatted(ui.text("我的预设 / My Presets").c_str());ImGui::SetNextItemWidth(-1);ImGui::InputText(ui.text("保存名称 / Save Name").c_str(),c.preset_name,sizeof(c.preset_name));ui.mark("designer.preset-name");
 if(ui.button("designer.save","保存新预设 / Save New Preset")){const auto file=files::save_new(c.preset_name,current_preset(c));c.message=file.empty()?"保存失败 / Save failed":"已保存 / Saved";c.refreshed=false;}
 ImGui::SameLine();if(ui.button("designer.refresh","刷新 / Refresh"))c.refreshed=false;
 if(ui.button("designer.import","导入 / Import")){const auto path=files::choose(ui.window(),false);if(!path.empty()){FlarePreset value;if(files::read(path,value)){changed|=load_preset(c,value);const auto copy=files::save_new(files::utf8_name(path),value);c.message=copy.empty()?"保存失败 / Save failed":"已导入 / Imported";c.refreshed=false;}else c.message="文件无效或版本不支持 / Invalid file or unsupported version";}}
 ImGui::SameLine();if(ui.button("designer.export","导出 / Export")){const auto path=files::choose(ui.window(),true);if(!path.empty())c.message=files::write(path,current_preset(c),true)?"已保存 / Saved":"文件写入失败 / File write failed";}
 if(!c.refreshed){c.saved_presets=files::list();c.refreshed=true;}
 for(std::size_t i=0;i<c.saved_presets.size();i++){ImGui::PushID(int(i));if(ImGui::Selectable(files::utf8_name(c.saved_presets[i]).c_str())){FlarePreset value;if(files::read(c.saved_presets[i],value))changed|=load_preset(c,value);else c.message="加载失败 / Load failed";}ui.mark(("designer.saved/"+std::to_string(i)).c_str());ImGui::PopID();}
 if(!c.message.empty())ImGui::TextWrapped("%s",ui.text(c.message.c_str()).c_str());return changed;
}
void element_icon(int type,ImVec2 center,float radius){
 auto* list=ImGui::GetWindowDrawList();const auto blue=IM_COL32(110,205,255,255),dim=IM_COL32(60,110,170,255);const float pi=3.14159265f;
 if(type==0||type==1){for(int i=5;i>=1;--i)list->AddCircleFilled(center,radius*i/5,IM_COL32(70,165,255,40+(6-i)*20));if(type==0)list->AddCircleFilled(center,radius*.2f,IM_COL32_WHITE);}
 else if(type==2||type==3||type==8){const int n=type==3?2:type==8?4:12;for(int i=0;i<n;i++){const float a=2*pi*i/n;list->AddLine(center,{center.x+std::cos(a)*radius,center.y+std::sin(a)*radius},blue,type==8?2:1.5f);}list->AddCircleFilled(center,2,IM_COL32_WHITE);}
 else if(type==5)list->AddNgon(center,radius,blue,6,2);
 else if(type==7){for(int i=0;i<5;i++)list->AddNgon({center.x+radius*(i-2)*.5f,center.y},radius*(.3f+i*.035f),i%2?blue:dim,6,1.5f);}
 else if(type==9){for(int i=0;i<9;i++){const float a=i*2.39996f,r=radius*std::sqrt((i+.5f)/9);const ImVec2 p{center.x+std::cos(a)*r,center.y+std::sin(a)*r};list->AddLine({p.x-3,p.y},{p.x+3,p.y},blue,1.3f);list->AddLine({p.x,p.y-3},{p.x,p.y+3},blue,1.3f);}}
 else if(type==11){for(int i=0;i<20;i++){const float t=i/20.f;const auto rgb=ImGui::ColorConvertFloat4ToU32({.5f+.5f*std::cos(2*pi*t),.5f+.5f*std::cos(2*pi*(t-.33f)),.5f+.5f*std::cos(2*pi*(t-.67f)),1});list->AddRectFilled({center.x-radius+2*radius*t,center.y-radius*.25f},{center.x-radius+2*radius*(t+.05f),center.y+radius*.25f},rgb);}}
 else if(type==6){list->PathArcTo(center,radius,-pi*.8f,pi*.4f,32);list->PathStroke(blue,0,2.5f);}
 else {list->AddCircle(center,radius,blue,40,1.5f);if(type==10){list->AddCircle(center,radius*.7f,dim,40,1.5f);list->AddCircle(center,radius*.45f,blue,40,1);}}
}
bool element_editor(psui::LibraryPanel& ui,Context& c){
 bool changed=false;const float s=ui.scale();const bool chinese=ui.language()==psui::Language::Chinese;ImGui::BeginChild("element-list",{225*s,0},ImGuiChildFlags_Borders);
 auto add_element=[&](int type){if(c.design.elements.size()>=max_elements)return;c.design.elements.push_back(make_element(type));c.element=int(c.design.elements.size())-1;c.scroll_to_element=c.element;c.solo=-1;changed=true;};
 auto duplicate=[&]{if(c.element<0||c.element>=int(c.design.elements.size())||c.design.elements.size()>=max_elements)return;auto copy=duplicate_element(c.design.elements[c.element],chinese);c.design.elements.insert(c.design.elements.begin()+c.element+1,std::move(copy));c.scroll_to_element=++c.element;c.solo=-1;changed=true;};
 auto remove=[&](int index){if(index<0||index>=int(c.design.elements.size()))return;c.design.elements.erase(c.design.elements.begin()+index);if(c.element>index)--c.element;c.element=std::min(c.element,int(c.design.elements.size())-1);c.solo=-1;changed=true;};
 if(!ImGui::IsAnyItemActive()&&!ui.picking_color()&&!ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopup)){
  if(ImGui::GetIO().KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_D,false))duplicate();
  if(ImGui::IsKeyPressed(ImGuiKey_Delete,false))remove(c.element);
 }
 ImGui::BeginDisabled(c.design.elements.size()>=max_elements);if(ui.button("designer.add","+ 添加元素 / + Add Element",-1))ImGui::OpenPopup("add-elements");ImGui::EndDisabled();
 ImGui::SetNextWindowSize({std::min(560*s,ImGui::GetIO().DisplaySize.x-32*s),0},ImGuiCond_Appearing);
 if(ImGui::BeginPopup("add-elements")){
  if(ImGui::BeginTable("element-cards",4,ImGuiTableFlags_SizingStretchSame)){
   for(int type=0;type<element_type_count;type++){ImGui::TableNextColumn();ImGui::PushID(type);const auto pos=ImGui::GetCursorScreenPos();const float w=ImGui::GetContentRegionAvail().x,h=82*s;ImGui::BeginDisabled(c.design.elements.size()>=max_elements);const bool clicked=ImGui::InvisibleButton("add-card",{w,h});ui.mark(("designer.add-type/"+std::to_string(type)).c_str());ImGui::EndDisabled();const bool hover=ImGui::IsItemHovered();auto* list=ImGui::GetWindowDrawList();list->AddRectFilled(pos,{pos.x+w,pos.y+h},hover?IM_COL32(65,80,98,255):IM_COL32(37,42,51,255),4*s);element_icon(type,{pos.x+w*.5f,pos.y+29*s},20*s);const auto name=ui.text(element_types[type]);const auto size=ImGui::CalcTextSize(name.c_str());list->AddText({pos.x+(w-size.x)*.5f,pos.y+58*s},IM_COL32_WHITE,name.c_str());
    if(clicked){add_element(type);if(!ImGui::GetIO().KeyShift)ImGui::CloseCurrentPopup();}ImGui::PopID();
   }ImGui::EndTable();
  }ImGui::TextUnformatted(ui.text("Shift+点击可连续添加 / Shift-click to add multiple").c_str());ImGui::EndPopup();
 }
 ImGui::BeginChild("stack",{0,std::max(100*s,ImGui::GetContentRegionAvail().y-3*(ui.button_height()+ImGui::GetStyle().ItemSpacing.y))});int erase=-1,drag_from=-1,drag_to=-1;ImGui::PushStyleColor(ImGuiCol_Header,{.13f,.25f,.35f,1});
 const float row_height=std::max(25*s,ImGui::GetFrameHeight());
 ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign,{0,.5f});
 ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{ImGui::GetStyle().ItemSpacing.x,0});
 ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,{0,2*s});
 if(ImGui::BeginTable("element-rows",3,ImGuiTableFlags_SizingStretchProp|ImGuiTableFlags_NoSavedSettings)){
  ImGui::TableSetupColumn("visible",ImGuiTableColumnFlags_WidthFixed,ImGui::GetFrameHeight()+ImGui::GetStyle().ItemSpacing.x);
  ImGui::TableSetupColumn("name",ImGuiTableColumnFlags_WidthStretch);
  ImGui::TableSetupColumn("delete",ImGuiTableColumnFlags_WidthFixed,26*s);
  for(std::size_t i=0;i<c.design.elements.size();i++){
   auto& e=c.design.elements[i];ImGui::PushID(int(i));const auto name=element_name(e,chinese);ImGui::TableNextRow();
   // Submit the label first so framed controls cannot shift its text baseline.
   ImGui::TableSetColumnIndex(1);
   if(ImGui::Selectable(name.c_str(),int(i)==c.element,ImGuiSelectableFlags_None,{ImGui::GetContentRegionAvail().x,row_height})){c.element=int(i);if(c.solo>=0){c.solo=c.element;c.dirty=true;}}ui.mark(("designer.element/"+std::to_string(i)).c_str());
   if(ImGui::IsItemHovered())ImGui::SetTooltip("%s\n%s",name.c_str(),ui.text("拖动排序；Delete 删除；Ctrl+D 复制 / Drag to reorder; Delete removes; Ctrl+D duplicates").c_str());
   if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)){const int index=int(i);ImGui::SetDragDropPayload("FLARE_ELEMENT",&index,sizeof(index));ImGui::TextUnformatted(name.c_str());ImGui::EndDragDropSource();}
   if(ImGui::BeginDragDropTarget()){if(const auto* payload=ImGui::AcceptDragDropPayload("FLARE_ELEMENT"))if(payload->DataSize==sizeof(int)){drag_from=*static_cast<const int*>(payload->Data);drag_to=int(i);}ImGui::EndDragDropTarget();}
   if(c.scroll_to_element==int(i)){ImGui::SetScrollHereY();c.scroll_to_element=-1;}
   ImGui::TableSetColumnIndex(2);if(ImGui::Button("×",{26*s,row_height}))erase=int(i);ui.mark(("designer.delete/"+std::to_string(i)).c_str());if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",ui.text("删除 / Delete").c_str());
   ImGui::TableSetColumnIndex(0);ImGui::SetCursorPosY(ImGui::GetCursorPosY()+(row_height-ImGui::GetFrameHeight())*.5f);
   changed|=ImGui::Checkbox("##visible",&e.enabled);ui.mark(("designer.visible/"+std::to_string(i)).c_str());ImGui::PopID();
  }ImGui::EndTable();
 }
 ImGui::PopStyleVar(3);
 ImGui::PopStyleColor();ImGui::EndChild();
 if(erase>=0)remove(erase);
 else if(drag_from>=0&&drag_from<int(c.design.elements.size())&&drag_to>=0&&drag_to<int(c.design.elements.size())&&drag_from!=drag_to){auto value=std::move(c.design.elements[drag_from]);c.design.elements.erase(c.design.elements.begin()+drag_from);c.design.elements.insert(c.design.elements.begin()+drag_to,std::move(value));if(c.element==drag_from)c.element=drag_to;else if(c.element>drag_from&&c.element<=drag_to)--c.element;else if(c.element<drag_from&&c.element>=drag_to)++c.element;c.solo=-1;changed=true;}
 const bool selected=c.element>=0&&c.element<int(c.design.elements.size());const float half=(ImGui::GetContentRegionAvail().x-ImGui::GetStyle().ItemSpacing.x)*.5f;ImGui::BeginDisabled(!selected);ImGui::BeginDisabled(c.design.elements.size()>=max_elements);if(ui.button("designer.duplicate","复制 / Duplicate",half))duplicate();ImGui::EndDisabled();ImGui::SameLine();if(ui.button("designer.delete","删除 / Delete",half))remove(c.element);
 ImGui::BeginDisabled(c.element<=0);if(ui.button("designer.up","上移 / Up",half)){std::swap(c.design.elements[c.element],c.design.elements[c.element-1]);--c.element;c.solo=-1;changed=true;}ImGui::EndDisabled();ImGui::SameLine();ImGui::BeginDisabled(c.element+1>=int(c.design.elements.size()));if(ui.button("designer.down","下移 / Down",half)){std::swap(c.design.elements[c.element],c.design.elements[c.element+1]);++c.element;c.solo=-1;changed=true;}ImGui::EndDisabled();ImGui::EndDisabled();
 if(ui.button("designer.clear","清空 / Clear",-1)){c.design.elements.clear();c.element=0;c.solo=-1;changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("element-properties",{0,0});c.element=std::clamp(c.element,0,std::max(0,int(c.design.elements.size())-1));
 if(!c.design.elements.empty()){
  auto& e=c.design.elements[c.element];int type=e.type;if(ui.choice("元素类型 / Element Type",type,element_types,element_type_count)){auto replacement=make_element(type);replacement.brightness=e.brightness;replacement.size=e.size;replacement.aspect=e.aspect;replacement.axis=e.axis;replacement.x=e.x;replacement.y=e.y;replacement.rotation=e.rotation;replacement.color=e.color;replacement.name=e.name;replacement.custom_name=e.custom_name;replacement.copies=e.copies;replacement.enabled=e.enabled;e=std::move(replacement);changed=true;}
  const auto factory=make_element(e.type);ui.parameter_defaults(static_cast<ElementValues&>(e),static_cast<const ElementValues&>(factory));const auto label=element_name(e,chinese);char name[129]{};std::snprintf(name,sizeof(name),"%s",label.c_str());ImGui::SetNextItemWidth(-1);if(ImGui::InputText(ui.text("名称 / Name").c_str(),name,97)){e.name=name;e.custom_name=true;changed=true;}ui.mark("designer.name");if(e.custom_name&&ui.button("designer.auto-name","自动名称 / Automatic Name")){e.name.clear();e.custom_name=false;changed=true;}ui.items["designer.naming"]={e.custom_name?1.f:0.f,chinese?1.f:0.f,float(e.type),0};changed|=ui.check("启用 / Enabled",e.enabled);
  if(ui.button("designer.solo",c.solo==c.element?"关闭独显 / Stop Solo":"独显预览 / Solo Preview",-1)){c.solo=c.solo==c.element?-1:c.element;c.dirty=true;}
  auto number=[&](const char* text,double& value,double lo,double hi,double a,double b){changed|=ui.number(text,value,lo,hi,a,b);};
  auto integer=[&](const char* text,int& field,int lo,int hi){double value=field;ui.inherit_number_default(text,field);if(ui.number(text,value,lo,hi,lo,hi,true,true)){field=int(value);changed=true;}};
  number("元素亮度 / Element Brightness",e.brightness,0,20,0,3);number("元素尺寸 / Element Size",e.size,.0001,20,.01,3);number("元素高宽比 / Element Aspect",e.aspect,.05,20,.1,3);number("轴向位置 / Axis Position",e.axis,-64,64,-1,3);number("水平偏移（%） / Offset X (%)",e.x,-1000,1000,-100,100);number("垂直偏移（%） / Offset Y (%)",e.y,-1000,1000,-100,100);number("旋转（度） / Element Rotation (deg)",e.rotation,-1e9,1e9,-180,180);changed|=psui::color_control(ui,c.host,"颜色 / Color",e.color.data());
  if(e.type==2||e.type==3){number("线条粗细 / Line Thickness",e.softness,.01,10,.05,3);number("衰减长度 / Falloff Length",e.spread,.01,10,.1,4);}
  if(e.type==2||e.type==8)integer("射线数量 / Ray Count",e.count,0,64);
  if(e.type==5||e.type==7)integer("光圈叶片 / Aperture Blades",e.blades,3,16);
  if(e.type==5||e.type==7)number("边缘柔和 / Edge Softness",e.softness,0,1,0,1);
  if(e.type==6){number("弧长（整圆比例） / Arc Coverage",e.spread,.01,1,.05,1);number("线条粗细 / Line Thickness",e.softness,0,10,0,2);}
  if(e.type==7){integer("光圈数量 / Aperture Count",e.count,0,32);number("光圈间距 / Aperture Spacing",e.spread,.01,10,.1,5);}
  if(e.type==8){number("星芒长度 / Glint Length",e.spread,.01,10,.1,4);number("星芒粗细 / Glint Thickness",e.softness,.01,10,.1,3);}
  if(e.type==9){integer("光点数量 / Sparkle Count",e.count,0,64);number("散布范围 / Scatter Radius",e.spread,.01,10,.1,5);number("星芒粗细 / Glint Thickness",e.softness,.01,10,.1,3);}
  if(e.type==10){integer("环数 / Ring Count",e.count,0,16);number("环间距 / Ring Spacing",e.spread,.01,10,.1,5);number("线条粗细 / Line Thickness",e.softness,0,10,0,2);}
  if(e.type==11){number("色带长度 / Band Length",e.spread,.01,10,.1,5);number("色带宽度 / Band Width",e.softness,0,10,0,3);}
  if(e.type==4||e.type==6||e.type==7||e.type==10||e.type==11)number("光谱色彩（%） / Spectral Color (%)",e.chromatic,0,100,0,100);
  ui.items["designer.selected"]={float(e.brightness),float(e.size),float(e.type),e.enabled?1.f:0.f};
 }else ImGui::TextWrapped("%s",ui.text("堆栈为空，输出没有光效。 / Empty stack: no emitted light.").c_str());
 ImGui::EndChild();return changed;
}
bool draw_designer(psui::LibraryPanel& ui,Context& c){
 bool changed=false,restored=false;const auto before=Context::Snapshot{c.p,c.design};const float s=ui.scale();
 if(ui.button("designer.back","返回参数 / Back",120*s)){c.editor=false;c.solo=-1;c.dirty=true;}
 ImGui::SameLine();ImGui::BeginDisabled(c.undo.empty()&&!c.history_pending);if(ui.button("designer.undo","撤销 / Undo",90*s)){finish_history(c);if(!c.undo.empty()){c.redo.push_back({c.p,c.design});auto state=std::move(c.undo.back());c.undo.pop_back();restore(c,state);changed=restored=true;}}ImGui::EndDisabled();
 ImGui::SameLine();ImGui::BeginDisabled(c.redo.empty());if(ui.button("designer.redo","重做 / Redo",90*s)){c.undo.push_back({c.p,c.design});auto state=std::move(c.redo.back());c.redo.pop_back();restore(c,state);changed=restored=true;}ImGui::EndDisabled();
 ui.items["designer.state"]={float(c.design.elements.size()),float(c.element),float(c.undo.size()),float(c.solo)};
 if(ui.button("designer.elements-tab","元素编辑 / Elements",140*s))c.editor_page=0;ImGui::SameLine();if(ui.button("designer.presets-tab","预设库 / Presets",140*s))c.editor_page=1;
 if(c.editor_page==0)changed|=element_editor(ui,c);else changed|=preset_browser(ui,c);
 if(changed&&!restored&&!c.history_pending){c.pending=before;c.history_pending=true;}if(!ImGui::IsAnyItemActive())finish_history(c);
 return changed;
}
