// Uses globals prepared by run-smart-persistence.ps1. Owns its test documents.
(function(){
 var s=stringIDToTypeID,c=charIDToTypeID,out=AEPS_SMART_OUT+'/',d=null,dialogs=app.displayDialogs,report=[];
 function assert(v,why){if(!v)throw Error(why);}
 function write(name,text){var f=new File(out+name);f.encoding='UTF8';if(!f.open('w'))throw Error('Cannot write report');f.write(text);f.close();}
 function json(v){if(v===null)return 'null';if(typeof v==='string')return '"'+v.replace(/\\/g,'\\\\').replace(/"/g,'\\"').replace(/\r/g,'\\r').replace(/\n/g,'\\n')+'"';if(typeof v==='number'||typeof v==='boolean')return String(v);var a=[],k;for(k=0;k<v.length;k++)a.push(json(v[k]));return '['+a.join(',')+']';}
 function layer(id){var r=new ActionReference();r.putIdentifier(c('Lyr '),id);return executeActionGet(r);}
 function filters(id){return layer(id).getObjectValue(s('smartObject')).getList(s('filterFX'));}
 function state(id,index){var a=filters(id).getObjectValue(index-1);return {event:a.getObjectType(s('filter')),parameters:a.getObjectValue(s('filter'))};}
 function set(id,index,event,parameters){var r=new ActionReference();r.putIndex(s('filterFX'),index);r.putIdentifier(c('Lyr '),id);var a=new ActionDescriptor(),fx=new ActionDescriptor();a.putReference(c('null'),r);fx.putObject(s('filter'),event,parameters);a.putObject(s('filterFX'),s('filterFX'),fx);executeAction(c('setd'),a,DialogModes.NO);}
 function params(row,variant){
  if(row.legacy==='surface')return ZySmart.params(row.kind,0,variant===1?'鎏金_参考图层':'鎏金_参考图层②',variant-1);
  if(row.legacy==='creative')return ZyCreative.params(row.kind,128,96,variant-1);
  if(row.legacy==='roughen')return ZyRoughen.params(variant===1?0:6,128,96,variant-1);
  var a=new ActionDescriptor(),fields=row.fixtures[variant-1];for(var i=0;i<fields.length;i++){var f=fields[i],key=c(f.id);if(f.type==='number')a.putDouble(key,f.value);else if(f.type==='integer')a.putInteger(key,f.value);else if(f.type==='flag')a.putBoolean(key,f.value);else a.putString(key,variant===1?f.value:'鎏金_参考图层②');}return a;
 }
 function snapshot(id,row){
  var result=[];assert(filters(id).count===2,'two independent instances '+row.name);
  for(var i=1;i<=2;i++){
   var a=state(id,i),values=[typeIDToCharID(a.event)];assert(values[0]===row.event,'stable effect event');
   for(var j=0;j<row.keys.length;j++){
    var key=c(row.keys[j]);assert(a.parameters.hasKey(key),'missing saved parameter '+row.name+' '+row.keys[j]);var t=a.parameters.getType(key),v;
    // Keep host getters in explicit branches for older ExtendScript hosts.
    try{
     if(t==DescValueType.STRINGTYPE){v=a.parameters.getString(key);}
     else if(t==DescValueType.INTEGERTYPE){v=a.parameters.getInteger(key);}
     else if(t==DescValueType.BOOLEANTYPE){v=a.parameters.getBoolean(key);}
     else{v=a.parameters.getDouble(key);}
     values.push([row.keys[j],String(t),v]);
    }catch(e){throw Error('snapshot '+row.name+' instance '+i+' '+row.keys[j]+' type '+String(t)+' : '+String(e));}
   }
   result.push(values);
  }
  return result;
 }
 function save(stem){var o=new TiffSaveOptions();o.imageCompression=TIFFEncoding.NONE;o.layers=false;o.transparency=true;d.saveAs(new File(out+stem+'.tif'),o,true,Extension.LOWERCASE);}
 function fill(red,green,blue){var color=new SolidColor();color.rgb.red=red;color.rgb.green=green;color.rgb.blue=blue;d.selection.selectAll();d.selection.fill(color);d.selection.deselect();}
 try{
  assert(app.documents.length===0,'expected empty test session');app.displayDialogs=DialogModes.NO;
  var depths=[BitsPerChannelType.EIGHT,BitsPerChannelType.SIXTEEN,BitsPerChannelType.THIRTYTWO];
  for(var bi=0;bi<3;bi++){
   var bits=[8,16,32][bi],targets=[],snapshots=[],stem='smart-'+bits;
   if(AEPS_SMART_PHASE==='create'){
    d=app.documents.add(UnitValue(128,'px'),UnitValue(96,'px'),72,'AEPS shared parameter persistence '+bits,NewDocumentMode.RGB,DocumentFill.WHITE,1,depths[bi]);
    var seed=d.activeLayer;seed.name='QA seed';var color=new SolidColor();for(var x=0;x<8;x++){color.rgb.red=30+x*28;color.rgb.green=230-x*23;color.rgb.blue=40+(x%3)*80;d.selection.select([[x*16,0],[x*16+16,0],[x*16+16,96],[x*16,96]]);d.selection.fill(color);}d.selection.deselect();
    var ref=d.artLayers.add();ref.name='鎏金_参考图层';fill(170,90,210);ref=d.artLayers.add();ref.name='鎏金_参考图层②';fill(50,190,110);
    for(var i=0;i<AEPS_SMART_CASES.length;i++){
     var row=AEPS_SMART_CASES[i];if(bits!==8&&row.module==='native')continue;
     d.activeLayer=seed;var target=seed.duplicate();d.activeLayer=target;executeAction(s('newPlacedLayer'),new ActionDescriptor(),DialogModes.NO);target=d.activeLayer;target.name='QA target '+i;targets.push(target);
     for(var variant=1;variant<=2;variant++){write('progress.txt','create '+bits+' '+row.name+' instance '+variant);executeAction(c(row.event),params(row,variant),DialogModes.NO);}
     snapshots.push([i,snapshot(target.id,row)]);write('progress.txt','create '+bits+' '+row.name);
    }
    for(i=0;i<targets.length;i++)targets[i].visible=false;seed.visible=false;d.artLayers.getByName('鎏金_参考图层').visible=false;d.artLayers.getByName('鎏金_参考图层②').visible=false;
    for(i=0;i<targets.length;i++){targets[i].visible=true;save(stem+'-'+snapshots[i][0]+'-base');targets[i].visible=false;}
    targets[0].visible=true;var options=new PhotoshopSaveOptions();options.layers=true;d.saveAs(new File(out+stem+'.psd'),options,false,Extension.LOWERCASE);
    write(stem+'-expected.json',json(snapshots));report.push([bits,targets.length,targets.length*2,'PASS']);
   }else if(AEPS_SMART_PHASE==='reopen'){
    d=app.open(new File(out+stem+'.psd'));assert(String(d.bitsPerChannel)===String(depths[bi]),'actual document depth');
    var f=new File(out+stem+'-expected.json');f.encoding='UTF8';f.open('r');var expected=eval('('+f.read()+')');f.close();
    for(i=0;i<expected.length;i++)d.artLayers.getByName('QA target '+expected[i][0]).visible=false;
    for(i=0;i<expected.length;i++){
     var index=expected[i][0],row=AEPS_SMART_CASES[index],target=d.artLayers.getByName('QA target '+index);d.activeLayer=target;target.visible=true;
     assert(json(snapshot(target.id,row))===json(expected[i][1]),'reopen parameters differ '+bits+' '+row.name);save(stem+'-'+index+'-reopened');
     var original=state(target.id,1),other=state(target.id,2);set(target.id,1,original.event,other.parameters);set(target.id,1,original.event,original.parameters);
     assert(json(snapshot(target.id,row))===json(expected[i][1]),'editing instance changed another instance '+row.name);save(stem+'-'+index+'-forced');target.visible=false;write('progress.txt','reopen '+bits+' '+row.name);
    }
    report.push([bits,expected.length,expected.length*2,'PASS']);
   }else throw Error('Unknown test phase');
   d.close(SaveOptions.DONOTSAVECHANGES);d=null;
  }
  write('results-'+AEPS_SMART_PHASE+'.json',json(report));return 'PASS: shared Smart Filters '+AEPS_SMART_PHASE+'; 55 effect/depth cases; 110 instances';
 }catch(e){write('error-'+AEPS_SMART_PHASE+'.txt',String(e)+' line '+e.line);throw e;}
 finally{app.displayDialogs=dialogs;if(d)d.close(SaveOptions.DONOTSAVECHANGES);}
})();
