// Owns only disposable documents; output folder is supplied by run-photoshop.ps1.
(function(){
 var root=AEPS_QA_OUTPUT+'/',prior=app.documents.length?app.activeDocument:null,d=null,dialogs=app.displayDialogs,log=[];
 var names=['Tint 色调','Fractal Noise 分形杂色','Displacement Map 置换图','Glow 发光','Deep Glow 深度发光','Chromatic Dispersion 色差色散','Vector Blur 矢量模糊','Grid 网格','Checkerboard 棋盘','Cell Pattern 单元格图案','Optics Compensation 光学补偿','Lens 镜头','JPEG Damage JPEG 损坏','Blobbylize 胶状化','Glass 玻璃','Turbulent Displace 湍流置换','Light Sweep 扫光','Kaleida 万花筒','Roughen Edges 毛边'];
 function save(name){var o=new TiffSaveOptions();o.imageCompression=TIFFEncoding.NONE;o.layers=false;o.transparency=true;d.saveAs(new File(root+name+'.tif'),o,true,Extension.LOWERCASE);}
 function write(name,text){var f=new File(root+name);f.encoding='UTF8';if(!f.open('w'))throw Error('Cannot write QA output');f.write(text);f.close();}
 try{
  app.displayDialogs=DialogModes.NO;var depths=[BitsPerChannelType.EIGHT,BitsPerChannelType.SIXTEEN,BitsPerChannelType.THIRTYTWO];
  for(var bi=0;bi<3;bi++){
   var bits=[8,16,32][bi];d=app.documents.add(UnitValue(128,'px'),UnitValue(96,'px'),72,'ZaYu disposable release regression '+bits,NewDocumentMode.RGB,DocumentFill.TRANSPARENT,1,depths[bi]);
   var c=new SolidColor();for(var x=0;x<8;x++){c.rgb.red=30+x*28;c.rgb.green=230-x*23;c.rgb.blue=40+(x%3)*80;d.selection.select([[x*16,16],[x*16+16,16],[x*16+16,80],[x*16,80]]);d.selection.fill(c);}d.selection.deselect();
   if(String(d.bitsPerChannel)!==String(depths[bi]))throw Error('Actual document depth differs');save('before-'+bits);var before=d.activeHistoryState;
   for(var effect=0;effect<names.length;effect++){
    if(effect===0&&bits!==8)continue;d.activeHistoryState=before;var desc=new ActionDescriptor();desc.putString(charIDToTypeID('Usng'),names[effect]);executeAction(charIDToTypeID('Fltr'),desc,DialogModes.NO);save(effect+'-'+bits+'-after');
    if(d.historyStates.length<2||d.historyStates[d.historyStates.length-2]!==before)throw Error('Expected one history state: '+names[effect]);d.activeHistoryState=before;save(effect+'-'+bits+'-undo');log.push(names[effect]+' RGB'+bits+' PASS');write('progress.txt',log.join('\n'));
   }
   d.close(SaveOptions.DONOTSAVECHANGES);d=null;
  }
  write('results.txt','PS '+app.version+'\n'+log.join('\n'));return 'PASS: nineteen filters; '+log.length+' effect/depth cases';
 }catch(e){write('error.txt',String(e)+' line '+e.line);throw e;}
 finally{app.displayDialogs=dialogs;if(d)d.close(SaveOptions.DONOTSAVECHANGES);if(prior)app.activeDocument=prior;}
})();
