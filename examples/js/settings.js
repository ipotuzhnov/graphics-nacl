function loadSettings(type, path) {
  settings.setMimeType(type);
  settings.setDocumentPath(path);
}

var settings = (function() {
  var document = { path: null };
  var mimetype = null;
  var type = null;
  var nmf = '/graphics-nacl-git/graphics_nacl.nmf';
  
  function setDocumentPath(path) {
    settings.document.path = path;
  }
  
  function setMimeType(type) {
    settings.type = type;
    if (type === 'ppapi') {
      settings.mimetype = 'application/x-ppapi';
    } else if (type === 'pnacl') {
      settings.mimetype = 'application/x-pnacl';
    }
  }
  
  // The symbols to export.
  return {
    // Properties
    document: document,
    mimetype: mimetype,
    type: type,
    nmf: nmf,

    // Functions
    setDocumentPath: setDocumentPath,
    setMimeType: setMimeType
  };
}());
