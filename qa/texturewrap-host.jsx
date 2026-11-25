(function () {
    var c = charIDToTypeID, s = stringIDToTypeID;
    var cfg = TEXTURE_WRAP, dir = cfg.out + '/', doc = null;
    var dialogs = app.displayDialogs, units = app.preferences.rulerUnits;
    var depths = [BitsPerChannelType.EIGHT, BitsPerChannelType.SIXTEEN, BitsPerChannelType.THIRTYTWO];
    var bitsList = [8, 16, 32], event = c(cfg.event), reports = [];
    var base = {ZwA0: 50, ZwA1: 50, ZwA2: 42, ZwA3: 0, ZwA4: 0, ZwA5: 0,
                ZwA6: 135, ZwA7: 50, ZwA8: 32, ZwA9: 68, ZwB0: 20, ZwB1: false};
    var smart = {ZwA0: 49, ZwA1: 51, ZwA2: 38, ZwA3: 13, ZwA4: -21, ZwA5: 8,
                 ZwA6: 121, ZwA7: 43, ZwA8: 31, ZwA9: 72, ZwB0: 24, ZwB1: true};
    var cylinder = cfg.effect === 'Cylinder', selfCylinder = cylinder && cfg.selfOverlay;
    var referenceName = '测试贴图源／藏蓝';
    var outputName = '测试贴图输出';
    if (cylinder) { base.ZwB2 = 70; smart.ZwB2 = 68; }
    smart.ZwB3 = cylinder ? 2 : 0;
    if (cylinder) smart.ZwB4 = 120.5;
    else {
        smart.ZwB5 = true;
        smart.ZwB6 = 27.5;
        smart.ZwB7 = 12.25;
        smart.ZwB8 = 63.5;
    }
    var legacy = {};
    for (var key in smart) legacy[key] = smart[key];
    smart.ZwB9 = referenceName;
    var previous = {};
    for (var key in smart) previous[key] = smart[key];
    smart.ZwC0 = true;
    var baseline = {};
    for (var key in smart) baseline[key] = smart[key];
    var priorCylinder = {};
    if (cylinder) {
        smart.ZwC1 = 43.5;
        smart.ZwC2 = 28.25;
        smart.ZwC3 = 32.75;
        if (selfCylinder) {
            for (var key in smart) priorCylinder[key] = smart[key];
            smart.ZwB9 = '';
            smart.ZwC0 = false;
            smart.ZwC4 = true;
        }
    } else {
        smart.ZwB9 = '';
        smart.ZwC0 = false;
        smart.ZwC4 = true;
    }

    function assert(value, message) { if (!value) throw Error(message); }
    function write(name, text) {
        var file = new File(dir + name); file.encoding = 'UTF8';
        assert(file.open('w'), 'write ' + name); file.write(text); file.close();
    }
    function close() { doc.close(SaveOptions.DONOTSAVECHANGES); doc = null; }
    function save(name) {
        var opts = new TiffSaveOptions(); opts.imageCompression = TIFFEncoding.NONE;
        opts.layers = false; opts.transparency = true;
        doc.saveAs(new File(dir + name + '.tif'), opts, true, Extension.LOWERCASE);
    }
    function select(x, y, right, bottom) {
        doc.selection.select([[x,y], [right,y], [right,bottom], [x,bottom]], SelectionType.REPLACE, 0, false);
    }
    function fill(x, y, right, bottom, r, g, b) {
        var color = new SolidColor(); color.rgb.red = r; color.rgb.green = g; color.rgb.blue = b;
        select(x, y, right, bottom);
        doc.selection.fill(color);
        doc.selection.deselect();
    }
    function create(depth, offcanvas) {
        doc = app.documents.add(193, 161, 72, 'Owned ' + cfg.effect + ' QA', NewDocumentMode.RGB,
                               DocumentFill.TRANSPARENT, 1, depth);
        fill(20, 32, 97, 128, 237, 16, 8);
        if (offcanvas) {
            fill(97, 32, 173, 128, 10, 218, 44);
            doc.activeLayer.translate(-100, 0);
        } else {
            fill(97, 32, 122, 128, 10, 218, 44);
            fill(133, 32, 173, 128, 17, 85, 238);
            fill(79, 32, 97, 128, 10, 218, 44);
        }
    }
    function addReference(target) {
        var layer = doc.artLayers.add();
        layer.name = referenceName;
        fill(25, 44, 115, 116, 27, 42, 235);
        fill(115, 44, 175, 116, 217, 60, 121);
        layer.translate(-100, 0);
        layer.visible = false;
        doc.activeLayer = target;
        return layer;
    }
    function expectRejected(action, message) {
        var previous = doc.activeHistoryState, rejected = false;
        try { executeAction(event, action, DialogModes.NO); }
        catch (error) { rejected = true; }
        assert(rejected && doc.activeHistoryState === previous, message);
    }
    function descriptor(values) {
        var desc = new ActionDescriptor();
        desc.putInteger(c('ZpSv'), 1);
        desc.putInteger(c('ZpId'), cfg.magic);
        for (var key in values) {
            if (key === 'ZwB3') desc.putInteger(c(key), values[key]);
            else if (typeof values[key] === 'string') desc.putString(c(key), values[key]);
            else if (typeof values[key] === 'boolean') desc.putBoolean(c(key), values[key]);
            else desc.putDouble(c(key), values[key]);
        }
        return desc;
    }
    function assertSaved(expected) {
        var ref = new ActionReference();
        ref.putEnumerated(c('Lyr '), c('Ordn'), c('Trgt'));
        var so = executeActionGet(ref).getObjectValue(s('smartObject'));
        var list = so.getList(s('filterFX'));
        assert(list.count === 1, 'one smart filter instance');
        var f = list.getObjectValue(0).getObjectValue(s('filter'));
        assert(f.getInteger(c('ZpSv')) === 1 && f.getInteger(c('ZpId')) === cfg.magic, 'schema and identity');
        for (var key in expected) {
            var saved;
            assert(f.hasKey(c(key)), 'missing saved field ' + key);
            if (key === 'ZwB3') saved = f.getInteger(c(key));
            else if (typeof expected[key] === 'string') saved = f.getString(c(key));
            else if (typeof expected[key] === 'boolean') saved = f.getBoolean(c(key));
            else saved = f.getDouble(c(key));
            assert(saved === expected[key], 'saved field ' + key + ' expected ' + expected[key] + ' received ' + saved);
        }
    }
    function reapply(values) {
        var ref = new ActionReference();
        ref.putIndex(s('filterFX'), 1);
        ref.putIdentifier(c('Lyr '), doc.activeLayer.id);
        var command = new ActionDescriptor(), fx = new ActionDescriptor();
        command.putReference(c('null'), ref);
        fx.putObject(s('filter'), event, descriptor(values));
        command.putObject(s('filterFX'), s('filterFX'), fx);
        executeAction(c('setd'), command, DialogModes.NO);
    }

    try {
        assert(app.documents.length === 0, 'empty owned session required');
        app.displayDialogs = DialogModes.NO;
        app.preferences.rulerUnits = Units.PIXELS;
        for (var i = 0; i < bitsList.length; ++i) {
            var bits = bitsList[i];
            if (cfg.phase === 'create') {
                create(depths[i], false);
                save('source-' + bits);
                var before = doc.activeHistoryState;
                executeAction(event, descriptor(base), DialogModes.NO);
                assert(doc.historyStates[doc.historyStates.length - 2] === before, 'single undo state');
                save('ordinary-' + bits);
                doc.activeHistoryState = before;
                save('undo-' + bits);
                select(0, 0, 103, 161);
                executeAction(event, descriptor(base), DialogModes.NO);
                doc.selection.deselect();
                save('selection-' + bits);
                close();

                if (cylinder) {
                    create(depths[i], false);
                    var sourceState = doc.activeHistoryState;
                    var names = ['tilted-front-', 'tilted-back-', 'tilted-both-'];
                    for (var mode = 0; mode < names.length; ++mode) {
                        var tilted = descriptor(base);
                        tilted.putDouble(c('ZwA3'), 30);
                        tilted.putInteger(c('ZwB3'), mode);
                        executeAction(event, tilted, DialogModes.NO);
                        save(names[mode] + bits);
                        doc.activeHistoryState = sourceState;
                    }
                    close();
                }

                create(depths[i], false);
                var start = doc.activeHistoryState;
                var local = descriptor(base);
                if (cylinder) local.putDouble(c('ZwB4'), 120);
                else {
                    local.putBoolean(c('ZwB5'), true);
                    local.putDouble(c('ZwB8'), 60);
                    local.putBoolean(c('ZwC0'), true);
                }
                executeAction(event, local, DialogModes.NO);
                save('local-' + bits);
                doc.activeHistoryState = start;
                if (cylinder) {
                    local.putDouble(c('ZwB4'), 40);
                    local.putDouble(c('ZwC1'), 40);
                    local.putDouble(c('ZwC2'), 28);
                    local.putDouble(c('ZwC3'), 28);
                    executeAction(event, local, DialogModes.NO);
                    save('positioned-decal-' + bits);
                    if (selfCylinder) {
                        doc.activeHistoryState = start;
                        local.putBoolean(c('ZwC4'), true);
                        executeAction(event, local, DialogModes.NO);
                        save('self-overlay-' + bits);
                    }
                } else {
                    local.putBoolean(c('ZwC0'), false);
                    local.putBoolean(c('ZwC4'), true);
                    executeAction(event, local, DialogModes.NO);
                    save('self-overlay-' + bits);
                    doc.activeHistoryState = start;
                    local.putBoolean(c('ZwC4'), false);
                    local.putDouble(c('ZwB6'), 42);
                    local.putDouble(c('ZwB7'), 18);
                    local.putDouble(c('ZwB8'), 45);
                    executeAction(event, local, DialogModes.NO);
                    save('moved-decal-' + bits);
                }
                close();

                create(depths[i], false);
                var target = doc.activeLayer;
                var reference = addReference(target);
                target.visible = false;
                reference.visible = true;
                save('reference-before-' + bits);
                reference.visible = false;
                target.visible = true;
                doc.activeLayer = target;
                var beforeSource = doc.activeHistoryState;
                var source = descriptor(base);
                if (cylinder) source.putDouble(c('ZwB4'), 120);
                else source.putBoolean(c('ZwB5'), true);
                source.putString(c('ZwB9'), referenceName);
                executeAction(event, source, DialogModes.NO);
                save('external-' + bits);
                target.visible = false;
                reference.visible = true;
                save('reference-after-' + bits);
                doc.activeHistoryState = beforeSource;
                doc.activeLayer = target;
                source.putBoolean(c('ZwC0'), true);
                executeAction(event, source, DialogModes.NO);
                save('overlay-' + bits);
                doc.activeHistoryState = beforeSource;
                doc.activeLayer = target;
                source.putString(c('ZwB9'), '');
                executeAction(event, source, DialogModes.NO);
                save('current-again-' + bits);
                if (bits === 8) {
                    var duplicate = reference.duplicate();
                    duplicate.name = referenceName;
                    duplicate.visible = false;
                    doc.activeLayer = target;
                    source.putString(c('ZwB9'), referenceName);
                    expectRejected(source, 'ambiguous source is rejected without applying');
                    source.putString(c('ZwB9'), '不存在的贴图图层');
                    expectRejected(source, 'missing source is rejected without applying');
                }
                close();

                create(depths[i], false);
                executeAction(s('newPlacedLayer'), new ActionDescriptor(), DialogModes.NO);
                target = doc.activeLayer;
                target.name = outputName;
                addReference(target);
                executeAction(event, descriptor(smart), DialogModes.NO);
                assertSaved(smart);
                save('smart-' + bits);
                doc.saveAs(new File(dir + 'smart-' + bits + '.psd'), new PhotoshopSaveOptions(), false, Extension.LOWERCASE);
                close();
            } else {
                doc = app.open(new File(dir + 'smart-' + bits + '.psd'));
                assert(doc.bitsPerChannel === depths[i], 'reopened depth ' + bits);
                doc.activeLayer = doc.artLayers.getByName(outputName);
                assertSaved(smart);
                save('reopened-' + bits);
                reapply(smart);
                assertSaved(smart);
                save('forced-' + bits);
                close();
                if (cfg.legacy) {
                    doc = app.open(new File(cfg.legacy + '/smart-' + bits + '.psd'));
                    assertSaved(legacy);
                    reapply(legacy);
                    assertSaved(legacy);
                    save('legacy-' + bits);
                    close();
                }
                if (cfg.previous) {
                    doc = app.open(new File(cfg.previous + '/smart-' + bits + '.psd'));
                    doc.activeLayer = doc.artLayers.getByName(outputName);
                    assertSaved(previous);
                    reapply(previous);
                    assertSaved(previous);
                    save('previous-' + bits);
                    close();
                }
                if (cfg.baseline) {
                    doc = app.open(new File(cfg.baseline + '/smart-' + bits + '.psd'));
                    doc.activeLayer = doc.artLayers.getByName(outputName);
                    assertSaved(baseline);
                    reapply(baseline);
                    assertSaved(baseline);
                    save('baseline-' + bits);
                    close();
                }
                if (cfg.priorCylinder) {
                    doc = app.open(new File(cfg.priorCylinder + '/smart-' + bits + '.psd'));
                    doc.activeLayer = doc.artLayers.getByName(outputName);
                    assertSaved(priorCylinder);
                    reapply(priorCylinder);
                    assertSaved(priorCylinder);
                    save('prior-cylinder-' + bits);
                    close();
                }
            }
            reports.push('RGB' + bits + ' ' + cfg.phase + ' PASS');
        }
        if (cfg.phase === 'create') {
            create(depths[0], true);
            var hidden = descriptor(base);
            hidden.putDouble(c('ZwA4'), 45);
            executeAction(event, hidden, DialogModes.NO);
            save('offcanvas-8');
            close();
        }
        write('report-' + cfg.phase + '.txt', reports.join('\n'));
        return 'PASS ' + cfg.effect + ' ' + reports.join(', ');
    } catch (error) {
        write('error-' + cfg.phase + '.txt', String(error) + ' line ' + error.line);
        throw error;
    } finally {
        if (doc) close();
        app.displayDialogs = dialogs;
        app.preferences.rulerUnits = units;
    }
})();
