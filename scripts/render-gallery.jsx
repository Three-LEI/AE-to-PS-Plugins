// Render the installed .8bf files. This script only changes its own documents.
(function(){
 var c=charIDToTypeID,s=stringIDToTypeID,out=GALLERY_OUT+'/',doc=null,prior=app.documents.length?app.activeDocument:null;
 var dialogs=app.displayDialogs,units=app.preferences.rulerUnits,done=[];
 function write(name,text){var f=new File(out+name);f.encoding='UTF8';if(!f.open('w'))throw Error('Cannot write gallery report');f.write(text);f.close();}
 function save(name){var options=new PNGSaveOptions();options.compression=6;options.interlaced=false;doc.saveAs(new File(out+name+'.png'),options,true,Extension.LOWERCASE);}
 function number(d,key,value){d.putDouble(c(key),value);}
 function descriptor(row){
  var d=new ActionDescriptor();
  if(row.legacy==='surface'){
   d=ZySmart.params(row.kind,0,'',0);d.putInteger(c('ZyLt'),0);
   var vals={ZySf:4,ZyCt:21,ZyHt:35,ZyDp:55,ZyIn:100,ZyLH:65,ZyDr:-45,ZyAm:60,ZyDf:45,ZySp:55,ZyRo:.06,ZyMe:30,ZyLR:1,ZyLG:1,ZyLB:1};
   for(var key in vals)number(d,key,vals[key]);
  }else if(row.legacy==='creative'){
   d=ZyCreative.params(row.kind,768,768,0);var values={ZcCX:384,ZcCY:330,ZcAm:35,ZcSz:90,ZcCo:2,ZcEv:0,ZcMx:100,ZcWi:100,ZcIn:85,ZcEI:50,ZcLR:1,ZcLG:.9,ZcLB:.7,ZcRo:0};
   if(row.kind===0){values.ZcAm=85;values.ZcSz=70;}
   for(var name in values)number(d,name,values[name]);d.putInteger(c('ZcMi'),5);d.putInteger(c('ZcPi'),0);
  }else if(row.legacy==='roughen'){
   d=ZyRoughen.params(0,768,768,0);number(d,'ZrBo',26);number(d,'ZrSc',64);number(d,'ZrMx',100);number(d,'ZrIn',1);d.putInteger(c('ZrCE'),0);
  }
  function raw(hex){var data='';for(var j=0;j<hex.length;j+=2)data+=String.fromCharCode(parseInt(hex.substr(j,2),16));return data;}
  for(var i=0;i<row.fields.length;i++){var f=row.fields[i],kind=f.kind||f.type;if(kind==='number')d.putDouble(c(f.id),f.value);else if(kind==='integer')d.putInteger(c(f.id),f.value);else if(kind==='flag')d.putBoolean(c(f.id),f.value);else if(kind==='binary')d.putData(c(f.id),raw(f.value));else d.putString(c(f.id),f.value);}
  if(row.geometry){d.putData(c('ZePG'),raw(row.geometry));d.putString(c('ZePN'),'Gallery ellipse');}
  return d;
 }
 try{
  app.displayDialogs=DialogModes.NO;app.preferences.rulerUnits=Units.PIXELS;
  for(var index=0;index<GALLERY_ROWS.length;index++){
   var row=GALLERY_ROWS[index],wanted=!GALLERY_ONLY.length;for(var pick=0;pick<GALLERY_ONLY.length;pick++)if(GALLERY_ONLY[pick]===row.slug)wanted=true;if(!wanted)continue;
   var texture=(row.input==='globe'||row.input==='label')&&typeof GALLERY_TEXTURE_SOURCE!=='undefined';
   var path=texture?GALLERY_TEXTURE_SOURCE+'/'+row.slug+'-before.png':GALLERY_SOURCE;
   doc=app.open(new File(path));if(doc.activeLayer.isBackgroundLayer)doc.activeLayer.isBackgroundLayer=false;
   try{doc.convertProfile('sRGB IEC61966-2.1',Intent.RELATIVECOLORIMETRIC,true,false);}catch(ignore){}
   var target=doc.activeLayer;target.name=texture?'Demo texture':'Demo photo';
   if(row.input==='map'){var ref=target.duplicate();ref.name='DemoMap';ref.visible=false;doc.activeLayer=target;}
   if(row.input==='alpha'){
    var points=[];for(var n=0;n<128;n++){var angle=n*Math.PI*2/128;points.push([384+302*Math.cos(angle),384+326*Math.sin(angle)]);}
    doc.selection.select(points,SelectionType.REPLACE,1,true);doc.selection.invert();doc.selection.clear();doc.selection.deselect();
   }
   if(row.input==='glints'){
    var white=new SolidColor();white.rgb.red=white.rgb.green=white.rgb.blue=255;
    var glints=[[316,274],[445,272],[696,325],[150,325]];
    for(var k=0;k<glints.length;k++){var x=glints[k][0],y=glints[k][1];doc.selection.select([[x-1,y-1],[x+2,y-1],[x+2,y+2],[x-1,y+2]]);doc.selection.fill(white);}doc.selection.deselect();
   }
   save(row.slug+'-before');executeAction(c(row.event),descriptor(row),DialogModes.NO);save(row.slug+'-after');
   doc.close(SaveOptions.DONOTSAVECHANGES);doc=null;done.push(row.slug);write('progress.txt',done.join('\n'));
  }
  write('results.txt','Photoshop '+app.version+'\n'+done.join('\n'));return 'PASS: '+done.length+' native plugin gallery examples';
 }catch(e){write('error.txt',String(e)+' line '+e.line+' profile '+(typeof row==='undefined'?'':row.slug));throw e;}
 finally{app.displayDialogs=dialogs;app.preferences.rulerUnits=units;if(doc)doc.close(SaveOptions.DONOTSAVECHANGES);if(prior)app.activeDocument=prior;}
})();
