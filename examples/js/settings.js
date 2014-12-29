function loadSettings(path) {
  settings.setMimeType('pnacl');
  if (path == null) {
    settings.setDocumentPath('http://172.16.2.97:88/graphics_nacl/1.djvu');
  } else {
    settings.setDocumentPath(path)
  }
}

var settings = (function() {
  var document = { path: null };
  var decoder = { mimetype: null, nmf: null };
  var renderer = { mimetype: null, nmf: null };
  var type = null;
  
  function setDocumentPath(path) {
    settings.document.path = path;
  }
  
  function setMimeType(type) {
    settings.type = type;
    if (type === 'ppapi') {
      settings.decoder.mimetype = 'application/x-ppapi-decoder';
      settings.renderer.mimetype = 'application/x-ppapi-renderer';
    } else if (type === 'pnacl') {
      settings.decoder.mimetype = 'application/x-pnacl';
      settings.decoder.nmf = '/graphics-nacl-git/graphics-nacl-decoder.nmf';
      settings.renderer.mimetype = 'application/x-pnacl';
      settings.renderer.nmf = '/graphics-nacl-git/graphics-nacl-renderer.nmf';
    }
  }
  
  // The symbols to export.
  return {
    // Properties
    document: document,
    decoder: decoder,
    renderer: renderer,
    type: type,

    // Functions
    setDocumentPath: setDocumentPath,
    setMimeType: setMimeType
  };
}());
