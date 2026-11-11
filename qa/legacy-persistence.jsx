// Reopen existing local test PSDs without saving changes to them.
(function(){
 var s=stringIDToTypeID,c=charIDToTypeID,d=null,dialogs=app.displayDialogs,result=[];
 function assert(v,t){if(!v)throw Error(t);}
 function json(v){if(v===null)return 'null';if(typeof v==='string')return '"'+v.replace(/\\/g,'\\\\').replace(/"/g,'\\"').replace(/\r/g,'\\r').replace(/\n/g,'\\n')+'"';if(typeof v==='number'||typeof v==='boolean')return String(v);var a=[];for(var i=0;i<v.length;i++)a.push(json(v[i]));return '['+a.join(',')+']';}
 function write(name,value){var f=new File(AEPS_LEGACY_OUT+'/'+name);f.encoding='UTF8';assert(f.open('w'),'report open');f.write(json(value));f.close();}
 function filters(id){var r=new ActionReference();r.putIdentifier(c('Lyr '),id);return executeActionGet(r).getObjectValue(s('smartObject')).getList(s('filterFX'));}
 function values(a,keys){var result=[];for(var i=0;i<keys.length;i++){var key=c(keys[i]),t=a.getType(key),v;if(t==DescValueType.STRINGTYPE){v=a.getString(key);}else if(t==DescValueType.INTEGERTYPE){v=a.getInteger(key);}else if(t==DescValueType.BOOLEANTYPE){v=a.getBoolean(key);}else{v=a.getDouble(key);}result.push([keys[i],String(t),v]);}return result;}
 function targets(layers,out){for(var i=0;i<layers.length;i++){var a=layers[i];if(a.typename==='LayerSet')targets(a.layers,out);else if(a.kind==LayerKind.SMARTOBJECT)out.push(a);}}
 try{
  assert(app.documents.length===0,'empty test session required');app.displayDialogs=DialogModes.NO;
  for(var i=0;i<AEPS_LEGACY_CASES.length;i++){
   var row=AEPS_LEGACY_CASES[i],instances=0;d=app.open(new File(row.file));var layers=[];targets(d.layers,layers);
   for(var j=0;j<layers.length;j++){
    var target=layers[j],fx=filters(target.id),before=[],definitions=[];d.activeLayer=target;
    for(var k=0;k<fx.count;k++){var item=fx.getObjectValue(k),event=item.getObjectType(s('filter')),id=typeIDToCharID(event),keys=AEPS_LEGACY_KEYS[id];assert(keys,'known legacy effect');var a=item.getObjectValue(s('filter'));before.push(values(a,keys));definitions.push([event,a]);}
    assert(fx.count>0,'saved legacy filters');
    for(k=0;k<definitions.length;k++){
     var r=new ActionReference();r.putIndex(s('filterFX'),k+1);r.putIdentifier(c('Lyr '),target.id);var a=new ActionDescriptor(),item=new ActionDescriptor();a.putReference(c('null'),r);item.putObject(s('filter'),definitions[k][0],definitions[k][1]);a.putObject(s('filterFX'),s('filterFX'),item);executeAction(c('setd'),a,DialogModes.NO);
    }
    fx=filters(target.id);var after=[];for(k=0;k<fx.count;k++){var item=fx.getObjectValue(k);after.push(values(item.getObjectValue(s('filter')),AEPS_LEGACY_KEYS[typeIDToCharID(item.getObjectType(s('filter')))]));}
    assert(json(before)===json(after),'legacy parameters changed during rerender');instances+=fx.count;
   }
   result.push([row.label,instances,'PASS']);assert(instances>0,'legacy instances found');d.close(SaveOptions.DONOTSAVECHANGES);d=null;write('legacy-results.json',result);
  }
  return 'PASS: twelve existing PSDs; six legacy effects; parameters preserved after rerender';
 }catch(e){write('legacy-error.json',[String(e),e.line]);throw e;}
 finally{app.displayDialogs=dialogs;if(d)d.close(SaveOptions.DONOTSAVECHANGES);}
})();
